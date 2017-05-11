#ifndef _JSON_H_INCLUDED
#define _JSON_H_INCLUDED

#include <stddef.h>


enum json_value_type_t {
    JSON_VALUE_TYPE_NONE,
    JSON_VALUE_TYPE_NULL,
    JSON_VALUE_TYPE_BOOL,
    JSON_VALUE_TYPE_INT,
    JSON_VALUE_TYPE_DOUBLE,
    JSON_VALUE_TYPE_STRING,
    JSON_VALUE_TYPE_OBJECT,
    JSON_VALUE_TYPE_ARRAY,
};

struct json_string_t {
    char *data;
    size_t len;
};

typedef double json_number_t;

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
        char b;
        int i;
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


struct json_stream_vtbl_t {
    char(*peek)(void *ctx);
    char(*take)(void *ctx);
    size_t(*tell)(void *ctx);
    char*(*put_begin)(void *ctx);
    void(*put)(void *ctx, char c);
    void(*flush)(void *ctx);
    size_t(*put_end)(void *ctx, char *begin);
};


struct json_stream_t {
    struct json_stream_vtbl_t *vtbl;
    void *ctx;
};


struct json_allocator_vtbl_t {
    void *(*on_alloc)(void *ctx, size_t size);
    void(*on_free)(void *ctx, void *p);
};


struct json_allocator_t {
    struct json_allocator_vtbl_t *vtbl;
    void *ctx;
};


void json_value_free(struct json_allocator_t *a, struct json_value_t *v, int dont_free);

struct json_value_t *json_value_add(struct json_allocator_t *a, struct json_value_t *v, enum json_value_type_t type);

struct json_object_elt_t *json_value_add_key(struct json_allocator_t *a, struct json_value_t *v, char *str, size_t len);


#endif //_JSON_H_INCLUDED
