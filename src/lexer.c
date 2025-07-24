#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "lexer.h"

int is_whitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

void lex_advance(LexerState *state, int n)
{
    state->pos += n;
}

char lex_consume(LexerState *state)
{
    return state->prog[state->pos++];
}

char lex_peek(LexerState *state)
{
    return state->prog[state->pos];
}

char lex_peek_next(LexerState *state)
{
    return state->prog[state->pos + 1];
}

char lex_peek_n(LexerState *state, int n)
{
    return state->prog[state->pos + n];
}

void lex_emit(LexerState *state, TokenKind type, int start_pos, int end_pos)
{
    state->tail->type = type;
    state->tail->start_pos = start_pos;
    state->tail->end_pos = end_pos;
    state->tail->line_start_pos = start_pos - state->line_pos;

    int len = end_pos - start_pos;
    state->tail->value = (char *)malloc(len + 1);
    strncpy(
        state->tail->value,
        state->prog + start_pos,
        len);
    state->tail->value[len] = '\0';

    state->tail->line = state->line_num;

    state->tail->next = (Token *)malloc(sizeof(Token));
    state->tail = state->tail->next;
}

void lex_number(LexerState *state)
{
    int start_pos = state->pos;
    int len = 0;
    while (lex_peek(state) != '\0' && isdigit(lex_peek(state)))
    {
        lex_consume(state);
        len++;
    }

    lex_emit(state, NUMBER, start_pos, start_pos + len);
}

void lex_identifier(LexerState *state)
{
    int start_pos = state->pos;
    int len = 0;
    while (isalnum(lex_peek(state)) && lex_peek(state) != '\0')
    {
        lex_consume(state);
        len++;
    }
    char id[len + 1];
    strncpy(id, state->prog + start_pos, len);
    id[len] = '\0';

    // Implement into hashmap when I have time.
    if (strcmp(id, "if") == 0)
    {
        lex_emit(state, IF, start_pos, start_pos + len);
    }
    else if (strcmp(id, "else") == 0)
    {
        lex_emit(state, ELSE, start_pos, start_pos + len);
    }
    else if (strcmp(id, "while") == 0)
    {
        lex_emit(state, WHILE, start_pos, start_pos + len);
    }
    else if (strcmp(id, "for") == 0)
    {
        lex_emit(state, FOR, start_pos, start_pos + len);
    }
    else if (strcmp(id, "function") == 0)
    {
        lex_emit(state, FUNCTION, start_pos, start_pos + len);
    }
    else if (strcmp(id, "return") == 0)
    {
        lex_emit(state, RETURN, start_pos, start_pos + len);
    }
    else
    {
        lex_emit(state, IDENTIFIER, start_pos, start_pos + len);
    }
}

void lex_string(LexerState *state)
{
    // Consume the ".
    lex_consume(state);

    int start_pos = state->pos;
    int len = 0;
    while (lex_peek(state) != '\0' && lex_peek(state) != '"' && lex_peek(state) != '\n')
    {
        len++;
        char c = lex_consume(state);

        if (c == '\\')
        {
            // Implementing valid escape sequences
            // \n: Newline (line break)
            // \t: Horizontal tab
            // \r: Carriage return
            // \b: Backspace
            // \f: Form feed
            // \a: Alert (bell)
            // \\: Backslash (to escape the backslash itself)
            // \': Single quote (useful inside single-quoted characters)
            // \": Double quote (useful inside double-quoted strings)
            // \0: Null character (end of string in C)
            // \xNN: Hexadecimal escape (where NN is a hexadecimal number, like \x7A for the character 'z')
            // \ooo: Octal escape (where ooo is an octal value, like \141 for 'a')
            switch (lex_peek_next(state))
            {
            case 'n':
            case 't':
            case 'r':
            case 'b':
            case 'f':
            case 'a':
            case '\\':
            case '\'':
            case '"':
            case '0':
                lex_consume(state);
                break;
            // worry about this later
            case 'o':
            case 'x':
                lex_advance(state, 3);
            }
        }
    }

    // This should be a ".
    char c = lex_consume(state);
    if (c == '\0' || c == '\n')
    {
        fprintf(stderr, "Unterminated string.");
        exit(1);
    }
    lex_emit(state, STRING, start_pos, start_pos + len);
}

int lex_match(LexerState *state, const char *str)
{
    int len = strlen(str);

    for (int i = 0; i < len; i++)
    {
        if (lex_peek_n(state, i) != str[i])
        {
            return 0;
        }
    }

    return 1;
}

Token *lexer(LexerState *state)
{
    // 1 + 2 + 3
    // NUMBER(1), PLUS, NUMBER(2), PLUS, NUMBER(3), EOF
    //  Token_t {
    //      TokenKind.INTEGER,
    //      "1",
    //      Token_t {
    //          TokenKind.PLUS
    //          "+" - IDC
    //          Token_t {
    //              TokenKind.INTEGER,
    //              "2",
    //              null
    //          }
    //      }
    // }
    Token *head = state->tail;

    while (lex_peek(state) != '\0')
    {
        char ch = state->prog[state->pos];
        if (is_whitespace(ch))
        {
            if (ch == '\n' || ch == '\r')
            {
                state->line_pos = state->pos + 1;
                state->line_num++;
            }
            lex_consume(state);
            continue;
        }

        int i;
        switch (ch)
        {
        case '#':
            i = 1;
            while (
                lex_peek_n(state, i) != '\0' &&
                lex_peek_n(state, i) != '\n')
            {
                i++;
            }
            lex_advance(state, i);
            break;
        case '+':
            lex_emit(state, PLUS, state->pos, state->pos + 1);
            lex_consume(state);
            break;
        case '-':
            lex_emit(state, MINUS, state->pos, state->pos + 1);
            lex_consume(state);
            break;
        case '*':
            if (lex_peek_next(state) == '*')
            {
                lex_emit(state, DOUBLE_STAR, state->pos, state->pos + 2);
                lex_advance(state, 2);
                break;
            }
            lex_emit(state, STAR, state->pos, state->pos + 1);
            lex_consume(state);
            break;
        case '/':
            if (lex_peek_next(state) == '/')
            {
                lex_emit(state, DOUBLE_SLASH, state->pos, state->pos + 2);
                lex_advance(state, 2);
                break;
            }
            lex_emit(state, SLASH, state->pos, state->pos + 1);
            lex_consume(state);
            break;
        case '>':
            if (lex_peek_next(state) == '=')
            {
                lex_emit(state, GREATER_EQUAL, state->pos, state->pos + 2);
                lex_advance(state, 2);
                break;
            }
            lex_emit(state, GREATER, state->pos, state->pos + 1);
            lex_consume(state);
            break;
        case '<':
            if (lex_peek_next(state) == '=')
            {
                lex_emit(state, LESS_EQUAL, state->pos, state->pos + 2);
                lex_advance(state, 2);
                break;
            }
            lex_emit(state, LESS, state->pos, state->pos + 1);
            lex_consume(state);
            break;
        case '~':
            lex_emit(state, TILDE, state->pos, state->pos + 1);
            lex_consume(state);
            break;
        case '!':
            if (lex_peek_next(state) == '=')
            {
                lex_emit(state, BANG_EQUAL, state->pos, state->pos + 2);
                lex_advance(state, 2);
                break;
            }
            lex_emit(state, BANG, state->pos, state->pos + 1);
            lex_consume(state);
            break;
        case '=':
            if (lex_peek_next(state) == '=')
            {
                lex_emit(state, EQUAL_EQUAL, state->pos, state->pos + 2);
                lex_advance(state, 2);
                break;
            }
            lex_emit(state, EQUALS, state->pos, state->pos + 1);
            lex_consume(state);
            break;
        case '(':
            lex_emit(state, LEFT_PAREN, state->pos, state->pos + 1);
            lex_consume(state);
            break;
        case ')':
            lex_emit(state, RIGHT_PAREN, state->pos, state->pos + 1);
            lex_consume(state);
            break;
        case '{':
            lex_emit(state, LEFT_BRACKET, state->pos, state->pos + 1);
            lex_consume(state);
            break;
        case '}':
            lex_emit(state, RIGHT_BRACKET, state->pos, state->pos + 1);
            lex_consume(state);
            break;
        case ';':
            lex_emit(state, SEMICOLON, state->pos, state->pos + 1);
            lex_consume(state);
            break;
        case ',':
            lex_emit(state, COMMA, state->pos, state->pos + 1);
            lex_consume(state);
            break;
        case '"':
            lex_string(state);
            break;
        default:
            if (isdigit(ch))
            {
                lex_number(state);
                break;
            }
            else if (isalnum(ch))
            {
                if (lex_match(state, "while"))
                {
                    lex_emit(state, WHILE, state->pos, state->pos + 5);
                    lex_advance(state, 5);
                    break;
                }
                if (lex_match(state, "print"))
                {
                    lex_emit(state, PRINT, state->pos, state->pos + 5);
                    lex_advance(state, 5);
                    break;
                }
                if (lex_match(state, "for"))
                {
                    lex_emit(state, FOR, state->pos, state->pos + 5);
                    lex_advance(state, 5);
                    break;
                }
                lex_identifier(state);
                break;
            }

            fprintf(stderr, "Unexpected character '%c'.\n", ch);
            exit(1);
            continue;
        }
    }

    state->tail->type = EOF_TOKEN;
    state->tail->start_pos = state->pos;
    state->tail->end_pos = state->pos;
    state->tail->line = state->line_num;
    state->tail->next = NULL;

    return head;
}

LexerState *create_lexer_state(char *program)
{
    LexerState *state = (LexerState *)malloc(sizeof(LexerState));
    if (state == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for LexerState.\n");
        exit(EXIT_FAILURE);
    }
    state->pos = 0;
    state->prog = program;

    state->line_num = 0;

    Token *tail = (Token *)malloc(sizeof(Token));
    if (tail == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for LexerState->tail.\n");
        exit(EXIT_FAILURE);
    }
    state->tail = tail;

    return state;
}

void free_lexer_state(LexerState *state)
{
    // THIS FUNCTION DOESN'T DO ANYTHING BECAUSE I DON'T CARE ENOUGH
}