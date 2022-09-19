#include "katie.h"

// --------------------------------------------------------------------------
//                          - Lexer -
// --------------------------------------------------------------------------
static Token make_token(char *text, usize text_length, TokenKind kind, TokenPos pos,
                        char *line_start, i64 number) {
    return (Token){
        .text = text,
        .text_length = text_length,
        .kind = kind,
        .pos = pos,
        .line_start = line_start,
        .number = number,
    };
}

static void token_print(Token *token) {
    printf("%s, ", token_kind_to_cstring[token->kind]);
    printf("text: %.*s, ", (int)token->text_length, token->text);
    printf("position: %zu:%zu", token->pos.row, token->pos.col);
}

#define lexer_current_char(l) l->src[l->index]
#define lexer_peeknext(l) l->src[l->index + 1]
#define lexer_is_end(l) (l->src[l->index] == '\0' ? true : false)
#define lexer_is_line_end(l) (l->src[l->index] == '\n')

static TokenPos lexer_current_pos(Katie_Lexer *l) {
    return (TokenPos){.row = l->row, .col = l->col};
}

void katie_init_lexer(Katie_Lexer *l, char *filepath, char *src) {
    l->exhausted = false;

    l->src = src;
    l->filepath = filepath;
    l->index = 0;

    l->row = 0;
    l->col = 0;
    l->line_start = l->src;
    l->error_count = 0;

    l->token_begin = NULL;
    l->token_kind = TokenKind_Invaild;
    l->token_start_pos = lexer_current_pos(l);
}

static void lexer_nextchar(Katie_Lexer *l) {
    l->index += 1;
    l->col += 1;
}

static bool lexer_is_current_number(Katie_Lexer *l) {
    char ch = lexer_current_char(l);

    if (lexer_current_char(l) == '.') {
        if (lexer_peeknext(l) != '\0') {
            ch = lexer_peeknext(l);
        }
    }
    return is_decimal_digit(ch);
}

static void lexer_skip_whitespaces(Katie_Lexer *l) {
    while (!lexer_is_end(l) && lexer_current_char(l) == ' ') {
        lexer_nextchar(l);
    }
}

static void lexer_skip_line_comments(Katie_Lexer *l) {
    while (!lexer_is_end(l) && lexer_current_char(l) != '\n') {
        lexer_nextchar(l);
    }
}

static void lexer_incnewline(Katie_Lexer *l) {
    l->row += 1;
    l->col = 0;
    lexer_current_char(l) = '\0'; /* replace '\n' with \0, so line would be printted easly
                        using `line_start` in TOKEKN */
}

static u8 lexer_digit_value(Katie_Lexer *l) {
    if (is_decimal_digit(lexer_current_char(l))) return lexer_current_char(l) - '0';
    if (lexer_current_char(l) >= 'a' && lexer_current_char(l) <= 'f')
        return lexer_current_char(l) - 'a' + 10;
    if (lexer_current_char(l) >= 'A' && lexer_current_char(l) <= 'F')
        return lexer_current_char(l) - 'a' + 10;
    return 17;
}

static Token lexer_scan_number(Katie_Lexer *l) {
    i64 number;
    u8 base, digit_value;

    l->token_begin = &lexer_current_char(l);
    l->token_start_pos = lexer_current_pos(l);
    l->token_start_index = l->index;

    if (lexer_current_char(l) == '.') {
        while (!lexer_is_end(l) && is_decimal_digit(lexer_current_char(l))) {
            lexer_nextchar(l);
        }
    } else {
        l->token_kind = TokenKind_Number;
        base = 10;

        if (lexer_current_char(l) == '0') {
            switch (lexer_peeknext(l)) {
            case 'b':
                base = 2;
                lexer_nextchar(l);
                lexer_nextchar(l);
                break;

            case 'o':
                base = 10;
                lexer_nextchar(l);
                lexer_nextchar(l);
                break;

            case 'x':
                base = 16;
                lexer_nextchar(l);
                lexer_nextchar(l);
                break;

            default: break;
            }
        }
    }

    number = 0;

    while (!lexer_is_end(l)) {
        digit_value = lexer_digit_value(l);
        if (digit_value > base) break;
        number = (number * base) + digit_value;
        lexer_nextchar(l);
    }

    Token token = make_token(l->token_begin, l->index - l->token_start_index, l->token_kind,
                             l->token_start_pos, l->line_start, number);

    if (base != 10 && l->index - l->token_start_index <= 2) {
        katie_syntax_error(l->filepath, &token, "lexer error", "Invaild number");
        token.kind = TokenKind_Invaild;
    }

    return token;
}

static bool lexer_is_reserved(char sym) {
    switch (sym) {
    case '(':
    case ')':
    case '{':
    case '}':
    case '[':
    case ']':
    case '\'':
    case '~':
    case '^':
    case '@': return true;
    default: return false;
    }
}

static Token lexer_scan_symbol(Katie_Lexer *l) {
    l->token_begin = &lexer_current_char(l);
    l->token_start_index = l->index;
    l->token_start_pos = lexer_current_pos(l);

    switch (lexer_current_char(l)) {
    case '(':
        l->token_kind = TokenKind_LeftParen;
        lexer_nextchar(l);
        break;

    case ')':
        l->token_kind = TokenKind_RightParen;
        lexer_nextchar(l);
        break;

    case '{':
        l->token_kind = TokenKind_LeftCurly;
        lexer_nextchar(l);
        break;

    case '}':
        l->token_kind = TokenKind_RightCurly;
        lexer_nextchar(l);
        break;

    case '[':
        l->token_kind = TokenKind_LeftBracket;
        lexer_nextchar(l);
        break;

    case ']':
        l->token_kind = TokenKind_RightBracket;
        lexer_nextchar(l);
        break;

    case '\'':
        l->token_kind = TokenKind_Quote;
        lexer_nextchar(l);
        break;

    case '`':
        l->token_kind = TokenKind_Backtick;
        lexer_nextchar(l);
        break;

    case '~':
        l->token_kind = TokenKind_Tilda;
        lexer_nextchar(l);
        break;

    case '@':
        l->token_kind = TokenKind_At;
        lexer_nextchar(l);
        break;

    default: {
        l->token_kind = TokenKind_Symbol;
        while (!lexer_is_end(l) && !(lexer_is_reserved(lexer_current_char(l)) ||
                                     lexer_current_char(l) == ' ' || lexer_is_line_end(l))) {
            lexer_nextchar(l);
        }
    }
    }

    usize token_length = l->index - l->token_start_index;
    if (token_length <= 4) {
        if (are_cstrings_equal_length(l->token_begin, "def!", token_length))
            l->token_kind = TokenKind_Special_Define;
        if (are_cstrings_equal_length(l->token_begin, "let*", token_length))
            l->token_kind = TokenKind_Special_Let;
    }

    return make_token(l->token_begin, token_length, l->token_kind, l->token_start_pos,
                      l->line_start, 0);
}

static Token lexer_next_token(Katie_Lexer *l) {
    Debug_Assert(!l->exhausted);

    lexer_skip_whitespaces(l);

    if (!lexer_is_end(l)) {
        if (lexer_current_char(l) == '\n') {
            lexer_incnewline(l);
            while (!lexer_is_end(l) && lexer_current_char(l) == '\n') {
                lexer_incnewline(l);
                lexer_nextchar(l);
            }

            /* set only if there is a new line starting after '\n', so when we
             * hit EOF, we would be pointing to second last line */
            if (!lexer_is_end(l)) {
                l->line_start = &lexer_peeknext(l);
            }
            lexer_nextchar(l);
            return lexer_next_token(l);
        } else if (lexer_is_current_number(l)) {
            return lexer_scan_number(l);
        } else if (lexer_current_char(l) == ';') {
            lexer_skip_line_comments(l);
            return lexer_next_token(l);
        } else {
            return lexer_scan_symbol(l);
        }
    }

    l->index += 1;
    l->col += 1;
    l->exhausted = true;
    return make_token(&lexer_current_char(l), 1, TokenKind_EOS, lexer_current_pos(l), l->line_start,
                      0);
}

Array(Token) katie_lexer_slurp_tokens(Katie_Lexer *l) {
    Token tok;
    Array(Token) tokens;

    init_array(tokens);

    while (tok = lexer_next_token(l), tok.kind != TokenKind_EOS) {
        if (tok.kind != TokenKind_Invaild) {
            array_push(tokens, tok);
        }
    }

    array_push(tokens, tok);
    return tokens;
}

KatieVal *alloc_val(KatieValKind kind) {
    KatieVal *val = xmalloc(sizeof(KatieVal));
    val->kind = kind;
    return val;
}

KatieVal *alloc_number(i64 number) {
    KatieVal *val = alloc_val(KatieValKind_Number);
    val->as.number = number;
    return val;
}

KatieVal *alloc_bool(bool _bool) {
    KatieVal *val = alloc_val(KatieValKind_Bool);
    val->as._bool = _bool;
    return val;
}

KatieVal *alloc_list(Array(KatieVal *) list) {
    KatieVal *val = alloc_val(KatieValKind_List);
    val->as.list = list;
    return val;
}

KatieVal *alloc_symbol(char *text, usize length) {
    KatieVal *val = alloc_val(KatieValKind_Symbol);
    val->as.symbol = make_string(text, length);
    return val;
}

KatieVal *alloc_special(Katie_SpecialKind special_kind) {
    KatieVal *val = alloc_val(KatieValKind_Special);
    val->as.special = special_kind;
    return val;
}

KatieVal *alloc_proc(Katie_Proc proc) {
    KatieVal *val = alloc_val(KatieValKind_Proc);
    val->as.proc = proc;
    return val;
}

void dealloc_val(KatieVal *val) {
    switch (val->kind) {
    case KatieValKind_Number:
    case KatieValKind_Bool:
    case KatieValKind_Special: break;
    case KatieValKind_Symbol: free_string(val->as.symbol); break;

    case KatieValKind_List: {
        array_for_each(val->as.list, i) { dealloc_val(val->as.list[i]); }
        free_array(val->as.list);
    } break;

    default: Unreachable();
    }

    free(val);
}

String katie_value_as_string(String strResult, KatieVal *val) {
    switch (val->kind) {
    case KatieValKind_Number: {
        char buf[256];
        sprintf(buf, "%ld", val->as.number);
        strResult = append_string_length(strResult, buf, strlen(buf));
    } break;

    case KatieValKind_Symbol:
        strResult = append_string_length(strResult, val->as.symbol, string_length(val->as.symbol));
        break;

    case KatieValKind_Special:
        strResult =
            append_cstring(strResult, cast(char *) katie_special_kind_to_cstring[val->as.special]);
        break;

    case KatieValKind_List:
        strResult = append_cstring(strResult, "(");
        array_for_each(val->as.list, i) {
            strResult = katie_value_as_string(strResult, val->as.list[i]);
            strResult = append_cstring(strResult, " ");
        }
        strResult = append_cstring(strResult, ")");
        break;

    default: Unreachable();
    }

    return strResult;
}

void katie_print_value(KatieVal *val) {
    switch (val->kind) {
    case KatieValKind_Number: printf("%ld", val->as.number); break;
    case KatieValKind_Symbol: printf("%s", val->as.symbol); break;
    case KatieValKind_Special: printf("%s", katie_special_kind_to_cstring[val->as.special]); break;
    case KatieValKind_List:
        printf("(");
        array_for_each(val->as.list, i) {
            katie_print_value(val->as.list[i]);
            printf(" ");
        }
        printf(")");
        break;

    default: Unreachable();
    }
}

// --------------------------------------------------------------------------
//                          - Reader -
// --------------------------------------------------------------------------
#define reader_curr_token(r) r->tokens[r->index]
#define reader_peek_token(r) r->tokens[r->index + 1]
#define reader_is_end(r) (r->tokens[r->index].kind == TokenKind_EOS)

void katie_init_reader(Katie_Reader *r, char *source_filepath, char *src) {
    Katie_Lexer l;
    katie_init_lexer(&l, source_filepath, src);
    r->source_filepath = source_filepath;
    r->src = src;
    r->tokens = katie_lexer_slurp_tokens(&l);
    r->index = 0;
}

void katie_deinit_reader(Katie_Reader *r) {
    free_array(r->tokens);
}

static void reader_next_token(Katie_Reader *r) {
    r->index += 1;
}

static void reader_expect(Katie_Reader *r, TokenKind kind) {
    if (reader_curr_token(r).kind != kind) {
        katie_syntax_error(r->source_filepath, &reader_curr_token(r), "reader error",
                           "expected kind '%s' instead got '%s'", token_kind_to_cstring[kind],
                           token_kind_to_cstring[reader_curr_token(r).kind]);
    }
    reader_next_token(r);
}

static KatieVal *read_list(Katie_Reader *r) {
    KatieVal *val;
    Array(KatieVal *) list;

    init_array(list);

    while (!reader_is_end(r) && reader_curr_token(r).kind != TokenKind_RightParen) {
        val = katie_read_form(r);
        if (val) {
            array_push(list, val);
        }
    }

    return alloc_list(list);
}

KatieVal *katie_read_form(Katie_Reader *r) {
    KatieVal *val;

    if (reader_is_end(r)) {
        return NULL;
    }

    switch (reader_curr_token(r).kind) {
    case TokenKind_Number:
        val = alloc_number(reader_curr_token(r).number);
        reader_next_token(r);
        break;

    case TokenKind_Symbol:
        val = alloc_symbol(reader_curr_token(r).text, reader_curr_token(r).text_length);
        reader_next_token(r);
        break;

    case TokenKind_Special_Define:
        val = alloc_special(Katie_Special_Define);
        reader_next_token(r);
        break;

    case TokenKind_Special_Let:
        val = alloc_special(Katie_Special_Let);
        reader_next_token(r);
        break;

    case TokenKind_LeftParen:
        reader_next_token(r);
        val = read_list(r);
        reader_expect(r, TokenKind_RightParen);
        break;

    default: Unreachable();
    }

    return val;
}

Katie_Module *katie_read_module(Katie_Reader *r) {
    return read_list(r);
}

// --------------------------------------------------------------------------
//                          - Error Reporting -
// --------------------------------------------------------------------------
void katie_syntax_error(char *filepath, Token *token, char *prefix, char *msg, ...) {
    va_list ap;
    va_start(ap, msg);
    fprintf(stderr, "%s:%zu:%zu: %s: ", filepath, token->pos.row, token->pos.col, prefix);
    vfprintf(stderr, msg, ap);
    va_end(ap);

    putc('\n', stderr);
    fprintf(stderr, "  %s\n", token->line_start);

    /* print swiggly lines under faulty_token */
    fprintf(stderr, "  %*.s^", (int)(token->text - token->line_start), "");
    for (usize i = 0; i < token->text_length; i++)
        putc('-', stderr);

    fprintf(stderr, "\n\n");
}

// --------------------------------------------------------------------------
//                          - Native Functions -
// --------------------------------------------------------------------------
static i64 katie_get_op_value(KatieVal *val) {
    switch (val->kind) {
    case KatieValKind_Number: return val->as.number;
    case KatieValKind_Bool: return cast(i64) val->as._bool;
    default: return -1;
    }
}

static KatieVal *native_op_add(Katie *ctx, int argc, KatieVal **argv) {
    i64 result, temp;
    result = 0;
    for (int i = 0; i < argc; ++i) {
        temp = katie_get_op_value(argv[i]);
        if (temp >= 0) {
            result += temp;
        }
    }
    return alloc_number(result);
}

static KatieVal *native_op_sub(Katie *ctx, int argc, KatieVal **argv) {
    i64 result, temp;
    int i = 0;

    do {
        result = katie_get_op_value(argv[i]);
        i++;
    } while (i < argc && result < 0);

    for (; i < argc; ++i) {
        temp = katie_get_op_value(argv[i]);
        if (temp >= 0) {
            result -= temp;
        }
    }
    return alloc_number(result);
}

static KatieVal *native_op_mul(Katie *ctx, int argc, KatieVal **argv) {
    i64 result, temp;
    result = 1;
    for (int i = 0; i < argc; ++i) {
        temp = katie_get_op_value(argv[i]);
        if (temp >= 0) {
            result *= temp;
        }
    }
    return alloc_number(result);
}

static KatieVal *native_op_div(Katie *ctx, int argc, KatieVal **argv) {
    i64 result, temp;
    int i = 0;

    do {
        result = katie_get_op_value(argv[i]);
        i++;
    } while (i < argc && result < 0);

    for (; i < argc; ++i) {
        temp = katie_get_op_value(argv[i]);
        if (temp >= 0) {
            result = cast(i64)(result / temp);
        }
    }
    return alloc_number(result);
}

// --------------------------------------------------------------------------
//                          - Env -
// --------------------------------------------------------------------------
static KatieEnv *alloc_env(KatieEnv *outer) {
    KatieEnv *env = xmalloc(sizeof(KatieEnv));
    init_array(env->keys);
    init_array(env->values);
    env->outer = outer;
    return env;
}

static void dealloc_env(KatieEnv *env) {
    free_array(env->keys);
    free_array(env->values);
    if (env->outer) dealloc_env(env->outer);
    free(env);
}

static void env_define(KatieEnv *env, char *key, KatieVal *val) {
    array_push(env->keys, key);
    array_push(env->values, val);
}

static KatieVal *env_lookup(KatieEnv *env, String key) {
    array_for_each(env->keys, i) {
        if (are_equal_cstring(key, env->keys[i])) {
            return env->values[i];
        }
    }

    if (env->outer) return env_lookup(env, key);
    return NULL;
}

// --------------------------------------------------------------------------
//                          - Evaluate -
// --------------------------------------------------------------------------
static KatieVal *reduce_val(Katie *ctx, KatieVal *val);

KatieVal *katie_eval(Katie *ctx, KatieVal *val) {
    KatieVal *newList, *first;

    if (val->kind != KatieValKind_List) return reduce_val(ctx, val);
    if (array_is_empty(val->as.list)) return NULL;

    /* handle builtin envirnoment modifiers */
    first = val->as.list[0];

    if (first->kind == KatieValKind_Special) { /* special forms */
        KatieVal *newVal;
        Array(KatieVal *) list = val->as.list;

        Debug_Assert(array_length(list) >= 3);

        switch (first->as.special) {
        case Katie_Special_Define:
            Debug_Assert(list[1]->kind == KatieValKind_Symbol);
            newVal = reduce_val(ctx, list[2]);
            env_define(ctx->env, list[1]->as.symbol, newVal);
            break;

        case Katie_Special_Let:
            Todo();

        default: Unreachable();
        }

        return newVal;
    } else {
        Debug_Assert(first->kind == KatieValKind_Symbol);

        newList = reduce_val(ctx, val);
        if (!newList) return NULL;
        if (array_is_empty(newList->as.list) || array_length(newList->as.list) < 2) return NULL;

        first = newList->as.list[0];
        if (first->kind != KatieValKind_Proc) {
            Unreachable();
        }

        return first->as.proc(ctx, array_length(newList->as.list) - 1, &newList->as.list[1]);
    }
}

static KatieVal *reduce_val(Katie *ctx, KatieVal *val) {
    switch (val->kind) {
    case KatieValKind_Number:
    case KatieValKind_Bool:
    case KatieValKind_Special: return val;

    case KatieValKind_Symbol: {
        KatieVal *newVal = env_lookup(ctx->env, val->as.symbol);
        if (!newVal) {
            Unreachable();
        }
        return newVal;
    };

    case KatieValKind_List: {
        Array(KatieVal *) newList;
        init_array(newList);
        array_for_each(val->as.list, i) { array_push(newList, katie_eval(ctx, val->as.list[i])); }
        return alloc_list(newList);
    } break;

    default: Unreachable();
    }
}

// --------------------------------------------------------------------------
//                          - Katie -
// --------------------------------------------------------------------------
void init_katie_ctx(Katie *k) {
    k->env = alloc_env(NULL);
    env_define(k->env, "+", alloc_proc(native_op_add));
    env_define(k->env, "-", alloc_proc(native_op_sub));
    env_define(k->env, "*", alloc_proc(native_op_mul));
    env_define(k->env, "/", alloc_proc(native_op_div));
    env_define(k->env, "true", alloc_bool(true));
    env_define(k->env, "false", alloc_bool(false));
}

void deinit_katie_ctx(Katie *k) {
    dealloc_env(k->env);
}

void katie_take_file_source(Katie *k, char *source_filepath, String source) {
    Katie_Reader r;
    KatieVal *valResult;
    Katie_Module *module;
    String strResult;

    katie_init_reader(&r, source_filepath, source);

    module = katie_read_module(&r);
    if (!module) {
        eprintln("error: failed to read: %s", source_filepath);
        katie_deinit_reader(&r);
        free_string(source);
        return;
    }

    array_for_each(module->as.list, i) {
        valResult = katie_eval(k, module->as.list[i]);
        katie_print_value(valResult);
        printf("\n");
    }

    dealloc_val(module);
    katie_deinit_reader(&r);
}
