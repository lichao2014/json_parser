#include "json_parser.h"
#include <stdlib.h>
#include <assert.h>


static int
json_read_value(struct json_stream_t *stream, struct json_parser_handler_t *handler);


static int
json_read_string_opt(struct json_stream_t *stream, struct json_parser_handler_t *handler, int is_key)
{
    if (JSON_PARSER_CONSUME(stream, '"')) {
        return JSON_PARSER_ERROR_VALUE_INVALID;
    }

    char *head = JSON_PARSER_PUT_BEGIN(stream);

    char c;
    while (c = JSON_PARSER_TAKE(stream), '"' != c) {
        JSON_PARSER_PUT(stream, c);
    }

    size_t length = JSON_PARSER_PUT_END(stream, head);
    assert(length <= 0xFFFFFFFF);

    if ((is_key ? handler->vtbl->on_key : handler->vtbl->on_string)(handler->ctx, head, length)) {
        return JSON_PARSER_ERROR_TERMINATION;
    }

    return JSON_PARSER_ERROR_OK;
}


static int
json_read_object(struct json_stream_t *stream, struct json_parser_handler_t *handler)
{
    if (JSON_PARSER_CONSUME(stream, '{')) {
        return JSON_PARSER_ERROR_VALUE_INVALID;
    }

    if (JSON_PARSER_HANDLER(handler, on_start_object)) {
        return JSON_PARSER_ERROR_TERMINATION;
    }

    JSON_PARSER_SKIP_WS(stream);

    if (!JSON_PARSER_CONSUME(stream, '}')) {
        if (JSON_PARSER_HANDLER(handler, on_end_object, 0)) {
            return JSON_PARSER_ERROR_TERMINATION;
        }

        return JSON_PARSER_ERROR_OK;
    }

    size_t count = 0;
    while (1) {

        if (json_read_string_opt(stream, handler, 1)) {
            return JSON_PARSER_ERROR_OBJECT_MISS_NAME;
        }

        JSON_PARSER_SKIP_WS(stream);

        if (JSON_PARSER_CONSUME(stream, ':')) {
            return JSON_PARSER_ERROR_OBJECT_MISS_COLON;
        }

        JSON_PARSER_SKIP_WS(stream);

        if (json_read_value(stream, handler)) {
            return JSON_PARSER_ERROR_OBJECT_MISS_COMMA_OR_CURLY_BRACKET;
        }

        JSON_PARSER_SKIP_WS(stream);

        ++count;

        switch (JSON_PARSER_PEEK(stream)) {
        case ',':
            JSON_PARSER_TAKE(stream);
            JSON_PARSER_SKIP_WS(stream);
            break;
        case '}':
            JSON_PARSER_TAKE(stream);

            if (JSON_PARSER_HANDLER(handler, on_end_object, count)) {
                return JSON_PARSER_ERROR_TERMINATION;
            }

            return JSON_PARSER_ERROR_OK;
        default:
            return JSON_PARSER_ERROR_OBJECT_MISS_COMMA_OR_CURLY_BRACKET;
            break;
        }
    }

    return JSON_PARSER_ERROR_OK;
}


static int
json_read_string(struct json_stream_t *stream, struct json_parser_handler_t *handler)
{
    return json_read_string_opt(stream, handler, 0);
}


static inline int
json_read_null(struct json_stream_t *stream, struct json_parser_handler_t *handler)
{
    if (JSON_PARSER_CONSUME(stream, 'n')
        || JSON_PARSER_CONSUME(stream, 'u')
        || JSON_PARSER_CONSUME(stream, 'l')
        || JSON_PARSER_CONSUME(stream, 'l')) {

        return JSON_PARSER_ERROR_VALUE_INVALID;
    }

    if (JSON_PARSER_HANDLER(handler, on_null)) {
        return JSON_PARSER_ERROR_TERMINATION;
    }

    return JSON_PARSER_ERROR_OK;
}


static inline int
json_read_true(struct json_stream_t *stream, struct json_parser_handler_t *handler)
{
    if (JSON_PARSER_CONSUME(stream, 't')
        || JSON_PARSER_CONSUME(stream, 'r')
        || JSON_PARSER_CONSUME(stream, 'u')
        || JSON_PARSER_CONSUME(stream, 'e')) {

        return JSON_PARSER_ERROR_VALUE_INVALID;
    }

    if (JSON_PARSER_HANDLER(handler, on_bool, 1)) {
        return JSON_PARSER_ERROR_TERMINATION;
    }

    return JSON_PARSER_ERROR_OK;
}


static inline int
json_read_false(struct json_stream_t *stream, struct json_parser_handler_t *handler)
{
    if (JSON_PARSER_CONSUME(stream, 'f')
        || JSON_PARSER_CONSUME(stream, 'a')
        || JSON_PARSER_CONSUME(stream, 'l')
        || JSON_PARSER_CONSUME(stream, 's')
        || JSON_PARSER_CONSUME(stream, 'e')) {

        return JSON_PARSER_ERROR_VALUE_INVALID;
    }

    if (JSON_PARSER_HANDLER(handler, on_bool, 0)) {
        return JSON_PARSER_ERROR_TERMINATION;
    }

    return JSON_PARSER_ERROR_OK;
}


static int
json_read_number(struct json_stream_t *stream, struct json_parser_handler_t *handler)
{
    int minus = 1;
    int a = 0;
    int d = 0;
    double f = 1;
    int e = 0;
    int e_minus = 1;

    if (!JSON_PARSER_CONSUME(stream, '-')) {
        minus = -1;
    }

    char c = JSON_PARSER_PEEK(stream);
    if ('0' == c) {
        JSON_PARSER_TAKE(stream);
    }
    else if ((c >= '1') && (c <= '9')) {
        a = a * 10 + (c - '0');
        JSON_PARSER_TAKE(stream);

        while (c = JSON_PARSER_PEEK(stream), (c >= '0') && (c <= '9')) {
            a = a * 10 + (c - '0');
            JSON_PARSER_TAKE(stream);
        }
    }
    else {
        return JSON_PARSER_ERROR_VALUE_INVALID;
    }

    c = JSON_PARSER_PEEK(stream);
    if ('.' == c) {
        JSON_PARSER_TAKE(stream);

        while (c = JSON_PARSER_PEEK(stream), (c >= '0') && (c <= '9')) {
            d = d * 10 + (c - '0');
            f *= 0.1;
            JSON_PARSER_TAKE(stream);
        }
    }

    c = JSON_PARSER_PEEK(stream);
    if (('e' == c) || ('E' == c)) {
        JSON_PARSER_TAKE(stream);

        e_minus = JSON_PARSER_CONSUME(stream, '-');

        while (c = JSON_PARSER_PEEK(stream), (c >= '0') && (c <= '9')) {
            e = e * 10 + (e - '0');
            JSON_PARSER_TAKE(stream);
        }
    }

    if (d) {
        f *= d;
        f += a;

        if (JSON_PARSER_HANDLER(handler, on_double, minus * f)) {
            return JSON_PARSER_ERROR_TERMINATION;
        }
    }
    else {
        if (JSON_PARSER_HANDLER(handler, on_int, minus * a)) {
            return JSON_PARSER_ERROR_TERMINATION;
        }
    }

    return JSON_PARSER_ERROR_OK;
}


static int
json_read_array(struct json_stream_t *stream, struct json_parser_handler_t *handler)
{
    if (JSON_PARSER_CONSUME(stream, '[')) {
        return JSON_PARSER_ERROR_VALUE_INVALID;
    }

    if (JSON_PARSER_HANDLER(handler, on_start_array)) {
        return JSON_PARSER_ERROR_TERMINATION;
    }

    JSON_PARSER_SKIP_WS(stream);

    if (!JSON_PARSER_CONSUME(stream, ']')) {
        if (JSON_PARSER_HANDLER(handler, on_end_array, 0)) {
            return JSON_PARSER_ERROR_TERMINATION;
        }

        return JSON_PARSER_ERROR_OK;
    }

    size_t count = 0;
    while (1) {

        if (json_read_value(stream, handler)) {
            return JSON_PARSER_ERROR_VALUE_INVALID;
        }

        ++count;

        JSON_PARSER_SKIP_WS(stream);

        if (!JSON_PARSER_CONSUME(stream, ']')) {
            if (JSON_PARSER_HANDLER(handler, on_end_array, count)) {
                return JSON_PARSER_ERROR_TERMINATION;
            }

            return JSON_PARSER_ERROR_OK;
        }

        if (JSON_PARSER_CONSUME(stream, ',')) {
            return JSON_PARSER_ERROR_ARRAY_MISS_COMMA_OR_SQUARE_BRACKET;
        }

        JSON_PARSER_SKIP_WS(stream);
    }

    return JSON_PARSER_ERROR_OK;
}


static int
json_read_value(struct json_stream_t *stream, struct json_parser_handler_t *handler)
{
    switch (JSON_PARSER_PEEK(stream)) {

        case '"':
            return json_read_string(stream, handler);

        case '{':
            return json_read_object(stream, handler);

        case '[':
            return json_read_array(stream, handler);

        case 't':
            return json_read_true(stream, handler);

        case 'f':
            return json_read_false(stream, handler);

        case 'n':
            return json_read_null(stream, handler);

        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return json_read_number(stream, handler);

        default:
            return JSON_PARSER_ERROR_VALUE_INVALID;
    }

    assert(0);
}


int
json_read(struct json_stream_t *stream, struct json_parser_handler_t *handler)
{
    JSON_PARSER_SKIP_WS(stream);
    return json_read_value(stream, handler);
}


static inline struct json_value_t *
json_parser_add_value(struct json_parser_t *parser, enum json_value_type_t type)
{
    struct json_value_t *p = json_value_add(parser->a, parser->current, type);

    if (!parser->root) {
        parser->root = p;
    }

    return p;
}


static int
json_parser_on_null(void *ctx)
{
    struct json_parser_t *parser = ctx;
    json_parser_add_value(parser, JSON_VALUE_TYPE_NULL);

    return 0;
}


static int
json_parser_on_bool(void *ctx, int b)
{
    struct json_parser_t *parser = ctx;

    struct json_value_t *p = json_parser_add_value(parser, JSON_VALUE_TYPE_BOOL);
    assert(p);

    p->b = !!b;

    return 0;
}


static int
json_parser_on_int(void *ctx, int i)
{
    struct json_parser_t *parser = ctx;

    struct json_value_t *p = json_parser_add_value(parser, JSON_VALUE_TYPE_INT);
    assert(p);

    p->i = i;

    return 0;
}


static int
json_parser_on_uint(void *ctx, unsigned int i)
{
    return -1;
}


static int
json_parser_on_int64(void *ctx, int64_t i)
{
    return -1;
}


static int
json_parser_on_uint64(void *ctx, uint64_t i)
{
    return -1;
}


static int
json_parser_on_double(void *ctx, double d)
{
    struct json_parser_t *parser = ctx;

    struct json_value_t *p = json_parser_add_value(parser, JSON_VALUE_TYPE_DOUBLE);
    assert(p);

    p->d = d;

    return 0;
}


static int
json_parser_on_string(void *ctx, const char *str, size_t len)
{
    struct json_parser_t *parser = ctx;

    struct json_value_t *p = json_parser_add_value(parser, JSON_VALUE_TYPE_STRING);
    assert(p);

    p->str.data = (char *)str;
    p->str.len = len;

    return 0;
}


static int
json_parser_on_key(void *ctx, const char *str, size_t len)
{
    struct json_parser_t *parser = ctx;

    json_value_add_key(parser->a, parser->current, (char *)str, len);

    return 0;
}


static int
json_parser_on_start_object(void *ctx)
{
    struct json_parser_t *parser = ctx;

    if ((parser->depth + 1) > parser->max_depth) {
        return -1;
    }

    struct json_value_t *p = json_parser_add_value(parser, JSON_VALUE_TYPE_OBJECT);
    assert(p);

    p->obj.size = 0;
    p->obj.head = p->obj.tail = NULL;

    parser->current = p;

    ++parser->depth;

    return 0;
}


static int
json_parser_on_end_object(void *ctx, size_t count)
{
    struct json_parser_t *parser = ctx;
    struct json_value_t *v = parser->current;

    parser->current = v->parent;

    return 0;
}

static int
json_parser_on_start_array(void *ctx)
{
    struct json_parser_t *parser = ctx;

    if ((parser->depth + 1) > parser->max_depth) {
        return -1;
    }

    struct json_value_t *p = json_parser_add_value(parser, JSON_VALUE_TYPE_ARRAY);
    assert(p);

    p->arr.size = 0;
    p->arr.head = p->arr.tail = NULL;

    parser->current = p;

    ++parser->depth;

    return 0;
}

static int
json_parser_on_end_array(void *ctx, size_t count)
{
    struct json_parser_t *parser = ctx;
    struct json_value_t *v = parser->current;

    parser->current = v->parent;

    return 0;
}


static struct json_parser_handler_vtbl_t 
json_parser_handler_vtbl = {
    &json_parser_on_null,
    &json_parser_on_bool,
    &json_parser_on_int,
    &json_parser_on_uint,
    &json_parser_on_int64,
    &json_parser_on_uint64,
    &json_parser_on_double,
    &json_parser_on_key,
    &json_parser_on_string,
    &json_parser_on_start_object,
    &json_parser_on_end_object,
    &json_parser_on_start_array,
    &json_parser_on_end_array
};


static void *
json_parser_on_alloc(void *ctx, size_t size)
{
    return malloc(size);
}


static void
json_parser_on_free(void *ctx, void *p)
{
    free(p);
}


static struct json_allocator_vtbl_t
json_parser_allocator_vtbl = {
    &json_parser_on_alloc,
    &json_parser_on_free
};


static struct json_allocator_t
json_parser_allocator = {
    &json_parser_allocator_vtbl,
    NULL
};


int 
json_parse_stream(struct json_parser_t *parser, struct json_stream_t *stream)
{
    json_parser_clear(parser);

    if (!parser->a) {
        parser->a = &json_parser_allocator;
    }

    struct json_parser_handler_t h;
    h.vtbl = &json_parser_handler_vtbl;
    h.ctx = parser;

    int r = json_read(stream, &h);
    if (JSON_PARSER_ERROR_OK != r) {
        json_value_free(parser->a, parser->root, 0);
    }

    return r;
}


struct json_str_stream_ctx_t {
    char *src;
    char *dst;
    char *head;
    char *tail;
};


static char
json_str_stream_peek(void *ctx)
{
    struct json_str_stream_ctx_t *stream = ctx;
    if (stream->src >= stream->tail) {
        return -1;
    }

    return *stream->src;
}


static char
json_str_stream_take(void *ctx)
{
    struct json_str_stream_ctx_t *stream = ctx;
    if (stream->src >= stream->tail) {
        return -1;
    }

    return *stream->src++;
}


static size_t
json_str_stream_tell(void *ctx)
{
    struct json_str_stream_ctx_t *stream = ctx;
    return stream->src - stream->head;
}


static void
json_str_stream_put(void *ctx, char c)
{
    struct json_str_stream_ctx_t *stream = ctx;
    *stream->dst++;
}


static char *
json_str_stream_put_begin(void *ctx)
{
    struct json_str_stream_ctx_t *stream = ctx;
    return stream->dst = stream->src;
}

static size_t
json_str_stream_put_end(void *ctx, char *begin)
{
    struct json_str_stream_ctx_t *stream = ctx;
    return stream->dst - begin;
}


static struct json_stream_vtbl_t
json_parser_str_stream_vtbl = {
    &json_str_stream_peek,
    &json_str_stream_take,
    &json_str_stream_tell,
    &json_str_stream_put_begin,
    &json_str_stream_put,
    NULL,
    &json_str_stream_put_end
};


int
json_parse_str(struct json_parser_t *parser, const char *str, size_t len)
{
    struct json_str_stream_ctx_t ctx;

    ctx.src = ctx.head = (char *)str;
    ctx.tail = ctx.src + len;
    ctx.dst = NULL;

    struct json_stream_t stream;
    stream.vtbl = &json_parser_str_stream_vtbl;
    stream.ctx = &ctx;

    return json_parse_stream(parser, &stream);
}

