#ifndef __NOW__LIB_H__ 
#define __NOW__LIB_H__
#include <stdio.h>

void remote_now_set_macadr(uint8_t * macadr);
void remote_now_init(void); 
void remote_now_send_data(char * data); 

#endif