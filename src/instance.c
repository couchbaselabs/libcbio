/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2012 Couchbase, Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */
#include "internal.h"

#include <stdlib.h>
#include <string.h>

static int cbio_is_local_id(const void *data, size_t nb)
{
    return (nb > 6 && memcmp(data, "_local/", 7) == 0) ? 1 : 0;
}

static int cbio_is_local_document(DocInfo *info)
{
    return cbio_is_local_id(info->id.buf, info->id.size);
}

LIBCBIO_API
cbio_error_t cbio_open_handle(const char *name,
                              libcbio_open_mode_t mode,
                              libcbio_t *handle)
{
    couchstore_error_t err;
    uint64_t flags;
    libcbio_t ret;

    ret = calloc(1, sizeof(*ret));
    if (ret == NULL) {
        return CBIO_ERROR_ENOMEM;
    }

    ret->mode = mode;
    if (mode == CBIO_OPEN_RDONLY) {
        flags = COUCHSTORE_OPEN_FLAG_RDONLY;
    } else if (CBIO_OPEN_CREATE) {
        flags = COUCHSTORE_OPEN_FLAG_CREATE;
    } else {
        flags = 0;
    }

    err = couchstore_open_db(name, flags, &ret->couchstore_handle);
    if (err != COUCHSTORE_SUCCESS) {
        free(ret);
        return cbio_remap_error(err);
    }

    *handle = ret;
    return CBIO_SUCCESS;
}

LIBCBIO_API
void cbio_close_handle(libcbio_t handle)
{
    if (handle->mode != CBIO_OPEN_RDONLY) {
        (void)cbio_commit(handle);
    }

    couchstore_close_db(handle->couchstore_handle);
    free(handle);
}

LIBCBIO_API
off_t cbio_get_header_position(libcbio_t handle)
{
    return (off_t)couchstore_get_header_position(handle->couchstore_handle);
}

static cbio_error_t cbio_ldoc2doc(libcbio_t handle, const LocalDoc *ldoc, libcbio_document_t *doc)
{
    cbio_error_t e;

    if ((e = cbio_create_empty_document(handle, doc)) == CBIO_SUCCESS &&
        (e = cbio_document_set_id(*doc, ldoc->id.buf, ldoc->id.size, 1)) == CBIO_SUCCESS &&
        (e = cbio_document_set_value(*doc, ldoc->json.buf, ldoc->json.size, 1)) == CBIO_SUCCESS &&
        (e = cbio_document_set_deleted(*doc, ldoc->deleted)) == CBIO_SUCCESS) {
        return CBIO_SUCCESS;
    }

    return e;
}

static cbio_error_t cbio_get_local_document(libcbio_t handle,
                                            const void *id,
                                            size_t nid,
                                            libcbio_document_t *doc)
{
    couchstore_error_t err;
    LocalDoc *ldoc;

    err = couchstore_open_local_document(handle->couchstore_handle, id,
                                         nid, &ldoc);
    if (err == COUCHSTORE_SUCCESS) {
        cbio_error_t ret;
        if (ldoc->deleted) {
            ret = CBIO_ERROR_ENOENT;
        } else {
            ret = cbio_ldoc2doc(handle, ldoc, doc);
        }
        couchstore_free_local_document(ldoc);
        return ret;
    }

    return cbio_remap_error(err);
}

LIBCBIO_API
cbio_error_t cbio_get_document(libcbio_t handle,
                               const void *id,
                               size_t nid,
                               libcbio_document_t *doc)
{
    if (cbio_is_local_id(id, nid)) {
        return cbio_get_local_document(handle, id, nid, doc);
    }

    libcbio_document_t ret = calloc(1, sizeof(*ret));
    couchstore_error_t err;

    if (ret == NULL) {
        return CBIO_ERROR_ENOMEM;
    }


    err = couchstore_docinfo_by_id(handle->couchstore_handle, id,
                                   nid, &ret->info);
    if (err != COUCHSTORE_SUCCESS) {
        cbio_document_release(ret);
        return cbio_remap_error(err);
    }

    err = couchstore_open_doc_with_docinfo(handle->couchstore_handle,
                                           ret->info,
                                           &ret->doc, 0);
    if (err != COUCHSTORE_SUCCESS || ret->info->deleted) {
        cbio_document_release(ret);
        if (err == COUCHSTORE_SUCCESS) {
            return CBIO_ERROR_ENOENT;
        }
        return cbio_remap_error(err);
    }

    *doc = ret;
    return CBIO_SUCCESS;
}

LIBCBIO_API
cbio_error_t cbio_store_document(libcbio_t handle,
                                 libcbio_document_t doc)
{
    return cbio_store_documents(handle, &doc, 1);
}

static cbio_error_t cbio_store_local_documents(libcbio_t handle,
                                               libcbio_document_t *doc,
                                               size_t ndocs)
{
    for (size_t ii = 0; ii < ndocs; ++ii) {
        couchstore_error_t err;
        LocalDoc mydoc;
        mydoc.id = doc[ii]->info->id;
        mydoc.json = doc[ii]->doc->data;
        mydoc.deleted = doc[ii]->info->deleted;

        err = couchstore_save_local_document(handle->couchstore_handle,
                                             &mydoc);
        if (err != COUCHSTORE_SUCCESS) {
            return cbio_remap_error(err);
        }
    }
    return CBIO_SUCCESS;
}

LIBCBIO_API
cbio_error_t cbio_store_documents(libcbio_t handle,
                                  libcbio_document_t *doc,
                                  size_t ndocs)
{
    Doc **docs;
    DocInfo **info;
    size_t ii;
    couchstore_error_t err;

    if (handle->mode == CBIO_OPEN_RDONLY || ndocs == 0) {
        return CBIO_ERROR_EINVAL;
    }

    if (cbio_is_local_document(doc[0]->info)) {
        return cbio_store_local_documents(handle, doc, ndocs);
    }

    docs = calloc(ndocs, sizeof(Doc *));
    info = calloc(ndocs, sizeof(DocInfo *));
    if (docs == NULL || info == NULL) {
        free(docs);
        free(info);
        return CBIO_ERROR_ENOMEM;
    }

    for (ii = 0; ii < ndocs; ++ii) {
        docs[ii] = doc[ii]->doc;
        info[ii] = doc[ii]->info;
    }

    err = couchstore_save_documents(handle->couchstore_handle, docs, info,
                                    ndocs, 0);
    free(docs);
    free(info);

    return cbio_remap_error(err);
}

LIBCBIO_API
cbio_error_t cbio_commit(libcbio_t handle)
{
    if (handle->mode == CBIO_OPEN_RDONLY) {
        return CBIO_ERROR_EINVAL;
    }
    return cbio_remap_error(couchstore_commit(handle->couchstore_handle));
}

struct cbio_wrap_ctx {
    cbio_changes_callback_fn callback;
    libcbio_t handle;
    void *ctx;
};

static int couchstore_changes_callback(Db *db, DocInfo *docinfo, void *ctx)
{
    (void)db;
    int ret = 0;
    libcbio_document_t doc = calloc(1, sizeof(*doc));
    if (doc) {
        struct cbio_wrap_ctx *uctx = ctx;
        doc->info = docinfo;

        ret = uctx->callback(uctx->handle, doc, uctx->ctx);
        if (ret == 0) {
            free(doc);
        }
    }

    return ret;
}

LIBCBIO_API
cbio_error_t cbio_changes_since(libcbio_t handle,
                                uint64_t since,
                                cbio_changes_callback_fn callback,
                                void *ctx)
{
    struct cbio_wrap_ctx uctx = { .callback = callback,
        .handle = handle,
         .ctx = ctx
    };
    couchstore_error_t err;
    err = couchstore_changes_since(handle->couchstore_handle,
                                   since, 0,
                                   couchstore_changes_callback,
                                   &uctx);

    return cbio_remap_error(err);
}
