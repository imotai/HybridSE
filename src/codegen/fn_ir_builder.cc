/*
 * fn_ir_builder.cc
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

#include "codegen/fn_ir_builder.h"

#include <stack>
#include "codegen/expr_ir_builder.h"
#include "glog/logging.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/IRBuilder.h"

namespace fesql {
namespace codegen {

FnIRBuilder::FnIRBuilder(::llvm::Module *module) : module_(module) {}

FnIRBuilder::~FnIRBuilder() {}

bool FnIRBuilder::Build(const ::fesql::node::FnNodeList *root) {
    if (root == NULL || root->GetType() != ::fesql::node::kFnList) {
        LOG(WARNING) << "node is null";
        return false;
    }
    ::llvm::Function *fn = NULL;
    ::llvm::BasicBlock *block = NULL;
    std::stack<int32_t> indent_stack;
    sv_.Enter("module");
    ::std::vector<::fesql::node::FnNode *>::const_iterator it =
        root->children.begin();
    for (; it != root->children.end(); ++it) {
        ::fesql::node::FnNode *node = *it;
        // TODO(wangtaize) use switch
        if (node->GetType() == ::fesql::node::kFnDef) {
            const ::fesql::node::FnNodeFnDef *fn_def =
                (const ::fesql::node::FnNodeFnDef *)node;
            while (indent_stack.size() > 0 &&
                   indent_stack.top() > fn_def->indent) {
                indent_stack.pop();
                sv_.Exit();
            }
            sv_.Enter(fn_def->name_);
            indent_stack.push(fn_def->indent);
            fn = NULL;
            block = NULL;
            bool ok = BuildFnHead(fn_def, &fn);
            if (!ok) {
                return false;
            }
            block =
                ::llvm::BasicBlock::Create(module_->getContext(), "entry", fn);
        } else {
            bool ok = BuildStmt(indent_stack.top(), node, block);
            if (!ok) {
                return false;
            }
        }
    }
    return true;
}

bool FnIRBuilder::BuildStmt(int32_t pindent, const ::fesql::node::FnNode *node,
                            ::llvm::BasicBlock *block) {
    if (node == NULL || block == NULL) {
        LOG(WARNING) << "node or block is null ";
        return false;
    }

    // TODO(wangtaize) check it before codegen
    if (node->indent - pindent != 4) {
        LOG(WARNING) << "syntax error indent mismatch";
        return false;
    }

    switch (node->GetType()) {
        case ::fesql::node::kFnAssignStmt: {
            return BuildAssignStmt((::fesql::node::FnAssignNode *)node, block);
        }
        case ::fesql::node::kFnReturnStmt: {
            return BuildReturnStmt((::fesql::node::FnReturnStmt *)node, block);
        }
        default: {
            return false;
        }
    }
}

bool FnIRBuilder::BuildReturnStmt(const ::fesql::node::FnReturnStmt *node,
                                  ::llvm::BasicBlock *block) {
    if (node == nullptr || block == nullptr || node->return_expr_ == nullptr) {
        LOG(WARNING) << "node or block or return expr is null";
        return true;
    }

    ExprIRBuilder builder(block, &sv_);
    ::llvm::Value *value = NULL;
    bool ok = builder.Build(node->return_expr_, &value);
    if (!ok) {
        LOG(WARNING) << "fail to build expr ";
        return false;
    }
    ::llvm::IRBuilder<> ir_builder(block);
    ir_builder.CreateRet(value);
    return true;
}

bool FnIRBuilder::BuildAssignStmt(const ::fesql::node::FnAssignNode *node,
                                  ::llvm::BasicBlock *block) {
    if (node == NULL || block == NULL || node->expression_ == nullptr) {
        LOG(WARNING) << "node or block is null";
        return true;
    }
    ExprIRBuilder builder(block, &sv_);
    ::llvm::Value *value = NULL;
    bool ok = builder.Build(node->expression_, &value);
    if (!ok) {
        LOG(WARNING) << "fail to build expr ";
        return false;
    }
    return sv_.AddVar(node->name_, value);
}

bool FnIRBuilder::BuildFnHead(const ::fesql::node::FnNodeFnDef *fn_def,
                              ::llvm::Function **fn) {
    if (fn_def == NULL || fn == NULL) {
        LOG(WARNING) << "input is null";
        return false;
    }

    ::llvm::Type *ret_type = NULL;
    bool ok = MapLLVMType(fn_def->ret_type_, &ret_type);
    if (!ok) {
        LOG(WARNING) << "fail to get llvm type";
        return false;
    }

    std::vector<::llvm::Type *> paras;
    if (nullptr != fn_def->parameters_) {
        bool ok = BuildParas(fn_def->parameters_, paras);
        if (!ok) {
            return false;
        }
    }

    ::llvm::ArrayRef<::llvm::Type *> array_ref(paras);
    ::llvm::FunctionType *fnt =
        ::llvm::FunctionType::get(ret_type, array_ref, false);
    *fn = ::llvm::Function::Create(fnt, ::llvm::Function::ExternalLinkage,
                                   fn_def->name_, module_);
    if (fn_def->parameters_) {
        bool ok = FillArgs(fn_def->parameters_, *fn);
        if (!ok) {
            return false;
        }
    }
    LOG(INFO) << "build fn " << fn_def->name_ << " header done";
    return true;
}

bool FnIRBuilder::FillArgs(const ::fesql::node::FnNodeList *node,
                           ::llvm::Function *fn) {
    if (node == NULL) {
        LOG(WARNING) << "node is null or node type mismatch";
        return false;
    }

    ::llvm::Function::arg_iterator it = fn->arg_begin();
    uint32_t index = 0;
    for (; it != fn->arg_end() && index < node->children.size(); ++it) {
        ::fesql::node::FnParaNode *pnode =
            (::fesql::node::FnParaNode *)node->children[index];
        ::llvm::Argument *argu = &*it;
        bool ok = sv_.AddVar(pnode->GetName(), argu);
        if (!ok) {
            LOG(WARNING) << "fail to define var " << pnode->GetName();
            return false;
        }
        index++;
    }
    return true;
}

bool FnIRBuilder::BuildParas(const ::fesql::node::FnNodeList *node,
                             std::vector<::llvm::Type *> &paras) {
    if (node == NULL) {
        LOG(WARNING) << "node is null or node type mismatch";
        return false;
    }

    for (uint32_t i = 0; i < node->children.size(); i++) {
        ::fesql::node::FnParaNode *pnode =
            (::fesql::node::FnParaNode *)node->children[i];
        ::llvm::Type *type = NULL;
        bool ok = MapLLVMType(pnode->GetParaType(), &type);
        if (!ok) {
            LOG(WARNING) << "fail to get primary type for pname "
                         << pnode->GetName();
            return false;
        }
        paras.push_back(type);
    }
    return true;
}

bool FnIRBuilder::MapLLVMType(const ::fesql::node::DataType &data_type,
                              ::llvm::Type **type) {
    if (type == NULL) {
        LOG(WARNING) << "input type is null";
        return false;
    }

    switch (data_type) {
        case ::fesql::node::kTypeInt16: {
            *type = ::llvm::Type::getInt16Ty(module_->getContext());
            return true;
        }
        case ::fesql::node::kTypeInt32: {
            *type = ::llvm::Type::getInt32Ty(module_->getContext());
            return true;
        }
        case ::fesql::node::kTypeInt64: {
            *type = ::llvm::Type::getInt64Ty(module_->getContext());
        }
        default:
            LOG(WARNING) << "not support";
            return false;
    }
}

}  // namespace codegen
}  // namespace fesql