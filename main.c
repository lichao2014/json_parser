#include <assert.h>
#include "json_parser.h"

#define TEST_JSON_STR       "   {\"x\" : { \"y\" : \"123\", \"z\" : { \"w\" : [\"1\", \"2\"] } } }"

int
main()
{
    struct json_parser_str_stream_t stream;
    struct json_parser_t parser = { 0 };

    while (1) {
        json_parser_prepare_str_stream(&parser, &stream, TEST_JSON_STR, sizeof(TEST_JSON_STR) - 1);
        int ret = json_parse(&parser);
        assert(0 == ret);
        json_free(&parser);
    }


    return 0;
}



