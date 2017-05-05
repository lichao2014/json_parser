#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <assert.h>
#include "json_parser.h"


enum json_value_type_t {
    JSON_VALUE_TYPE_NONE,
    JSON_VALUE_TYPE_NULL,
    JSON_VALUE_TYPE_BOOL,
    JSON_VALUE_TYPE_NUMBER,
    JSON_VALUE_TYPE_STRING,
    JSON_VALUE_TYPE_OBJECT,
    JSON_VALUE_TYPE_ARRAY,
};


//obj
//  { str : val }[...]

// array
//  {val} [...]


// str 
// x

// number
// i

//bool 
// i


struct json_string_t {
    char *data;
    size_t len;
};


typedef double json_number_t;

typedef int json_bool_t;


struct json_array_elt_t;

struct json_array_t {
    struct json_array_elt_t *head;
    struct json_array_elt_t *tail;
    size_t size;
};

struct json_object_t {
    struct json_object_elt_t *head;
    struct json_object_elt_t *tail;
    size_t size;
};

struct json_value_t {
    enum json_value_type_t type;

    union {
        double d;
        struct json_string_t str;
        struct json_array_t arr;
        struct json_object_t obj;
    };

    struct json_value_t *parent;
};

struct json_array_elt_t {
    struct json_value_t data;
    struct json_array_elt_t *next;
};

struct json_object_elt_t {
    struct json_string_t key;
    struct json_value_t val;

    struct json_object_elt_t *next;
};

struct json_parser_ctx_t {
    char *src;
    char *dst;
    char *head;
    char *tail;

    struct json_value_t *root;
    struct json_value_t *current;
};


static void
json_parser_ctx_init(struct json_parser_ctx_t *ctx, char *str, size_t len) {
    ctx->src = ctx->head = str;
    ctx->tail = str + len;
    ctx->dst = NULL;

    ctx->root = NULL;
    ctx->current = NULL;
}

static char 
json_parser_ctx_peek(void *ctx)
{
    struct json_parser_ctx_t *pc = ctx;
    if (pc->src >= pc->tail) {
        return -1;
    }

    return *pc->src;
}

static char
json_parser_ctx_take(void *ctx)
{
    struct json_parser_ctx_t *pc = ctx;
    if (pc->src >= pc->tail) {
        return -1;
    }

    return *pc->src++;
}

static size_t
json_parser_ctx_tell(void *ctx)
{
    struct json_parser_ctx_t *pc = ctx;
    return pc->src - pc->head;
}

static void
json_parser_ctx_put(void *ctx, char c)
{
    struct json_parser_ctx_t *pc = ctx;
    *pc->dst++;
}

static char *
json_parser_ctx_put_begin(void *ctx)
{
    struct json_parser_ctx_t *pc = ctx;
    return pc->dst = pc->src;
}

static size_t
json_parser_ctx_put_end(void *ctx, char *begin)
{
    struct json_parser_ctx_t *pc = ctx;
    return pc->dst - begin;
}


static int
json_parser_ctx_on_null(void *ctx)
{
    printf("null ");
    return 0;
}


int
json_parser_ctx_on_bool(void *ctx, int b)
{
    printf(b ? "true" : "false");
    return 0;
}


static int
json_parser_ctx_on_int(void *ctx, int i)
{
    printf("%d", i);
    return 0;
}

static int
json_parser_ctx_on_uint(void *ctx, unsigned int i)
{
    printf("%u", i);
    return 0;
}

static int
json_parser_ctx_on_int64(void *ctx, int64_t i)
{
    printf("%" PRId64, i);
    return 0;
}


static int
json_parser_ctx_on_uint64(void *ctx, uint64_t i)
{
    printf("%" PRIu64, i);
    return 0;
}

static int 
json_parser_ctx_on_double(void *ctx, double d)
{
    printf("%f", d);
    return 0;
}


static int
json_parser_ctx_on_key(void *ctx, const char *str, size_t len)
{
    struct json_parser_ctx_t *pc = ctx;
    struct json_value_t *v = pc->current;
    struct json_object_elt_t *p;

    assert(JSON_VALUE_TYPE_OBJECT == v->type);

    p = malloc(sizeof(struct json_object_elt_t));
    p->key.data = (char *)str;
    p->key.len = len;
    p->val.type = JSON_VALUE_TYPE_NONE;
    p->next = NULL;

    if (v->obj.tail) {
        v->obj.tail->next = p;
        v->obj.tail = p;
    }
    else {
        v->obj.head = v->obj.tail = p;
    }

    v->obj.size++;

    return 0;
}

static int
json_parser_ctx_on_string(void *ctx, const char *str, size_t len)
{
    struct json_parser_ctx_t *pc = ctx;
    struct json_value_t *v = pc->current;

    switch (v->type) {
    case JSON_VALUE_TYPE_OBJECT:
        v->obj.tail->val.type = JSON_VALUE_TYPE_STRING;
        v->obj.tail->val.str.data = (char *)str;
        v->obj.tail->val.str.len = len;
        break;

    }


    return 0;
}

static int
json_parser_ctx_on_start_object(void *ctx)
{
    struct json_parser_ctx_t *pc = ctx;

    struct json_value_t *p = malloc(sizeof(struct json_value_t));

    p->type = JSON_VALUE_TYPE_OBJECT;
    p->obj.head = p->obj.tail = NULL;
    p->obj.size = 0;

    p->parent = pc->current;
    pc->current = p;

    if (!pc->root) {
        pc->root = pc->current;
    }

    return 0;
}


static int
json_parser_ctx_on_end_object(void *ctx, size_t count)
{
    struct json_parser_ctx_t *pc = ctx;
    struct json_value_t *v = pc->current->parent;

    switch (v->type) {
    case JSON_VALUE_TYPE_OBJECT:
        v->obj.tail->val.type = JSON_VALUE_TYPE_OBJECT;

        v->obj.tail->val.obj = pc->current->obj;
        v->obj.tail->val.str.len = len;

        break;

    }

    return 0;
}

static int
json_parser_ctx_on_start_array(void *ctx)
{
    struct json_parser_ctx_t *pc = ctx;

    struct json_value_t *v = malloc(sizeof(struct json_value_t));
    v->type = JSON_VALUE_TYPE_ARRAY;
    v->arr.size = 0;
    v->arr.data = NULL;

    v->parent = pc->current;
    pc->current = v;

    if (!pc->root) {
        pc->root = pc->current;
    }

    return 0;
}

static int
json_parser_ctx_on_end_array(void *ctx, size_t count)
{
    struct json_parser_ctx_t *pc = ctx;
    pc->current = pc->current->parent;



    return 0;
}


#define TEST_JSON_STR       "   {\"x\" : { \"y\" : \"123\" } }"

int
main()
{
    struct json_parser_ctx_t ctx;
    json_parser_ctx_init(&ctx, TEST_JSON_STR, sizeof(TEST_JSON_STR) - 1);


    struct json_parser_stream_t stream = { 0 };
    stream.peek = &json_parser_ctx_peek;
    stream.take = &json_parser_ctx_take;
    stream.tell = &json_parser_ctx_tell;
    stream.put = &json_parser_ctx_put;
    stream.put_begin = &json_parser_ctx_put_begin;
    stream.put_end = &json_parser_ctx_put_end;

    struct json_parser_handler_t handler = { 0 };
    handler.on_start_array = &json_parser_ctx_on_start_array;
    handler.on_end_array = &json_parser_ctx_on_end_array;
    handler.on_start_object = &json_parser_ctx_on_start_object;
    handler.on_end_object = &json_parser_ctx_on_end_object;
    handler.on_null = &json_parser_ctx_on_null;
    handler.on_string = &json_parser_ctx_on_string;
    handler.on_key = &json_parser_ctx_on_key;
    handler.on_int = &json_parser_ctx_on_int;
    handler.on_double = &json_parser_ctx_on_double;


    int ret = json_parse(&stream, &handler, &ctx);


    return 0;
}



