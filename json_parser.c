#include "json_parser.h"

static int
json_parse_string_opt(struct json_parser_stream_t *stream, struct json_parser_handler_t *handler, void *ctx, int is_key)
{
    if (JSON_PARSER_CONSUME(stream, ctx, '"')) {
        return JSON_PARSER_ERROR_VALUE_INVALID;
    }

    char *head = stream->put_begin(ctx);

    char c;
    while (c = stream->take(ctx), '"' != c) {
        stream->put(ctx, c);
    }

    size_t length = stream->put_end(ctx, head);
    JSON_PARSER_ASSERT(length <= 0xFFFFFFFF);

    if ((is_key ? handler->on_key : handler->on_string)(ctx, head, length)) {
        return JSON_PARSER_ERROR_TERMINATION;
    }

    return JSON_PARSER_ERROR_OK;
}


json_parse_fn_t 
json_parse_test(struct json_parser_stream_t *stream, void *ctx)
{
    switch (stream->peek(ctx)) {
    case '"':
        return &json_parse_string;
    case '{':
        return &json_parse_object;
    case '[':
        return &json_parse_array;
    case 't':
        return &json_parse_true;
    case 'f':
        return &json_parse_false;
    case 'n':
        return &json_parse_null;
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
        return &json_parse_number;
    default:
        return NULL;
    }
}

int
json_parse_object(struct json_parser_stream_t *stream, struct json_parser_handler_t *handler, void *ctx)
{
    if (JSON_PARSER_CONSUME(stream, ctx, '{')) {
        return JSON_PARSER_ERROR_VALUE_INVALID;
    }

    if (handler->on_start_object(ctx)) {
        return JSON_PARSER_ERROR_TERMINATION;
    }

    JSON_PARSER_SKIP_WS(stream, ctx);

    if (!JSON_PARSER_CONSUME(stream, ctx, '}')) {
        if (handler->on_end_object(ctx, 0)) {
            return JSON_PARSER_ERROR_TERMINATION;
        }

        return JSON_PARSER_ERROR_OK;
    }

    size_t count = 0;
    while (1) {

        if (json_parse_string_opt(stream, handler, ctx, 1)) {
            return JSON_PARSER_ERROR_OBJECT_MISS_NAME;
        }

        JSON_PARSER_SKIP_WS(stream, ctx);

        if (JSON_PARSER_CONSUME(stream, ctx, ':')) {
            return JSON_PARSER_ERROR_OBJECT_MISS_COLON;
        }

        JSON_PARSER_SKIP_WS(stream, ctx);

        if (json_parse_value(stream, handler, ctx)) {
            return JSON_PARSER_ERROR_OBJECT_MISS_COMMA_OR_CURLY_BRACKET;
        }

        JSON_PARSER_SKIP_WS(stream, ctx);

        ++count;

        switch (stream->peek(ctx)) {
        case ',':
            stream->take(ctx);
            JSON_PARSER_SKIP_WS(stream, ctx);
            break;
        case '}':
            stream->take(ctx);

            if (handler->on_end_object(ctx, count)) {
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


int
json_parse_string(struct json_parser_stream_t *stream, struct json_parser_handler_t *handler, void *ctx)
{
    return json_parse_string_opt(stream, handler, ctx, 0);
}


int
json_parse_null(struct json_parser_stream_t *stream, struct json_parser_handler_t *handler, void *ctx)
{
    if (JSON_PARSER_CONSUME(stream, ctx, 'n')
        || JSON_PARSER_CONSUME(stream, ctx, 'u')
        || JSON_PARSER_CONSUME(stream, ctx, 'l')
        || JSON_PARSER_CONSUME(stream, ctx, 'l')) {

        return JSON_PARSER_ERROR_VALUE_INVALID;
    }

    if (handler->on_null(ctx)) {
        return JSON_PARSER_ERROR_TERMINATION;
    }

    return JSON_PARSER_ERROR_OK;
}


int 
json_parse_true(struct json_parser_stream_t *stream, struct json_parser_handler_t *handler, void *ctx)
{
    if (JSON_PARSER_CONSUME(stream, ctx, 't')
        || JSON_PARSER_CONSUME(stream, ctx, 'r')
        || JSON_PARSER_CONSUME(stream, ctx, 'u')
        || JSON_PARSER_CONSUME(stream, ctx, 'e')) {

        return JSON_PARSER_ERROR_VALUE_INVALID;
    }

    if (handler->on_bool(ctx, 1)) {
        return JSON_PARSER_ERROR_TERMINATION;
    }

    return JSON_PARSER_ERROR_OK;
}


int 
json_parse_false(struct json_parser_stream_t *stream, struct json_parser_handler_t *handler, void *ctx)
{
    if (JSON_PARSER_CONSUME(stream, ctx, 'f')
        || JSON_PARSER_CONSUME(stream, ctx, 'a')
        || JSON_PARSER_CONSUME(stream, ctx, 'l')
        || JSON_PARSER_CONSUME(stream, ctx, 's')
        || JSON_PARSER_CONSUME(stream, ctx, 'e')) {

        return JSON_PARSER_ERROR_VALUE_INVALID;
    }

    if (handler->on_bool(ctx, 0)) {
        return JSON_PARSER_ERROR_TERMINATION;
    }

    return JSON_PARSER_ERROR_OK;
}


int 
json_parse_number(struct json_parser_stream_t *stream, struct json_parser_handler_t *handler, void *ctx)
{
    int minus = 1;
    int a = 0;
    int d = 0;
    double f = 1;
    int e = 0;
    int e_minus = 1;

    if (!JSON_PARSER_CONSUME(stream, ctx, '-')) {
        minus = -1;
    }

    char c = stream->peek(ctx);
    if ('0' == c) {
        stream->take(ctx);
    }
    else if ((c >= '1') && (c <= '9')) {
        a = a * 10 + (c - '0');
        stream->take(ctx);

        while (c = stream->peek(ctx), (c >= '0') && (c <= '9')) {
            a = a * 10 + (c - '0');
            stream->take(ctx);
        }
    }
    else {
        return JSON_PARSER_ERROR_VALUE_INVALID;
    }

    c = stream->peek(ctx);
    if ('.' == c) {
        stream->take(ctx);

        while (c = stream->peek(ctx), (c >= '0') && (c <= '9')) {
            d = d * 10 + (c - '0');
            f *= 0.1;
            stream->take(ctx);
        }
    }

    c = stream->peek(ctx);
    if (('e' == c) || ('E' == c)) {
        stream->take(ctx);

        e_minus = JSON_PARSER_CONSUME(stream, ctx, '-');

        while (c = stream->peek(ctx), (c >= '0') && (c <= '9')) {
            e = e * 10 + (e - '0');
            stream->take(ctx);
        }
    }

    if (d) {
        f *= d;
        f += a;

        if (handler->on_double(ctx, minus * f)) {
            return JSON_PARSER_ERROR_TERMINATION;
        }
    }
    else {
        if (handler->on_int(ctx, minus * a)) {
            return JSON_PARSER_ERROR_TERMINATION;
        }
    }

    return JSON_PARSER_ERROR_OK;
}

int 
json_parse_array(struct json_parser_stream_t *stream, struct json_parser_handler_t *handler, void *ctx)
{
    if (JSON_PARSER_CONSUME(stream, ctx, '[')) {
        return JSON_PARSER_ERROR_VALUE_INVALID;
    }

    if (handler->on_start_array(ctx)) {
        return JSON_PARSER_ERROR_TERMINATION;
    }

    JSON_PARSER_SKIP_WS(stream, ctx);

    if (!JSON_PARSER_CONSUME(stream, ctx, ']')) {
        if (handler->on_end_array(ctx, 0)) {
            return JSON_PARSER_ERROR_TERMINATION;
        }

        return JSON_PARSER_ERROR_OK;
    }

    size_t count = 0;
    while (1) {

        if (json_parse_value(stream, handler, ctx)) {
            return JSON_PARSER_ERROR_VALUE_INVALID;
        }

        ++count;

        JSON_PARSER_SKIP_WS(stream, ctx);

        if (!JSON_PARSER_CONSUME(stream, ctx, ']')) {
            if (handler->on_end_array(ctx, count)) {
                return JSON_PARSER_ERROR_TERMINATION;
            }

            return JSON_PARSER_ERROR_OK;
        }

        if (JSON_PARSER_CONSUME(stream, ctx, ',')) {
            return JSON_PARSER_ERROR_ARRAY_MISS_COMMA_OR_SQUARE_BRACKET;
        }

        JSON_PARSER_SKIP_WS(stream, ctx);
    }

    return JSON_PARSER_ERROR_OK;
}

int
json_parse_value(struct json_parser_stream_t *stream, struct json_parser_handler_t *handler, void *ctx)
{
    json_parse_fn_t fn = json_parse_test(stream, ctx);
    return fn ? fn(stream, handler, ctx) : JSON_PARSER_ERROR_VALUE_INVALID;
}

int
json_parse(struct json_parser_stream_t *stream, struct json_parser_handler_t *handler, void *ctx)
{
    JSON_PARSER_SKIP_WS(stream, ctx);
    return json_parse_value(stream, handler, ctx);
}

