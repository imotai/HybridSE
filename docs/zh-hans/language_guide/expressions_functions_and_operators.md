## 表达式，函数和运算

#### 语法描述

```SQL
/* type syntax */
sql_type: 
	{ 'INT' | 'SMALLINT' | 'BIGINT' | 'FLOAT' | 'DOUBLE' | 'STRING' | 'TIMESTAMP' | 'DATE' | 'BOOL' }
	
/*SQL expression syntax*/
sql_expr:
	{ column_ref | expr_const | sql_call_expr | sql_cast_expr | sql_unary_expr | sql_binary_expr | 
  	sql_case_when_expr | sql_between_expr| sql_in_expr | sql_like_expr }

/*column reference expression syntax*/
column_ref:
	{ column_name | relation_name '.' column_name | relation_name."*" }

/*type cast expression syntax*/
sql_cast_expr: 
	{ CAST '(' sql_expr 'AS' sql_type ')'  |  sql_type '(' sql_expr ')' }

/*const expression syntax*/
expr_const: 
	{STRING | INTNUM | LONGNUM | DOUBLENUM | FLOATNUM | BOOLVALUE | NULLX}

/* unary expression syntax */
sql_unary_expr:
	{ '!' | 'NOT' | '-' } sql_expr

/*binary expression syntax*/
sql_binary_expr:
	sql_expr  {
	'+' | '-' | '*' | '/' | 'DIV' | '%' | 'MOD' | 
	'>' | '<' | '=' | '>=' | '<=' | '<>'  |
	'AND' | '&&' | 'OR' | '||' | 'XOR' |
	'LIKE' | 'NOT LIKE' | 
	} sql_expr
	
sql_case_when_expr:
	{
		CASE value WHEN [compare_value] THEN result [WHEN [compare_value] THEN result ...] [ELSE result] END
		| CASE WHEN [condition] THEN result [WHEN [condition] THEN result ...] [ELSE result] END
	}

sql_between_expr:
	sql_expr BETWEEN sql_expr AND sql_expr

sql_in_expr:
	sql_expr [NOT] IN '(' sql_expr [, sql_expr ...] ')'

sql_like_expr:
	sql_expr [NOT] LIKE sql_expr
	
/* call function expression */
sql_call_expr: 
	function_name '(' [sql_expr, ...] ')'

```

#### 边界说明：

| 语句类型        | 状态                                                         |
| :-------------- | :----------------------------------------------------------- |
| 常量表达式      | 已支持                                                       |
| 列表达式        | 已支持                                                       |
| 算术运算表达式  | 已支持                                                       |
| 逻辑运算表达式  | 已支持                                                       |
| 比较函数与运算  | 已支持                                                       |
| 类型运算表达式  | 已支持                                                       |
| 位运算表达式    | 部分支持                                                     |
| CASE WHEN表达式 | 已支持                                                       |
| Between表达式   | 支持中                                                       |
| IN 表达式       | 支持中                                                       |
| LIKE 表达式     | 尚未支持                                                     |
| 函数表达式      | 支持大部分SQL标准支持的UDF和UDAF函数，具体参考函数手册（待补充） |
|                 |                                                              |

### 1. 比较运算

| Name        | Description                                                  | case覆盖 |
| :---------- | :----------------------------------------------------------- | :------- |
| `>`         | Greater than operator操作类型边界：L Rnumberdatetimestampstringnumber    date    timestamp    string |          |
| `>=`        | Greater than or equal operator操作类型边界同上               |          |
| `<=`        | Less than operator操作类型边界同上                           |          |
| `!=` , `<>` | Not equal operator操作类型边界同上                           |          |
| `<=`        | Less than or equal operator操作类型边界同上                  |          |
| `=`         | Equal operator操作类型边界同上                               |          |

### 2. 逻辑运算

| Name        | Description                                                  | case覆盖 |
| :---------- | :----------------------------------------------------------- | :------- |
| `AND`, `&&` | Logical AND操作数类型边界L Rnumbertimestampdatestringnumber    timestamp    date    string |          |
| `OR`, `||`  | Logical OR操作数边界同上                                     |          |
| `XOR`       | Logical XOR操作数边界同上                                    |          |
| `NOT`, `!`  | Negates value支持所有基础类型numbertimestampdatestring       |          |

### 3. 算术运算

| Name       | Description                                              | 边界 |
| :--------- | :------------------------------------------------------- | :--- |
| `%`, `MOD` | Modulo operator                                          |      |
| `*`        | Multiplication operator                                  |      |
| `+`        | Addition operator                                        |      |
| `-`        | Minus operator                                           |      |
| `-`        | Change the sign of the argument只支持数值型操作数-number |      |
| `/`        | Division operator                                        |      |
| `DIV`      | Integer division                                         |      |

###  4. 位运算

**Table 12.16 Bit Functions and Operators**

| Name | Description |
| :--- | :---------- |
| `&`  | Bitwise AND |
| `>>` | Right shift |
| `<<` | Left shift  |

### 5. 类型运算

**Table 12.14 类型转换兼容情况**

| src\|dist     | bool   | smallint | int    | float  | int64  | double | timestamp | date   | string |
| :------------ | :----- | :------- | :----- | :----- | :----- | :----- | :-------- | :----- | :----- |
| **bool**      | Safe   | Safe     | Safe   | Safe   | Safe   | Safe   | UnSafe    | X      | Safe   |
| ·**smallint** | UnSafe | Safe     | Safe   | Safe   | Safe   | Safe   | UnSafe    | X      | Safe   |
| **int**       | UnSafe | UnSafe   | Safe   | Safe   | Safe   | Safe   | UnSafe    | X      | Safe   |
| **float**     | UnSafe | UnSafe   | UnSafe | Safe   | Safe   | Safe   | UnSafe    | X      | Safe   |
| **bigint**    | UnSafe | UnSafe   | UnSafe | UnSafe | Safe   | UnSafe | UnSafe    | X      | Safe   |
| **double**    | UnSafe | UnSafe   | UnSafe | UnSafe | UnSafe | Safe   | UnSafe    | X      | Safe   |
| **timestamp** | UnSafe | UnSafe   | UnSafe | UnSafe | Safe   | UnSafe | Safe      | UnSafe | Safe   |
| **date**      | UnSafe | X        | X      | X      | X      | X      | UnSafe    | Safe   | Safe   |
| **string**    | UnSafe | UnSafe   | UnSafe | UnSafe | UnSafe | UnSafe | UnSafe    | UnSafe | Safe   |
