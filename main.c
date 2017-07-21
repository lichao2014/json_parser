#include <assert.h>
#include "json_parser.h"

#define TEST_JSON_STR       "\"\ta\\\"\\bc\/\""

int
main()
{
    struct json_parser_t parser = { 0 };

    json_parser_init(&parser, 10, NULL);

    while (1) {
        int ret = json_parse_str(&parser, TEST_JSON_STR, sizeof(TEST_JSON_STR) - 1);

        struct json_value_t *v = parser.root;
        assert(0 == ret && JSON_VALUE_TYPE_STRING == v->type);

        char a[64];
        size_t n =json_strcpy(a, &v->str, sizeof a);
        assert(n == v->str.len);
    }

    json_parser_clear(&parser);

    return 0;
}



