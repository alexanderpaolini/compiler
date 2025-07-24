#include "lexer.h"

#ifndef PARSER_H
#define PARSER_H

/**
 * I would like to thank ChatGPT for help making the following breakdown:
 *
 * PROGRAM is a series of statements, each followed by a semicolon
 * PROGRAM         -> STATEMENT ( STATEMENT )*
 *
 * STATEMENT is, for now, just a VARIABLE_ASSIGNMENT or a VARIABLE DECLARATION
 * STATEMENT       -> VARIABLE_DECLARATION
 *                  | VARIABLE_ASSIGNMENT
 *                  | BLOCK_STATEMENT
 *                  | WHILE_STATEMENT
 *                  | FOR_STATEMENT
 *                  | PRINT_STATEMENT
 *                  | IF_STATEMENT
 *
 * VARIABLE_DECLARATION -> TYPE IDENTIFIER ('=' EXPRESSION)? ';'
 *
 * VARIABLE_ASSIGNMENT -> IDENTIFIER '=' EXPRESSION ';'
 *
 *
 * BLOCK_STATEMENT -> '{' STATEMENT ( ';' STATEMENT )* '}'
 *
 * WHILE_STATEMENT -> WHILE EXPRESSION STATEMENT
 * 
 * FOR_STATEMENT -> FOR '(' VARIABLE_DECLARATION | VARIABLE ASSIGNMENT ';' EXPRESSION? ';' VARIABLE_ASSIGNMENT ')' STATEMENT
 *
 * IF_STATEMENT -> IF EXPRESSION STATEMENT (ELSE STATEMENT)?
 *
 * PRINT_STATEMENT -> PRINT EXPRESSION ';'
 *
 * For now, a TYPE is just an identifier (it will be deleted and cast to int.)
 * TYPE -> IDENTIFIER
 *
 * An EXPRESSION can be a TERM, or it can include binary operators applied to FACTORS or TERMS
 * EXPRESSION      -> TERM ( ADDITIVE_OPERATOR TERM )* ( COMPARISON_OPERATOR TERM )*
 *
 * TERM can be a FACTOR, or it can include multiplication or division operators applied to factors
 * TERM            -> FACTOR ( MULTIPLICATIVE_OPERATOR FACTOR )* // Multiplication or division applied to factors
 *
 * FACTOR can be a NUMBER, a parenthesized EXPRESSION, a unary operator applied to a TERM, or a comparison
 * FACTOR          -> STRING
 *                 | NUMBER
 *                 | '(' EXPRESSION ')'
 *                 | UNARY_OPERATOR FACTOR
 *                 | IDENTIFIER
 *                 | EXPRESSION COMPARISON_OPERATOR EXPRESSION
 *
 * COMPARISON_OPERATOR -> '<' | '<=' | '>' | '>=' | '==' | '!='
 * ADDITIVE_OPERATOR  -> '+' | '-'
 * MULTIPLICATIVE_OPERATOR -> '*' | '/'
 * UNARY_OPERATOR  -> '-' | '!'
 */

typedef enum
{
    NODE_PROGRAM,
    NODE_STATEMENT,
    NODE_ASSIGNMENT,
    NODE_DECLARATION,
    NODE_STRING,
    NODE_INTEGER,
    NODE_TYPE,
    NODE_IDENTIFIER,
    NODE_BINARY_OP,
    NODE_UNARY_OP,
    NODE_FUNCTION_DECLARATION,
    NODE_FUNCTION_CALL,
    NODE_RETURN,
    NODE_EOF,
} NodeType;

typedef enum
{
    DECLARATION,
    ASSIGNMENT,
    BLOCK_STATEMENT,
    WHILE_STATEMENT,
    FOR_STATEMENT,
    PRINT_STATEMENT,
    IF_STATEMENT,
    FUNCTION_STATEMENT,
    RETURN_STATEMENT,
    EXPRESSION_STATEMENT,
} StatementType;

typedef enum BinaryOp
{
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE,
    IS_EQUAL,
    IS_LESS_THAN,
    LESS_THAN_EQUAL,
    IS_GREATER_THAN,
    GREATER_THAN_EQUAL,
    IS_NOT_EQUAL,
} BinaryOp;

typedef enum UnaryOp
{
    NEGATE,
    LOGICAL_NOT,
} UnaryOp;

typedef struct
{
    char *name;
} Identifier;

typedef struct ASTNode
{
    NodeType type;
    union
    {
        // Integer
        int integer_value;

        // Identifier
        char *identifier_value;

        // String
        char *string_value;

        // Binary Operation
        struct
        {
            BinaryOp op;
            struct ASTNode *left;
            struct ASTNode *right;
        } binary_op;

        // Unary Operation
        struct
        {
            UnaryOp op;
            struct ASTNode *right;
        } unary_op;

        // Variable Assignment
        struct
        {
            struct ASTNode *right;
            struct ASTNode *identifier;
        } assignment;

        // Variable Declaration
        struct
        {
            struct ASTNode *type;
            struct ASTNode *right;
            struct ASTNode *identifier;
        } declaration;

        // Statement
        struct
        {
            StatementType type;
            union
            {
                struct ASTNode *declaration;
                struct ASTNode *assignment;
                struct ASTNode *head;
                struct ASTNode *expression;
            } data;
        } statement;

        // Type Declaration
        struct
        {
            struct ASTNode *identifier;
        } type;

        // Function Declaration
        struct
        {
            struct ASTNode *identifier;
            struct ASTNode *parameters;  // linked list of parameters
            struct ASTNode *body;        // block statement
        } function_declaration;

        // Function Call
        struct
        {
            struct ASTNode *identifier;
            struct ASTNode *arguments;   // linked list of arguments
        } function_call;

        // Return Statement
        struct
        {
            struct ASTNode *expression;  // can be NULL for void return
        } return_statement;

        // Program
        struct
        {
            struct ASTNode *head;
            struct ASTNode *tail;
        } program;
    } data;
    struct ASTNode *next;
} ASTNode;

typedef struct
{
    Token *head;
    Token *cur;
    ASTNode *node;
    char *prog;
} ParserState;

int parse_is_at_eof(ParserState *state);
Token *parse_peek(ParserState *state);
Token *parse_peek_next(ParserState *state);
Token *parse_consume(ParserState *state, const TokenKind expected_tokens[], int len);
ASTNode *parse_eof(ParserState *state);
ASTNode *parse_number(ParserState *state);
ASTNode *parse_unary_operator(ParserState *state);
ASTNode *parse_additive_operator(ParserState *state);
ASTNode *parse_multiplicative_operator(ParserState *state);
ASTNode *parse_comparison_operator(ParserState *state);
ASTNode *parse_factor(ParserState *state);
ASTNode *parse_term(ParserState *state);
ASTNode *parse_expression(ParserState *state);
ASTNode *parse_identifier(ParserState *state);
ASTNode *parse_type(ParserState *state);
ASTNode *parse_variable_declaration(ParserState *state);
ASTNode *parse_variable_assignment(ParserState *state);
ASTNode *parse_function_declaration(ParserState *state);
ASTNode *parse_function_call(ParserState *state);
ASTNode *parse_return_statement(ParserState *state);
ASTNode *parse_statement(ParserState *state);
ASTNode *parser(ParserState *state);
ASTNode *create_empty_ast_node();
ParserState *create_parser_state(char *program, Token *head);
void free_parser_state(ParserState *state);

#endif // PARSER_H