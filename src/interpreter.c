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
int get(Environment *environment, char *name, Variable *value)
{
    if (environment == NULL)
    {
        return FAILURE;
    }
    State *dummy = environment->state;
    while (dummy != NULL)
    {
        if (strcmp(name, dummy->data.name) == 0)
        {
            value->data = dummy->data.data;
            value->type = dummy->data.type;
            value->name = dummy->data.name;
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

int set(Environment *environment, char *name, void *data)
{
    Variable temp;
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
        if (strcmp(name, dummy->data.name) == 0)
        {
            dummy->data.data = data;
            return SUCCESS;
        }
        dummy = dummy->next;
    }

    return FAILURE;
}

// Declares the variable in the current (lowest) environment.
int declare(Environment *environment, char *name, char *type, void *data)
{
    // Declare is either called when "int NAME (= VALUE)?;" is written.
    // As such, it cannot be used when a variable already exists.
    Variable temp;
    int exists = get(environment, name, &temp);
    if (exists != -1)
    {
        return FAILURE;
    }

    State *new = (State *)malloc(sizeof(State));
    new->data.name = (char *)malloc(strlen(name));
    strcpy(new->data.name, name);
    new->data.type = (char *)malloc(strlen(type));
    strcpy(new->data.type, type);
    new->data.data = data;
    new->next = NULL;

    State *dummy = environment->state;
    while (dummy && dummy->next != NULL)
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
    State *state = (State *)malloc(sizeof(State));

    state->data.name = (char *)malloc(1 * sizeof(char));
    strcpy(state->data.name, "");
    int val = -1;
    state->data.data = (int *)&val;
    state->next = NULL;
    env->state = state;

    return env;
}

// INT as in STATUS
// 0 = good     !0 = bad
// Interpret takes the nodes capable of holding an environment
// Program; Block Statement
int interpret(Environment *environment, ASTNode *node)
{
    if (node == NULL)
    {
        fprintf(stderr, "Runtime Error: Attempting to interpret invalid AST.\n");
        return FAILURE;
    }

    if (environment == NULL)
        environment = create_empty_environment(NULL);

    ASTNode *dummy = node->data.program.head;
    int status = SUCCESS;
    while (dummy)
    {
        switch (dummy->type)
        {
        case NODE_PROGRAM:
            status = interpret(environment, dummy);
            break;
        case NODE_STATEMENT:
            status = visit_statement(environment, dummy);
            break;
        case NODE_EOF:
            return SUCCESS;
            break;
        default:
            fprintf(stderr, "Runtime Error: Unexpected node type %d!\n", dummy->type);
            return FAILURE;
            break;
        }

        if (status == FAILURE)
        {
            return FAILURE;
        }

        dummy = dummy->next;
    }

    return SUCCESS;
}

Variable *visit_string(Environment *env, ASTNode *node)
{
    Variable *res = (Variable *)malloc(sizeof(Variable));
    res->type = "str";
    res->name = "dummy";

    char *value = (char *)malloc(sizeof(char) * (strlen(node->data.string_value) + 1));
    strcpy(value, node->data.string_value);
    res->data = value;

    return res;
}

Variable *visit_integer(Environment *env, ASTNode *node)
{
    Variable *res = (Variable *)malloc(sizeof(Variable));
    res->type = "int";
    res->name = "dummy";

    int *value = (int *)malloc(sizeof(int));
    *value = node->data.integer_value;
    res->data = value;

    return res;
}

Variable *visit_identifier(Environment *env, ASTNode *node)
{
    Variable *res = (Variable *)malloc(sizeof(Variable));
    int status = get(env, node->data.identifier_value, res);
    if (status == FAILURE)
    {
        fprintf(stderr, "Runtime Error: Unknown variable '%s'\n", node->data.identifier_value);
        exit(EXIT_FAILURE); // TODO: handle this
    }
    return res;
}

Variable *visit_binary_op(Environment *env, ASTNode *node)
{
    Variable *res = (Variable *)malloc(sizeof(Variable));
    res->name = "dummy";

    Variable *left = visit_expression(env, node->data.binary_op.left);
    Variable *right = visit_expression(env, node->data.binary_op.right);

    if (strcmp(left->type, right->type) != 0)
    {
        fprintf(stderr, "Runtime Error: Unsupported Binary Operation on two different types.\n");
        exit(EXIT_FAILURE); // TODO: handle this
    }

    if (strcmp(left->type, "str") == 0)
    {
        res->type = "int";
        int *lhs = malloc(sizeof(int));
        switch (node->data.binary_op.op)
        {
        case ADD:
            res->type = "str";
            lhs = malloc(strlen(left->data) + strlen(right->data) + 1);
            strcpy((char *)lhs, left->data);
            strcat((char *)lhs, right->data);
            break;
        case SUBTRACT:
            fprintf(stderr, "Runtime Error: Cannot subtract strings.\n");
            exit(EXIT_FAILURE); // TODO: handle this
            break;
        case MULTIPLY:
            fprintf(stderr, "Runtime Error: Cannot multiply strings.\n");
            exit(EXIT_FAILURE); // TODO: handle this
            break;
        case DIVIDE:
            fprintf(stderr, "Cannot divide strings.\n");
            exit(EXIT_FAILURE); // TODO: handle this
            break;
        case IS_EQUAL:
            *lhs = (strcmp(left->data, right->data) == 0);
            break;
        case IS_LESS_THAN:
            *lhs = (strcmp(left->data, right->data) < 0);
            break;
        case LESS_THAN_EQUAL:
            *lhs = (strcmp(left->data, right->data) <= 0);
            break;
        case IS_GREATER_THAN:
            *lhs = (strcmp(left->data, right->data) > 0);
            break;
        case GREATER_THAN_EQUAL:
            *lhs = (strcmp(left->data, right->data) >= 0);
            break;
        case IS_NOT_EQUAL:
            *lhs = (strcmp(left->data, right->data) != 0);
            break;
        default:
            fprintf(stderr, "Runtime Error: Unsupported Binary Operation.\n");
            exit(EXIT_FAILURE); // TODO: handle this
            break;
        }
        res->data = lhs;
    }
    else if (strcmp(left->type, "int") == 0)
    {
        res->type = "int";
        char *lhs = malloc(sizeof(int));
        switch (node->data.binary_op.op)
        {
        case ADD:
            *lhs = *(int *)left->data + *(int *)right->data;
            break;
        case SUBTRACT:
            *lhs = *(int *)left->data - *(int *)right->data;
            break;
        case MULTIPLY:
            *lhs = *(int *)left->data * *(int *)right->data;
            break;
        case DIVIDE:
            *lhs = *(int *)left->data / *(int *)right->data;
            break;
        case IS_EQUAL:
            *lhs = *(int *)left->data == *(int *)right->data;
            break;
        case IS_LESS_THAN:
            *lhs = *(int *)left->data < *(int *)right->data;
            break;
        case LESS_THAN_EQUAL:
            *lhs = *(int *)left->data <= *(int *)right->data;
            break;
        case IS_GREATER_THAN:
            *lhs = *(int *)left->data > *(int *)right->data;
            break;
        case GREATER_THAN_EQUAL:
            *lhs = *(int *)left->data >= *(int *)right->data;
            break;
        case IS_NOT_EQUAL:
            *lhs = *(int *)left->data != *(int *)right->data;
            break;
        default:
            fprintf(stderr, "Runtime Error: Unsupported Binary Operation.\n");
            exit(EXIT_FAILURE); // TODO: handle this
            break;
        }
        res->data = lhs;
    }
    else
    {
        fprintf(stderr, "Runtime Error: Unexpected type '%s'.\n", left->type);
        exit(EXIT_FAILURE); // TODO: handle this
    }

    return res;
}

Variable *visit_unary_op(Environment *env, ASTNode *node)
{
    Variable *res = (Variable *)malloc(sizeof(Variable));
    res->type = "int";
    res->name = "dummy";

    Variable *right = visit_expression(env, node->data.unary_op.right);

    if (strcmp(right->type, "int") != 0)
    {
        fprintf(stderr, "Runtime Error: Unsupported Unary Operation on non-integer value.\n");
        exit(EXIT_FAILURE); // TODO: handle this
    }

    int *lhs = malloc(sizeof(int));
    switch (node->data.unary_op.op)
    {
    case NEGATE:
        *lhs = 0 - (*(int *)right->data);
        break;
    case LOGICAL_NOT:
        *lhs = !*(int *)right->data;
        break;
    default:
        fprintf(stderr, "Runtime Error: Unsupported Unary Operation.\n");
        exit(EXIT_FAILURE); // TODO: handle this
        break;
    }
    res->data = lhs;

    return res;
}

// Integer will set variable.data to the value of the integer
Variable *visit_expression(Environment *env, ASTNode *node)
{
    switch (node->type)
    {
    case NODE_STRING:
        return visit_string(env, node);
        break;
    case NODE_INTEGER:
        return visit_integer(env, node);
        break;
    case NODE_IDENTIFIER:
        return visit_identifier(env, node);
        break;
    case NODE_BINARY_OP:
        return visit_binary_op(env, node);
        break;
    case NODE_UNARY_OP:
        return visit_unary_op(env, node);
        break;
    default:
        fprintf(stderr, "Runtime Error: Invalid Expression.\n");
        return NULL;
        break;
    }
}

int visit_declaration(Environment *env, ASTNode *node)
{
    Variable *data;
    if (node->data.declaration.right)
    {
        data = visit_expression(env, node->data.declaration.right);
    }
    else
    {
        data = (Variable *)malloc(sizeof(Variable));

        int *value = (int *)malloc(sizeof(int));
        *value = -1;
        data->data = value;

        data->name = "dummy";
        data->type = "void";
    }

    int status = declare(
        env,
        node->data.declaration.identifier->data.identifier_value,
        node->data.declaration.type->data.type.identifier->data.identifier_value,
        data->data);

    if (status == FAILURE)
    {
        fprintf(stderr, "Runtime Error: Unable to declare variable '%s'. Has it already been declared?\n", node->data.declaration.identifier->data.identifier_value);
        return FAILURE;
    }

    return SUCCESS;
}

int visit_assignment(Environment *env, ASTNode *node)
{
    Variable *data = visit_expression(env, node->data.assignment.right);

    int status = set(
        env,
        node->data.assignment.identifier->data.identifier_value,
        data->data);

    if (status == FAILURE)
    {
        fprintf(stderr, "Runtime Error: Unable to update variable '%s'. Has it been declared yet?\n", node->data.assignment.identifier->data.identifier_value);
        exit(EXIT_FAILURE); // TODO: handle this
    }

    return SUCCESS;
}

int visit_statement(Environment *env, ASTNode *node)
{
    Environment *new_env;
    // int status = SUCCESS;
    switch (node->data.statement.type)
    {
    case DECLARATION:
        return visit_declaration(env, node->data.statement.data.declaration);
        break;
    case ASSIGNMENT:
        return visit_assignment(env, node->data.statement.data.assignment);
        break;
    case BLOCK_STATEMENT:
        new_env = create_empty_environment(env);
        int status = visit_block_statement(new_env, node->data.statement.data.head);
        // TODO: free env
        return status;
        break;
    case WHILE_STATEMENT:
        return visit_while_statement(env, node->data.statement.data.expression);
        break;
    // case FOR_STATEMENT:
    //     visit_for_statement(env, dummy);
    //     break;
    case PRINT_STATEMENT:
        return visit_print_statement(env, node->data.statement.data.expression);
        break;
    case IF_STATEMENT:
        return visit_if_statement(env, node->data.statement.data.expression);
        break;
    default:
        fprintf(stderr, "Runtime Error: Unknown statement type %d!\n", node->data.statement.type);
        return FAILURE;
        break;
    }
}

int visit_block_statement(Environment *env, ASTNode *node)
{
    ASTNode *dummy = node;
    while (dummy)
    {
        int status = visit_statement(env, dummy);
        if (status == FAILURE)
        {
            return FAILURE;
        }
        dummy = dummy->next;
    }
    return SUCCESS;
}

int visit_while_statement(Environment *env, ASTNode *node)
{
    Variable *data;
    while (data = visit_expression(env, node), *(int *)data->data)
    {
        int status = visit_statement(env, node->next);
        if (status == FAILURE)
        {
            return FAILURE;
        }
    }
    return SUCCESS;
}

int visit_if_statement(Environment *env, ASTNode *node)
{
    Variable *data = visit_expression(env, node);
    if (*(int *)data->data)
    {
        return visit_statement(env, node->next);
    }
    else
    {
        if (node->next->next)
        {
            return visit_statement(env, node->next->next);
        }
    }
    return SUCCESS;
}

int visit_for_statement(Environment *env, ASTNode *node)
{
    return SUCCESS;
}

int visit_print_statement(Environment *env, ASTNode *node)
{
    Variable *data = visit_expression(env, node);
    if (strcmp(data->type, "int") == 0)
    {
        int *int_ptr = (int *)data->data;
        printf("%d\n", *int_ptr);
    }
    else if (strcmp(data->type, "str") == 0)
    {
        char *str_ptr = (char *)data->data;
        printf("%s\n", str_ptr);
    }
    else
    {
        // SHOULD NOT HAPPEN?
        fprintf(stderr, "Runtime Error: Cannot print invalid type\n");
        return FAILURE;
    }
    return SUCCESS;
}

int visit_eof(Environment *env, ASTNode *node)
{
    return SUCCESS;
}
