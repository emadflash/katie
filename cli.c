#include <errno.h>
#include <limits.h>

// --------------------------------------------------------------------------
//                          - Cli Parser -
// --------------------------------------------------------------------------

/* Supported flag types */
typedef enum {
    Cli_FlagType_Bool,
    Cli_FlagType_Int,
    Cli_FlagType_CString,
} Cli_FlagType;

static char const *flag_type_to_cstring[] = {
    [Cli_FlagType_Bool] = "bool",
    [Cli_FlagType_Int] = "int",
    [Cli_FlagType_CString] = "string",
};

typedef struct Cli_Flag Cli_Flag;
struct Cli_Flag {
    char *big_name;
    char *small_name;
    char *description;

    Cli_FlagType type;
    union {
        bool *_bool;
        char **cstring;
        int *integer;
    } value_ref;
};

#define cli_create_flag(ref, small, big, desc, flag_type, REF)                                     \
    (Cli_Flag) {                                                                                   \
        .small_name = small, .big_name = big, .description = desc, .type = flag_type,              \
        .value_ref.REF = ref                                                                       \
    }

#define Flag_Bool(bool_ref, small, big, description)                                               \
    cli_create_flag(bool_ref, small, big, description, Cli_FlagType_Bool, _bool)
#define Flag_Int(int_ref, small, big, description)                                                 \
    cli_create_flag(int_ref, small, big, description, Cli_FlagType_Int, integer)
#define Flag_CString(cstring_ref, big, small, description)                                         \
    cli_create_flag(cstring_ref, big, small, description, Cli_FlagType_CString, cstring)

#define Flag_Bool_Positional(bool_ref, big, description) Flag_Bool(bool_ref, NULL, big, description)
#define Flag_Int_Positional(int_ref, big, description) Flag_Int(int_ref, NULL, big, description)
#define Flag_CString_Positional(cstring_ref, big, description)                                     \
    Flag_CString(cstring_ref, NULL, big, description)

typedef struct Cli Cli;
struct Cli {
    int argc;
    char **argv;
    char *program_name;

    Cli_Flag *positionals;
    int positionals_count;
    Cli_Flag *optionals;
    int optionals_count;

    int error_count;
};

Cli create_cli(int argc, char **argv, char *program_name, Cli_Flag *positionals,
               int positionals_count, Cli_Flag *optionals, int optionals_count) {

    return (Cli){
        .argc = argc,
        .argv = argv,
        .program_name = program_name,
        .positionals = positionals,
        .positionals_count = positionals_count,
        .optionals = optionals,
        .optionals_count = optionals_count,
        .error_count = 0,
    };
}

#define cli_report_error(cli, ...)                                                                 \
    do {                                                                                           \
        fprintf(stderr, __VA_ARGS__);                                                              \
        cli->error_count += 1;                                                                     \
    } while (0);

#define cli_report_warning(cli, ...)                                                               \
    do {                                                                                           \
        fprintf(stderr, __VA_ARGS__);                                                              \
    } while (0);

bool cli_has_error(Cli *cli) {
    return cli->error_count != 0 ? true : false;
}

static Cli_Flag *cli_find_optional_flag_big(Cli *cli, char *flag_name, u32 flag_name_len) {
    for (u8 i = 0; i < cli->optionals_count; ++i) {
        if (are_cstrings_equal_length(flag_name, cli->optionals[i].big_name, flag_name_len))
            return &cli->optionals[i];
    }
    return NULL;
}

static Cli_Flag *cli_find_optional_flag_small(Cli *cli, char *flag_name, u32 flag_name_len) {
    for (u8 i = 0; i < cli->optionals_count; ++i) {
        if (cli->optionals[i].small_name &&
            are_cstrings_equal_length(flag_name, cli->optionals[i].small_name, flag_name_len))
            return &cli->optionals[i];
    }
    return NULL;
}

void cli_show_usage_message(Cli *cli, FILE *stream) {
    Cli_Flag *temp_flag;

#ifdef CLI_MORE_INFO
#if !defined(CLI_VERSION) || !defined(CLI_PROGRAM_DESC) || !defined(CLI_AUTHOR_NAME) ||            \
    !defined(CLI_AUTHOR_EMAIL_ADDRESS)
#error                                                                                             \
    "CLI_MORE_INFO: expected CLI_VERSION, CLI_PROGRAM_DESC, CLI_AUTHOR_NAME, CLI_AUTHOR_EMAIL_ADDRESS defined"
#else
    fprintf(stream, "%s %s\n%s\n\n", cli->program_name, CLI_VERSION, CLI_PROGRAM_DESC);
    fprintf(stream, "%s <%s>\n\n", CLI_AUTHOR_NAME, CLI_AUTHOR_EMAIL_ADDRESS);
#endif
#endif

    fprintf(stream, "Usage: %s ", cli->program_name);
    for (int i = 0; i < cli->positionals_count; ++i) {
        fprintf(stream, "%s ", cli->positionals[i].big_name);
    }
    fputs("[opts]\n", stream);

    fprintf(stream, "\nOptions:\n");
    for (int i = 0; i < cli->optionals_count; ++i) {
        temp_flag = &cli->optionals[i];
        if (temp_flag->type == Cli_FlagType_Bool) {
            fprintf(stream, "  -%s| --%s    %s\n", temp_flag->small_name, temp_flag->big_name,
                    temp_flag->description);
        } else {
            fprintf(stream, "  -%s| --%s=<%s>    %s\n", temp_flag->small_name, temp_flag->big_name,
                    flag_type_to_cstring[temp_flag->type], temp_flag->description);
        }
    }

    fprintf(stream, "  -h| --help    show this help message\n\n");
}

static int strip_dashes(char **arg) {
    int c = 0;
    while (**arg && *(*arg)++ == '-')
        c++;
    (*arg)--;
    return c;
}

static int cli_find_and_set_value(Cli *cli, int curr_idx, char *curr, char *flag_name,
                                  char **flag_value) {
    if (*curr) {
        /* case: --flag=something
         * case: --flag =something */
        *flag_value = curr;
    } else {
        /* case: --flag= something
         * case: --flag = something */
        if (curr_idx + 1 >= cli->argc) {
            cli_report_error(cli, "error: expected value after '=', near '%s'\n", flag_name);
        } else {
            curr_idx += 1;
            *flag_value = cli->argv[curr_idx];
        }
    }

    return curr_idx;
}

/*
 * Extracts flag name and its value if its present or flag demands it.
 *
 * For example:
 *   --flag             , flag_name = flag, flag_value = NULL
 *   --flag = something , flag_name = flag, flag_value = something
 *   --flag=something   , flag_name = flag, flag_value = something
 *   --flag =something   , flag_name = flag, flag_value = something
 *
 *   -f                 , flag_name = f, flag_value = NULL
 */
static int split_flag_name_and_value_at_equal_sign(Cli *cli, int curr_idx, char *dash_stripped_curr,
                                                   char **flag_name, u32 *flag_name_len,
                                                   char **flag_value) {
    *flag_name = dash_stripped_curr;
    *flag_name_len = 0;

    while (*dash_stripped_curr && *dash_stripped_curr != '=') { /* find equal sign */
        *flag_name_len += 1;
        dash_stripped_curr++;
    }

    if (*dash_stripped_curr && *dash_stripped_curr == '=') { /* equal sign is present in arg */
        *dash_stripped_curr = '\0';
        dash_stripped_curr++;
        curr_idx =
            cli_find_and_set_value(cli, curr_idx, dash_stripped_curr, *flag_name, flag_value);
    } else { /* equal sign is not present in arg */
        if (curr_idx + 1 >= cli->argc ||
            *cli->argv[curr_idx + 1] != '=') { /* didn't have any value. Skip it */
            return curr_idx;
        }

        curr_idx += 1;
        dash_stripped_curr = cli->argv[curr_idx];
        dash_stripped_curr++;
        curr_idx =
            cli_find_and_set_value(cli, curr_idx, dash_stripped_curr, *flag_name, flag_value);
    }

    return curr_idx;
}

static inline bool cli_is_help_flag(char *flag_name, int flag_name_len) {
    Debug_Assert(flag_name_len > 0);
    if (flag_name_len == 1) return cast(bool)(*flag_name == 'h');
    if (flag_name_len != 4) return false;
    return cast(bool)(flag_name[0] == 'h' && flag_name[1] == 'e' && flag_name[2] == 'l' &&
                      flag_name[3] == 'p');
}

static inline bool cli_is_flag_value_true(char *flag_value, int flag_value_len) {
    if (flag_value_len > 4) return false;
    return (are_cstrings_equal_length("true", flag_value, strlen(flag_value)) ||
            are_cstrings_equal_length("yes", flag_value, strlen(flag_value)) ||
            are_cstrings_equal_length("y", flag_value, strlen(flag_value)))
               ? true
               : false;
}

static inline bool cli_is_flag_value_false(char *flag_value, int flag_value_len) {
    if (flag_value_len > 5) return false;
    return (are_cstrings_equal_length("false", flag_value, strlen(flag_value)) ||
            are_cstrings_equal_length("no", flag_value, strlen(flag_value)) ||
            are_cstrings_equal_length("n", flag_value, strlen(flag_value)))
               ? true
               : false;
}

static void cli_set_value_bool(Cli *cli, bool *value, char *flag_name, char *flag_value) {
    if (flag_value) {
        if (cli_is_flag_value_true(flag_value, strlen(flag_value))) {
            *value = true;
        } else if (cli_is_flag_value_false(flag_value, strlen(flag_value))) {
            *value = false;
        } else {
            cli_report_error(cli,
                             "error: expected bool( yes,true,no,false,y,n ) for '%s', got '%s'\n",
                             flag_name, flag_value);
        }
    } else {
        *value = *value ? false : true;
    }
}

static void cli_set_value_integer(Cli *cli, int *value, char *flag_name, char *flag_value) {
    char *endptr;

    errno = 0;
    long int val = strtol(flag_value, &endptr, 10);

    if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) || (errno != 0 && val == 0)) {
        cli_report_error(cli, "error: expected a 64-bit signed integer for '%s'\n", flag_name);
        return;
    }

    Assert(endptr >= flag_value);
    if (cast(usize)(endptr - flag_value) != strlen(flag_value)) {
        cli_report_error(cli, "error: expected a integer for '%s', got '%s'\n", flag_name,
                         flag_value);
        return;
    }

    *value = cast(int) val;
}

static void cli_handle_help_flag(Cli *cli, char *flag_name, char *flag_value) {
    bool is_help_flag_on = false;
    cli_set_value_bool(cli, &is_help_flag_on, flag_name, flag_value);

    if (is_help_flag_on) {
        cli_show_usage_message(cli, stdout);
        exit(1);
    }
}

static void cli_report_if_missing_positionals(Cli *cli, int idx) {
    if (idx != cli->positionals_count) {
        cli_report_error(cli, "error: missing positionals\n");
        for (; idx < cli->positionals_count; ++idx) {
            cli_report_error(cli, "  %s\n", cli->positionals[idx].big_name);
        }
    }
}

static void cli_set_found_flag_value(Cli *cli, Cli_Flag *found_flag, char *flag_name,
                                     char *flag_value) {

    switch (found_flag->type) {
    case Cli_FlagType_Bool:
        cli_set_value_bool(cli, found_flag->value_ref._bool, flag_name, flag_value);
        break;

    case Cli_FlagType_Int:
        cli_set_value_integer(cli, found_flag->value_ref.integer, flag_name, flag_value);
        break;

    case Cli_FlagType_CString: *found_flag->value_ref.cstring = flag_value; break;

    default: Unreachable();
    }
}

int cli_parse_args(Cli *cli) {
    Cli_Flag *found_flag;
    char *flag_name, *flag_value;
    u32 flag_name_len;
    int argv_idx, positionals_idx;

    argv_idx = 1;
    positionals_idx = 0;

    while (argv_idx < cli->argc) {
        if (*cli->argv[argv_idx] != '-') { /* Positional arg */
            if (positionals_idx >= cli->positionals_count) {
                cli_report_warning(cli, "warning: ignoring '%s'\n", cli->argv[argv_idx]);
            } else {
                flag_value = cli->argv[argv_idx];
                found_flag = &cli->positionals[positionals_idx];
                cli_set_found_flag_value(cli, found_flag, found_flag->big_name, flag_value);
                positionals_idx += 1;
            }
        } else { /* Flag */
            flag_name = NULL;
            flag_value = NULL;

            /* Strip dashes from arg */
            char *dash_stripped_curr = cli->argv[argv_idx];
            int dash_count = strip_dashes(&dash_stripped_curr);

            /*
             * Handle multiple short flags passed with '-'.
             * All these flags will be considered boolean in nature.
             * If say a Flag with value type integer is passed, it will be ignored.
             * And a warning message will be printed.
             *
             * For example:
             *   -hdlf  ---> h, d, l, f are all seperate flags, maybe all booleans, h for help etc
             */
            if (dash_count == 1 && strlen(dash_stripped_curr) > 1) { /* with single dash */
                while (*dash_stripped_curr) {
                    found_flag = cli_find_optional_flag_small(cli, dash_stripped_curr, 1);
                    if (!found_flag) {
                        if (cli_is_help_flag(dash_stripped_curr, 1)) {
                            cli_handle_help_flag(cli, flag_name, flag_value);
                            goto next_iter;
                        }
                        cli_report_warning(cli, "warning: ignoring %c\n", *dash_stripped_curr);
                    } else if (found_flag->type != Cli_FlagType_Bool) {
                        cli_report_warning(
                            cli, "warning: ignoring -%c, expected flag type boolean, got %s\n",
                            *dash_stripped_curr, flag_type_to_cstring[found_flag->type]);
                    } else {
                        cli_set_value_bool(cli, found_flag->value_ref._bool, dash_stripped_curr,
                                           flag_value);
                    }
                    dash_stripped_curr++;
                }

                goto next_iter;
            }

            argv_idx = split_flag_name_and_value_at_equal_sign(
                cli, argv_idx, dash_stripped_curr, &flag_name, &flag_name_len, &flag_value);

            if (cli_is_help_flag(flag_name, flag_name_len)) {
                cli_handle_help_flag(cli, flag_name, flag_value);
                goto next_iter;
            }

            if (dash_count == 1) { /* Is short flag */
                found_flag = cli_find_optional_flag_small(cli, flag_name, flag_name_len);
            } else { /* Is big flag */
                found_flag = cli_find_optional_flag_big(cli, flag_name, flag_name_len);
            }

            if (!found_flag) {
                cli_report_error(cli, "error: unknown flag: %s\n", flag_name);
            } else {
                if (found_flag->type != Cli_FlagType_Bool && !flag_value) {
                    if (argv_idx + 1 >= cli->argc) { // has no flag value after name
                        cli_report_error(cli, "error: expected %s for %s\n",
                                         flag_type_to_cstring[found_flag->type], flag_name);
                    } else {
                        cli_set_found_flag_value(cli, found_flag, flag_name,
                                                 cli->argv[argv_idx + 1]);
                        argv_idx += 1;
                    }
                } else {
                    cli_set_found_flag_value(cli, found_flag, flag_name, flag_value);
                }
            }
        }

    next_iter:
        argv_idx += 1;
    }

    cli_report_if_missing_positionals(cli, positionals_idx);

    return argv_idx;
}
