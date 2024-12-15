#include "parser.h"

// State is a linked list of NAME->VALUE
// Ex. int x = 5;
// name: x; value: 5
typedef struct State
{
    char *name;
    int value;
    struct State *next;
} State;

// Environment holds a list of states and its outer environment
typedef struct Environment
{
    struct Environment *outer;
    State *state;
} Environment;

int interpret(Environment *environment, ASTNode *node);