#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "util.h"

int parse_is_at_eof(ParserState *state)
{
    return state->cur->type == EOF_TOKEN;
}

Token *parse_peek(ParserState *state)
{
    return state->cur;
}

Token *parse_peek_next(ParserState *state)
{
    return state->cur->next;
}

int parse_match(ParserState *state, const TokenKind expected_tokens[], int len)
{
    Token *current_token = parse_peek(state);

    for (int i = 0; i < len; i++)
    {
        if (expected_tokens[i] == current_token->type)
        {
            return 1;
        }
    }

    return 0;
}

Token *parse_consume(ParserState *state, const TokenKind expected_tokens[], int len)
{
    if (expected_tokens != NULL && !parse_match(state, expected_tokens, len))
    {
        Token *current_token = parse_peek(state);
        char *str = token_to_string(current_token);

        fprintf(stderr, len == 1 ? "Expected: " : "Expected one of: ");
        for (int i = 0; i < len; i++)
        {
            fprintf(stderr, "'%s' ", token_kind_to_string(expected_tokens[i]));
        }

        fprintf(stderr, "\nFound: %s\n", str);
        free(str);
        exit(EXIT_FAILURE);
    }

    Token *temp = state->cur;
    state->cur = state->cur->next;
    return temp;
}

ASTNode *parse_eof(ParserState *state)
{
    TokenKind expected[] = {EOF_TOKEN};
    // Token *current_token =
    // TODO: FIX HERE
    parse_consume(state, expected, sizeof(expected) / sizeof(TokenKind));

    ASTNode *node = create_empty_ast_node();
    node->type = NODE_EOF;

    return node;
}

ASTNode *parse_string(ParserState *state)
{
    TokenKind expected[] = {STRING};
    Token *current_token = parse_consume(state, expected, sizeof(expected) / sizeof(TokenKind));

    ASTNode *node = create_empty_ast_node();

    node->type = NODE_STRING;

    // Should this be moved to its own function?
    // I vote yes because malloc
    node->data.string_value = (char *)malloc(sizeof(char) * (current_token->end_pos - current_token->start_pos + 1));
    strcpy(node->data.string_value, current_token->value);

    return node;
}

ASTNode *parse_number(ParserState *state)
{
    TokenKind expected[] = {NUMBER};
    Token *current_token = parse_consume(state, expected, sizeof(expected) / sizeof(TokenKind));

    ASTNode *node = create_empty_ast_node();

    node->type = NODE_INTEGER;

    // Should this be moved to its own function?
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
    TokenKind expected[] = {TILDE, BANG};
    Token *current_token = parse_consume(state, expected, sizeof(expected) / sizeof(TokenKind));

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
        break;
    }

    return node;
}

ASTNode *parse_additive_operator(ParserState *state)
{
    TokenKind expected[] = {PLUS, MINUS};
    Token *current_token = parse_consume(state, expected, sizeof(expected) / sizeof(TokenKind));

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
        break;
    }

    return node;
}

ASTNode *parse_multiplicative_operator(ParserState *state)
{
    TokenKind expected[] = {STAR, SLASH};
    Token *current_token = parse_consume(state, expected, sizeof(expected) / sizeof(TokenKind));

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
        break;
    }

    return node;
}

ASTNode *parse_comparison_operator(ParserState *state)
{
    TokenKind expected[] = {GREATER, GREATER_EQUAL, LESS, LESS_EQUAL, BANG_EQUAL, EQUAL_EQUAL};
    Token *current_token = parse_consume(state, expected, sizeof(expected) / sizeof(TokenKind));

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
    case BANG_EQUAL:
        node->data.binary_op.op = IS_NOT_EQUAL;
        break;
    case EQUAL_EQUAL:
        node->data.binary_op.op = IS_EQUAL;
        break;
    default:
        break;
    }

    return node;
}

ASTNode *parse_factor(ParserState *state)
{
    Token *current_token = parse_peek(state);

    if (current_token->type == STRING)
    {
        return parse_string(state);
    }

    if (current_token->type == NUMBER)
    {
        return parse_number(state);
    }

    if (current_token->type == LEFT_PAREN)
    {
        parse_consume(state, NULL, 0); // Consume LEFT_PAREN.

        ASTNode *node = parse_expression(state);

        if (parse_peek(state)->type != RIGHT_PAREN)
        {
            char *str = token_to_string(parse_peek(state));
            fprintf(stderr, "Expected 'RIGHT_PAREN'. Found: %s\n", str);
            free(str);
            exit(EXIT_FAILURE);
        }

        parse_consume(state, NULL, 0); // Consume RIGHT_PAREN.

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
        // Check if it's a function call (identifier followed by left paren)
        if (parse_peek_next(state)->type == LEFT_PAREN)
        {
            return parse_function_call(state);
        }
        else
        {
            return parse_identifier(state);
        }
    }

    char *str = token_to_string(current_token);
    fprintf(stderr, "Unexpected Token in Expression. Found: %s\n", str);
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
    TokenKind expected[] = {IDENTIFIER};
    Token *current_token = parse_consume(state, expected, sizeof(expected) / sizeof(TokenKind));

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
        parse_consume(state, NULL, 0); // Consume EQUALS.

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

    TokenKind expected[] = {EQUALS};
    // Token *current_token =
    // TODO: FIX
    parse_consume(state, expected, sizeof(expected) / sizeof(TokenKind));

    ASTNode *node = create_empty_ast_node();
    node->type = NODE_ASSIGNMENT;
    node->data.assignment.identifier = identifier_node;
    node->data.assignment.right = parse_expression(state);

    return node;
}

ASTNode *parse_function_declaration(ParserState *state)
{
    // Parse function name
    ASTNode *identifier_node = parse_identifier(state);
    
    // Expect '('
    TokenKind expected_lparen[] = {LEFT_PAREN};
    parse_consume(state, expected_lparen, sizeof(expected_lparen) / sizeof(TokenKind));
    
    // Parse parameters
    ASTNode *parameters = NULL;
    ASTNode *param_tail = NULL;
    
    if (parse_peek(state)->type != RIGHT_PAREN)
    {
        parameters = parse_identifier(state);
        param_tail = parameters;
        
        while (parse_peek(state)->type == COMMA)
        {
            parse_consume(state, NULL, 0); // Consume COMMA
            param_tail->next = parse_identifier(state);
            param_tail = param_tail->next;
        }
    }
    
    // Expect ')'
    TokenKind expected_rparen[] = {RIGHT_PAREN};
    parse_consume(state, expected_rparen, sizeof(expected_rparen) / sizeof(TokenKind));
    
    // Parse function body (block statement)
    ASTNode *body = parse_statement(state);
    
    // Create function declaration node
    ASTNode *node = create_empty_ast_node();
    node->type = NODE_FUNCTION_DECLARATION;
    node->data.function_declaration.identifier = identifier_node;
    node->data.function_declaration.parameters = parameters;
    node->data.function_declaration.body = body;
    
    return node;
}

ASTNode *parse_function_call(ParserState *state)
{
    // Parse function name
    ASTNode *identifier_node = parse_identifier(state);
    
    // Expect '('
    TokenKind expected_lparen[] = {LEFT_PAREN};
    parse_consume(state, expected_lparen, sizeof(expected_lparen) / sizeof(TokenKind));
    
    // Parse arguments
    ASTNode *arguments = NULL;
    ASTNode *arg_tail = NULL;
    
    if (parse_peek(state)->type != RIGHT_PAREN)
    {
        arguments = parse_expression(state);
        arg_tail = arguments;
        
        while (parse_peek(state)->type == COMMA)
        {
            parse_consume(state, NULL, 0); // Consume COMMA
            arg_tail->next = parse_expression(state);
            arg_tail = arg_tail->next;
        }
    }
    
    // Expect ')'
    TokenKind expected_rparen[] = {RIGHT_PAREN};
    parse_consume(state, expected_rparen, sizeof(expected_rparen) / sizeof(TokenKind));
    
    // Create function call node
    ASTNode *node = create_empty_ast_node();
    node->type = NODE_FUNCTION_CALL;
    node->data.function_call.identifier = identifier_node;
    node->data.function_call.arguments = arguments;
    
    return node;
}

ASTNode *parse_return_statement(ParserState *state)
{
    ASTNode *node = create_empty_ast_node();
    node->type = NODE_RETURN;
    
    // Check if there's an expression to return
    if (parse_peek(state)->type != SEMICOLON)
    {
        node->data.return_statement.expression = parse_expression(state);
    }
    else
    {
        node->data.return_statement.expression = NULL;
    }
    
    return node;
}

ASTNode *parse_statement(ParserState *state)
{
    while (parse_peek(state)->type == SEMICOLON)
    {
        parse_consume(state, NULL, 0); // Consume SEMICOLON.
    }

    ASTNode *node = create_empty_ast_node();
    node->type = NODE_STATEMENT;

    TokenKind expected_semi[] = {SEMICOLON};
    switch (parse_peek(state)->type)
    {
    case IF:
        node->data.statement.type = IF_STATEMENT;
        parse_consume(state, NULL, 0); // Consume IF.
        node->data.statement.data.expression = parse_expression(state);
        node->data.statement.data.expression->next = parse_statement(state);
        if (parse_peek(state)->type == ELSE)
        {
            parse_consume(state, NULL, 0); // Consume ELSE.
            node->data.statement.data.expression->next->next = parse_statement(state);
        }
        return node;
        break;
    case PRINT:
        node->data.statement.type = PRINT_STATEMENT;
        parse_consume(state, NULL, 0); // Consume PRINT.
        node->data.statement.data.expression = parse_expression(state);
        parse_consume(state, expected_semi, sizeof(expected_semi) / sizeof(TokenKind)); // Consume SEMICOLON.
        return node;
        break;
    case WHILE:
        node->data.statement.type = WHILE_STATEMENT;
        parse_consume(state, NULL, 0); // Consume WHILE.
        node->data.statement.data.expression = parse_expression(state);
        node->data.statement.data.expression->next = parse_statement(state);
        return node;
        break;
    case FUNCTION:
        node->data.statement.type = FUNCTION_STATEMENT;
        parse_consume(state, NULL, 0); // Consume FUNCTION.
        node->data.statement.data.expression = parse_function_declaration(state);
        return node;
        break;
    case RETURN:
        node->data.statement.type = RETURN_STATEMENT;
        parse_consume(state, NULL, 0); // Consume RETURN.
        node->data.statement.data.expression = parse_return_statement(state);
        parse_consume(state, expected_semi, sizeof(expected_semi) / sizeof(TokenKind)); // Consume SEMICOLON.
        return node;
        break;
    case IDENTIFIER:
        if (parse_peek_next(state)->type == IDENTIFIER)
        {
            node->data.statement.type = DECLARATION;
            node->data.statement.data.declaration = parse_variable_declaration(state);
        }
        else if (parse_peek_next(state)->type == LEFT_PAREN)
        {
            // Function call as a statement - treat it as expression statement
            node->data.statement.type = EXPRESSION_STATEMENT;
            node->data.statement.data.expression = parse_function_call(state);
        }
        else
        {
            node->data.statement.type = ASSIGNMENT;
            node->data.statement.data.assignment = parse_variable_assignment(state);
        }

        parse_consume(state, expected_semi, sizeof(expected_semi) / sizeof(TokenKind)); // Consume SEMICOLON.

        return node;
        break;
    case LEFT_BRACKET:
        node->data.statement.type = BLOCK_STATEMENT;
        parse_consume(state, NULL, 0); // Consume LEFT_BRACKET.

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

        parse_consume(state, NULL, 0); // Consume RIGHT_BRACKET.

        node->data.statement.data.head = dummy_head->next;
        return node;
    default:
        free(node);

        if (parse_peek(state)->type != RIGHT_BRACKET)
        {
            char *str = token_to_string(parse_peek(state));
            printf("Unexpected Token. Expected Statement. Found: %s\n", str);
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
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
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