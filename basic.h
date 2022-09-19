#ifndef __basic_h__
#define __basic_h__

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --------------------------------------------------------------------------
//                          - Basic -
// --------------------------------------------------------------------------

#define U8_MIN 0x0
#define U8_MAX 0xff
#define U16_MIN 0x0
#define U16_MAX 0xffff
#define U32_MIN 0x0
#define U32_MAX 0xffffffff
#define U64_MIN 0x0
#define U64_MAX 0xffffffffffffffff

#define I8_MIN (-0x7f - 0x1)
#define I8_MAX 0x7f
#define I16_MIN (-0x7fff - 0x1)
#define I16_MAX 0x7fff
#define I32_MIN (-0x7fffffff - 0x1)
#define I32_MAX 0x7fffffff
#define I64_MIN (-0x7fffffffffffffff - 0x1)
#define I64_MAX 0x7fffffffffffffff

typedef uint8_t u8;
typedef int8_t i8;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint64_t u64;
typedef int64_t i64;

typedef float f32;
typedef double f64;

typedef unsigned int uint;
typedef uintptr_t uintptr;
typedef intptr_t intptr;

typedef size_t usize;
typedef ptrdiff_t isize;

#define cast(Type) (Type)
#define swap(a, b, Type)                                                                           \
    do {                                                                                           \
        Type tmp = a;                                                                              \
        a = b;                                                                                     \
        b = tmp;                                                                                   \
    } while (0);

#define array_sizeof(arr, Type) (sizeof(arr) / sizeof(Type))

#define println(...)                                                                               \
    do {                                                                                           \
        fprintf(stdout, __VA_ARGS__);                                                              \
        putc('\n', stdout);                                                                        \
    } while (0);
#define eprintln(...)                                                                              \
    do {                                                                                           \
        fprintf(stderr, __VA_ARGS__);                                                              \
        putc('\n', stderr);                                                                        \
    } while (0);

#ifndef Debug
#define debug_println(...)
#else
#define debug_println(...) println(__VA_ARGS__)
#endif

void die(const char *fmt);
void *xmalloc(usize size);
void *xrealloc(void *ptr, usize size);
void m_puts(char *cstring);

// --------------------------------------------------------------------------
//                          - Asserts -
// --------------------------------------------------------------------------

#ifndef DEBUG_TRAP
#define DEBUG_TRAP() __builtin_trap()
#endif

#ifndef Assert
#define Assert(cond)                                                                               \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            report_assertion_failure("Assertion Failure", __FILE__, __LINE__, __func__, #cond,     \
                                     NULL);                                                        \
            DEBUG_TRAP();                                                                          \
        }                                                                                          \
    } while (0);
#endif

#ifndef Assert_Message
#define Assert_Message(cond, msg)                                                                  \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            report_assertion_failure("Assertion Failure", __FILE__, __LINE__, __func__, #cond,     \
                                     msg);                                                         \
            DEBUG_TRAP();                                                                          \
        }                                                                                          \
    } while (0);
#endif

#ifndef Debug_Assert
#ifndef Debug
#define Debug_Assert(cond)
#else
#define Debug_Assert(cond)                                                                         \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            report_assertion_failure("Debug Assert Failure", __FILE__, __LINE__, __func__, #cond,  \
                                     NULL);                                                        \
            DEBUG_TRAP();                                                                          \
        }                                                                                          \
    } while (0);
#endif
#endif

#ifndef Debug_Assert_Message
#ifndef Debug
#define Debug_Assert_Message(cond, msg)
#else
#define Debug_Assert_Message(cond, msg)                                                            \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            report_assertion_failure("Debug Assert Failure", __FILE__, __LINE__, __func__, #cond,  \
                                     msg);                                                         \
            DEBUG_TRAP();                                                                          \
        }                                                                                          \
    } while (0);
#endif
#endif

#ifndef Unreachable
#define Unreachable()                                                                              \
    do {                                                                                           \
        report_assertion_failure("Unreachable", __FILE__, __LINE__, __func__, NULL, NULL);         \
        DEBUG_TRAP();                                                                              \
    } while (0);
#endif

#ifndef Todo
#define Todo()                                                                                     \
    do {                                                                                           \
        report_assertion_failure("Todo", __FILE__, __LINE__, __func__, NULL, NULL);                \
        DEBUG_TRAP();                                                                              \
    } while (0);
#endif

#ifndef Todo_Message
#define Todo_Message(...)                                                                          \
    do {                                                                                           \
        report_assertion_failure("Todo", __FILE__, __LINE__, __func__, NULL, __VA_ARGS__);         \
        DEBUG_TRAP();                                                                              \
    } while (0);
#endif

void report_assertion_failure(char *prefix, const char *filename, usize line_number,
                              const char *function_name, char *cond, char *msg, ...);

// --------------------------------------------------------------------------
//                          - Character -
// --------------------------------------------------------------------------

bool is_binary_digit(int ch);
bool is_octal_digit(int ch);
bool is_decimal_digit(int ch);
bool is_hex_digit(int ch);
bool is_alphabet(int ch);
bool is_alphanumeric(int ch);

int to_lowercase(int ch);
int to_uppercase(int ch);

#define binary_digit_to_int(digit) (digit - '0')
#define octal_digit_to_int(digit) (digit - '0')
#define decimal_digit_to_int(digit) (digit - '0')
#define hex_digit_to_int(digit)                                                                    \
    ((digit >= 'a' && digit <= 'f')                                                                \
         ? (digit - 'a' + 10)                                                                      \
         : ((digit >= 'A' && digit <= 'F') ? (digit - 'A' + 10) : (digit - '0')))

// --------------------------------------------------------------------------
//                          - Array -
// --------------------------------------------------------------------------

#if 0 // Array Example
void main(void) {
    Array(int) a;

    init_array(a);

    array_push(a, 1);
    array_push(a, 2);
    array_push(a, 3);
    array_push(a, 4);

    array_for_each(a, i) {
        printf("%d\n", a[i]);
    }

    free_array(a);
}
#endif

typedef struct ArrayHeader ArrayHeader;
struct ArrayHeader {
    usize length;
    usize capacity;
};

#define ARRAY_GROW_FORMULA(x) (2 * (x) + 8)
#define ARRAY_HEADER(a) ((ArrayHeader *)(a)-1)

#define Array(Type) Type *
#define array_length(a) ARRAY_HEADER(a)->length
#define array_capacity(a) ARRAY_HEADER(a)->capacity
#define free_array(a) free(ARRAY_HEADER(a))
#define array_is_empty(a) (array_length(a) == 0)
#define array_for_each(a, counter)                                                                 \
    for (usize(counter) = 0; (counter) < array_length(a); ++(counter))

#define array_reserve(a, cap)                                                                      \
    do {                                                                                           \
        void **__array = (void **)&(a);                                                            \
        ArrayHeader *h = (ArrayHeader *)xmalloc(sizeof(ArrayHeader) + (sizeof(*(a)) * (cap)));     \
        h->capacity = cap;                                                                         \
        h->length = 0;                                                                             \
        *__array = (void *)(h + 1);                                                                \
    } while (0);

#define init_array(a) array_reserve(a, ARRAY_GROW_FORMULA(0))
#define array_push(a, VAL)                                                                         \
    do {                                                                                           \
        if (array_length(a) >= array_capacity(a)) {                                                \
            void **__array = (void **)&(a);                                                        \
            ArrayHeader *h = ARRAY_HEADER(a);                                                      \
            ArrayHeader *nh = (ArrayHeader *)xmalloc(                                              \
                sizeof(ArrayHeader) + sizeof(*(a)) * ARRAY_GROW_FORMULA(array_capacity(a)));       \
            memmove(nh, h, sizeof(ArrayHeader) + sizeof(*(a)) * array_length(a));                  \
            nh->capacity = ARRAY_GROW_FORMULA(array_length(a));                                    \
            nh->length = h->length;                                                                \
            free(h);                                                                               \
            *__array = (void *)(nh + 1);                                                           \
        }                                                                                          \
        a[array_length(a)] = VAL;                                                                  \
        array_length(a) += 1;                                                                      \
    } while (0);

// --------------------------------------------------------------------------
//                          - String -
// --------------------------------------------------------------------------

#if 0 // String Example
void main(void) {
    String s = make_string_empty();

    s = append_string("Hello");
    s = append_string(", World");

    printf("%s", s);

    free_string(a);
}
#endif

typedef char *String;

#define STRING_HEADER(s) ((StringHeader *)(s)-1)
#define free_string(s) free(STRING_HEADER(s))
#define string_length(s) (STRING_HEADER(s)->length)
#define string_capacity(s) (STRING_HEADER(s)->capacity)
#define string_for_each(s, i) for (usize i = 0; i < string_length(s); ++i)
#define append_string(s, str) append_string_length(s, str, string_length(s))
#define append_cstring(s, cstr) append_string_length(s, cstr, strlen(cstr))

typedef struct StringHeader StringHeader;
struct StringHeader {
    usize length;
    usize capacity;
};

String string_reserve(usize cap);
String make_string_empty();
String make_string(char *str, usize len);
String append_string_length(String s, char *str, usize len);
bool are_strings_equal(String lhs, String rhs);
bool are_strings_equal_length(String lhs, char *rhs, usize rhs_length);
bool are_equal_cstring(String lhs, char *rhs);
String file_as_string(char *filepath);
bool are_cstrings_equal_length(char *a, char const *b, usize length);

#endif
