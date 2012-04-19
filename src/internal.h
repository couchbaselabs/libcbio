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
#ifndef LIBCBIO_INTERNAL
#error "This is a private interface to libcbio!"
#endif

#include <libcbio/cbio.h>
#include <libcouchstore/couch_db.h>

#ifndef INTERNAL_H
#define INTERNAL_H 1

#ifdef __cplusplus
#error "What are you thinking?? this is a C project"
#endif

struct libcbio_st {
    Db *couchstore_handle;
    libcbio_open_mode_t mode;
};

struct libcbio_document_st {
    Doc *doc;
    DocInfo *info;

    void *tmp_alloc_id;
    void *tmp_alloc_meta;
    void *tmp_alloc_bp;
    int scratch;
};

cbio_error_t cbio_remap_error(couchstore_error_t in);

#endif
