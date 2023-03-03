#ifndef __NOW_IO_LIB_H__ 
#define __NOW_IO_LIB_H__
#include <stdio.h>

typedef void (* box_now_cb_t)(char * data, int len); 

void box_now_init(void); 
void box_now_set_cb(void * cb); 

#endif