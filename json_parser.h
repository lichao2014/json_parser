#ifndef _JSON_PARSER_H_INCLUDED
#define _JSON_PARSER_H_INCLUDED

#include <stdint.h>
#include "json.h"


#define JSON_PARSER_IS_WS(c)     \
    (' ' == (c)) || ('\n' == (c)) || ('\r' == (c)) || ('\t' == (c))


#define JSON_PARSER_PEEK(stream)                                    \
    (stream)->vtbl->peek((stream)->ctx)


#define JSON_PARSER_TAKE(stream)                                    \
    (stream)->vtbl->take((stream)->ctx)


#define JSON_PARSER_PUT_BEGIN(stream)                               \
    (stream)->vtbl->put_begin((stream)->ctx)


#define JSON_PARSER_PUT(stream, c)                                  \
    (stream)->vtbl->put((stream)->ctx, c)


#define JSON_PARSER_PUT_END(stream, begin)                          \
    (stream)->vtbl->put_end((stream)->ctx, begin)


#define JSON_PARSER_CONSUME(stream, expect)                         \
    ((expect == JSON_PARSER_PEEK(stream))                           \
        ? (JSON_PARSER_TAKE(stream), 0) : -1)


#define JSON_PARSER_SKIP_WS(stream)                                 \
    while (JSON_PARSER_IS_WS(JSON_PARSER_PEEK(stream))) {           \
        JSON_PARSER_TAKE(stream);                                   \
    }


#define JSON_PARSER_HANDLER(handler, h, ...)                        \
    (handler->vtbl->h)(handler->ctx, ##__VA_ARGS__)


#define JSON_PARSER_ALLOC(parser, size)                             \
    ((parser)->alloc.vtbl->on_alloc)((parser)->alloc.ctx, size)

#define JSON_PARSER_FREE(parser, p)                                 \
    ((parser)->alloc.vtbl->on_free)((parser)->alloc.ctx, p)


#define JSON_PARSER_ERR_MAP(XX)             \
    XX(OK)                                  \
    XX(DOCUMENT_EMPTY)                      \
    XX(DOCUMENT_ROOT_NOT_SINGULAR)          \
    XX(VALUE_INVALID)                       \
    XX(OBJECT_MISS_NAME)                    \
    XX(OBJECT_MISS_COLON)                   \
    XX(OBJECT_MISS_COMMA_OR_CURLY_BRACKET)  \
    XX(ARRAY_MISS_COMMA_OR_SQUARE_BRACKET)  \
    XX(STRING_UNICODE_ESCAPE_INVALID_HEX)   \
    XX(STRING_UNICODE_SURROGATE_INVALID)    \
    XX(STRING_ESCAPE_INVALID)               \
    XX(STRING_MISS_QUOTATION_MARK)          \
    XX(STRING_INVALID_ENCODING)             \
    XX(NUMBER_TOO_BIG)                      \
    XX(NUMBER_MISS_FRACTION)                \
    XX(NUMBER_MISS_EXPONENT)                \
    XX(TERMINATION)                         \
    XX(UNSPECIFIC_SYNTAX_ERROR)


enum json_parser_error_code_t {
#define XX(v)   JSON_PARSER_ERROR_##v,
    JSON_PARSER_ERR_MAP(XX)
#undef XX
};


struct json_parser_handler_vtbl_t {
    int(*on_null)(void *ctx);
    int(*on_bool)(void *ctx, int b);
    int(*on_int)(void *ctx, int i);
    int(*on_uint)(void *ctx, unsigned int i);
    int(*on_int64)(void *ctx, int64_t i);
    int(*on_uint64)(void *ctx, uint64_t i);
    int(*on_double)(void *ctx, double d);
    int(*on_key)(void *ctx, const char *str, size_t len);
    int(*on_string)(void *ctx, const char *str, size_t len);
    int(*on_start_object)(void *ctx);
    int(*on_end_object)(void *ctx, size_t count);
    int(*on_start_array)(void *ctx);
    int(*on_end_array)(void *ctx, size_t count);
};


struct json_parser_handler_t {
    struct json_parser_handler_vtbl_t *vtbl;
    void *ctx;
};


struct json_parser_t {
    struct json_value_t *current;

    uint16_t depth;
    uint16_t max_depth;

    struct json_value_t *root;
    struct json_allocator_t *a;
};


static inline void
json_parser_init(struct json_parser_t *parser, uint16_t max_depth, struct json_allocator_t *a)
{
    parser->root = parser->current = NULL;
    parser->depth = 0;
    parser->max_depth = max_depth;
    parser->a = a;
}


static inline void
json_parser_clear(struct json_parser_t *parser)
{
    if (parser->root) {
        json_value_free(parser->a, parser->root, 0);
    }

    parser->root = parser->current = NULL;
    parser->depth = 0;
}


int json_read(struct json_stream_t *stream, struct json_parser_handler_t *handler);

int json_parse_stream(struct json_parser_t *parser, struct json_stream_t *stream);

int json_parse_str(struct json_parser_t *parser, const char *str, size_t len);


#endif //_JSON_PARSER_H_INCLUDED

