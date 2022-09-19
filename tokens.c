#define TOKEN_KINDS                                                                                \
    TOKEN_KIND(Invaild, "Invaild token")                                                           \
    TOKEN_KIND(Number, "Number")                                                                   \
    TOKEN_KIND(String, "String")                                                                   \
    TOKEN_KIND(Symbol, "Symbol")                                                                   \
    TOKEN_KIND(LeftParen, "Left Paren")                                                            \
    TOKEN_KIND(RightParen, "Right Paren")                                                          \
    TOKEN_KIND(LeftCurly, "Left Curly")                                                            \
    TOKEN_KIND(RightCurly, "Right Curly")                                                          \
    TOKEN_KIND(LeftBracket, "Left Bracket")                                                        \
    TOKEN_KIND(RightBracket, "Right Bracket")                                                      \
    TOKEN_KIND(At, "At @")                                                                         \
    TOKEN_KIND(Quote, "Quote '")                                                                   \
    TOKEN_KIND(Backtick, "Backtick `")                                                                   \
    TOKEN_KIND(Tilda, "Tilda ~")                                                                   \
    TOKEN_KIND(EOS, "EOS")

typedef enum {
#define TOKEN_KIND(kind_name, ...) TokenKind_##kind_name,
    TOKEN_KINDS
#undef TOKEN_KIND
} TokenKind;

static char const *token_kind_to_cstring[] = {
#define TOKEN_KIND(kind_name, cstring) [TokenKind_##kind_name] = cstring,
    TOKEN_KINDS
#undef TOKEN_KIND
};

typedef struct TokenPos TokenPos;
struct TokenPos {
    usize row, col;
};

typedef struct Token Token;
struct Token {
    char *text;
    usize text_length;
    TokenKind kind;
    char *line_start;
    TokenPos pos;
    i64 number;
};

Token make_token(char *text, usize text_length, TokenKind kind, TokenPos pos, char *line_start,
                 i64 number) {
    return (Token){
        .text = text,
        .text_length = text_length,
        .kind = kind,
        .pos = pos,
        .line_start = line_start,
        .number = number,
    };
}

void token_print(Token *token) {
    printf("%s, ", token_kind_to_cstring[token->kind]);
    printf("text: %.*s, ", (int)token->text_length, token->text);
    printf("position: %zu:%zu", token->pos.row, token->pos.col);
}
