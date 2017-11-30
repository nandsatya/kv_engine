/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2015 Couchbase, Inc
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

/* The "crash" bucket is a bucket which simply crashes when it is initialized.
 * It is intended to be used to test crash catching using Google Breakpad.
 */

#include "config.h"

#include <stdlib.h>
#include <stdexcept>
#include <string>

#include <memcached/engine.h>
#include <memcached/visibility.h>
#include <memcached/util.h>
#include <memcached/config_parser.h>
#include <platform/cb_malloc.h>

extern "C" {
MEMCACHED_PUBLIC_API
ENGINE_ERROR_CODE create_instance(uint64_t interface,
                                  GET_SERVER_API gsa,
                                  ENGINE_HANDLE **handle);

MEMCACHED_PUBLIC_API
void destroy_engine(void);
} // extern "C"

struct CrashEngine {
    ENGINE_HANDLE_V1 engine;
    union {
        engine_info eng_info;
        char buffer[sizeof(engine_info) +
                    (sizeof(feature_info) * LAST_REGISTERED_ENGINE_FEATURE)];
    } info;
};

static CrashEngine* get_handle(ENGINE_HANDLE* handle)
{
    return reinterpret_cast<CrashEngine*>(handle);
}

static const engine_info* get_info(gsl::not_null<ENGINE_HANDLE*> handle) {
    return &get_handle(handle)->info.eng_info;
}

// How do I crash thee? Let me count the ways.
enum class CrashMode {
    SegFault,
    UncaughtStdException,
    UncaughtUnknownException
};

static char dummy;

/* Recursive functions which will crash using the given method after
 * 'depth' calls.
 * Note: mutates a dummy global variable to prevent optimization
 * removing the recursion.
 */
EXPORT_SYMBOL
char recursive_crash_function(char depth, CrashMode mode) {
    if (depth == 0) {
        switch (mode) {
        case CrashMode::SegFault: {
            char* death = (char*)0xdeadcbdb;
            return *death + dummy;
        }
        case CrashMode::UncaughtStdException:
            throw std::runtime_error(
                    "crash_engine: This exception wasn't handled");
        case CrashMode::UncaughtUnknownException:
            // Crash via exception not derived from std::exception
            class UnknownException {};
            throw UnknownException();
        }
    }
    recursive_crash_function(depth - 1, mode);
    return dummy++;
}

/* 'initializes' this engine - given this is the crash_engine that
 * means crashing it.
 */
static ENGINE_ERROR_CODE initialize(gsl::not_null<ENGINE_HANDLE*> handle,
                                    const char* config_str) {
    (void)handle;
    (void)config_str;
    std::string mode_string(getenv("MEMCACHED_CRASH_TEST"));
    CrashMode mode;
    if (mode_string == "segfault") {
        mode = CrashMode::SegFault;
    } else if (mode_string == "std_exception") {
        mode = CrashMode::UncaughtStdException;
    } else if (mode_string == "unknown_exception") {
        mode = CrashMode::UncaughtUnknownException;
    } else {
        fprintf(stderr, "crash_engine::initialize: could not find a valid "
                "CrashMode from MEMCACHED_CRASH_TEST env var ('%s')\n",
                mode_string.c_str());
        exit(1);
    }
    return ENGINE_ERROR_CODE(recursive_crash_function(25, mode));
}

static void destroy(gsl::not_null<ENGINE_HANDLE*> handle, const bool force) {
    (void)force;
    cb_free(handle);
}

static cb::EngineErrorItemPair item_allocate(
        gsl::not_null<ENGINE_HANDLE*> handle,
        gsl::not_null<const void*> cookie,
        const DocKey& key,
        const size_t nbytes,
        const int flags,
        const rel_time_t exptime,
        uint8_t datatype,
        uint16_t vbucket) {
    return cb::makeEngineErrorItemPair(cb::engine_errc::failed);
}

static std::pair<cb::unique_item_ptr, item_info> item_allocate_ex(
        gsl::not_null<ENGINE_HANDLE*> handle,
        gsl::not_null<const void*> cookie,
        const DocKey& key,
        size_t nbytes,
        size_t priv_nbytes,
        int flags,
        rel_time_t exptime,
        uint8_t datatype,
        uint16_t vbucket) {
    throw cb::engine_error{cb::engine_errc::failed, "crash_engine"};
}

static ENGINE_ERROR_CODE item_delete(gsl::not_null<ENGINE_HANDLE*> handle,
                                     gsl::not_null<const void*> cookie,
                                     const DocKey& key,
                                     uint64_t& cas,
                                     uint16_t vbucket,
                                     mutation_descr_t& mut_info) {
    return ENGINE_FAILED;
}

static void item_release(gsl::not_null<ENGINE_HANDLE*> handle,
                         gsl::not_null<item*> item) {
}

static cb::EngineErrorItemPair get(gsl::not_null<ENGINE_HANDLE*> handle,
                                   const void* cookie,
                                   const DocKey& key,
                                   uint16_t vbucket,
                                   DocStateFilter) {
    return cb::makeEngineErrorItemPair(cb::engine_errc::failed);
}

static cb::EngineErrorItemPair get_if(gsl::not_null<ENGINE_HANDLE*> handle,
                                      gsl::not_null<const void*>,
                                      const DocKey&,
                                      uint16_t,
                                      std::function<bool(const item_info&)>) {
    return cb::makeEngineErrorItemPair(cb::engine_errc::failed);
}

static cb::EngineErrorItemPair get_and_touch(
        gsl::not_null<ENGINE_HANDLE*> handle,
        gsl::not_null<const void*> cookie,
        const DocKey&,
        uint16_t,
        uint32_t) {
    return cb::makeEngineErrorItemPair(cb::engine_errc::failed);
}

static cb::EngineErrorItemPair get_locked(gsl::not_null<ENGINE_HANDLE*> handle,
                                          gsl::not_null<const void*> cookie,
                                          const DocKey& key,
                                          uint16_t vbucket,
                                          uint32_t lock_timeout) {
    return cb::makeEngineErrorItemPair(cb::engine_errc::failed);
}

static ENGINE_ERROR_CODE unlock(gsl::not_null<ENGINE_HANDLE*> handle,
                                gsl::not_null<const void*> cookie,
                                const DocKey& key,
                                uint16_t vbucket,
                                uint64_t cas) {
    return ENGINE_FAILED;
}

static ENGINE_ERROR_CODE get_stats(gsl::not_null<ENGINE_HANDLE*> handle,
                                   gsl::not_null<const void*> cookie,
                                   cb::const_char_buffer key,
                                   ADD_STAT add_stat) {
    return ENGINE_FAILED;
}

static ENGINE_ERROR_CODE store(gsl::not_null<ENGINE_HANDLE*> handle,
                               const void* cookie,
                               gsl::not_null<item*> item,
                               gsl::not_null<uint64_t*> cas,
                               ENGINE_STORE_OPERATION operation,
                               DocumentState) {
    return ENGINE_FAILED;
}

static cb::EngineErrorCasPair store_if(gsl::not_null<ENGINE_HANDLE*> handle,
                                       gsl::not_null<const void*> cookie,
                                       gsl::not_null<item*> item,
                                       uint64_t cas,
                                       ENGINE_STORE_OPERATION operation,
                                       cb::StoreIfPredicate,
                                       DocumentState) {
    return {cb::engine_errc::failed, 0};
}

static ENGINE_ERROR_CODE flush(gsl::not_null<ENGINE_HANDLE*> handle,
                               gsl::not_null<const void*> cookie) {
    return ENGINE_FAILED;
}

static void reset_stats(gsl::not_null<ENGINE_HANDLE*> handle,
                        gsl::not_null<const void*> cookie) {
}

static void item_set_cas(gsl::not_null<ENGINE_HANDLE*> handle,
                         gsl::not_null<item*> item,
                         uint64_t val) {
}

static bool get_item_info(gsl::not_null<ENGINE_HANDLE*> handle,
                          gsl::not_null<const item*> item,
                          gsl::not_null<item_info*> item_info) {
    return false;
}

static bool set_item_info(gsl::not_null<ENGINE_HANDLE*> handle,
                          gsl::not_null<item*> item,
                          gsl::not_null<const item_info*> itm_info) {
    return false;
}

static bool is_xattr_enabled(gsl::not_null<ENGINE_HANDLE*> handle) {
    return true;
}

ENGINE_ERROR_CODE create_instance(uint64_t interface,
                                  GET_SERVER_API gsa,
                                  ENGINE_HANDLE **handle)
{
    CrashEngine* engine;
    (void)gsa;

    if (interface != 1) {
        return ENGINE_ENOTSUP;
    }

    if ((engine = reinterpret_cast<CrashEngine*>(cb_calloc(1, sizeof(*engine)))) == NULL) {
        return ENGINE_ENOMEM;
    }

    engine->engine.interface.interface = 1;
    engine->engine.get_info = get_info;
    engine->engine.initialize = initialize;
    engine->engine.destroy = destroy;
    engine->engine.allocate = item_allocate;
    engine->engine.allocate_ex = item_allocate_ex;
    engine->engine.remove = item_delete;
    engine->engine.release = item_release;
    engine->engine.get = get;
    engine->engine.get_if = get_if;
    engine->engine.get_and_touch = get_and_touch;
    engine->engine.get_locked = get_locked;
    engine->engine.unlock = unlock;
    engine->engine.get_stats = get_stats;
    engine->engine.reset_stats = reset_stats;
    engine->engine.store = store;
    engine->engine.store_if = store_if;
    engine->engine.flush = flush;
    engine->engine.item_set_cas = item_set_cas;
    engine->engine.get_item_info = get_item_info;
    engine->engine.set_item_info = set_item_info;
    engine->engine.isXattrEnabled = is_xattr_enabled;
    engine->info.eng_info.description = "Crash Engine";
    engine->info.eng_info.num_features = 0;
    *handle = reinterpret_cast<ENGINE_HANDLE*>(&engine->engine);
    return ENGINE_SUCCESS;
}

void destroy_engine(){

}
