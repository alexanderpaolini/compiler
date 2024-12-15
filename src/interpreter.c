#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "interpreter.h"
#include "util.h"

// Variable management functions

#define FAILURE -1
#define SUCCESS 0

// Analysis of get()
// O(m*n)
// n = number of states
// m = length of strings
// Pretty bad...
// Returns:
// -1 = no value
// K>=0 = depth of environments
int get(Environment *environment, char *name, int *value)
{
    if (environment == NULL)
    {
        return FAILURE;
    }
    State *dummy = environment->state;
    while (dummy != NULL)
    {
        if (strcmp(name, dummy->name) == 0)
        {
            (*value) = dummy->value;
            return SUCCESS;
        }
        dummy = dummy->next;
    }

    // The only way you get here is if you didn't find it in the current environment.
    // That means you must check outer.
    int exists = get(environment->outer, name, value);
    if (exists == -1)
    {
        return FAILURE;
    }
    else
    {
        // Return the depth of the find.
        return exists + 1;
    }
}

int set(Environment *environment, char *name, int value)
{
    int temp;
    int exists = get(environment, name, &temp);
    if (exists == -1)
    {
        // If it doesn't exist, we CANNOT set it.
        return FAILURE;
    }

    Environment *env = environment;
    while (exists)
    {
        env = env->outer;
        exists--;
    }

    State *dummy = env->state->next;
    while (dummy)
    {
        if (strcmp(name, dummy->name) == 0)
        {
            dummy->value = value;
            return SUCCESS;
        }
        dummy = dummy->next;
    }

    return FAILURE;
}

// Declares the variable in the current (lowest) environment.
int declare(Environment *environment, char *name, int value)
{
    // Declare is either called when "int NAME (= VALUE)?;" is written.
    // As such, it cannot be used when a variable already exists.
    int temp;
    int exists = get(environment, name, &temp);
    if (exists != -1)
    {
        return FAILURE;
    }

    State *new = (State *)malloc(sizeof(State));
    new->name = (char *)malloc(strlen(name));
    strcpy(new->name, name);
    new->value = value;
    new->next = NULL;

    State *dummy = environment->state;
    while (dummy->next != NULL)
    {
        dummy = dummy->next;
    }
    dummy->next = new;

    return SUCCESS;
}

Environment *create_empty_environment(Environment *outer)
{
    Environment *env = (Environment *)malloc(sizeof(Environment));
    env->outer = outer;
    env->state = (State *)malloc(sizeof(State));
    env->state->name = (char *)malloc(1 * sizeof(char));
    strcpy(env->state->name, "");
    env->state->value = -1;
    env->state->next = NULL;
    return env;
}

int interpret(Environment *environment, ASTNode *node)
{
    if (node == NULL)
    {
        // fprintf(stderr, "Attempting to interpret NULL.\n");
        return 0;
    }

    int rhs;
    int lhs;
    int status;
    int value;
    int exists;
    switch (node->type)
    {
    case NODE_PROGRAM:
        Environment *env = create_empty_environment(NULL);
        interpret(env, node->data.program.head->next);
        break;
    case NODE_STATEMENT:
        switch (node->data.statement.type)
        {
        case IF_STATEMENT:
            if (interpret(environment, node->data.statement.data.expression))
            {
                interpret(environment, node->data.statement.data.expression->next);
            }
            else
            {
                if (node->data.statement.data.expression->next->next)
                {
                    interpret(environment, node->data.statement.data.expression->next->next);
                }
            }
        case PRINT_STATEMENT:
            value = interpret(environment, node->data.statement.data.expression);
            printf("%d\n", value);
            break;
        case DECLARATION:
            interpret(environment, node->data.statement.data.declaration);
            break;
        case WHILE_STATEMENT:
            while (interpret(environment, node->data.statement.data.expression))
            {
                interpret(environment, node->data.statement.data.expression->next);
            }
            break;
        case ASSIGNMENT:
            interpret(environment, node->data.statement.data.assignment);
            break;
        case BLOCK_STATEMENT:
            Environment *new_env = create_empty_environment(environment);
            interpret(new_env, node->data.statement.data.head);
            break;
        default:
            fprintf(stderr, "Unknown statement type %d!", node->data.statement.type);
            exit(EXIT_FAILURE);
            break;
        }
        break;
    case NODE_INTEGER:
        return node->data.integer_value;
    case NODE_TYPE:
        fprintf(stderr, "How did we get here?\n");
        exit(EXIT_FAILURE);
        break;
    case NODE_BINARY_OP:
        lhs = interpret(environment, node->data.binary_op.left);
        rhs = interpret(environment, node->data.binary_op.right);
        switch (node->data.binary_op.op)
        {
        case ADD:
            return lhs + rhs;
        case SUBTRACT:
            return lhs - rhs;
        case MULTIPLY:
            return lhs * rhs;
        case DIVIDE:
            return lhs / rhs;
        case IS_EQUAL:
            return lhs == rhs;
        case IS_LESS_THAN:
            return lhs < rhs;
        case LESS_THAN_EQUAL:
            return lhs <= rhs;
        case IS_GREATER_THAN:
            return lhs > rhs;
        case GREATER_THAN_EQUAL:
            return lhs >= rhs;
        case IS_NOT_EQUAL:
            return lhs != rhs;
        default:
            fprintf(stderr, "Unknown binary operation\n");
            exit(EXIT_FAILURE);
        }
    case NODE_UNARY_OP:
        rhs = interpret(environment, node->data.unary_op.right);
        switch (node->data.unary_op.op)
        {
        case NEGATE:
            return (-1) * rhs;
        case LOGICAL_NOT:
            return !(rhs == 0);
        default:
            fprintf(stderr, "Unknown unary operation\n");
            exit(EXIT_FAILURE);
        }
        break;
    case NODE_IDENTIFIER:
        value = -1;
        exists = get(environment, node->data.identifier_value, &value);
        if (exists == FAILURE)
        {
            fprintf(stderr, "Unknown variable: '%s'\n", node->data.identifier_value);
            exit(EXIT_FAILURE);
        }
        return value;
        break;
    case NODE_DECLARATION:
        status = declare(environment,
                         node->data.declaration.identifier->data.identifier_value,
                         node->data.declaration.right ? interpret(environment, node->data.declaration.right) : -1);
        if (status == FAILURE)
        {
            printf("Failed to declare variable '%s'. Has it been declared before?\n",
                   node->data.declaration.identifier->data.identifier_value);
            exit(EXIT_FAILURE);
        }
        break;
    case NODE_ASSIGNMENT:
        status = set(environment,
                     node->data.assignment.identifier->data.identifier_value,
                     interpret(environment, node->data.assignment.right));
        if (status == FAILURE)
        {
            printf("Failed to set variable '%s'. Has it been declared?\n",
                   node->data.assignment.identifier->data.identifier_value);
            exit(EXIT_FAILURE);
        }
        break;
    case NODE_EOF:
        return SUCCESS;
        break;
    default:
        printf("Unknown node type %d\n", node->type);
        exit(EXIT_FAILURE);
        break;
    }

    if (node->next)
    {
        interpret(environment, node->next);
    }

    return SUCCESS;
}
