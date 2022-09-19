#include "basic.c"
#include "cli.c"
#include "katie.c"

void repl() {
    bool is_quit = false;
    char *buf = NULL;
    size_t len, ret;

    while (true) {
        printf("\e[1;95mkatie>\e[0m ");
        ret = getline(&buf, &len, stdin);
    }
}

#ifdef Debug
void cli_dump_tokens(char *source_filepath) {
    String source = file_as_string(source_filepath);
    Katie_Lexer l;
    katie_init_lexer(&l, source_filepath, source);
    Array(Token) tokens = katie_lexer_slurp_tokens(&l);
    array_for_each(tokens, i) {
        token_print(&tokens[i]);
        printf("\n");
    }
    free_string(source);
    free_array(tokens);
}

void cli_stringify_source(char *source_filepath) {
    Katie_Reader r;
    Katie_Module *module;
    String strResult;

    String source = file_as_string(source_filepath);
    katie_init_reader(&r, source_filepath, source);

    module = katie_read_module(&r);
    if (!module) {
        eprintln("error: failed to read: %s", source_filepath);
        katie_deinit_reader(&r);
        free_string(source);
        return;
    }

    strResult = make_string_empty();
    strResult = katie_value_as_string(strResult, module);
    printf("%s", strResult);

    dealloc_val(module);
    free_string(strResult);
    katie_deinit_reader(&r);
    free_string(source);
}
#endif

int main(int argc, char **argv) {
    char *source_filepath;

#ifdef Debug
    bool is_lex_tokens = false;
    bool is_stringify = false;
#endif

    Cli_Flag positionals[] = {
        Flag_CString_Positional(&source_filepath, "SOURCE_FILEPATH", "lisp filepath")};

    Cli_Flag optionals[] = {
#ifdef Debug
        Flag_Bool(&is_lex_tokens, "l", "lex-tokens", "output lexical tokens"),
        Flag_Bool(&is_stringify, "s", "stringify", "convert source into string repr"),
#endif
    };

    Cli cli = create_cli(argc, argv, "katie", positionals, array_sizeof(positionals, Cli_Flag),
                         optionals, array_sizeof(optionals, Cli_Flag));

    cli_parse_args(&cli);
    if (cli_has_error(&cli)) {
        exit(EXIT_FAILURE);
    }

#ifdef Debug
    if (is_lex_tokens) {
        cli_dump_tokens(source_filepath);
        return 0;
    } else if (is_stringify) {
        cli_stringify_source(source_filepath);
        return 0;
    }
#endif

    Katie k;
    String source = file_as_string(source_filepath);

    init_katie_ctx(&k);
    katie_take_file_source(&k, source_filepath, source);

    deinit_katie_ctx(&k);
    free_string(source);

    return 0;
}
