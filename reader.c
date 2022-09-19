typedef struct Reader Reader;
struct Reader {
    char *source_filepath;
    char *src;
    Array(Token) tokens;
    u32 index; /* Current token index */
};

#define reader_curr_token(r) r->tokens[r->index]
#define reader_peek_token(r) r->tokens[r->index + 1]
#define reader_is_end(r) (r->tokens[r->index].kind == TokenKind_EOS)

void init_reader(Reader *r, char *source_filepath, char *src) {
    Lexer *l = make_lexer(source_filepath, src);
    r->source_filepath = source_filepath;
    r->src = src;
    r->tokens = lexer_slurp_tokens(l);
    r->index = 0;
    free_lexer(l);
}

void deinit_reader(Reader *r) {
    free_array(r->tokens);
}

void reader_next_token(Reader *r) {
    r->index += 1;
}

void reader_expect(Reader *r, TokenKind kind) {
    if (reader_curr_token(r).kind != kind) {
        katie_syntax_error(r->source_filepath, &reader_curr_token(r), "reader error",
                           "expected kind '%s' instead got '%s'", token_kind_to_cstring[kind],
                           token_kind_to_cstring[reader_curr_token(r).kind]);
    }
    reader_next_token(r);
}

KatieVal *read_form(Reader *r);

KatieVal *read_list(Reader *r) {
    KatieVal *val;
    Array(KatieVal *) list;

    init_array(list);
    reader_next_token(r);

    while (!reader_is_end(r) && reader_curr_token(r).kind != TokenKind_RightParen) {
        val = read_form(r);
        if (val) {
            array_push(list, val);
        }
    }

    return alloc_list(list);
}

KatieVal *read_form(Reader *r) {
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

    case TokenKind_LeftParen:
        val = read_list(r);
        reader_expect(r, TokenKind_RightParen);
        break;

    default: Unreachable();
    }

    return val;
}
