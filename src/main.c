#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"

int main(int argc, char *argv[])
{
    if (argc > 2)
    {
        fprintf(stderr, "Invalid arguments.\n");
        fprintf(stderr, "Correct use: mccp [filename]\n");
        exit(EXIT_FAILURE);
    }

    if (argc == 2)
    {
        const char *file_name = argv[1];
        FILE *file = fopen(file_name, "r");
        if (file == NULL)
        {
            fprintf(stderr, "Failed to open file.\n");
            exit(EXIT_FAILURE);
        }

        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        char *program = (char *)malloc(file_size + 1);
        if (program == NULL)
        {
            fprintf(stderr, "Failed to allocate memory for program contents.\n");
            exit(EXIT_FAILURE);
        }
        fread(program, 1, file_size, file);
        program[file_size] = '\0';
        fclose(file);

        // // Debug: Print actual input
        // printf("Program input:\n");
        // printf("%s\n", program);

        LexerState *lexer_state = create_lexer_state(program);
        Token *head = lexer(lexer_state);

        // // Debug: Print Token list
        // printf("Token list:\n");
        // print_list(head, lexer_state->prog);

        ParserState *parser_state = create_parser_state(program, head);
        parser(parser_state);

        // // Debug: Print AST
        // printf("Parsed expression list:\n");
        // print_ast(parser_state->node);

        // Debug: Print Output and Status
        // printf("Program Output:\n");
        // int value =
        interpret(NULL, parser_state->node);
        // printf("Program status: %d\n", value);

        free_parser_state(parser_state);
        free_lexer_state(lexer_state);

        return 0;
    }

    printf("-----------------REPL-----------------\n");
    printf("Write out statements to run.\n");
    printf("--------------------------------------\n");

    Environment *env = create_empty_environment(NULL);

    do
    {
        printf("> ");
        char input_line[1024];
        fgets(input_line, sizeof(input_line), stdin);
        input_line[strlen(input_line) - 1] = '\0';

        LexerState *lexer_state = create_lexer_state(input_line);
        Token *head = lexer(lexer_state);

        ParserState *parser_state = create_parser_state(input_line, head);
        parser(parser_state);

        interpret(env, parser_state->node);

        free_parser_state(parser_state);
        free_lexer_state(lexer_state);
    } while (1);

    return 0;
}