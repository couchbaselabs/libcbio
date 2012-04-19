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
#include <stdlib.h>
#include <stdio.h>
#include <libcbio/cbio.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

void *blob;
size_t blobsize;

const char *dbfile = "testcase.cbio";

typedef int (*testcase)(void);
struct test {
    const char *name;
    testcase func;
};

int verbose;
int core;

static void report(const char *fmt, ...)
{
    if (verbose) {
        va_list ap;

        va_start(ap, fmt);
        (void)vfprintf(stderr, fmt, ap);
        va_end(ap);
        fprintf(stderr, " ");
        fflush(stderr);
    }

    if (core) {
        abort();
    }
}

static int open_empty_filename(void)
{
    libcbio_t handle;
    cbio_error_t err = cbio_open_handle("", CBIO_OPEN_RDONLY, &handle);
    if (err != CBIO_ERROR_ENOENT) {
        report("Expected open of \"\" to fail, but it \"%s\" ",
               cbio_strerror(err));
        return 1;
    }

    return 0;
}

static int create_database(void)
{
    libcbio_t handle;
    cbio_error_t err = cbio_open_handle(dbfile,
                                        CBIO_OPEN_RDONLY,
                                        &handle);
    if (err != CBIO_ERROR_ENOENT) {
        report("Expected open of \"%s\" to fail, but it \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    err = cbio_open_handle(dbfile, CBIO_OPEN_RW, &handle);
    if (err != CBIO_SUCCESS) {
        report("Expected open of \"%s\" to succeed, but it \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    cbio_close_handle(handle);
    err = cbio_open_handle(dbfile, CBIO_OPEN_RDONLY, &handle);
    if (err != CBIO_SUCCESS) {
        report("Expected open of \"%s\" to succeed after I created it, "
               "but it \"%s\"", cbio_strerror(err));
        return 1;
    }
    cbio_close_handle(handle);

    return 0;
}

static int get_miss(void)
{
    libcbio_t handle;
    libcbio_document_t doc;
    cbio_error_t err;

    err = cbio_open_handle(dbfile, CBIO_OPEN_RW, &handle);
    if (err != CBIO_SUCCESS) {
        report("Expected open of \"%s\" to succeed, but it \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    err = cbio_get_document(handle, "hi-there", sizeof("hi-there"), &doc);
    if (err != CBIO_ERROR_ENOENT) {
        report("I did not expect to find \"hi-there\" in the database"
               ", but I got \"%s\"", cbio_strerror(err));
        return 1;
    }

    cbio_close_handle(handle);
    return 0;
}

static int store_document(void)
{
    libcbio_t handle;
    libcbio_document_t doc;
    cbio_error_t err;

    err = cbio_open_handle(dbfile, CBIO_OPEN_RW, &handle);
    if (err != CBIO_SUCCESS) {
        report("Expected open of \"%s\" to succeed, but it \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    err = cbio_create_empty_document(handle, &doc);
    if (err != CBIO_SUCCESS) {
        report("Failed to create an empty document: \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    err = cbio_document_set_id(doc, "hi-there", sizeof("hi-there"), 0);
    if (err != CBIO_SUCCESS) {
        report("Failed to set document id \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    err = cbio_document_set_revision(doc, 0);
    if (err != CBIO_SUCCESS) {
        report("Failed to set revision \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    err = cbio_document_set_value(doc, "hei", 3, 0);
    if (err != CBIO_SUCCESS) {
        report("Failed ot set value \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    err = cbio_store_document(handle, doc);
    if (err != CBIO_SUCCESS) {
        report("Failed to store document \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    cbio_commit(handle);
    cbio_document_release(doc);
    cbio_close_handle(handle);
    return 0;
}

static int get_hit(void)
{
    libcbio_t handle;
    libcbio_document_t doc;
    cbio_error_t err;

    if (get_miss() != 0 || store_document() != 0) {
        /* Error already reported */
        return 1;
    }

    err = cbio_open_handle(dbfile, CBIO_OPEN_RDONLY, &handle);
    if (err != CBIO_SUCCESS) {
        report("Expected open of \"%s\" to succeed, but it \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    err = cbio_get_document(handle, "hi-there", sizeof("hi-there"), &doc);
    if (err != CBIO_SUCCESS) {
        report("Expected to find the document, but I got \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    cbio_document_release(doc);
    cbio_close_handle(handle);
    return 0;
}

static int delete_document(void)
{
    libcbio_t handle;
    libcbio_document_t doc;
    cbio_error_t err;

    if (get_miss() != 0 || store_document() != 0) {
        /* Error already reported */
        return 1;
    }

    err = cbio_open_handle(dbfile, CBIO_OPEN_RW, &handle);
    if (err != CBIO_SUCCESS) {
        report("Expected open of \"%s\" to succeed, but it \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    err = cbio_create_empty_document(handle, &doc);
    if (err != CBIO_SUCCESS) {
        report("Failed to create an empty document: \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    err = cbio_document_set_id(doc, "hi-there", sizeof("hi-there"), 0);
    if (err != CBIO_SUCCESS) {
        report("Failed to set document id \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    err = cbio_document_set_revision(doc, 1);
    if (err != CBIO_SUCCESS) {
        report("Failed to set revision \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    err = cbio_document_set_deleted(doc, 1);
    if (err != CBIO_SUCCESS) {
        report("Failed to set deleted \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    err = cbio_store_document(handle, doc);
    if (err != CBIO_SUCCESS) {
        report("Failed to store document \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    cbio_commit(handle);
    cbio_document_release(doc);
    cbio_close_handle(handle);

    return 0;
}

static int delete_nonexistent_document(void)
{
    libcbio_t handle;
    libcbio_document_t doc;
    cbio_error_t err;

    err = cbio_open_handle(dbfile, CBIO_OPEN_RW, &handle);
    if (err != CBIO_SUCCESS) {
        report("Expected open of \"%s\" to succeed, but it \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    err = cbio_create_empty_document(handle, &doc);
    if (err != CBIO_SUCCESS) {
        report("Failed to create an empty document: \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    err = cbio_document_set_id(doc, "wtf", sizeof("wtf"), 0);
    if (err != CBIO_SUCCESS) {
        report("Failed to set document id \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    err = cbio_document_set_revision(doc, 1);
    if (err != CBIO_SUCCESS) {
        report("Failed to set revision \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    err = cbio_document_set_deleted(doc, 1);
    if (err != CBIO_SUCCESS) {
        report("Failed to set deleted \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    err = cbio_store_document(handle, doc);
    if (err != CBIO_SUCCESS) {
        report("Failed to store document \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    cbio_commit(handle);
    cbio_document_release(doc);
    cbio_close_handle(handle);
    return 0;
}

static int get_deleted_document(void)
{
    return (delete_document() != 0 || get_miss() != 0) ? 1 : 0;
}

static int create_random_doc(libcbio_t handle, int idx, libcbio_document_t *doc)
{
    cbio_error_t err;

    err = cbio_create_empty_document(handle, doc);
    if (err != CBIO_SUCCESS) {
        report("Failed to create an empty document: \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    char id[20];
    int len = snprintf(id, sizeof(id), "%d", idx);
    err = cbio_document_set_id(*doc, id, len, 1);
    if (err != CBIO_SUCCESS) {
        report("Failed to set document id \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    err = cbio_document_set_revision(*doc, 1);
    if (err != CBIO_SUCCESS) {
        report("Failed to set revision \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    err = cbio_document_set_value(*doc, blob, random() % blobsize, 0);
    if (err != CBIO_SUCCESS) {
        report("Failed ot set value \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    return 0;
}

const int maxdoc = 100000;

static int bulk_store_documents(void)
{
    const int chunksize = 1000;

    cbio_error_t err;
    libcbio_document_t *docs = calloc(chunksize, sizeof(libcbio_document_t));
    blobsize = 8192;
    blob = malloc(blobsize);

    if (docs == NULL || blob == NULL) {
        report("Failed to allocate memory to keep track of documents");
        return 1;
    }
    libcbio_t handle;

    err = cbio_open_handle(dbfile, CBIO_OPEN_RW, &handle);
    if (err != CBIO_SUCCESS) {
        report("Failed to open handle \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    srand(0);

    int total = 0;
    do {
        int currtx = random() % chunksize;

        if (total + currtx > maxdoc) {
            currtx = maxdoc - total;
        }

        for (int ii = 0; ii < currtx; ++ii) {
            libcbio_document_t doc;
            if (create_random_doc(handle, total + ii, &doc)) {
                report("Failed to create a document");
                return 1;
            }
            docs[ii] = doc;
        }

        fprintf(stdout, "\rStoring %d of %d..", currtx, total);
        fflush(stdout);
        err = cbio_store_documents(handle, docs, currtx);
        if (err != CBIO_SUCCESS) {
            report("Failed to store document \"%s\"",
                   cbio_strerror(err));
            return 1;
        }

        cbio_commit(handle);
        total += currtx;

        for (int ii = 0; ii < currtx; ++ii) {
            cbio_document_release(docs[ii]);
        }
    } while (total < maxdoc);

    fprintf(stdout, "\r                                     \r");
    for (int ii = 0; ii < maxdoc; ++ii) {
        libcbio_document_t doc;
        char id[20];
        int len = snprintf(id, sizeof(id), "%d", ii);
        fprintf(stdout, "\rVerify %d....", ii);
        err = cbio_get_document(handle, id, len, &doc);
        if (err != CBIO_SUCCESS) {
            report("Expected to find the document \"%s\", but I got \"%s\"",
                   id, cbio_strerror(err));
            /* return 1; */
        } else {
            cbio_document_release(doc);
        }
    }
    fprintf(stdout, "\r                        \r");

    cbio_close_handle(handle);

    free(docs);
    return 0;
}

static int count_callback(libcbio_t handle, libcbio_document_t doc, void *ctx)
{
    (void)handle;
    (void)doc;
    int *count = ctx;
    (*count)++;
    return 0;
}


static int test_changes_since(void)
{
    libcbio_t handle;
    cbio_error_t err;
    err = cbio_open_handle(dbfile, CBIO_OPEN_RW, &handle);
    if (err != CBIO_SUCCESS) {
        report("Failed to open handle \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    uint64_t offset = (uint64_t)cbio_get_header_position(handle);

    fprintf(stderr, "Creating test database");
    fflush(stderr);
    if (bulk_store_documents() != 0) {
        return 1;
    }

    fprintf(stderr, "\r                      \r");
    fflush(stderr);

    err = cbio_open_handle(dbfile, CBIO_OPEN_RDONLY, &handle);
    if (err != CBIO_SUCCESS) {
        report("Failed to open handle \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    int total = 0;
    err = cbio_changes_since(handle, offset, count_callback, &total);
    if (err != CBIO_SUCCESS) {
        report("Failed to call cbio_changes_since \"%s\"",
               cbio_strerror(err));
        return 1;
    }

    if (total != maxdoc) {
        report("changes since did not report all of the changes %d of %d",
               total, maxdoc);
        return 1;
    }

    cbio_close_handle(handle);
    return 0;
}

static void remove_dbfiles(void)
{
    if (remove(dbfile) == -1 && errno != ENOENT) {
        report("Failed to remove test case files: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

static const char *get_testcase(const char *fname)
{
    char *ptr = strrchr(fname, '/');
    if (ptr == NULL) {
        return fname;
    }
    return ptr + 1;
}

static struct test tests[] = {
    { .name = "test_open_empty_filename", .func = open_empty_filename },
    { .name = "test_create_database", .func = create_database },
    { .name = "test_get_miss", .func = get_miss },
    { .name = "test_store_single_document", .func = store_document },
    { .name = "test_get_hit", .func = get_hit },
    { .name = "test_delete_document", .func = delete_document },
    { .name = "test_delete_nonexistent_document", .func = delete_nonexistent_document },
    { .name = "test_get_deleted_document", .func = get_deleted_document },
    { .name = "test_bulk_store_documents", .func = bulk_store_documents },
    { .name = "test_changes_since", .func = test_changes_since },
    { .name = NULL, .func = NULL }
};

static void run_single(const char *name)
{
    remove_dbfiles();
    for (int ii = 0; tests[ii].name != NULL; ++ii) {
        if (strcmp(name, tests[ii].name) == 0) {
            if (tests[ii].func() != 0) {
                fprintf(stdout, "FAILED\n");
                exit(EXIT_FAILURE);
            }
            remove_dbfiles();
            exit(EXIT_SUCCESS);
        }
    }

    fprintf(stdout, "Failed to locate a testcase named: \"%s\" ", name);
    exit(EXIT_FAILURE);
}

static void run_all(void)
{
    char line[80];
    int ii = 0;
    int failed = 0;
    int total;

    memset(line, ' ', sizeof(line));
    line[sizeof(line) - 1] = '\0';

    while (tests[ii].name != NULL) {
        ++ii;
    }

    total = ii;
    remove_dbfiles();
    for (ii = 0; ii < total; ++ii) {
        fprintf(stdout, "\r%s\r%s - [%d/%d]: ", line, tests[ii].name,
                ii + 1, total);
        fflush(stdout);
        if (tests[ii].func() != 0) {
            fprintf(stdout, "FAILED\n");
            ++failed;
        }
        remove_dbfiles();
    }

    if (failed == 0) {
        fprintf(stdout, "SUCCESS\n");
        exit(EXIT_SUCCESS);
    } else {
        fprintf(stdout, "%d of %d tests failed\n", failed, total);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv)
{
    verbose = getenv("LIBCBIO_TESTS_VERBOSE") ? 1 : 0;
    core = getenv("LIBCBIO_TESTS_COREDUMP") ? 1 : 0;

    ++verbose;

    if (argc == 1) {
        run_single(get_testcase(argv[0]));
    } else {
        run_all();
    }

    /* Not reached */
    return 0;
}
