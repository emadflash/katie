#include <stdarg.h>
#include <stdio.h>

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

// --------------------------------------------------------------------------
//                          - Value -
// --------------------------------------------------------------------------
typedef struct KatieVal KatieVal;
typedef i64 Katie_Number;
typedef u8 Katie_Bool;
typedef Array(KatieVal *) Katie_List;
typedef String Katie_Symbol;
typedef KatieVal* (*Katie_Proc)(KatieEnv *env, int argc, KatieVal **argv);

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

KatieVal *alloc_proc(Katie_Proc proc) {
    KatieVal *val = alloc_val(KatieValKind_Proc);
    val->as.proc = proc;
    return val;
}

void dealloc_val(KatieVal *val) {
    switch (val->kind) {
    case KatieValKind_Number:
    case KatieValKind_Bool: break;
    case KatieValKind_Symbol: free_string(val->as.symbol); break;

    case KatieValKind_List: {
        array_for_each(val->as.list, i) { dealloc_val(val->as.list[i]); }
        free_array(val->as.list);
    } break;

    default: Unreachable();
    }

    free(val);
}

String katie_value_as_string(String strResult, KatieVal *type) {
    switch (type->kind) {
    case KatieValKind_Number: {
        char buf[256];
        sprintf(buf, "%ld", type->as.number);
        strResult = append_string_length(strResult, buf, strlen(buf));
    } break;

    case KatieValKind_Symbol:
        strResult =
            append_string_length(strResult, type->as.symbol, string_length(type->as.symbol));
        break;

    case KatieValKind_List:
        strResult = append_cstring(strResult, "(");
        array_for_each(type->as.list, i) {
            strResult = katie_value_as_string(strResult, type->as.list[i]);
            strResult = append_cstring(strResult, " ");
        }
        strResult = append_cstring(strResult, ")");
        break;

    default: Unreachable();
    }

    return strResult;
}

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
//                          - Env -
// --------------------------------------------------------------------------
struct KatieEnv {
    Array(char*) keys;
    Array(KatieVal*) values;
    KatieEnv *outer;
};

void init_env(KatieEnv *env) {
    init_array(env->keys);
    init_array(env->values);
    env->outer = NULL;
}

void deinit_env(KatieEnv *env) {
    free_array(env->keys);
    free_array(env->values);
}

void env_define(KatieEnv *env, char *key, KatieVal *val) {
    array_push(env->keys, key);
    array_push(env->values, val);
}

KatieVal *env_lookup(KatieEnv *env, String key) {
    array_for_each(env->keys, i) {
        if (are_equal_cstring(key, env->keys[i])) {
            return env->values[i];
        }
    }
    return NULL;
}

// --------------------------------------------------------------------------
//                          - Natives -
// --------------------------------------------------------------------------
i64 katie_get_op_value(KatieVal *val) {
    switch (val->kind) {
        case KatieValKind_Number: return val->as.number;
        case KatieValKind_Bool: return cast(i64) val->as._bool;
        default: return -1;
    }
}

KatieVal *native_op_add(KatieEnv *env, int argc, KatieVal **argv) {
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

KatieVal *native_op_sub(KatieEnv *env, int argc, KatieVal **argv) {
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

KatieVal *native_op_mul(KatieEnv *env, int argc, KatieVal **argv) {
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

KatieVal *native_op_div(KatieEnv *env, int argc, KatieVal **argv) {
    i64 result, temp;
    int i = 0;

    do {
        result = katie_get_op_value(argv[i]);
        i++;
    } while (i < argc && result < 0);

    for (; i < argc; ++i) {
        temp = katie_get_op_value(argv[i]);
        if (temp >= 0) {
            result = cast(i64) (result / temp);
        }
    }
    return alloc_number(result);
}

// --------------------------------------------------------------------------
//                          - Katie -
// --------------------------------------------------------------------------
typedef struct Katie Katie;
struct Katie {
    KatieEnv env;
};

void init_katie_w_natives(Katie *k) {
    init_env(&k->env);
    env_define(&k->env, "+", alloc_proc(native_op_add)); 
    env_define(&k->env, "-", alloc_proc(native_op_sub)); 
    env_define(&k->env, "*", alloc_proc(native_op_mul)); 
    env_define(&k->env, "/", alloc_proc(native_op_div)); 
}

void deinit_katie(Katie *k) {
    deinit_env(&k->env);
}
