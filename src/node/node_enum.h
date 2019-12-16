/*-------------------------------------------------------------------------
 * Copyright (C) 2019, 4paradigm
 * node_enum.h
 *
 * Author: chenjing
 * Date: 2019/10/29
 *--------------------------------------------------------------------------
 **/

#ifndef SRC_NODE_NODE_ENUM_H_
#define SRC_NODE_NODE_ENUM_H_

#include <string>
#include "proto/common.pb.h"
namespace fesql {
namespace node {

const char SPACE_ST[] = "+-";
const char SPACE_ED[] = "";
const char OR_INDENT[] = "|\t";
const char INDENT[] = " \t";

enum SQLNodeType {
    // SQL
    kSelectStmt = 0,
    kCreateStmt,
    kInsertStmt,
    kCmdStmt,
    kExpr,
    kResTarget,
    kTable,
    kWindowFunc,
    kWindowDef,
    kFrameBound,
    kFrames,
    kColumnDesc,
    kColumnIndex,
    kIndexKey,
    kIndexTs,
    kIndexVersion,
    kIndexTTL,
    kName,
    kConst,
    kLimit,
    kList,
    kOrderBy,

    kDesc,
    kAsc,

    kFrameRange,
    kFrameRows,

    kPreceding,
    kFollowing,
    kCurrent,

    kFn,
    kFnDef,
    kFnValue,
    kFnAssignStmt,
    kFnReturnStmt,
    kFnPara,
    kFnParaList,
    kFnList,
    kUnknow
};

enum ExprType {
    kExprBinary,
    kExprUnary,
    kExprIn,
    kExprCall,
    kExprCase,
    kExprCast,
    kExprId,
    kExprColumnRef,
    kExprPrimary,
    kExprList,
    kExprAll,
    kExprStruct,
    kExprUnknow = 9999
};
enum DataType {
    kTypeBool,
    kTypeInt16,
    kTypeInt32,
    kTypeInt64,
    kTypeFloat,
    kTypeDouble,
    kTypeString,
    kTypeTimestamp,
    kTypeHour,
    kTypeDay,
    kTypeMinute,
    kTypeSecond,
    kTypeRow,
    kTypeNull,
    kTypeVoid,
    kTypeInt8Ptr,
};

enum TimeUnit {
    kTimeUnitHour,
    kTimeUnitDay,
    kTimeUnitMinute,
    kTimeUnitSecond,
};
enum FnOperator {
    kFnOpAdd,
    kFnOpMinus,
    kFnOpMulti,
    kFnOpDiv,
    kFnOpBracket,
    kFnOpNone
};

enum CmdType {
    kCmdCreateGroup,
    kCmdCreateDatabase,
    kCmdCreateTable,
    kCmdUseDatabase,
    kCmdShowDatabases,
    kCmdShowTables,
    kCmdDescTable,
    kCmdDropTable,
    kCmdExit
};
/**
 * Planner:
 *  basic class for plan
 *
 */
enum PlanType {
    kPlanTypeCmd,
    kPlanTypeFuncDef,
    kPlanTypeSelect,
    kPlanTypeCreate,
    kPlanTypeInsert,
    kPlanTypeScan,
    kPlanTypeMerge,
    kPlanTypeLimit,
    kPlanTypeFilter,
    kProjectList,
    kPlanTypeWindow,
    kProject,
    kScalarFunction,
    kOpExpr,
    kAggFunction,
    kAggWindowFunction,
    kUnknowPlan,

    kScanTypeSeqScan,
    kScanTypeIndexScan,
};

}  // namespace node

}  // namespace fesql

#endif  // SRC_NODE_NODE_ENUM_H_