/*
 * This file is open source software, licensed to you under the terms
 * of the Apache License, Version 2.0 (the "License").  See the NOTICE file
 * distributed with this work for additional information regarding copyright
 * ownership.  You may not use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
/*
 * Copyright (C) 2016 ScyllaDB.
 */

#pragma once

#include "metrics.hh"
#include <unordered_map>
#include "sharded.hh"
/*!
 * \file metrics_api.hh
 * \brief header file for metric API layer (like promehteus or collectd)
 *
 *
 *
 */
namespace seastar {
namespace metrics {
namespace impl {
/**
 * Metrics are collected in groups that belongs to some logical entity.
 * For example, different measurements of the cpu, will belong to group "cpu".
 *
 * Name is the metric name like used_objects or used_bytes
 *
 * Inherit type allows customizing one of the basic types (gauge, counter, derive).
 *
 * Instance_id is used to differentiate multiple instance of the metrics.
 * In the seastar environment it is typical to have a metric per shard.
 *
 */

class metric_id {
public:
    metric_id() = default;
    metric_id(group_name_type group, instance_id_type instance, metric_name_type name,
                    metrics::metric_type_def iht = std::string())
                    : _group(std::move(group)), _instance_id(std::move(instance)), _name(
                                    std::move(name)), _inherit_type(std::move(iht)) {
    }
    metric_id(metric_id &&) = default;
    metric_id(const metric_id &) = default;

    metric_id & operator=(metric_id &&) = default;
    metric_id & operator=(const metric_id &) = default;

    const group_name_type & group_name() const {
        return _group;
    }
    void group_name(const group_name_type & name) {
        _group = name;
    }
    const instance_id_type & instance_id() const {
        return _instance_id;
    }
    const metric_name_type & name() const {
        return _name;
    }
    const metrics::metric_type_def & inherit_type() const {
        return _inherit_type;
    }
    bool operator<(const metric_id&) const;
    bool operator==(const metric_id&) const;
private:
    group_name_type _group;
    instance_id_type _instance_id;
    metric_name_type _name;
    metric_type_def _inherit_type;
};
}
}
}

namespace std {

template<>
struct hash<seastar::metrics::impl::metric_id>
{
    typedef seastar::metrics::impl::metric_id argument_type;
    typedef ::std::size_t result_type;
    result_type operator()(argument_type const& s) const
    {
        result_type const h1 ( std::hash<sstring>{}(s.group_name()) );
        result_type const h2 ( std::hash<sstring>{}(s.instance_id()) );
        return h1 ^ (h2 << 1); // or use boost::hash_combine
    }
};

}

namespace seastar {
namespace metrics {
namespace impl {

using metrics_registration = std::vector<metric_id>;

class metric_groups_impl : public metric_groups_def {
    metrics_registration _registration;
public:
    metric_groups_impl() = default;
    ~metric_groups_impl();
    metric_groups_impl(const metric_groups_impl&) = delete;
    metric_groups_impl(metric_groups_impl&&) = default;
    metric_groups_impl& add_metric(group_name_type name, const metric_definition& md);
    metric_groups_impl& add_group(group_name_type name, const std::initializer_list<metric_definition>& l);
    metric_groups_impl& add_group(group_name_type name, const std::vector<metric_definition>& l);
};

class impl;

class registered_metric {
    data_type _type;
    description _d;
    bool _enabled;
    metric_function _f;
    shared_ptr<impl> _impl;
public:
    registered_metric(data_type type, metric_function f, description d = description(), bool enabled=true);
    virtual ~registered_metric() {}
    virtual metric_value operator()() const {
        return _f();
    }
    data_type get_type() const {
        return _type;
    }

    bool is_enabled() const {
        return _enabled;
    }

    void set_enabled(bool b) {
        _enabled = b;
    }

    const description& get_description() const {
        return _d;
    }
};

typedef std::unordered_map<metric_id, shared_ptr<registered_metric> > value_map;
typedef std::unordered_map<metric_id, metric_value> values_copy;

class impl {
    value_map _value_map;
public:
    value_map& get_value_map() {
        return _value_map;
    }

    const value_map& get_value_map() const {
        return _value_map;
    }

    void add_registration(const metric_id& id, shared_ptr<registered_metric> rm);

    future<> stop() {
        return make_ready_future<>();
    }
};

const value_map& get_value_map();

values_copy get_values();

shared_ptr<impl> get_local_impl();

void unregister_metric(const metric_id & id);

/*!
 * \brief initialize metric group
 *
 * Create a metric_group_def.
 * No need to use it directly.
 */
std::unique_ptr<metric_groups_def> create_metric_groups();

}
}
}
