#ifndef __katie_h__
#define __katie_h__

#include "basic.h"

// --------------------------------------------------------------------------
//                          - Tokens -
// --------------------------------------------------------------------------
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
    TOKEN_KIND(Backtick, "Backtick `")                                                             \
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

// --------------------------------------------------------------------------
//                          - Lexer -
// --------------------------------------------------------------------------
typedef struct Katie_Lexer Katie_Lexer;
struct Katie_Lexer {
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

void katie_init_lexer(Katie_Lexer *l, char *filepath, char *src);
Array(Token) katie_lexer_slurp_tokens(Katie_Lexer *l);

// --------------------------------------------------------------------------
//                          - Value -
// --------------------------------------------------------------------------
typedef enum {
    KatieValKind_List,
    KatieValKind_Number,
    KatieValKind_Symbol,
    KatieValKind_Proc,
    KatieValKind_Bool,
    KatieValKind_Vector,
    KatieValKind_HashMap,
} KatieValKind;

typedef struct KatieEnv KatieEnv;
typedef struct Katie Katie;

typedef struct KatieVal KatieVal;
typedef i64 Katie_Number;
typedef u8 Katie_Bool;
typedef Array(KatieVal *) Katie_List;
typedef String Katie_Symbol;
typedef KatieVal *(*Katie_Proc)(Katie *env, int argc, KatieVal **argv);

struct KatieVal {
    KatieValKind kind;
    union {
        Katie_Number number;
        Katie_Bool _bool;
        Katie_List list;
        Katie_Symbol symbol;
        Katie_Proc proc;
    } as;
};

// --------------------------------------------------------------------------
//                          - Reader -
// --------------------------------------------------------------------------
typedef struct Katie_Reader Katie_Reader;
struct Katie_Reader {
    char *source_filepath;
    char *src;
    Array(Token) tokens;
    u32 index; /* Current token index */
};

void katie_init_reader(Katie_Reader *r, char *source_filepath, char *src);
void katie_deinit_reader(Katie_Reader *r);
KatieVal *katie_read_form(Katie_Reader *r);

// --------------------------------------------------------------------------
//                          - Env -
// --------------------------------------------------------------------------
struct KatieEnv {
    Array(char *) keys;
    Array(KatieVal *) values;
    KatieEnv *outer;
};

// --------------------------------------------------------------------------
//                          - Katie Context -
// --------------------------------------------------------------------------
struct Katie {
    KatieEnv env;
};

void init_katie_ctx(Katie *k);
void deinit_katie_ctx(Katie *k);

KatieVal *alloc_val(KatieValKind kind);
KatieVal *alloc_number(i64 number);
KatieVal *alloc_bool(bool _bool);
KatieVal *alloc_list(Array(KatieVal *) list);
KatieVal *alloc_symbol(char *text, usize length);
KatieVal *alloc_proc(Katie_Proc proc);
void dealloc_val(KatieVal *val);

String katie_value_as_string(String strResult, KatieVal *type);

// --------------------------------------------------------------------------
//                          - Error Reporting -
// --------------------------------------------------------------------------
void katie_syntax_error(char *filepath, Token *token, char *prefix, char *msg, ...);

#endif
