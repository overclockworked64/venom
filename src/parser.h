#ifndef venom_parser_h
#define venom_parser_h

#include <stdbool.h>
#include "dynarray.h"
#include "tokenizer.h"
#include "object.h"

typedef enum {
    EXP_LITERAL,
    EXP_VARIABLE,
    EXP_UNARY,
    EXP_BINARY,
    EXP_CALL,
} ExpressionKind;

typedef struct BinaryExpression BinaryExpression;

typedef struct Expression Expression;

typedef DynArray(Expression) Expression_DynArray;

typedef struct Expression {
    ExpressionKind kind;
    union {
        struct Expression *exp;
        BinaryExpression *binexp;
        double dval;
    } data;
    char *name;
    char *operator;
    Expression_DynArray arguments;
} Expression;

typedef struct BinaryExpression {
    Expression lhs;
    Expression rhs;    
} BinaryExpression;

typedef enum {
    STMT_LET,
    STMT_ASSIGN,
    STMT_PRINT,
    STMT_BLOCK,
    STMT_IF,
    STMT_WHILE,
    STMT_FN,
    STMT_RETURN,
} StatementKind;

typedef struct Statement Statement;

typedef DynArray(Statement) Statement_DynArray;

typedef struct Statement {
    StatementKind kind;
    char *name; /* used by let & assign */
    Expression exp;
    Statement_DynArray stmts;  /* used by block */
    Statement *then_branch; /* used by if */
    Statement *else_branch; /* used by if */
    Statement *body; /* used by while */
    String_DynArray parameters;
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