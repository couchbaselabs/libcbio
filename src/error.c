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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

cbio_error_t cbio_remap_error(couchstore_error_t in)
{
    switch (in) {
    case COUCHSTORE_SUCCESS:
        return CBIO_SUCCESS;
    case COUCHSTORE_ERROR_OPEN_FILE:
        return CBIO_ERROR_OPEN_FILE;
    case COUCHSTORE_ERROR_CORRUPT:
        return CBIO_ERROR_CORRUPT;
    case COUCHSTORE_ERROR_ALLOC_FAIL:
        return CBIO_ERROR_ENOMEM;
    case COUCHSTORE_ERROR_READ:
        return CBIO_ERROR_EIO;
    case COUCHSTORE_ERROR_DOC_NOT_FOUND:
        return CBIO_ERROR_ENOENT;
    case COUCHSTORE_ERROR_NO_HEADER:
        return CBIO_ERROR_NO_HEADER;
    case COUCHSTORE_ERROR_WRITE:
        return CBIO_ERROR_EIO;
    case COUCHSTORE_ERROR_HEADER_VERSION:
        return CBIO_ERROR_HEADER_VERSION;
    case COUCHSTORE_ERROR_CHECKSUM_FAIL:
        return CBIO_ERROR_CHECKSUM_FAIL;
    case COUCHSTORE_ERROR_INVALID_ARGUMENTS:
        return CBIO_ERROR_EINVAL;
    case COUCHSTORE_ERROR_NO_SUCH_FILE:
        return CBIO_ERROR_ENOENT;
    default:
        return CBIO_ERROR_INTERNAL;

    }
}

LIBCBIO_API
const char *cbio_strerror(cbio_error_t err)
{
    switch (err) {
    case CBIO_SUCCESS:
        return "success";
    case CBIO_ERROR_ENOMEM:
        return "allocation failed";
    case CBIO_ERROR_EIO:
        return "io error";
    case CBIO_ERROR_EINVAL:
        return "invalid arguments";
    case CBIO_ERROR_OPEN_FILE:
        return "failed to open file";
    case CBIO_ERROR_CORRUPT:
        return "file corrupt";
    case CBIO_ERROR_ENOENT:
        return "no entry";
    case CBIO_ERROR_NO_HEADER:
        return "no header";
    case CBIO_ERROR_HEADER_VERSION:
        return "illegal header version";
    case CBIO_ERROR_CHECKSUM_FAIL:
        return "checksum fail";
    case CBIO_ERROR_INTERNAL:
    default:
        return "Internal error";
    }
}
