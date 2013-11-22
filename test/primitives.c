#include <laxjson.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static enum LaxJsonType expected_type;
static const char *expected_string;

static const char *err_to_str(enum LaxJsonError err) {
    switch(err) {
        case LaxJsonErrorNone:
            return "";
        case LaxJsonErrorUnexpectedChar:
            return "unexpected char";
        case LaxJsonErrorExpectedEof:
            return "expected EOF";
        case LaxJsonErrorExceededMaxStack:
            return "exceeded max stack";
        case LaxJsonErrorNoMem:
            return "no mem";
        case LaxJsonErrorExceededMaxValueSize:
            return "exceeded max value size";
        case LaxJsonErrorInvalidHexDigit:
            return "invalid hex digit";
        case LaxJsonErrorInvalidUnicodePoint:
            return "invalid unicode point";
        case LaxJsonErrorExpectedColon:
            return "expected colon";
    }
    exit(1);
}

static const char *type_to_str(enum LaxJsonType type) {
    switch (type) {
        case LaxJsonTypeString:
            return "string";
        case LaxJsonTypeProperty:
            return "property";
        case LaxJsonTypeNumber:
            return "number";
        case LaxJsonTypeObject:
            return "object";
        case LaxJsonTypeArray:
            return "array";
        case LaxJsonTypeTrue:
            return "true";
        case LaxJsonTypeFalse:
            return "false";
        case LaxJsonTypeNull:
            return "null";
    }
    exit(1);
}

static void feed(struct LaxJsonContext *context, const char *data) {
    int size = strlen(data);

    enum LaxJsonError err = lax_json_feed(context, size, data);

    if (!err)
        return;

    fprintf(stderr, "line %d column %d parse error: %s\n", context->line,
            context->column, err_to_str(err));
    exit(1);
}

static void on_string_fail(struct LaxJsonContext *context,
    enum LaxJsonType type, const char *value, int length)
{
    fprintf(stderr, "unexpected string\n");
    exit(1);
}

static void on_string_expect(struct LaxJsonContext *context,
    enum LaxJsonType type, const char *value, int length)
{
    int expected_len;
    if (type != expected_type) {
        fprintf(stderr, "got type: %s expected type: %s\n", type_to_str(type),
                type_to_str(expected_type));
        exit(1);
    }
    expected_len = strlen(expected_string);
    if (length != expected_len) {
        fprintf(stderr, "got string length: %d expected length: %d\n",
                length, expected_len);
        exit(1);
    }
    if (memcmp(value, expected_string, length) != 0) {
        fprintf(stderr, "expected %s, got %s\n", expected_string, value);
        exit(1);
    }
}

static void on_number_fail(struct LaxJsonContext *context, double x)
{
    fprintf(stderr, "unexpected number\n");
    exit(1);
}

static void on_primitive_expect(struct LaxJsonContext *context, enum LaxJsonType type)
{
    if (type != expected_type) {
        fprintf(stderr, "expected %s, got %s\n", type_to_str(expected_type),
                type_to_str(type));
        exit(1);
    }
}

static void on_primitive_fail(struct LaxJsonContext *context, enum LaxJsonType type)
{
    fprintf(stderr, "unexpected primitive: %s\n", type_to_str(type));
    exit(1);
}

static void on_begin_fail(struct LaxJsonContext *context, enum LaxJsonType type)
{
    fprintf(stderr, "unexpected array or object\n");
    exit(1);
}

static void on_end_fail(struct LaxJsonContext *context, enum LaxJsonType type)
{
    fprintf(stderr, "unexpected end of array or object\n");
    exit(1);
}

static void test_false() {
    struct LaxJsonContext *context;
    
    context = lax_json_create();
    if (!context)
        exit(1);

    context->userdata = NULL;
    context->string = on_string_fail;
    context->number = on_number_fail;
    expected_type = LaxJsonTypeFalse;
    context->primitive = on_primitive_expect;
    context->begin = on_begin_fail;
    context->end = on_end_fail;

    feed(context,
        "// this is a comment\n"
        " false"
        );

    lax_json_destroy(context);
}

static void test_true() {
    struct LaxJsonContext *context;
    
    context = lax_json_create();
    if (!context)
        exit(1);

    context->userdata = NULL;
    context->string = on_string_fail;
    context->number = on_number_fail;
    expected_type = LaxJsonTypeTrue;
    context->primitive = on_primitive_expect;
    context->begin = on_begin_fail;
    context->end = on_end_fail;

    feed(context,
        " /* before comment */true"
        );

    lax_json_destroy(context);
}

static void test_null() {
    struct LaxJsonContext *context;
    
    context = lax_json_create();
    if (!context)
        exit(1);

    context->userdata = NULL;
    context->string = on_string_fail;
    context->number = on_number_fail;
    expected_type = LaxJsonTypeNull;
    context->primitive = on_primitive_expect;
    context->begin = on_begin_fail;
    context->end = on_end_fail;

    feed(context,
        "null/* after comment*/ // line comment"
        );

    lax_json_destroy(context);
}

static void test_string() {
    struct LaxJsonContext *context;
    
    context = lax_json_create();
    if (!context)
        exit(1);

    context->userdata = NULL;
    expected_string = "foo";
    expected_type = LaxJsonTypeString;
    context->string = on_string_expect;
    context->number = on_number_fail;
    context->primitive = on_primitive_fail;
    context->begin = on_begin_fail;
    context->end = on_end_fail;

    feed(context,
        "\"foo\""
        );

    lax_json_destroy(context);
}

struct Test {
    const char *name;
    void (*fn)(void);
};

static struct Test tests[] = {
    {"false primitive", test_false},
    {"true primitive", test_true},
    {"null primitive", test_null},
    {"string primitive", test_string},
    {NULL, NULL},
};

int main(int argc, char *argv[]) {
    struct Test *test = &tests[0];

    while (test->name) {
        fprintf(stderr, "testing %s...", test->name);
        test->fn();
        fprintf(stderr, "OK\n");
        test += 1;
    }

    return 0;
}
