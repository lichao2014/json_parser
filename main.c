#include <assert.h>
#include "json_parser.h"

#define TEST_JSON_STR       "[1, false, null, \"x\", { \"x\" : 2.343 }]"

int
main()
{
    struct json_parser_t parser = { 0 };

    json_parser_init(&parser, 10, NULL);

    while (1) {
        int ret = json_parse_str(&parser, TEST_JSON_STR, sizeof(TEST_JSON_STR) - 1);
        assert(0 == ret);
    }

    json_parser_clear(&parser);

    return 0;
}



