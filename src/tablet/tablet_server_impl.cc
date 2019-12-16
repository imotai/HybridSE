/*
 * tablet_server_impl.cc
 * Copyright (C) 4paradigm.com 2019 wangtaize <wangtaize@4paradigm.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "tablet/tablet_server_impl.h"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "base/strings.h"

namespace fesql {
namespace tablet {

TabletServerImpl::TabletServerImpl() : slock_(), tables_(), engine_() {}

TabletServerImpl::~TabletServerImpl() {}

bool TabletServerImpl::Init() {
    engine_ = std::move(std::unique_ptr<vm::Engine>(
        new vm::Engine(dynamic_cast<vm::TableMgr*>(this))));
    LOG(INFO) << "init tablet ok";
    return true;
}

void TabletServerImpl::CreateTable(RpcController* ctrl,
                                   const CreateTableRequest* request,
                                   CreateTableResponse* response,
                                   Closure* done) {
    brpc::ClosureGuard done_guard(done);
    ::fesql::common::Status* status = response->mutable_status();
    if (request->pids_size() == 0) {
        status->set_code(common::kBadRequest);
        status->set_msg("create table without pid");
        return;
    }
    if (request->tid() <= 0) {
        status->set_code(common::kBadRequest);
        status->set_msg("create table with invalid tid " +
                        std::to_string(request->tid()));
        return;
    }

    for (int32_t i = 0; i < request->pids_size(); ++i) {
        std::shared_ptr<vm::TableStatus> table_status(new vm::TableStatus(
            request->tid(), request->pids(i), request->db(), request->table()));

        std::unique_ptr<storage::Table> table(new storage::Table(
            request->tid(), request->pids(i), request->table()));

        bool ok = table->Init();

        if (!ok) {
            LOG(WARNING) << "fail to init table storage for table "
                         << request->table().name();
            status->set_code(common::kBadRequest);
            status->set_msg("fail to init table storage");
            return;
        }

        table_status->table = std::move(table);
        ok = AddTableLocked(table_status);
        if (!ok) {
            LOG(WARNING) << "table with name " << table_status->table_def.name()
                         << " exists";
            status->set_code(common::kTableExists);
            status->set_msg("table exist");
            return;
        }
    }
    status->set_code(common::kOk);
    LOG(INFO) << "create table with name " << request->table().name()
              << " done";
}

void TabletServerImpl::Insert(RpcController* ctrl, const InsertRequest* request,
                              InsertResponse* response, Closure* done) {
    brpc::ClosureGuard done_guard(done);
    ::fesql::common::Status* status = response->mutable_status();
    if (request->db().empty() || request->table().empty()) {
        status->set_code(common::kBadRequest);
        status->set_msg("db or table name is empty");
        return;
    }

    std::shared_ptr<vm::TableStatus> table =
        GetTableDef(request->db(), request->table());

    if (!table) {
        status->set_code(common::kTableNotFound);
        status->set_msg("table is not found");
        return;
    }

    DLOG(INFO) << "put key " << request->key() << " value "
               << base::DebugString(request->row());
    bool ok = table->table->Put(request->row().c_str(), request->row().size());
    if (!ok) {
        status->set_code(common::kTablePutFailed);
        status->set_msg("fail to put row");
        LOG(WARNING) << "fail to put data to table " << request->table()
                     << " with key " << request->key();
        return;
    }
    status->set_code(common::kOk);
}

bool TabletServerImpl::AddTableLocked(std::shared_ptr<vm::TableStatus>& table) {
    std::lock_guard<base::SpinMutex> lock(slock_);
    return AddTableUnLocked(table);
}

bool TabletServerImpl::AddTableUnLocked(
    std::shared_ptr<vm::TableStatus>& table) {
    Partition& partition = tables_[table->db][table->tid];
    Partition::iterator it = partition.find(table->pid);
    if (it != partition.end()) {
        return false;
    }
    table_names_[table->db].insert(
        std::make_pair(table->table_def.name(), table->tid));
    partition.insert(std::make_pair(table->pid, table));
    return true;
}

std::shared_ptr<vm::TableStatus> TabletServerImpl::GetTableUnLocked(
    const std::string& db, uint32_t tid, uint32_t pid) {
    Tables::iterator it = tables_.find(db);
    if (it == tables_.end()) {
        return std::shared_ptr<vm::TableStatus>();
    }

    Table::iterator tit = it->second.find(tid);
    if (tit == it->second.end()) {
        return std::shared_ptr<vm::TableStatus>();
    }

    Partition::iterator pit = tit->second.find(pid);
    if (pit == tit->second.end()) {
        return std::shared_ptr<vm::TableStatus>();
    }

    return pit->second;
}

std::shared_ptr<vm::TableStatus> TabletServerImpl::GetTableLocked(
    const std::string& db, uint32_t tid, uint32_t pid) {
    std::lock_guard<base::SpinMutex> lock(slock_);
    return GetTableUnLocked(db, tid, pid);
}

std::shared_ptr<vm::TableStatus> TabletServerImpl::GetTableDefUnLocked(
    const std::string& db, uint32_t tid) {
    Tables::iterator tit = tables_.find(db);
    if (tit == tables_.end()) {
        return std::shared_ptr<vm::TableStatus>();
    }

    Table::iterator pit = tit->second.find(tid);
    if (pit == tit->second.end()) {
        return std::shared_ptr<vm::TableStatus>();
    }

    if (pit->second.size() <= 0) {
        return std::shared_ptr<vm::TableStatus>();
    }
    return pit->second.begin()->second;
}

std::shared_ptr<vm::TableStatus> TabletServerImpl::GetTableDef(
    const std::string& db, uint32_t tid) {
    std::lock_guard<base::SpinMutex> lock(slock_);
    return GetTableDefUnLocked(db, tid);
}

std::shared_ptr<vm::TableStatus> TabletServerImpl::GetTableDef(
    const std::string& db, const std::string& name) {
    std::lock_guard<base::SpinMutex> lock(slock_);
    TableNames::iterator it = table_names_.find(db);
    if (it == table_names_.end()) {
        return std::shared_ptr<vm::TableStatus>();
    }

    std::map<std::string, uint32_t>::iterator iit = it->second.find(name);
    if (iit == it->second.end()) {
        return std::shared_ptr<vm::TableStatus>();
    }

    uint32_t tid = iit->second;
    return GetTableDefUnLocked(db, tid);
}

void TabletServerImpl::Query(RpcController* ctrl, const QueryRequest* request,
                             QueryResponse* response, Closure* done) {
    brpc::ClosureGuard done_guard(done);
    common::Status* status = response->mutable_status();
    status->set_code(common::kOk);
    status->set_msg("ok");
    vm::RunSession session;

    {
        base::Status base_status;
        bool ok =
            engine_->Get(request->sql(), request->db(), session, base_status);
        if (!ok) {
            status->set_msg(base_status.msg);
            status->set_code(base_status.code);
            LOG(WARNING) << status->msg();
            return;
        }
    }

    std::vector<int8_t*> buf;
    buf.reserve(100);
    int32_t code = session.Run(buf, 100);
    if (code != 0) {
        LOG(WARNING) << "fail to run sql " << request->sql();
        status->set_code(common::kSQLError);
        status->set_msg("fail to run sql");
        return;
    }
    // TODO(wangtaize) opt the result buf
    std::vector<int8_t*>::iterator it = buf.begin();
    for (; it != buf.end(); ++it) {
        int8_t* ptr = *it;
        response->add_result_set(ptr, *reinterpret_cast<uint32_t*>(ptr + 2));
        free(ptr);
    }
    // TODO(wangtaize) opt the schema
    std::vector<::fesql::type::ColumnDef>::const_iterator sit =
        session.GetSchema().begin();
    for (; sit != session.GetSchema().end(); ++sit) {
        ::fesql::type::ColumnDef* column = response->add_schema();
        column->CopyFrom(*sit);
    }
    status->set_code(common::kOk);
}
void TabletServerImpl::GetTableSchema(RpcController* ctrl,
                                      const GetTablesSchemaRequest* request,
                                      GetTableSchemaReponse* response,
                                      Closure* done) {
    brpc::ClosureGuard done_guard(done);
    std::shared_ptr<vm::TableStatus> table =
        GetTableDef(request->db(), request->name());

    ::fesql::common::Status* status = response->mutable_status();
    if (!table) {
        status->set_msg("Fail to get table schema");
        status->set_code(common::kBadRequest);
        return;
    }
    *(response->mutable_schema()) = table.get()->table_def;
    LOG(INFO) << response->schema().name();
    return;
}

}  // namespace tablet
}  // namespace fesql