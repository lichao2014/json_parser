#ifndef _JSON_PARSER_H_INCLUDED
#define _JSON_PARSER_H_INCLUDED

#include <stdint.h>

#define JSON_PARSER_ASSERT(opt)

#define JSON_PARSER_IS_WS(c)     \
    (' ' == (c)) || ('\n' == (c)) || ('\r' == (c)) || ('\t' == (c))

#define JSON_PARSER_SKIP_WS(stream, ctx)                \
    while (JSON_PARSER_IS_WS((stream)->peek(ctx))) {    \
        (stream)->take(ctx);                            \
    }

#define JSON_PARSER_CONSUME(stream, ctx, expect)                 \
    ((expect == (stream)->peek(ctx)) ? ((stream)->take(ctx), 0) : -1)


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

struct json_parser_handler_t {
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


struct json_parser_stream_t {
    char(*peek)(void *ctx);
    char(*take)(void *ctx);
    size_t(*tell)(void *ctx);
    char*(*put_begin)(void *ctx);
    void(*put)(void *ctx, char c);
    void(*flush)(void *ctx);
    size_t(*put_end)(void *ctx, char *begin);
};


int json_parse_string(struct json_parser_stream_t *stream, struct json_parser_handler_t *handler, void *ctx);

int json_parse_number(struct json_parser_stream_t *stream, struct json_parser_handler_t *handler, void *ctx);

int json_parse_object(struct json_parser_stream_t *stream, struct json_parser_handler_t *handler, void *ctx);

int json_parse_array(struct json_parser_stream_t *stream, struct json_parser_handler_t *handler, void *ctx);

int json_parse_true(struct json_parser_stream_t *stream, struct json_parser_handler_t *handler, void *ctx);

int json_parse_false(struct json_parser_stream_t *stream, struct json_parser_handler_t *handler, void *ctx);

int json_parse_null(struct json_parser_stream_t *stream, struct json_parser_handler_t *handler, void *ctx);

int json_parse_value(struct json_parser_stream_t *stream, struct json_parser_handler_t *handler, void *ctx);

typedef int(*json_parse_fn_t)(struct json_parser_stream_t *, struct json_parser_handler_t *, void *);
json_parse_fn_t json_parse_test(struct json_parser_stream_t *, void *);

int json_parse(struct json_parser_stream_t *stream, struct json_parser_handler_t *handler, void *ctx);

#endif //_JSON_PARSER_H_INCLUDED

