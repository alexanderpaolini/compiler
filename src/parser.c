#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "util.h"

int parse_is_at_eof(ParserState *state)
{
    return state->cur->type == _EOF;
}

Token *parse_peek(ParserState *state)
{
    return state->cur;
}

Token *parse_peek_next(ParserState *state)
{
    return state->cur->next;
}

Token *parse_consume(ParserState *state)
{
    Token *temp = state->cur;
    state->cur = state->cur->next;
    return temp;
}

ASTNode *parse_eof(ParserState *state)
{
    Token *current_token = parse_consume(state);
    TokenKind token_type = current_token->type;

    if (token_type != _EOF)
    {
        char *str = token_to_string(current_token);
        fprintf(stderr, "Expected 'EOF'. Found: %s\n", str);
        free(str);
        exit(EXIT_FAILURE);
    }

    ASTNode *node = create_empty_ast_node();
    node->type = NODE_EOF;

    return node;
}

ASTNode *parse_number(ParserState *state)
{
    Token *current_token = parse_consume(state);

    if (current_token->type != NUMBER)
    {
        char *str = token_to_string(current_token);
        fprintf(stderr, "Expected 'NUMBER'. Found: %s\n", str);
        free(str);
        exit(EXIT_FAILURE);
    }

    ASTNode *node = create_empty_ast_node();

    node->type = NODE_INTEGER;

    // Should this be moved to its own function?
    // calculate_integer_value(ParserState *state, Token *token)
    node->data.integer_value = 0;
    for (int i = current_token->start_pos; i < current_token->end_pos; i++)
    {
        node->data.integer_value *= 10;
        node->data.integer_value += state->prog[i] - '0';
    }

    return node;
}

ASTNode *parse_unary_operator(ParserState *state)
{
    Token *current_token = parse_consume(state);

    ASTNode *node = create_empty_ast_node();
    node->type = NODE_UNARY_OP;
    switch (current_token->type)
    {
    case TILDE:
        node->data.unary_op.op = NEGATE;
        break;
    case BANG:
        node->data.unary_op.op = LOGICAL_NOT;
        break;
    default:
        char *str = token_to_string(current_token);
        fprintf(stderr, "Expected 'UnaryOperator'. Found: %s\n", str);
        free(str);
        exit(EXIT_FAILURE);
    }

    return node;
}

ASTNode *parse_additive_operator(ParserState *state)
{
    Token *current_token = parse_consume(state);

    ASTNode *node = create_empty_ast_node();
    node->type = NODE_BINARY_OP;

    switch (current_token->type)
    {
    case PLUS:
        node->data.binary_op.op = ADD;
        break;
    case MINUS:
        node->data.binary_op.op = SUBTRACT;
        break;
    default:
        char *str = token_to_string(current_token);
        fprintf(stderr, "Expected 'AdditiveOperator'. Found: %s\n", str);
        free(str);
        exit(EXIT_FAILURE);
    }

    return node;
}

ASTNode *parse_multiplicative_operator(ParserState *state)
{
    Token *current_token = parse_consume(state);

    ASTNode *node = create_empty_ast_node();
    node->type = NODE_BINARY_OP;

    switch (current_token->type)
    {
    case STAR:
        node->data.binary_op.op = MULTIPLY;
        break;
    case SLASH:
        node->data.binary_op.op = DIVIDE;
        break;
    default:
        char *str = token_to_string(current_token);
        fprintf(stderr, "Expected 'MultiplicativeOperator'. Found: %s\n", str);
        free(str);
        exit(EXIT_FAILURE);
    }

    return node;
}

ASTNode *parse_comparison_operator(ParserState *state)
{
    Token *current_token = parse_consume(state);

    ASTNode *node = create_empty_ast_node();
    node->type = NODE_BINARY_OP;

    switch (current_token->type)
    {
    case GREATER:
        node->data.binary_op.op = IS_GREATER_THAN;
        break;
    case GREATER_EQUAL:
        node->data.binary_op.op = GREATER_THAN_EQUAL;
        break;
    case LESS:
        node->data.binary_op.op = IS_LESS_THAN;
        break;
    case LESS_EQUAL:
        node->data.binary_op.op = IS_GREATER_THAN;
        break;
    case EQUALS:
        node->data.binary_op.op = IS_EQUAL;
        break;
    case BANG_EQUAL:
        node->data.binary_op.op = IS_NOT_EQUAL;
        break;
    case EQUAL_EQUAL:
        node->data.binary_op.op = IS_EQUAL;
        break;
    default:
        char *str = token_to_string(current_token);
        fprintf(stderr, "Expected 'ComparisonOperator'. Found: %s\n", str);
        free(str);
        exit(EXIT_FAILURE);
    }

    return node;
}

ASTNode *parse_factor(ParserState *state)
{
    Token *current_token = parse_peek(state);

    if (current_token->type == NUMBER)
    {
        return parse_number(state);
    }

    if (current_token->type == LEFT_PAREN)
    {
        parse_consume(state); // Consume LEFT_PAREN.

        ASTNode *node = parse_expression(state);

        if (parse_peek(state)->type != RIGHT_PAREN)
        {
            char *str = token_to_string(parse_peek(state));
            fprintf(stderr, "Expected 'RIGHT_PAREN'. Found: %s\n", str);
            free(str);
            exit(EXIT_FAILURE);
        }
        parse_consume(state); // Consume RIGHT_PAREN.

        return node;
    }

    if (current_token->type == BANG || current_token->type == TILDE)
    {
        ASTNode *node = parse_unary_operator(state);
        node->data.unary_op.right = parse_factor(state);
        return node;
    }

    if (current_token->type == IDENTIFIER)
    {
        return parse_identifier(state);
    }

    char *str = token_to_string(current_token);
    fprintf(stderr, "Unexpected Token. Found: %s\n", str);
    free(str);
    exit(EXIT_FAILURE);
}

ASTNode *parse_term(ParserState *state)
{
    ASTNode *node = parse_factor(state);

    while (parse_peek(state)->type == STAR || parse_peek(state)->type == SLASH)
    {
        ASTNode *op_node = parse_multiplicative_operator(state);
        op_node->data.binary_op.left = node;
        op_node->data.binary_op.right = parse_factor(state);
        node = op_node;
    }

    return node;
}

ASTNode *parse_expression(ParserState *state)
{
    ASTNode *node = parse_term(state);

    while (parse_peek(state)->type == PLUS || parse_peek(state)->type == MINUS)
    {
        ASTNode *op_node = parse_additive_operator(state);
        op_node->data.binary_op.left = node;
        op_node->data.binary_op.right = parse_term(state);
        node = op_node;
    }

    while (
        parse_peek(state)->type == GREATER ||
        parse_peek(state)->type == GREATER_EQUAL ||
        parse_peek(state)->type == LESS ||
        parse_peek(state)->type == LESS_EQUAL ||
        parse_peek(state)->type == BANG_EQUAL ||
        parse_peek(state)->type == EQUAL_EQUAL)
    {
        ASTNode *op_node = parse_comparison_operator(state);
        op_node->data.binary_op.left = node;
        op_node->data.binary_op.right = parse_term(state);
        node = op_node;
    }

    return node;
}

ASTNode *parse_identifier(ParserState *state)
{
    Token *current_token = parse_consume(state);

    if (current_token->type != IDENTIFIER)
    {
        char *str = token_to_string(current_token);
        fprintf(stderr, "Expected 'IDENTIFIER'. Found: %s\n", str);
        free(str);
        exit(EXIT_FAILURE);
    }

    ASTNode *node = create_empty_ast_node();
    node->type = NODE_IDENTIFIER;

    size_t identifier_length = current_token->end_pos - current_token->start_pos;
    node->data.identifier_value = (char *)malloc(identifier_length + 1);
    if (node->data.identifier_value == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for 'IDENTIFIER'.\n");
        exit(EXIT_FAILURE);
    }
    strcpy(node->data.identifier_value, current_token->value);

    return node;
}

ASTNode *parse_type(ParserState *state)
{
    ASTNode *node = create_empty_ast_node();
    node->type = NODE_TYPE;
    node->data.type.identifier = parse_identifier(state);
    // This should be changed soon.
    if (strcmp(node->data.type.identifier->data.identifier_value, "int") != 0)
    {
        fprintf(stderr, "Expected 'int'. Found: %s.\n", node->data.type.identifier->data.identifier_value);
        exit(EXIT_FAILURE);
    }
    return node;
}

ASTNode *parse_variable_declaration(ParserState *state)
{
    ASTNode *type_node = parse_type(state);
    ASTNode *identifier_node = parse_identifier(state);

    ASTNode *node = create_empty_ast_node();
    node->type = NODE_DECLARATION;
    node->data.declaration.type = type_node;
    node->data.declaration.identifier = identifier_node;

    if (parse_peek(state)->type == EQUALS)
    {
        parse_consume(state); // Consume EQUALS.

        node->data.declaration.right = parse_expression(state);
    }
    else
    {
        node->data.declaration.right = NULL;
    }

    return node;
}

ASTNode *parse_variable_assignment(ParserState *state)
{
    ASTNode *identifier_node = parse_identifier(state);

    Token *current_token = parse_consume(state);

    if (current_token->type != EQUALS)
    {
        char *str = token_to_string(current_token);
        printf("Expected 'EQUALS'. Found: %s\n", str);
        free(str);
        exit(EXIT_FAILURE);
    }

    ASTNode *node = create_empty_ast_node();
    node->type = NODE_ASSIGNMENT;
    node->data.assignment.identifier = identifier_node;
    node->data.assignment.right = parse_expression(state);

    return node;
}

ASTNode *parse_statement(ParserState *state)
{
    while (parse_peek(state)->type == SEMICOLON)
    {
        parse_consume(state); // Consume SEMICOLON.
    }

    ASTNode *node = create_empty_ast_node();
    node->type = NODE_STATEMENT;

    switch (parse_peek(state)->type)
    {
    case IF:
        node->data.statement.type = IF_STATEMENT;
        parse_consume(state); // Consume IF.
        node->data.statement.data.expression = parse_expression(state);
        node->data.statement.data.expression->next = parse_statement(state);
        if (parse_peek(state)->type == ELSE)
        {
            parse_consume(state); // Consume ELSE.
            node->data.statement.data.expression->next->next = parse_statement(state);
        }
        return node;
        break;
    case PRINT:
        node->data.statement.type = PRINT_STATEMENT;
        parse_consume(state); // Consume PRINT.
        node->data.statement.data.expression = parse_expression(state);
        return node;
        break;
    case WHILE:
        node->data.statement.type = WHILE_STATEMENT;
        parse_consume(state); // Consume WHILE.
        node->data.statement.data.expression = parse_expression(state);
        node->data.statement.data.expression->next = parse_statement(state);
        return node;
        break;
    case IDENTIFIER:
        if (parse_peek_next(state)->type == IDENTIFIER)
        {
            node->data.statement.type = DECLARATION;
            node->data.statement.data.declaration = parse_variable_declaration(state);
        }
        else
        {
            node->data.statement.type = ASSIGNMENT;
            node->data.statement.data.assignment = parse_variable_assignment(state);
        }

        parse_consume(state); // Consume SEMICOLON.

        return node;
        break;
    case LEFT_BRACKET:
        node->data.statement.type = BLOCK_STATEMENT;
        parse_consume(state); // Consume LEFT_BRACKET.

        ASTNode *dummy_head = create_empty_ast_node();
        ASTNode *dummy_tail = dummy_head;

        while (parse_peek(state)->type != RIGHT_BRACKET)
        {
            ASTNode *temp = parse_statement(state);

            if (temp != NULL)
            {
                dummy_tail->next = temp;
                dummy_tail = dummy_tail->next;
                dummy_tail->next = NULL;
            }
        }
        parse_consume(state); // Consume RIGHT_BRACKET.

        node->data.statement.data.head = dummy_head->next;
        return node;
    case SEMICOLON:
        parse_consume(state); // Consume SEMICOLON.
        return NULL;
    default:
        free(node);

        if (parse_peek(state)->type != RIGHT_BRACKET)
        {
            char *str = token_to_string(parse_peek(state));
            printf("Unexpected Token. Found: %s\n", str);
            free(str);
            exit(EXIT_FAILURE);
        }

        return NULL;
    }

    return NULL;
}

ASTNode *parser(ParserState *state)
{
    while (!parse_is_at_eof(state))
    {
        ASTNode *node = parse_statement(state);

        if (node)
        {
            state->node->data.program.tail->next = node;
            state->node->data.program.tail = state->node->data.program.tail->next;
            state->node->data.program.tail->next = NULL;
        }
    }

    ASTNode *eof_node = parse_eof(state);
    state->node->data.program.tail->next = eof_node;
    state->node->data.program.tail = state->node->data.program.tail->next;

    return state->node;
}

ASTNode *create_empty_ast_node()
{
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (node == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for ASTNode.\n");
        exit(EXIT_FAILURE);
    }
    node->next = NULL;
    return node;
}

ParserState *create_parser_state(char *program, Token *head)
{
    ParserState *parser_state = (ParserState *)malloc(sizeof(ParserState));
    if (parser_state == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for ParserState.\n");
        exit(EXIT_FAILURE);
    }

    // Program
    parser_state->prog = program;
    // Tokens
    parser_state->head = head;
    parser_state->cur = head;
    // PROGRAM_NODE
    parser_state->node = create_empty_ast_node();
    if (parser_state->node == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for ParserState->head_node.\n");
        exit(EXIT_FAILURE);
    }
    parser_state->node->type = NODE_PROGRAM;
    parser_state->node->data.program.head = create_empty_ast_node();
    parser_state->node->data.program.tail = parser_state->node->data.program.head;

    return parser_state;
}

void free_parser_state(ParserState *state) {
    // THIS FUNCTION DOESN'T DO ANYTHING BECAUSE I DON'T CARE ENOUGH
};