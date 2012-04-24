/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
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

#include <libcbio/cbio.h>

#include <cstdlib>
#include <gtest/gtest.h>

#include <cerrno>
#include <cstring>

using namespace std;

static const char dbfile[] = "testcase.couch";

// The fixture for testing class Foo.
class LibcbioTest : public ::testing::Test {
protected:
    LibcbioTest() {}
    virtual ~LibcbioTest() {}
    virtual void SetUp(void) { removeDb(); }
    virtual void TearDown(void) { removeDb(); }

protected:
    void removeDb(void) {
        EXPECT_EQ(0, (remove(dbfile) == -1 && errno != ENOENT))
            << "Failed to remove test case files: " << strerror(errno);
    }
};

class LibcbioOpenTest : public LibcbioTest {};

TEST_F(LibcbioOpenTest, HandleEmptyNameOpenRDONLY) {
    libcbio_t handle;
    EXPECT_EQ(CBIO_ERROR_ENOENT,
              cbio_open_handle("", CBIO_OPEN_RDONLY, &handle));
}

TEST_F(LibcbioOpenTest, HandleNullNameOpenRDONLY) {
    libcbio_t handle;
    EXPECT_EQ(CBIO_ERROR_EINVAL,
              cbio_open_handle(NULL, CBIO_OPEN_RDONLY, &handle));
}

TEST_F(LibcbioOpenTest, HandleNonexistentNameOpenRDONLY) {
    libcbio_t handle;
    EXPECT_EQ(CBIO_ERROR_ENOENT,
              cbio_open_handle("/this/path/should/not/exist",
                               CBIO_OPEN_RDONLY, &handle));
}

TEST_F(LibcbioOpenTest, HandleEmptyNameOpenRW) {
    libcbio_t handle;
    EXPECT_EQ(CBIO_ERROR_ENOENT,
              cbio_open_handle("", CBIO_OPEN_RW, &handle));
}

TEST_F(LibcbioOpenTest, HandleNullNameOpenRW) {
    libcbio_t handle;
    EXPECT_EQ(CBIO_ERROR_EINVAL,
              cbio_open_handle(NULL, CBIO_OPEN_RW, &handle));
}

TEST_F(LibcbioOpenTest, HandleNonexistentNameOpenRW) {
    libcbio_t handle;
    EXPECT_EQ(CBIO_ERROR_ENOENT,
              cbio_open_handle("/this/path/should/not/exist",
                               CBIO_OPEN_RW, &handle));
}

TEST_F(LibcbioOpenTest, HandleEmptyNameOpenCREATE) {
    libcbio_t handle;
    EXPECT_EQ(CBIO_ERROR_ENOENT,
              cbio_open_handle("", CBIO_OPEN_CREATE, &handle));
}

TEST_F(LibcbioOpenTest, HandleNullNameOpenCREATE) {
    libcbio_t handle;
    EXPECT_EQ(CBIO_ERROR_EINVAL,
              cbio_open_handle(NULL, CBIO_OPEN_CREATE, &handle));
}

TEST_F(LibcbioOpenTest, HandleNonexistentNameOpenCREATE) {
    libcbio_t handle;
    EXPECT_EQ(CBIO_ERROR_ENOENT,
              cbio_open_handle("/this/path/should/not/exist",
                               CBIO_OPEN_CREATE, &handle));
}

class LibcbioCreateDatabaseTest : public LibcbioTest {};


TEST_F(LibcbioCreateDatabaseTest, createDatabase) {
    libcbio_t handle;
    EXPECT_EQ(CBIO_SUCCESS,
              cbio_open_handle(dbfile, CBIO_OPEN_CREATE, &handle));

    cbio_close_handle(handle);
}

TEST_F(LibcbioCreateDatabaseTest, reopenDatabaseReadOnly) {
    libcbio_t handle;
    EXPECT_EQ(CBIO_SUCCESS,
              cbio_open_handle(dbfile, CBIO_OPEN_CREATE, &handle));
    cbio_close_handle(handle);

    EXPECT_EQ(CBIO_SUCCESS,
              cbio_open_handle(dbfile, CBIO_OPEN_RDONLY, &handle));

    cbio_close_handle(handle);
}

TEST_F(LibcbioCreateDatabaseTest, reopenDatabaseReadWrite) {
    libcbio_t handle;
    EXPECT_EQ(CBIO_SUCCESS,
              cbio_open_handle(dbfile, CBIO_OPEN_CREATE, &handle));
    cbio_close_handle(handle);

    EXPECT_EQ(CBIO_SUCCESS,
              cbio_open_handle(dbfile, CBIO_OPEN_RW, &handle));

    cbio_close_handle(handle);
}

TEST_F(LibcbioCreateDatabaseTest, reopenDatabaseCreate) {
    libcbio_t handle;
    EXPECT_EQ(CBIO_SUCCESS,
              cbio_open_handle(dbfile, CBIO_OPEN_CREATE, &handle));
    cbio_close_handle(handle);

    EXPECT_EQ(CBIO_SUCCESS,
              cbio_open_handle(dbfile, CBIO_OPEN_CREATE, &handle));

    cbio_close_handle(handle);
}

// @todo create a test suite to test the document!

class LibcbioDataAccessTest : public LibcbioTest {
public:
    LibcbioDataAccessTest() {
        blob = new char[8192];
        blobsize = 8192;
        maxdoc = 10000;
    }

    virtual ~LibcbioDataAccessTest() {
        delete []blob;
    }

    virtual void SetUp(void) {
        removeDb();
        ASSERT_EQ(CBIO_SUCCESS,
                  cbio_open_handle(dbfile, CBIO_OPEN_CREATE, &handle));
    }
    virtual void TearDown(void) {
        cbio_close_handle(handle);
        removeDb();
    }

protected:

    void storeSingleDocument(const string &key, const string &value) {
        libcbio_document_t doc;
        EXPECT_EQ(CBIO_SUCCESS,
                  cbio_create_empty_document(handle, &doc));
        EXPECT_EQ(CBIO_SUCCESS,
                  cbio_document_set_id(doc, key.data(), key.length(), 0));
        EXPECT_EQ(CBIO_SUCCESS,
                  cbio_document_set_value(doc, value.data(),
                                          value.length(), 0));
        EXPECT_EQ(CBIO_SUCCESS,
                  cbio_store_document(handle, doc));
        EXPECT_EQ(CBIO_SUCCESS,
                  cbio_commit(handle));
        cbio_document_release(doc);
    }

    void deleteSingleDocument(const string &key) {
        libcbio_document_t doc;
        EXPECT_EQ(CBIO_SUCCESS,
                  cbio_create_empty_document(handle, &doc));
        EXPECT_EQ(CBIO_SUCCESS,
                  cbio_document_set_id(doc, key.data(), key.length(), 0));
        EXPECT_EQ(CBIO_SUCCESS,
                  cbio_document_set_deleted(doc, 1));
        EXPECT_EQ(CBIO_SUCCESS,
                  cbio_store_document(handle, doc));
        EXPECT_EQ(CBIO_SUCCESS,
                  cbio_commit(handle));
        cbio_document_release(doc);
    }

    void validateExistingDocument(const string &key, const string &value) {
        libcbio_document_t doc;
        EXPECT_EQ(CBIO_SUCCESS,
                  cbio_get_document(handle, key.data(), key.length(), &doc));
        const void *ptr;
        size_t nbytes;
        EXPECT_EQ(CBIO_SUCCESS,
                  cbio_document_get_value(doc, &ptr, &nbytes));
        EXPECT_EQ(value.length(), nbytes);
        EXPECT_EQ(0, memcmp(value.data(), ptr, nbytes));
    }

    void validateNonExistingDocument(const string &key) {
        libcbio_document_t doc;
        EXPECT_EQ(CBIO_ERROR_ENOENT,
                  cbio_get_document(handle, key.data(), key.length(), &doc));
    }

    string generateKey(int id) {
        stringstream ss;
        ss << "mykey-" << id;
        return ss.str();
    }

    libcbio_document_t generateRandomDocument(int id) {
        libcbio_document_t doc;
        EXPECT_EQ(CBIO_SUCCESS,
                  cbio_create_empty_document(handle, &doc));
        string key = generateKey(id);

        EXPECT_EQ(CBIO_SUCCESS,
                  cbio_document_set_id(doc, key.data(), key.length(), 1));

        EXPECT_EQ(CBIO_SUCCESS,
                  cbio_document_set_value(doc, blob, random() % blobsize, 0));

        return doc;
    }


    void bulkStoreDocuments(void) {
        const int chunksize = 1000;
        libcbio_document_t *docs = new libcbio_document_t[chunksize];
        int total = 0;
        do {
            int currtx = random() % chunksize;

            if (total + currtx > maxdoc) {
                currtx = maxdoc - total;
            }

            for (int ii = 0; ii < currtx; ++ii) {
                docs[ii] = generateRandomDocument(total + ii);
            }

            EXPECT_EQ(CBIO_SUCCESS, cbio_store_documents(handle, docs, currtx));
            EXPECT_EQ(CBIO_SUCCESS, cbio_commit(handle));
            total += currtx;

            for (int ii = 0; ii < currtx; ++ii) {
                cbio_document_release(docs[ii]);
            }
        } while (total < maxdoc);

        for (int ii = 0; ii < maxdoc; ++ii) {
            libcbio_document_t doc;
            string key = generateKey(ii);
            EXPECT_EQ(CBIO_SUCCESS,
                      cbio_get_document(handle, key.data(), key.length(), &doc));
            cbio_document_release(doc);
        }

        cbio_close_handle(handle);
    }

    char *blob;
    size_t blobsize;
    int maxdoc;
    libcbio_t handle;
};

TEST_F(LibcbioDataAccessTest, getMiss) {
    validateNonExistingDocument("key");
}

TEST_F(LibcbioDataAccessTest, storeSingleDocument) {
    storeSingleDocument("key", "value");
}

TEST_F(LibcbioDataAccessTest, getHit) {
    storeSingleDocument("key", "value");
    validateExistingDocument("key", "value");
}

TEST_F(LibcbioDataAccessTest, deleteNonExistingDocument) {
    string key = "key";
    deleteSingleDocument(key);
    validateNonExistingDocument(key);
}

TEST_F(LibcbioDataAccessTest, deleteExistingDocument) {
    string key = "key";
    string value = "value";
    storeSingleDocument(key, value);
    validateExistingDocument(key, value);
    deleteSingleDocument(key);
    validateNonExistingDocument(key);
}

TEST_F(LibcbioDataAccessTest, testBulkStoreDocuments) {
    bulkStoreDocuments();
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    exit(RUN_ALL_TESTS());
}


