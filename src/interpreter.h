#include "parser.h"


typedef struct
{
    char *name;
    char *type;
    void *data;
} Variable;

// Function structure to store function definitions
typedef struct Function
{
    char *name;
    struct ASTNode *parameters;  // linked list of parameter names
    struct ASTNode *body;        // function body (block statement)
    struct Function *next;       // for linked list
} Function;

// State is a linked list of Variables
// Ex. int x = 5;
// type: int; name: x; value: 5
typedef struct State
{
    Variable data;
    struct State *next;
} State;

// Environment holds a list of states, functions, and its outer environment
typedef struct Environment
{
    struct Environment *outer;
    State *state;
    Function *functions;  // linked list of functions
} Environment;

// Return value handling - we need a way to pass return values and handle early returns
typedef struct
{
    Variable *value;
    int has_returned;
} ReturnContext;

int interpret(Environment *environment, ASTNode *node);
int visit_declaration(Environment *env, ASTNode *node);
int visit_assignment(Environment *env, ASTNode *node);
int visit_statement(Environment *env, ASTNode *node);
int visit_statement_with_return(Environment *env, ASTNode *node, ReturnContext *ret_ctx);
int visit_block_statement(Environment *env, ASTNode *node);
int visit_block_statement_with_return(Environment *env, ASTNode *node, ReturnContext *ret_ctx);
int visit_while_statement(Environment *env, ASTNode *node);
int visit_for_statement(Environment *env, ASTNode *node);
int visit_print_statement(Environment *env, ASTNode *node);
int visit_if_statement(Environment *env, ASTNode *node);
int visit_function_declaration(Environment *env, ASTNode *node);
int visit_return_statement(Environment *env, ASTNode *node);
int visit_return_statement_with_context(Environment *env, ASTNode *node, ReturnContext *ret_ctx);

Variable *visit_string(Environment *env, ASTNode *node);
Variable *visit_integer(Environment *env, ASTNode *node);
Variable *visit_identifier(Environment *env, ASTNode *node);
Variable *visit_binary_op(Environment *env, ASTNode *node);
Variable *visit_unary_op(Environment *env, ASTNode *node);
Variable *visit_function_call(Environment *env, ASTNode *node);
Variable *visit_expression(Environment *env, ASTNode *node);

int visit_eof(Environment *env, ASTNode *node);

// Function management
Function *lookup_function(Environment *env, char *name);
void store_function(Environment *env, char *name, ASTNode *parameters, ASTNode *body);
Environment *create_function_environment(Environment *parent);
