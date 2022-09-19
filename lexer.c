typedef struct Lexer Lexer;
struct Lexer {
    bool exhausted;
    char *filepath, *src;
    usize index;
    usize row, col;
    char *line_start;
    u32 error_count;

    /* Info of Current token being processed */
    char *token_begin;
    TokenKind token_kind;
    usize token_start_index;
    TokenPos token_start_pos;
};

#define lexer_current_char(l) l->src[l->index]
#define lexer_peeknext(l) l->src[l->index + 1]
#define lexer_is_end(l) (l->src[l->index] == '\0' ? true : false)

TokenPos lexer_current_pos(Lexer *l) {
    return (TokenPos){.row = l->row, .col = l->col};
}

Lexer *make_lexer(char *filepath, char *src) {
    Lexer *l = xmalloc(sizeof(Lexer));
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

    return l;
}

void free_lexer(Lexer *l) {
    free(l);
}

void lexer_nextchar(Lexer *l) {
    l->index += 1;
    l->col += 1;
}

bool lexer_is_current_number(Lexer *l) {
    char ch = lexer_current_char(l);

    if (lexer_current_char(l) == '.') {
        if (lexer_peeknext(l) != '\0') {
            ch = lexer_peeknext(l);
        }
    }
    return is_decimal_digit(ch);
}

void lexer_skip_whitespaces(Lexer *l) {
    while (!lexer_is_end(l) && lexer_current_char(l) == ' ') {
        lexer_nextchar(l);
    }
}

void lexer_skip_line_comments(Lexer *l) {
    while (!lexer_is_end(l) && lexer_current_char(l) != '\n') {
        lexer_nextchar(l);
    }
}

void lexer_incnewline(Lexer *l) {
    l->row += 1;
    l->col = 0;
    lexer_current_char(l) = '\0'; /* replace '\n' with \0, so line would be printted easly
                        using `line_start` in TOKEKN */
}

u8 lexer_digit_value(Lexer *l) {
    if (is_decimal_digit(lexer_current_char(l))) return lexer_current_char(l) - '0';
    if (lexer_current_char(l) >= 'a' && lexer_current_char(l) <= 'f')
        return lexer_current_char(l) - 'a' + 10;
    if (lexer_current_char(l) >= 'A' && lexer_current_char(l) <= 'F')
        return lexer_current_char(l) - 'a' + 10;
    return 17;
}

Token lexer_scan_number(Lexer *l) {
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

bool lexer_is_reserved(char sym) {
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

Token lexer_scan_symbol(Lexer *l) {
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
        while (!lexer_is_end(l) &&
               !(lexer_is_reserved(lexer_current_char(l)) || lexer_current_char(l) == ' ')) {
            lexer_nextchar(l);
        }
    }
    }

    return make_token(l->token_begin, l->index - l->token_start_index, l->token_kind,
                      l->token_start_pos, l->line_start, 0);
}

Token lexer_next_token(Lexer *l) {
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

Array(Token) lexer_slurp_tokens(Lexer *l) {
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
