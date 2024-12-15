#ifndef UTIL_H
#define UTIL_H

/**
 * Utility functions to help with development
 */
#include "lexer.h"
#include "parser.h"

const char *token_kind_to_string(TokenKind type);

char *token_to_string(Token *tok);

void print_list(Token *head, char *prog);

const char *binary_op_to_str(BinaryOp op);

const char *unary_op_to_str(UnaryOp op);

void print_ast(ASTNode *node);

#endif // UTIL_H