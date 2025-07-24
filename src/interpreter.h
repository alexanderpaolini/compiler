#include "parser.h"

typedef struct
{
    char *name;
    char *type;
    void *data;
} Variable;

// State is a linked list of Variables
// Ex. int x = 5;
// type: int; name: x; value: 5
typedef struct State
{
    Variable data;
    struct State *next;
} State;

// Environment holds a list of states and its outer environment
typedef struct Environment
{
    struct Environment *outer;
    State *state;
} Environment;

Environment *create_empty_environment(Environment *outer);

int interpret(Environment *environment, ASTNode *node);
int visit_declaration(Environment *env, ASTNode *node);
int visit_assignment(Environment *env, ASTNode *node);
int visit_statement(Environment *env, ASTNode *node);
int visit_block_statement(Environment *env, ASTNode *node);
int visit_while_statement(Environment *env, ASTNode *node);
int visit_for_statement(Environment *env, ASTNode *node);
int visit_print_statement(Environment *env, ASTNode *node);
int visit_if_statement(Environment *env, ASTNode *node);
int visit_function_declaration(Environment *env, ASTNode *node);
int visit_return_statement(Environment *env, ASTNode *node);

Variable *visit_string(Environment *env, ASTNode *node);
Variable *visit_integer(Environment *env, ASTNode *node);
Variable *visit_identifier(Environment *env, ASTNode *node);
Variable *visit_binary_op(Environment *env, ASTNode *node);
Variable *visit_unary_op(Environment *env, ASTNode *node);
Variable *visit_function_call(Environment *env, ASTNode *node);
Variable *visit_expression(Environment *env, ASTNode *node);

int visit_eof(Environment *env, ASTNode *node);
