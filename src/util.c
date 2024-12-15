#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"

const char *token_kind_to_string(TokenKind type)
{
    switch (type)
    {
    case IF:
        return "IF";
    case ELSE:
        return "ELSE";
    case WHILE:
        return "WHILE";
    case FOR:
        return "FOR";
    case PRINT:
        return "PRINT";
    case PLUS:
        return "PLUS";
    case MINUS:
        return "MINUS";
    case STAR:
        return "STAR";
    case DOUBLE_STAR:
        return "DOUBLE_STAR";
    case SLASH:
        return "SLASH";
    case DOUBLE_SLASH:
        return "DOUBLE_SLASH";
    case GREATER:
        return "GREATER";
    case GREATER_EQUAL:
        return "GREATER_EQUAL";
    case LESS:
        return "LESS";
    case LESS_EQUAL:
        return "LESS_EQUAL";
    case BANG_EQUAL:
        return "BANG_EQUAL";
    case EQUAL_EQUAL:
        return "EQUAL_EQUAL";
    case TILDE:
        return "TILDE";
    case BANG:
        return "BANG";
    case LEFT_PAREN:
        return "LEFT_PAREN";
    case RIGHT_PAREN:
        return "RIGHT_PAREN";
    case LEFT_BRACKET:
        return "LEFT_BRACKET";
    case RIGHT_BRACKET:
        return "RIGHT_BRACKET";
    case NUMBER:
        return "NUMBER";
    case STRING:
        return "STRING";
    case IDENTIFIER:
        return "IDENTIFIER";
    case EQUALS:
        return "EQUALS";
    case _EOF:
        return "_EOF";
    case SEMICOLON:
        return "SEMICOLON";

    default:
        return "UNKNOWN";
    }
}

char *token_to_string(Token *tok)
{
    char *x = (char *)malloc(100 * sizeof(char));

    if (x == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for TokenString.\n");
        exit(EXIT_FAILURE);
    }

    sprintf(x,
            "Token { type: %s, line: %d, pos: %d, length: %d, value: \"%s\" }\n",
            token_kind_to_string(tok->type),
            tok->line,
            tok->start_pos,
            tok->end_pos - tok->start_pos,
            tok->value);

    return x;
}

void print_list(Token *head, char *prog)
{
    Token *current = head;

    while (current != NULL)
    {
        char *str = token_to_string(current);
        printf("%s", str);
        free(str);

        current = current->next;
    }
}
// Helper function to print binary operation
const char *binary_op_to_str(BinaryOp op)
{
    switch (op)
    {
    case ADD:
        return "+";
    case SUBTRACT:
        return "-";
    case MULTIPLY:
        return "*";
    case DIVIDE:
        return "/";
    case IS_EQUAL:
        return "==";
    case IS_LESS_THAN:
        return "<";
    case IS_GREATER_THAN:
        return ">";
    case GREATER_THAN_EQUAL:
        return ">=";
    case LESS_THAN_EQUAL:
        return "<";
    case IS_NOT_EQUAL:
        return "!=";
    default:
        return "?";
    }
}

// Helper function to print unary operation
const char *unary_op_to_str(UnaryOp op)
{
    switch (op)
    {
    case LOGICAL_NOT:
        return "!";
    case NEGATE:
        return "~";
    default:
        return "?";
    }
}

int indent = 0;
// Recursive function to print AST nodes
void print_ast(ASTNode *node)
{
    if (node == NULL)
    {
        return;
    }

    switch (node->type)
    {
    case NODE_PROGRAM:
        print_ast(node->data.program.head->next);
        break;
    case NODE_STATEMENT:
        for (int i = 0; i < indent; i++)
            printf(" ");
        switch (node->data.statement.type)
        {
        case IF_STATEMENT:
            printf("IF (");
            ASTNode *if_body = node->data.statement.data.expression->next;
            node->data.statement.data.expression->next = NULL;
            print_ast(node->data.statement.data.expression);
            printf(") ");
            print_ast(if_body);
            node->data.statement.data.expression->next = if_body;
            if (if_body->next)
            {
                if_body = if_body->next;
                printf("ELSE ");
                print_ast(if_body);
            }
            break;
        case PRINT_STATEMENT:
            printf("PRINT ");
            print_ast(node->data.statement.data.expression);
            printf(";");
            break;
        case DECLARATION:
            print_ast(node->data.statement.data.declaration);
            printf(";");
            break;
        case WHILE_STATEMENT:
            printf("WHILE (");
            ASTNode *while_body = node->data.statement.data.expression->next;
            node->data.statement.data.expression->next = NULL;
            print_ast(node->data.statement.data.expression);
            printf(")\n");
            indent += 2;
            print_ast(while_body);
            indent -= 2;
            node->data.statement.data.expression->next = while_body;
            break;
        case ASSIGNMENT:
            print_ast(node->data.statement.data.assignment);
            printf(";");
            break;
        case BLOCK_STATEMENT:
            printf("{\n");
            indent += 2;
            print_ast(node->data.statement.data.head);
            printf("\n");
            indent -= 2;
            for (int i = 0; i < indent; i++)
                printf(" ");
            printf("}");
            break;
        default:
            printf("Unknown statement type %d", node->data.statement.type);
            break;
        }
        break;
    case NODE_INTEGER:
        printf("%d", node->data.integer_value);
        break;
    case NODE_TYPE:
        print_ast(node->data.type.identifier);
        break;
    case NODE_BINARY_OP:
        printf("(");
        print_ast(node->data.binary_op.left);                      // Print left operand
        printf(" %s ", binary_op_to_str(node->data.binary_op.op)); // Print operator
        print_ast(node->data.binary_op.right);                     // Print right operand
        printf(")");
        break;
    case NODE_UNARY_OP:
        printf("%s", unary_op_to_str(node->data.unary_op.op)); // Print operator
        print_ast(node->data.unary_op.right);                  // Print right operand
        break;
    case NODE_EOF:
        printf("EOF\n");
        break;
    case NODE_IDENTIFIER:
        printf("%s", node->data.identifier_value);
        break;
    case NODE_DECLARATION:
        print_ast(node->data.declaration.type);
        printf(" ");
        print_ast(node->data.declaration.identifier);
        if (node->data.declaration.right != NULL)
        {
            printf(" = ");
            print_ast(node->data.declaration.right);
        }
        break;
    case NODE_ASSIGNMENT:
        print_ast(node->data.assignment.identifier);
        printf(" = ");
        print_ast(node->data.assignment.right);
        break;
    default:
        printf("Unknown node type %d", node->type);
        break;
    }

    if (node->next != NULL)
    {
        printf("\n");
        print_ast(node->next);
    }
}
