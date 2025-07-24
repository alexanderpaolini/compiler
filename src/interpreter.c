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
    env->functions = NULL;  // Initialize functions list

    return env;
}

// Function management functions
Function *lookup_function(Environment *env, char *name)
{
    Function *current = env->functions;
    while (current != NULL)
    {
        if (strcmp(current->name, name) == 0)
        {
            return current;
        }
        current = current->next;
    }
    
    // Look in outer environment if not found
    if (env->outer != NULL)
    {
        return lookup_function(env->outer, name);
    }
    
    return NULL;
}

void store_function(Environment *env, char *name, ASTNode *parameters, ASTNode *body)
{
    Function *new_func = (Function *)malloc(sizeof(Function));
    new_func->name = (char *)malloc((strlen(name) + 1) * sizeof(char));
    strcpy(new_func->name, name);
    new_func->parameters = parameters;
    new_func->body = body;
    new_func->next = env->functions;
    env->functions = new_func;
}

Environment *create_function_environment(Environment *parent)
{
    Environment *env = (Environment *)malloc(sizeof(Environment));
    env->outer = parent;
    
    // Create a dummy state node like in create_empty_environment
    State *state = (State *)malloc(sizeof(State));
    state->data.name = (char *)malloc(1 * sizeof(char));
    strcpy(state->data.name, "");
    int val = -1;
    state->data.data = (int *)&val;
    state->next = NULL;
    env->state = state;
    
    env->functions = NULL;  // Functions are inherited from parent
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
        fprintf(stderr, "Attempting to interpret invalid AST.\n");
        return FAILURE;
    }

    if (environment == NULL)
        environment = create_empty_environment(NULL);

    ASTNode *dummy = node->data.program.head;
    while (dummy)
    {
        switch (dummy->type)
        {
        case NODE_PROGRAM:
            interpret(environment, dummy);
            break;
        case NODE_STATEMENT:
            visit_statement(environment, dummy);
            break;
        case NODE_EOF:
            return SUCCESS;
            break;
        default:
            fprintf(stderr, "Unexpected node type %d!\n", dummy->type);
            return FAILURE;
            break;
        }

        dummy = dummy->next;
    }

    return FAILURE;
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
        fprintf(stderr, "Unknown variable '%s'\n", node->data.identifier_value);
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
        fprintf(stderr, "Unsupported Binary Operation on two different types.\n");
        exit(EXIT_FAILURE);
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
            fprintf(stderr, "Cannot subtract strings.\n");
            exit(EXIT_FAILURE);
            break;
        case MULTIPLY:
            fprintf(stderr, "Cannot multiply strings.\n");
            exit(EXIT_FAILURE);
            break;
        case DIVIDE:
            fprintf(stderr, "Cannot divide strings.\n");
            exit(EXIT_FAILURE);
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
            fprintf(stderr, "Unsupported Binary Operation.\n");
            exit(EXIT_FAILURE);
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
            fprintf(stderr, "Unsupported Binary Operation.\n");
            exit(EXIT_FAILURE);
            break;
        }
        res->data = lhs;
    }
    else
    {
        fprintf(stderr, "Unexpected type '%s'.\n", left->type);
        exit(EXIT_FAILURE);
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
        fprintf(stderr, "Unsupported Unary Operation on non-integer value.\n");
        exit(EXIT_FAILURE);
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
        fprintf(stderr, "Unsupported Unary Operation.\n");
        exit(EXIT_FAILURE);
        break;
    }
    res->data = lhs;

    return res;
}

// Integer will set variable.data to the value of the integer
Variable *visit_expression(Environment *env, ASTNode *node)
{
    if (node == NULL)
    {
        return NULL;
    }

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
    case NODE_FUNCTION_CALL:
        return visit_function_call(env, node);
        break;
    default:
        fprintf(stderr, "How did we get here (1)?\n");
        exit(EXIT_FAILURE);
        break;
    }
}

int visit_declaration(Environment *env, ASTNode *node)
{
    Variable *data = visit_expression(env, node->data.declaration.right);

    // TODO: change this
    if (data == NULL)
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
        fprintf(stderr, "Unable to declare variable '%s'. Has it already bee  declared?\n", node->data.assignment.identifier->data.identifier_value);
        exit(EXIT_FAILURE); // TODO: handle this
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
        fprintf(stderr, "Unable to update variable '%s'. Has it been declared yet?\n", node->data.assignment.identifier->data.identifier_value);
        exit(EXIT_FAILURE); // TODO: handle this
    }

    return SUCCESS;
}

int visit_statement(Environment *env, ASTNode *node)
{
    Environment *new_env;
    switch (node->data.statement.type)
    {
    case DECLARATION:
        visit_declaration(env, node->data.statement.data.declaration);
        break;
    case ASSIGNMENT:
        visit_assignment(env, node->data.statement.data.assignment);
        break;
    case BLOCK_STATEMENT:
        new_env = create_empty_environment(env);
        visit_block_statement(new_env, node->data.statement.data.head);
        break;
    case WHILE_STATEMENT:
        visit_while_statement(env, node->data.statement.data.expression);
        break;
    // case FOR_STATEMENT:
    //     visit_for_statement(env, dummy);
    //     break;
    case PRINT_STATEMENT:
        visit_print_statement(env, node->data.statement.data.expression);
        break;
    case IF_STATEMENT:
        visit_if_statement(env, node->data.statement.data.expression);
        break;
    case FUNCTION_STATEMENT:
        visit_function_declaration(env, node->data.statement.data.expression);
        break;
    case RETURN_STATEMENT:
        visit_return_statement(env, node->data.statement.data.expression);
        break;
    case EXPRESSION_STATEMENT:
        visit_expression(env, node->data.statement.data.expression);
        break;
    default:
        fprintf(stderr, "Unknown statement type %d!\n", node->data.statement.type);
        return FAILURE;
        break;
    }

    return SUCCESS;
}

int visit_block_statement(Environment *env, ASTNode *node)
{
    ASTNode *dummy = node;
    while (dummy)
    {
        visit_statement(env, dummy);
        dummy = dummy->next;
    }
    return SUCCESS;
}

int visit_block_statement_with_return(Environment *env, ASTNode *node, ReturnContext *ret_ctx)
{
    ASTNode *dummy = node;
    while (dummy && !ret_ctx->has_returned)
    {
        visit_statement_with_return(env, dummy, ret_ctx);
        dummy = dummy->next;
    }
    return SUCCESS;
}

int visit_statement_with_return(Environment *env, ASTNode *node, ReturnContext *ret_ctx)
{
    Environment *new_env;
    switch (node->data.statement.type)
    {
    case DECLARATION:
        visit_declaration(env, node->data.statement.data.declaration);
        break;
    case ASSIGNMENT:
        visit_assignment(env, node->data.statement.data.assignment);
        break;
    case BLOCK_STATEMENT:
        new_env = create_empty_environment(env);
        visit_block_statement_with_return(new_env, node->data.statement.data.head, ret_ctx);
        break;
    case WHILE_STATEMENT:
        visit_while_statement(env, node->data.statement.data.expression);
        break;
    case PRINT_STATEMENT:
        visit_print_statement(env, node->data.statement.data.expression);
        break;
    case IF_STATEMENT:
        visit_if_statement(env, node->data.statement.data.expression);
        break;
    case FUNCTION_STATEMENT:
        visit_function_declaration(env, node->data.statement.data.expression);
        break;
    case RETURN_STATEMENT:
        visit_return_statement_with_context(env, node->data.statement.data.expression, ret_ctx);
        break;
    case EXPRESSION_STATEMENT:
        visit_expression(env, node->data.statement.data.expression);
        break;
    default:
        fprintf(stderr, "Unknown statement type %d!\n", node->data.statement.type);
        return FAILURE;
    }
    return SUCCESS;
}

int visit_return_statement_with_context(Environment *env, ASTNode *node, ReturnContext *ret_ctx)
{
    if (node->data.return_statement.expression)
    {
        ret_ctx->value = visit_expression(env, node->data.return_statement.expression);
    }
    else
    {
        ret_ctx->value = NULL;
    }
    ret_ctx->has_returned = 1;
    return SUCCESS;
}

int visit_while_statement(Environment *env, ASTNode *node)
{
    printf("while statement\n");
    Variable *data;
    while (data = visit_expression(env, node), *(int *)data->data)
    {
        visit_statement(env, node->next);
    }
    return SUCCESS;
}

int visit_if_statement(Environment *env, ASTNode *node)
{
    Variable *data = visit_expression(env, node);
    if (*(int *)data->data)
    {
        visit_statement(env, node->next);
    }
    else
    {
        if (node->next->next)
        {
            visit_statement(env, node->next->next);
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
    }
    return SUCCESS;
}

int visit_function_declaration(Environment *env, ASTNode *node)
{
    char *function_name = node->data.function_declaration.identifier->data.identifier_value;
    ASTNode *parameters = node->data.function_declaration.parameters;
    ASTNode *body = node->data.function_declaration.body;
    
    // Store the function in the environment
    store_function(env, function_name, parameters, body);
    
    return SUCCESS;
}

int visit_return_statement(Environment *env, ASTNode *node)
{
    // For now, just print the return value - implement proper return handling later
    if (node->data.return_statement.expression)
    {
        Variable *result = visit_expression(env, node->data.return_statement.expression);
        printf("Return value: %d\n", *(int*)result->data);
    }
    else
    {
        printf("Return (void)\n");
    }
    return SUCCESS;
}

Variable *visit_function_call(Environment *env, ASTNode *node)
{
    char *function_name = node->data.function_call.identifier->data.identifier_value;
    ASTNode *arguments = node->data.function_call.arguments;
    
    // Look up the function
    Function *func = lookup_function(env, function_name);
    if (func == NULL)
    {
        fprintf(stderr, "Function '%s' not found\n", function_name);
        exit(EXIT_FAILURE);
    }
    
    // Create new environment for function execution
    Environment *func_env = create_function_environment(env);
    
    // Bind parameters to arguments
    ASTNode *param = func->parameters;
    ASTNode *arg = arguments;
    
    while (param != NULL && arg != NULL)
    {
        // Evaluate the argument
        Variable *arg_value = visit_expression(env, arg);
        
        // Bind the parameter name to the argument value
        char *param_name = param->data.identifier_value;
        declare(func_env, param_name, arg_value->type, arg_value->data);
        
        param = param->next;
        arg = arg->next;
    }
    
    // Check if parameter and argument counts match
    if (param != NULL || arg != NULL)
    {
        fprintf(stderr, "Parameter/argument count mismatch for function '%s'\n", function_name);
        exit(EXIT_FAILURE);
    }
    
    // Execute function body
    ReturnContext ret_ctx = {NULL, 0};
    
    // Execute the function body (which should be a block statement)
    if (func->body->type == NODE_STATEMENT && func->body->data.statement.type == BLOCK_STATEMENT)
    {
        visit_block_statement_with_return(func_env, func->body->data.statement.data.head, &ret_ctx);
    }
    else
    {
        fprintf(stderr, "Invalid function body for '%s'\n", function_name);
        exit(EXIT_FAILURE);
    }
    
    // Return the value
    if (ret_ctx.has_returned && ret_ctx.value != NULL)
    {
        return ret_ctx.value;
    }
    else
    {
        // Return void/null - create a dummy variable for now
        Variable *res = (Variable *)malloc(sizeof(Variable));
        res->type = "void";
        res->name = "void_return";
        res->data = NULL;
        return res;
    }
}

int visit_eof(Environment *env, ASTNode *node)
{
    return SUCCESS;
}
