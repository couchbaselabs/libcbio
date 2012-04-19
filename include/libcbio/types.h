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

#ifndef LIBCBIO_TYPES_H
#define LIBCBIO_TYPES_H 1

#ifndef LIBCBIO_CBIO_H
#error "Include libcbio/cbio.h instead"
#endif

#include <sys/types.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct libcbio_st;
    typedef struct libcbio_st *libcbio_t;

    struct libcbio_document_st;
    typedef struct libcbio_document_st *libcbio_document_t;

    typedef enum {
        CBIO_OPEN_RDONLY,
        CBIO_OPEN_RW
    } libcbio_open_mode_t;

    typedef enum {
        CBIO_SUCCESS = 0x00,
        CBIO_ERROR_ENOMEM,
        CBIO_ERROR_EIO,
        CBIO_ERROR_EINVAL,
        CBIO_ERROR_INTERNAL,
        CBIO_ERROR_OPEN_FILE,
        CBIO_ERROR_CORRUPT,
        CBIO_ERROR_ENOENT,
        CBIO_ERROR_NO_HEADER,
        CBIO_ERROR_HEADER_VERSION,
        CBIO_ERROR_CHECKSUM_FAIL
    } cbio_error_t;

#ifdef __cplusplus
}
#endif

#endif
