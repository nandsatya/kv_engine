/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2018 Couchbase, Inc
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
#pragma once

#include "task.h"
#include <memcached/engine_common.h>
#include <memcached/engine_error.h>

class Connection;

class Cookie;

/**
 * Background LIBEVENT tasks which can be run as part of the StatsCommandContext
 * execution.
 */
class StatsTask : public Task {
public:
    StatsTask() = delete;

    StatsTask(const StatsTask&) = delete;

    StatsTask(Connection& connection_,
              Cookie& cookie_,
              const AddStatFn& add_stats_);

    void notifyExecutionComplete() override;

    ENGINE_ERROR_CODE getCommandError() const {
        return command_error;
    }

protected:
    Connection& connection;
    Cookie& cookie;
    AddStatFn add_stats;
    ENGINE_ERROR_CODE command_error;
};

class StatsTaskConnectionStats : public StatsTask {
public:
    StatsTaskConnectionStats() = delete;

    StatsTaskConnectionStats(const StatsTaskConnectionStats&) = delete;

    StatsTaskConnectionStats(Connection& connection_,
                             Cookie& cookie_,
                             const AddStatFn& add_stats_,
                             const int64_t fd_);

    Status execute() override;

protected:
    int64_t fd;
};
