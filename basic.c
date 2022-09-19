#include "basic.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void die(const char *fmt) {
    perror(fmt);
    exit(1);
}

void *xmalloc(usize size) {
    void *ptr = malloc(size);
    if (!ptr) die("malloc");
    return ptr;
}

void *xrealloc(void *ptr, usize size) {
    void *_ptr = realloc(ptr, size);
    if (!_ptr) die("realloc");
    return _ptr;
}

void m_puts(char *cstring) {
    while (*cstring)
        putc(*cstring++, stdout);
}

// --------------------------------------------------------------------------
//                          - Character -
// --------------------------------------------------------------------------
bool is_binary_digit(int ch) {
    return (ch == '0' || ch == '1');
}
bool is_octal_digit(int ch) {
    return ((ch >= '1' && ch <= '8'));
}
bool is_decimal_digit(int ch) {
    return (ch >= '0' && ch <= '9');
}
bool is_hex_digit(int ch) {
    return (is_decimal_digit(ch) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'));
}
bool is_alphabet(int ch) {
    return ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'));
}
bool is_alphanumeric(int ch) {
    return ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9'));
}

int to_lowercase(int ch) {
    return (ch >= 'A' && ch <= 'Z') ? ch + 32 : ch;
}
int to_uppercase(int ch) {
    return (ch >= 'a' && ch <= 'z') ? ch - 32 : ch;
}

// --------------------------------------------------------------------------
//                          - Asserts -
// --------------------------------------------------------------------------

void report_assertion_failure(char *prefix, const char *filename, usize line_number,
                              const char *function_name, char *cond, char *msg, ...) {
    fprintf(stderr, "%s:%zu: %s: %s: ", filename, line_number, prefix, function_name);
    if (cond) {
        fprintf(stderr, "`%s` ", cond);
    }

    if (msg) {
        va_list ap;
        va_start(ap, msg);
        vfprintf(stderr, msg, ap);
        va_end(ap);
    }

    fputc('\n', stderr);
}

// --------------------------------------------------------------------------
//                          - String -
// --------------------------------------------------------------------------
String string_reserve(usize cap) {
    StringHeader *h;

    h = (StringHeader *)malloc(sizeof(StringHeader) + cap + 1);
    h->length = 0;
    h->capacity = cap;

    return (String)(h + 1);
}

String make_string_empty() {
    return string_reserve(0);
}

String make_string(char *str, usize len) {
    String s = string_reserve(len);
    memcpy(s, str, len);
    s[len] = '\0';
    string_length(s) = len;
    return s;
}

String string_reset(String s) {
    string_length(s) = 0;
    return s;
}

String append_string_length(String s, char *str, usize len) {
    usize cap;
    usize rem = string_capacity(s) - string_length(s);

    if (rem < len) {
        StringHeader *h = STRING_HEADER(s);
        cap = h->capacity + len + (1 << 6);
        h = (StringHeader *)xrealloc(h, sizeof(StringHeader) + cap + 1);
        h->capacity = cap;
        s = (String)(h + 1);
    }

    memcpy(&s[string_length(s)], str, len);
    string_length(s) += len;
    s[string_length(s)] = '\0';

    return s;
}

bool are_strings_equal(String lhs, String rhs) {
    if (string_length(lhs) != string_length(rhs)) return false;
    string_for_each(lhs, i) {
        if (lhs[i] != rhs[i]) return false;
    }
    return true;
}

bool are_strings_equal_length(String lhs, char *rhs, usize rhs_length) {
    if (string_length(lhs) != rhs_length) return false;
    string_for_each(lhs, i) {
        if (lhs[i] != rhs[i]) return false;
    }
    return true;
}

bool are_equal_cstring(String lhs, char *rhs) {
    if (string_length(lhs) != strlen(rhs)) return false;
    string_for_each(lhs, i) {
        if (lhs[i] != rhs[i]) return false;
    }
    return true;
}

bool are_cstrings_equal_length(char *a, char const *b, usize length) {
    while (length && *b) {
        if (*a != *b) return false;
        a++;
        b++;
        length--;
    }
    if (length != 0 || *b != '\0') return false;
    return true;
}

String file_as_string(char *filepath) {
    FILE *f;
    String content;
    u32 content_size;

    f = fopen(filepath, "r");
    if (!f) {
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    content_size = ftell(f);
    rewind(f);

    content = string_reserve(content_size);
    fread(content, 1, content_size, f);
    content[content_size] = '\0';
    string_length(content) = content_size;

    fclose(f);
    return content;
}
