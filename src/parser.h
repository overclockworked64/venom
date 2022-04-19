#ifndef venom_parser_h
#define venom_parser_h

#include "tokenizer.h"

typedef enum {
    ADD, SUB, MUL, DIV, LITERAL,
} ExpressionKind;

typedef struct BinaryExpression BinaryExpression;

typedef struct Expression {
    ExpressionKind kind;
    union {
        BinaryExpression *binexp;
        int intval;
    } data;
} Expression;

typedef struct BinaryExpression {
    Expression lhs;
    Expression rhs;    
} BinaryExpression;

typedef enum {
    STATEMENT_PRINT,
} StatementKind;

typedef struct {
    StatementKind kind;
    Expression exp;
} Statement;

typedef struct {
    Token current;
    Token previous;
} Parser;

Statement parse(Parser *parser, Tokenizer *tokenizer);
void free_ast(BinaryExpression *binexp);

#endif