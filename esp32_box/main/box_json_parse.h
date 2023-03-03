#ifndef __BOX_JSON_PARSE__
#define __BOX_JSON_PARSE__
#include "stdio.h"

typedef enum
{
    BOX_JSON_P_PASS, 
    BOX_JSON_P_FAIL,
}box_json_t; 

box_json_t box_json_parse_start(char * js_data); 
box_json_t box_json_get_int_val(char * key, int * value); 
void box_json_parse_end(); 

#endif