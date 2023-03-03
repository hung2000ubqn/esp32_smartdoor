#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_now.h" 

#include "box_now.h"

static box_now_cb_t _box_now_cb = NULL;

static void _box_now_handle_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    char mData[255] = {0}; 
    for(int i = 0; i < len; i++)
	{
		mData[i] = data[i]; 
	}
    _box_now_cb(mData, len); 
}

void box_now_init(void)
{
    esp_now_init();
    esp_now_register_recv_cb(_box_now_handle_cb); 
}

void box_now_set_cb(void * cb)
{
    _box_now_cb = cb; 
}