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
    } else {
        flags = COUCHSTORE_OPEN_FLAG_CREATE;
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

LIBCBIO_API
cbio_error_t cbio_get_document(libcbio_t handle,
                               const void *id,
                               size_t nid,
                               libcbio_document_t *doc)
{
    libcbio_document_t ret = calloc(1, sizeof(*ret));
    couchstore_error_t err;

    if (ret == NULL) {
        return CBIO_ERROR_ENOMEM;
    }

    err = couchstore_docinfo_by_id(handle->couchstore_handle, id, nid, &ret->info);
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

LIBCBIO_API
cbio_error_t cbio_store_documents(libcbio_t handle,
                                  libcbio_document_t *doc,
                                  size_t ndocs)
{
    Doc **docs;
    DocInfo **info;
    size_t ii;
    couchstore_error_t err;

    if (handle->mode == CBIO_OPEN_RDONLY) {
        return CBIO_ERROR_EINVAL;
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
