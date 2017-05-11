#include "json.h"
#include <assert.h>


void
json_value_free(struct json_allocator_t *a, struct json_value_t *v, int dont_free)
{
    if (!v) {
        return;
    }

    if (JSON_VALUE_TYPE_OBJECT == v->type) {

        struct json_object_elt_t *elt = v->obj.head;
        struct json_object_elt_t *p;

        while (elt) {
            p = elt;
            elt = elt->next;

            json_value_free(a, &p->val, 1);
            a->vtbl->on_free(a->ctx, p);
        }
    }
    else if (JSON_VALUE_TYPE_ARRAY == v->type) {

        struct json_array_elt_t *elt = v->arr.head;
        struct json_array_elt_t *p;

        while (elt) {
            p = elt;
            elt = elt->next;

            json_value_free(a, &p->data, 1);
            a->vtbl->on_free(a->ctx, p);
        }
    }

    if (!dont_free) {
        a->vtbl->on_free(a->ctx, v);
    }
}


struct json_value_t *
json_value_add(struct json_allocator_t *a, struct json_value_t *v, enum json_value_type_t type)
{
    struct json_value_t *p = NULL;

    if (!v) {
        p = a->vtbl->on_alloc(a->ctx, sizeof(struct json_value_t));
    }
    else if (JSON_VALUE_TYPE_OBJECT == v->type) {
        assert(v->obj.tail);
        p = &v->obj.tail->val;
        ++v->obj.size;
    }
    else if (JSON_VALUE_TYPE_ARRAY == v->type) {
        struct json_array_elt_t *elt = a->vtbl->on_alloc(a->ctx, sizeof(struct json_array_elt_t));
        p = &elt->data;
        elt->next = NULL;

        if (v->arr.tail) {
            v->arr.tail->next = elt;
        }
        else {
            v->arr.head = elt;
        }

        v->arr.tail = elt;
        ++v->arr.size;
    }

    if (p) {
        p->type = type;
        p->parent = v;
    }

    return p;
}


struct json_object_elt_t *
json_value_add_key(struct json_allocator_t *a, struct json_value_t *v, char *str, size_t len)
{
    assert(JSON_VALUE_TYPE_OBJECT == v->type);

    struct json_object_elt_t *p = a->vtbl->on_alloc(a->ctx, sizeof(struct json_object_elt_t));
    p->key.data = str;
    p->key.len = len;
    p->val.type = JSON_VALUE_TYPE_NONE;
    p->val.parent = v;
    p->next = NULL;

    if (v->obj.tail) {
        v->obj.tail->next = p;
    }
    else {
        v->obj.head = p;
    }

    v->obj.tail = p;

    return p;
}
