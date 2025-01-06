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

        printf("Program input:\n");
        printf("%s\n", program);

        LexerState *lexer_state = create_lexer_state(program);
        Token *head = lexer(lexer_state);

        printf("Token list:\n");
        print_list(head, lexer_state->prog);

        // Create rudamentary parser state and pass it to the parser.
        ParserState *parser_state = create_parser_state(program, head);
        parser(parser_state);
        printf("Parsed expression list:\n");
        print_ast(parser_state->node);

        printf("Program Output:\n");
        int value = interpret(NULL, parser_state->node);
        printf("Program status: %d\n", value);

        free_parser_state(parser_state);
        free_lexer_state(lexer_state);

        return 0;
    }

    printf("-----------------REPL-----------------\n");
    printf("Write out statements to run.\n");
    printf("Execute them with \"execute\" command.\n");
    printf("--------------------------------------\n");

    do
    {
        char input[10240] = "";
        char input_line[1024];

        while (1)
        {
            fgets(input_line, sizeof(input_line), stdin);

            input_line[strlen(input_line) - 1] = '\0';
            if (strcmp(input_line, "execute") == 0)
            {
                break;
            }

            if (strlen(input) + strlen(input_line) < 10240 - 1)
            {
                strcat(input, input_line);
                strcat(input, "\n");
            }
            else
            {
                printf("Input buffer is full.\n");
                break;
            }
        }

        printf("Program Input:\n");
        printf("%s", input);

        LexerState *lexer_state = create_lexer_state(input);
        Token *head = lexer(lexer_state);

        printf("Token List:\n");
        print_list(head, lexer_state->prog);

        ParserState *parser_state = create_parser_state(input, head);
        parser(parser_state);

        printf("Re-rendered AST:\n");
        print_ast(parser_state->node);

        printf("Program Output:\n");
        int status = interpret(NULL, parser_state->node);
        printf("Program Status: %d\n", status);

        free_parser_state(parser_state);
        free_lexer_state(lexer_state);
    } while (1);

    return 0;
}