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

#ifndef LIBCBIO_CBIO_H
#define LIBCBIO_CBIO_H 1

#include <libcbio/visibility.h>
#include <libcbio/types.h>


#ifdef __cplusplus
extern "C" {
#endif

    LIBCBIO_API
    cbio_error_t cbio_open_handle(const char *name,
                                  libcbio_open_mode_t mode,
                                  libcbio_t *handle);

    /**
     * cbio_close_handle release all allocated resources for the handle
     * and invalidates it.
     */
    LIBCBIO_API
    void cbio_close_handle(libcbio_t handle);

    LIBCBIO_API
    off_t cbio_get_header_position(libcbio_t handle);


    LIBCBIO_API
    cbio_error_t cbio_create_empty_document(libcbio_t handle,
                                            libcbio_document_t *doc);

    /**
     * This is a helper function to avoid having to call
     * cbio_document_release() followed by
     * cbio_create_empty_document(). Please note that you can
     * <b>not</b> call cbio_document_reinitialize() on a document
     * returned from cbio_get_document() (that have undefined behaviour,
     * but will most likely crash your process)
     */
    LIBCBIO_API
    void cbio_document_reinitialize(libcbio_document_t doc);

    LIBCBIO_API
    cbio_error_t cbio_document_set_id(libcbio_document_t doc,
                                      const void *id,
                                      size_t nid,
                                      int allocate);

    LIBCBIO_API
    cbio_error_t cbio_document_set_meta(libcbio_document_t doc,
                                        const void *meta,
                                        size_t nmeta,
                                        int allocate);


    LIBCBIO_API
    cbio_error_t cbio_document_set_revision(libcbio_document_t doc, uint64_t revno);


    LIBCBIO_API
    cbio_error_t cbio_document_set_deleted(libcbio_document_t doc, int deleted);

    LIBCBIO_API
    cbio_error_t cbio_document_set_value(libcbio_document_t doc,
                                         const void *value,
                                         size_t nvalue,
                                         int allocate);

    LIBCBIO_API
    cbio_error_t cbio_document_set_content_type(libcbio_document_t doc,
                                                uint8_t content_type);


    LIBCBIO_API
    cbio_error_t cbio_document_get_id(libcbio_document_t doc,
                                      const void **id,
                                      size_t *nid);

    LIBCBIO_API
    cbio_error_t cbio_document_get_meta(libcbio_document_t doc,
                                        const void **meta,
                                        size_t *nmeta);


    LIBCBIO_API
    cbio_error_t cbio_document_get_revision(libcbio_document_t doc,
                                            uint64_t *revno);


    LIBCBIO_API
    cbio_error_t cbio_document_get_deleted(libcbio_document_t doc, int *deleted);

    LIBCBIO_API
    cbio_error_t cbio_document_get_value(libcbio_document_t doc,
                                         const void **value,
                                         size_t *nvalue);

    LIBCBIO_API
    cbio_error_t cbio_document_get_content_type(libcbio_document_t doc,
                                                uint8_t *content_type);

    LIBCBIO_API
    cbio_error_t cbio_get_document(libcbio_t handle,
                                   const void *id,
                                   size_t nid,
                                   libcbio_document_t *doc);

    LIBCBIO_API
    cbio_error_t cbio_store_document(libcbio_t handle,
                                     libcbio_document_t doc);

    LIBCBIO_API
    cbio_error_t cbio_store_documents(libcbio_t handle,
                                      libcbio_document_t *doc,
                                      size_t ndocs);

    LIBCBIO_API
    void cbio_document_release(libcbio_document_t doc);

    LIBCBIO_API
    cbio_error_t cbio_commit(libcbio_t handle);

    LIBCBIO_API
    const char *cbio_strerror(cbio_error_t err);




    /**
     * The callback function used by cbio_changes_since() to iterate
     * through the documents.
     *
     * The document automatically released if the callback
     * returns 0. A non-zero return value will preserve the document
     * for future use (should be freed with cbio_document_release() by the
     * caller)
     *
     * @param habdle the libcbio handle
     * @param doc the current document
     * @param ctx user context
     * @return 0 or 1. See description above
     */
    typedef int (*cbio_changes_callback_fn)(libcbio_t handle,
                                            libcbio_document_t doc,
                                            void *ctx);

    /**
     * Iterate through the changes since sequence number `since`.
     *
     * @param handle libcbio handle
     * @param since the sequence number to start iterating from
     * @param callback the callback function used to iterate over all changes
     * @param ctx client context (passed to the callback)
     * @return CBIO_SUCCESS upon success
     */
    LIBCBIO_API
    cbio_error_t cbio_changes_since(libcbio_t handle,
                                    uint64_t since,
                                    cbio_changes_callback_fn callback,
                                    void *ctx);

#ifdef __cplusplus
}
#endif

#endif
