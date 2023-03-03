#include "box_json_parse.h"

#include <string.h>
#include <json_parser.h>

jparse_ctx_t jctx;
box_json_t box_json_parse_start(char * js_data)
{
    int ret = json_parse_start(&jctx, js_data, strlen(js_data)); 
    if(ret != OS_SUCCESS)
    {
        return BOX_JSON_P_FAIL; 
    }
    return BOX_JSON_P_PASS; 
}

box_json_t box_json_get_int_val(char * key, int * value)
{
    int _value = 0;
    int ret = json_obj_get_int(&jctx, key, &_value); 

    if(ret != OS_SUCCESS)
    {
        return BOX_JSON_P_FAIL; 
    }
    *value = _value; 
    return BOX_JSON_P_PASS;
}

void box_json_parse_end()
{
    json_parse_end(&jctx); 
}