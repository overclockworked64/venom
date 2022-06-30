#ifndef venom_parser_h
#define venom_parser_h

#include <stdbool.h>
#include "dynarray.h"
#include "tokenizer.h"
#include "object.h"

typedef enum {
    EXP_LITERAL,
    EXP_VARIABLE,
    EXP_STRING,
    EXP_UNARY,
    EXP_BINARY,
    EXP_CALL,
    EXP_ASSIGN,
    EXP_LOGICAL,
} ExpressionKind;

typedef struct LiteralExpression LiteralExpression;
typedef struct VariableExpression VariableExpression;
typedef struct StringExpression StringExpression;
typedef struct UnaryExpression UnaryExpression;
typedef struct BinaryExpression BinaryExpression;
typedef struct CallExpression CallExpression;
typedef struct AssignExpression AssignExpression;
typedef struct LogicalExpression LogicalExpression;
typedef struct Expression Expression;

typedef DynArray(Expression) Expression_DynArray;

typedef struct Expression {
    ExpressionKind kind;
    union {
        LiteralExpression *expr_literal;
        VariableExpression *expr_variable;
        StringExpression *expr_string;
        UnaryExpression *expr_unary;
        BinaryExpression *expr_binary;
        CallExpression *expr_call;
        AssignExpression *expr_assign;
        LogicalExpression *expr_logical;
    } as;
} Expression;

typedef struct LiteralExpression {
    double dval;
    char *specval;
} LiteralExpression;

typedef struct VariableExpression {
    char *name;
} VariableExpression;

typedef struct StringExpression {
    char *str;
} StringExpression;

typedef struct UnaryExpression {
    Expression *exp;
} UnaryExpression;

typedef struct BinaryExpression {
    Expression lhs;
    Expression rhs;    
    char *operator;
} BinaryExpression;

typedef struct CallExpression {
    char *name;
    Expression_DynArray arguments;
} CallExpression;

typedef struct AssignExpression {
    Expression lhs;
    Expression rhs;
} AssignExpression;

typedef struct LogicalExpression {
    Expression lhs;
    Expression rhs;    
    char *operator;
} LogicalExpression;

typedef enum {
    STMT_LET,
    STMT_EXPR,
    STMT_PRINT,
    STMT_BLOCK,
    STMT_IF,
    STMT_WHILE,
    STMT_FN,
    STMT_RETURN,
} StatementKind;

typedef struct Statement Statement;

typedef DynArray(Statement) Statement_DynArray;

typedef struct {
    char *name;
    Expression initializer;
} LetStatement;

typedef struct {
    Expression exp;
} PrintStatement;

typedef struct {
    Expression exp;
} ExpressionStatement;

typedef struct {
    Statement_DynArray stmts;
} BlockStatement;

typedef struct {
    char *name;
    Statement_DynArray stmts;
    String_DynArray parameters;
} FunctionStatement;

typedef struct {
    Expression condition;
    Statement *then_branch;
    Statement *else_branch;
} IfStatement;

typedef struct {
    Expression condition;
    Statement *body;
} WhileStatement;

typedef struct {
    Expression returnval;
} ReturnStatement;

typedef struct Statement {
    StatementKind kind;
    union {
        PrintStatement stmt_print;
        LetStatement stmt_let;
        ExpressionStatement stmt_expr;
        BlockStatement stmt_block;
        FunctionStatement stmt_fn;
        IfStatement stmt_if;
        WhileStatement stmt_while;
        ReturnStatement stmt_return;
    } as;
} Statement;

typedef struct {
    Token current;
    Token previous;
    bool had_error;
} Parser;

void parse(Parser *parser, Tokenizer *tokenizer, Statement_DynArray *stmts);
void free_expression(Expression e);
void free_stmt(Statement stmt);

#endif