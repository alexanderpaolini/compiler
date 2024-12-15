#ifndef LEXER_H
#define LEXER_H

typedef enum TokenKind
{
    // Statements
    IF,
    ELSE,
    WHILE,
    FOR,
    PRINT,

    // Binary Expressions
    PLUS,
    MINUS,
    STAR,
    DOUBLE_STAR,
    SLASH,
    DOUBLE_SLASH,
    GREATER,
    GREATER_EQUAL,
    LESS,
    LESS_EQUAL,
    BANG_EQUAL,
    EQUAL_EQUAL,

    // Unary Expressions
    TILDE,
    BANG,

    // Parenthesis
    LEFT_PAREN,
    RIGHT_PAREN,

    // Blocks
    LEFT_BRACKET,
    RIGHT_BRACKET,

    // Utility
    _EOF,
    SEMICOLON,
    EQUALS,
    IDENTIFIER,
    NUMBER,
    STRING,
} TokenKind;

typedef struct Token
{
    TokenKind type;
    int start_pos;
    int end_pos;
    int line;
    char *value;
    struct Token *next;
} Token;

typedef struct LexerState
{
    int pos;
    char *prog;
    struct Token *tail;
    int line_num;
} LexerState;

int is_whitespace(char c);
void lex_advance(LexerState *state, int n);
char lex_consume(LexerState *state);
char lex_peek(LexerState *state);
char lex_peek_next(LexerState *state);
char lex_peek_n(LexerState *state, int n);
void lex_emit(LexerState *state, TokenKind type, int start_pos, int end_pos);
void lex_number(LexerState *state);
void lex_identifier(LexerState *state);
void lex_string(LexerState *state);
Token *lexer(LexerState *state);
LexerState *create_lexer_state(char *program);
void free_lexer_state(LexerState *state);

#endif // LEXER_H