/*-------------------------------------------------------------------------
 * Copyright (C) 2019, 4paradigm
 * sql_node.cc
 *
 * Author: chenjing
 * Date: 2019/10/11
 *--------------------------------------------------------------------------
 **/

#include "node/sql_node.h"
#include <numeric>
#include <utility>
#include "glog/logging.h"
#include "node/node_manager.h"
namespace fesql {
namespace node {

void PrintSQLNode(std::ostream &output, const std::string &org_tab,
                  const SQLNode *node_ptr, const std::string &item_name,
                  bool last_child) {
    output << org_tab << SPACE_ST << item_name << ":";

    if (nullptr == node_ptr) {
        output << " null";
    } else if (last_child) {
        output << "\n";
        node_ptr->Print(output, org_tab + INDENT);
    } else {
        output << "\n";
        node_ptr->Print(output, org_tab + OR_INDENT);
    }
}
void PrintSQLVector(std::ostream &output, const std::string &tab,
                    const std::vector<FnNode *> &vec,
                    const std::string &vector_name, bool last_item) {
    if (0 == vec.size()) {
        output << tab << SPACE_ST << vector_name << ": []";
        return;
    }
    output << tab << SPACE_ST << vector_name << "[list]: \n";
    const std::string space = last_item ? (tab + INDENT) : tab + OR_INDENT;
    int count = vec.size();
    int i = 0;
    for (i = 0; i < count - 1; ++i) {
        PrintSQLNode(output, space, vec[i], "" + std::to_string(i), false);
        output << "\n";
    }
    PrintSQLNode(output, space, vec[i], "" + std::to_string(i), true);
}
void PrintSQLVector(std::ostream &output, const std::string &tab,
                    const std::vector<ExprNode *> &vec,
                    const std::string &vector_name, bool last_item) {
    if (0 == vec.size()) {
        output << tab << SPACE_ST << vector_name << ": []";
        return;
    }
    output << tab << SPACE_ST << vector_name << "[list]: \n";
    const std::string space = last_item ? (tab + INDENT) : tab + OR_INDENT;
    int count = vec.size();
    int i = 0;
    for (i = 0; i < count - 1; ++i) {
        PrintSQLNode(output, space, vec[i], "" + std::to_string(i), false);
        output << "\n";
    }
    PrintSQLNode(output, space, vec[i], "" + std::to_string(i), true);
}

void PrintSQLVector(std::ostream &output, const std::string &tab,
                    const std::vector<std::pair<std::string, DataType>> &vec,
                    const std::string &vector_name, bool last_item) {
    if (0 == vec.size()) {
        output << tab << SPACE_ST << vector_name << ": []";
        return;
    }
    output << tab << SPACE_ST << vector_name << "[list]: \n";
    const std::string space = last_item ? (tab + INDENT) : tab + OR_INDENT;
    int count = vec.size();
    int i = 0;
    for (i = 0; i < count - 1; ++i) {
        PrintValue(output, space, DataTypeName(vec[i].second),
                   "" + vec[i].first, false);
        output << "\n";
    }
    PrintValue(output, space, DataTypeName(vec[i].second), "" + vec[i].first,
               true);
}

void PrintSQLVector(std::ostream &output, const std::string &tab,
                    const NodePointVector &vec, const std::string &vector_name,
                    bool last_item) {
    if (0 == vec.size()) {
        output << tab << SPACE_ST << vector_name << ": []";
        return;
    }
    output << tab << SPACE_ST << vector_name << "[list]: \n";
    const std::string space = last_item ? (tab + INDENT) : tab + OR_INDENT;
    int count = vec.size();
    int i = 0;
    for (i = 0; i < count - 1; ++i) {
        PrintSQLNode(output, space, vec[i], "" + std::to_string(i), false);
        output << "\n";
    }
    PrintSQLNode(output, space, vec[i], "" + std::to_string(i), true);
}

void PrintValue(std::ostream &output, const std::string &org_tab,
                const std::string &value, const std::string &item_name,
                bool last_child) {
    output << org_tab << SPACE_ST << item_name << ": " << value;
}

void PrintValue(std::ostream &output, const std::string &org_tab,
                const std::vector<std::string> &vec,
                const std::string &item_name, bool last_child) {
    std::string value = "";
    for (auto item : vec) {
        value.append(item).append(",");
    }
    if (vec.size() > 0) {
        value.pop_back();
    }
    output << org_tab << SPACE_ST << item_name << ": " << value;
}

void SQLNode::Print(std::ostream &output, const std::string &tab) const {
    output << tab << SPACE_ST << "node[" << NameOfSQLNodeType(type_) << "]";
}

void SQLNodeList::Print(std::ostream &output, const std::string &tab) const {
    PrintSQLVector(output, tab, list_, "list", true);
}

void ExprNode::Print(std::ostream &output, const std::string &org_tab) const {
    output << org_tab << SPACE_ST << "expr[" << ExprTypeName(expr_type_) << "]";
}

void ConstNode::Print(std::ostream &output, const std::string &org_tab) const {
    {
        ExprNode::Print(output, org_tab);
        output << "\n";
        const std::string tab = org_tab + INDENT + SPACE_ED;
        output << tab << SPACE_ST;
        switch (date_type_) {
            case kTypeInt32:
                output << "value: " << val_.vint;
                break;
            case kTypeInt64:
                output << "value: " << val_.vlong;
                break;
            case kTypeString:
                output << "value: " << val_.vstr;
                break;
            case kTypeFloat:
                output << "value: " << val_.vfloat;
                break;
            case kTypeDouble:
                output << "value: " << val_.vdouble;
                break;
            case kTypeNull:
                output << "value: null";
                break;
            default:
                output << "value: unknow";
        }
    }
}

void LimitNode::Print(std::ostream &output, const std::string &org_tab) const {
    SQLNode::Print(output, org_tab);
    const std::string tab = org_tab + INDENT + SPACE_ED;
    output << "\n";
    PrintValue(output, tab, std::to_string(limit_cnt_), "limit_cnt", true);
}

void TableNode::Print(std::ostream &output, const std::string &org_tab) const {
    SQLNode::Print(output, org_tab);
    const std::string tab = org_tab + INDENT + SPACE_ED;
    output << "\n";
    PrintValue(output, tab, org_table_name_, "table", false);
    output << "\n";
    PrintValue(output, tab, alias_table_name_, "alias", true);
}

void ColumnRefNode::Print(std::ostream &output,
                          const std::string &org_tab) const {
    ExprNode::Print(output, org_tab);
    const std::string tab = org_tab + INDENT + SPACE_ED;
    output << "\n";
    PrintValue(output, tab, relation_name_, "relation_name", false);
    output << "\n";
    PrintValue(output, tab, column_name_, "column_name", true);
}

void OrderByNode::Print(std::ostream &output,
                        const std::string &org_tab) const {
    SQLNode::Print(output, org_tab);
    const std::string tab = org_tab + INDENT + SPACE_ED;

    output << "\n";
    PrintValue(output, tab, NameOfSQLNodeType(sort_type_), "sort_type", false);

    output << "\n";
    PrintSQLNode(output, tab, order_by_, "order_by", true);
}

void FrameNode::Print(std::ostream &output, const std::string &org_tab) const {
    SQLNode::Print(output, org_tab);
    const std::string tab = org_tab + INDENT + SPACE_ED;

    output << "\n";
    PrintValue(output, tab, NameOfSQLNodeType(frame_type_), "type", false);

    output << "\n";
    if (NULL == start_) {
        PrintValue(output, tab, "UNBOUNDED", "start", false);
    } else {
        PrintSQLNode(output, tab, start_, "start", false);
    }

    output << "\n";
    if (NULL == end_) {
        PrintValue(output, tab, "UNBOUNDED", "end", true);
    } else {
        PrintSQLNode(output, tab, end_, "end", true);
    }
}

void CallExprNode::Print(std::ostream &output,
                         const std::string &org_tab) const {
    ExprNode::Print(output, org_tab);
    output << "\n";
    const std::string tab = org_tab + INDENT + SPACE_ED;
    output << tab << "function_name: " << function_name_;
    output << "\n";
    PrintSQLVector(output, tab, args_, "args", false);
    output << "\n";
    PrintSQLNode(output, tab, over_, "over", true);
}

void WindowDefNode::Print(std::ostream &output,
                          const std::string &org_tab) const {
    SQLNode::Print(output, org_tab);
    const std::string tab = org_tab;
    output << "\n";
    PrintValue(output, tab, window_name_, "window_name", false);

    output << "\n";
    PrintValue(output, tab, partitions_, "partitions", false);

    output << "\n";
    PrintValue(output, tab, orders_, "orders", false);

    output << "\n";
    PrintSQLNode(output, tab, frame_ptr_, "frame", true);
}

void ResTarget::Print(std::ostream &output, const std::string &org_tab) const {
    SQLNode::Print(output, org_tab);
    output << "\n";
    const std::string tab = org_tab + INDENT + SPACE_ED;
    PrintSQLNode(output, tab, val_, "val", false);
    output << "\n";
    PrintValue(output, tab, name_, "name", true);
}

void SelectStmt::Print(std::ostream &output, const std::string &org_tab) const {
    SQLNode::Print(output, org_tab);
    const std::string tab = org_tab + INDENT + SPACE_ED;
    output << "\n";
    bool last_child = false;
    PrintSQLNode(output, tab, where_clause_ptr_, "where_clause", last_child);
    output << "\n";
    PrintSQLNode(output, tab, group_clause_ptr_, "group_clause", last_child);
    output << "\n";
    PrintSQLNode(output, tab, having_clause_ptr_, "haveing_clause", last_child);
    output << "\n";
    PrintSQLNode(output, tab, order_clause_ptr_, "order_clause", last_child);
    output << "\n";
    PrintSQLNode(output, tab, limit_ptr_, "limit", last_child);
    output << "\n";
    PrintSQLVector(output, tab, select_list_ptr_, "select_list", last_child);
    output << "\n";
    PrintSQLVector(output, tab, tableref_list_ptr_, "tableref_list",
                   last_child);
    output << "\n";
    last_child = true;
    PrintSQLVector(output, tab, window_list_ptr_, "window_list", last_child);
}

/**
 * get the node type name
 * @param type
 * @param output
 */
std::string NameOfSQLNodeType(const SQLNodeType &type) {
    std::string output;
    switch (type) {
        case kSelectStmt:
            output = "SELECT";
            break;
        case kCreateStmt:
            output = "CREATE";
            break;
        case kCmdStmt:
            output = "CMD";
            break;
        case kName:
            output = "kName";
            break;
        case kResTarget:
            output = "kResTarget";
            break;
        case kTable:
            output = "kTable";
            break;
        case kColumnDesc:
            output = "kColumnDesc";
            break;
        case kColumnIndex:
            output = "kColumnIndex";
            break;
        case kExpr:
            output = "kExpr";
            break;
        case kWindowDef:
            output = "kWindowDef";
            break;
        case kFrames:
            output = "kFrame";
            break;
        case kFrameBound:
            output = "kBound";
            break;
        case kPreceding:
            output = "kPreceding";
            break;
        case kFollowing:
            output = "kFollowing";
            break;
        case kCurrent:
            output = "kCurrent";
            break;
        case kFrameRange:
            output = "kFrameRange";
            break;
        case kFrameRows:
            output = "kFrameRows";
            break;
        case kConst:
            output = "kConst";
            break;
        case kOrderBy:
            output = "kOrderBy";
            break;
        case kLimit:
            output = "kLimit";
            break;
        case kFnList:
            output = "kFnList";
            break;
        case kFnDef:
            output = "kFnDef";
            break;
        case kFnPara:
            output = "kFnPara";
            break;
        case kFnReturnStmt:
            output = "kFnReturnStmt";
            break;
        case kFnAssignStmt:
            output = "kFnAssignStmt";
            break;
        case kFnValue:
            output = "kFnValue";
            break;
        default:
            output = "unknown";
    }
    return output;
}

std::ostream &operator<<(std::ostream &output, const SQLNode &thiz) {
    thiz.Print(output, "");
    return output;
}

std::ostream &operator<<(std::ostream &output, const SQLNodeList &thiz) {
    thiz.Print(output, "");
    return output;
}

void FillSQLNodeList2NodeVector(
    SQLNodeList *node_list_ptr,
    std::vector<SQLNode *> &node_list  // NOLINT (runtime/references)
) {
    if (nullptr != node_list_ptr) {
        for (auto item : node_list_ptr->GetList()) {
            node_list.push_back(item);
        }
    }
}

WindowDefNode *WindowOfExpression(
    std::map<std::string, WindowDefNode *> windows, ExprNode *node_ptr) {
    WindowDefNode *w_ptr = nullptr;
    switch (node_ptr->GetExprType()) {
        case kExprCall: {
            CallExprNode *func_node_ptr =
                dynamic_cast<CallExprNode *>(node_ptr);
            if (nullptr != func_node_ptr->GetOver()) {
                return windows.at(func_node_ptr->GetOver()->GetName());
            }

            if (func_node_ptr->GetArgs().empty()) {
                return nullptr;
            }
            for (auto arg : func_node_ptr->GetArgs()) {
                WindowDefNode *ptr =
                    WindowOfExpression(windows, dynamic_cast<ExprNode *>(arg));
                if (nullptr != ptr && nullptr != w_ptr) {
                    LOG(WARNING)
                        << "Cannot handle more than 1 windows in an expression";
                    return nullptr;
                }
            }

            return w_ptr;
        }
        default:
            return w_ptr;
    }
}

void CreateStmt::Print(std::ostream &output, const std::string &org_tab) const {
    SQLNode::Print(output, org_tab);
    const std::string tab = org_tab + INDENT + SPACE_ED;
    output << "\n";
    PrintValue(output, tab, table_name_, "table", false);
    output << "\n";
    PrintValue(output, tab, std::to_string(op_if_not_exist_), "IF NOT EXIST",
               false);
    output << "\n";
    PrintSQLVector(output, tab, column_desc_list_, "column_desc_list_", true);
    output << "\n";
    //    if (nullptr != table_def_) {
    //        PrintValue(output, tab, table_def_->DebugString() , "table def",
    //        true);
    //    }
}

void ColumnDefNode::Print(std::ostream &output,
                          const std::string &org_tab) const {
    SQLNode::Print(output, org_tab);
    const std::string tab = org_tab + INDENT + SPACE_ED;
    output << "\n";
    PrintValue(output, tab, column_name_, "column_name", false);
    output << "\n";
    PrintValue(output, tab, DataTypeName(column_type_), "column_type", false);
    output << "\n";
    PrintValue(output, tab, std::to_string(op_not_null_), "NOT NULL", false);
}

void ColumnIndexNode::Print(std::ostream &output,
                            const std::string &org_tab) const {
    SQLNode::Print(output, org_tab);
    const std::string tab = org_tab + INDENT + SPACE_ED;
    output << "\n";
    std::string lastdata;
    lastdata = accumulate(key_.begin(), key_.end(), lastdata);
    PrintValue(output, tab, lastdata, "keys", false);
    output << "\n";
    PrintValue(output, tab, ts_, "ts_col", false);
    output << "\n";
    PrintValue(output, tab, std::to_string(ttl_), "ttl", false);
    output << "\n";
    PrintValue(output, tab, version_, "version_column", false);
    output << "\n";
    PrintValue(output, tab, std::to_string(version_count_), "version_count",
               true);
}
void CmdNode::Print(std::ostream &output, const std::string &org_tab) const {
    SQLNode::Print(output, org_tab);
    const std::string tab = org_tab + INDENT + SPACE_ED;
    output << "\n";
    PrintValue(output, tab, CmdTypeName(cmd_type_), "cmd_type", false);
    output << "\n";
    PrintValue(output, tab, args_, "args", true);
}

void InsertStmt::Print(std::ostream &output, const std::string &org_tab) const {
    SQLNode::Print(output, org_tab);
    const std::string tab = org_tab + INDENT + SPACE_ED;
    output << "\n";
    PrintValue(output, tab, table_name_, "table_name", false);
    output << "\n";
    if (is_all_) {
        PrintValue(output, tab, "all", "columns", false);
    } else {
        PrintValue(output, tab, columns_, "columns", false);
    }

    PrintSQLVector(output, tab, values_, "values", false);
}
void BinaryExpr::Print(std::ostream &output, const std::string &org_tab) const {
    ExprNode::Print(output, org_tab);
    const std::string tab = org_tab + INDENT + SPACE_ED;
    output << "\n";
    PrintSQLVector(output, tab, children, ExprOpTypeName(op_), true);
}
void UnaryExpr::Print(std::ostream &output, const std::string &org_tab) const {
    ExprNode::Print(output, org_tab);
    const std::string tab = org_tab + INDENT + SPACE_ED;
    output << "\n";
    PrintSQLVector(output, tab, children, ExprOpTypeName(op_), true);
}
void ExprIdNode::Print(std::ostream &output, const std::string &org_tab) const {
    ExprNode::Print(output, org_tab);
    const std::string tab = org_tab + INDENT + SPACE_ED;
    output << "\n";
    PrintValue(output, tab, name_, "var", true);
}

void ExprListNode::Print(std::ostream &output,
                         const std::string &org_tab) const {
    ExprNode::Print(output, org_tab);
    const std::string tab = org_tab + INDENT + SPACE_ED;
    output << "\n";
    PrintSQLVector(output, tab, children, "list", true);
}
void FnParaNode::Print(std::ostream &output, const std::string &org_tab) const {
    SQLNode::Print(output, org_tab);
    const std::string tab = org_tab + INDENT + SPACE_ED;
    output << "\n";
    PrintValue(output, tab, DataTypeName(para_type_), name_, true);
}
void FnNodeFnDef::Print(std::ostream &output,
                        const std::string &org_tab) const {
    SQLNode::Print(output, org_tab);
    const std::string tab = org_tab + INDENT + SPACE_ED;
    output << "\n";
    PrintValue(output, tab, this->name_, "func_name", true);
    output << "\n";
    PrintValue(output, tab, DataTypeName(this->ret_type_), "return_type", true);
    output << "\n";
    PrintSQLNode(output, tab, reinterpret_cast<const SQLNode *>(parameters_),
                 "parameters", true);
}
void FnNodeList::Print(std::ostream &output, const std::string &org_tab) const {
    SQLNode::Print(output, org_tab);
    const std::string tab = org_tab + INDENT + SPACE_ED;
    output << "\n";
    PrintSQLVector(output, tab, children, "list", true);
}
void FnAssignNode::Print(std::ostream &output,
                         const std::string &org_tab) const {
    SQLNode::Print(output, org_tab);
    const std::string tab = org_tab + INDENT + SPACE_ED;
    output << "\n";
    PrintSQLNode(output, tab, reinterpret_cast<const SQLNode *>(expression_),
                 name_, true);
}
void FnReturnStmt::Print(std::ostream &output,
                         const std::string &org_tab) const {
    SQLNode::Print(output, org_tab);
    const std::string tab = org_tab + INDENT + SPACE_ED;
    output << "\n";
    PrintSQLNode(output, tab, return_expr_, "return", true);
}
void StructExpr::Print(std::ostream &output, const std::string &org_tab) const {
    ExprNode::Print(output, org_tab);
    const std::string tab = org_tab + INDENT + SPACE_ED;
    PrintValue(output, tab, class_name_, "name", false);
    output << "\n";
    PrintSQLNode(output, tab, fileds_, "fileds", false);
    output << "\n";
    PrintSQLNode(output, tab, methods_, "methods", true);
}
}  // namespace node
}  // namespace fesql