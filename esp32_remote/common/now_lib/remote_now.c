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

#include "remote_now.h"

uint8_t _mac_addr[6]; 
esp_now_peer_info_t peerInfo; 

static void esp32_now_handle_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
    char * pData = calloc(100, sizeof(char)); 
    for(int i = 0; i < data_len; i++)
    {   
        pData[i] = data[i]; 
    }
    printf("Remote Now Data Is: %s\n", pData); 
    free(pData); 
}

static void esp32_now_handle_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if(status == ESP_NOW_SEND_SUCCESS)
    {                                                            
        printf("Send OK!!!\n");
    }
    else if(status == ESP_NOW_SEND_FAIL)
    {
        printf("Send ERROR!!!\n");
    }
}

void remote_now_set_macadr(uint8_t * macadr)
{
    memcpy(_mac_addr, macadr, 6);
}
void remote_now_init(void)
{
    esp_now_init(); 
    esp_now_register_recv_cb(esp32_now_handle_recv_cb); 
    esp_now_register_send_cb(esp32_now_handle_send_cb); 

    memset(&peerInfo, 0, sizeof(esp_now_peer_info_t));
    
    peerInfo.channel = 11; 
    printf("Remote Chanel 11 wf\n");
    peerInfo.encrypt = false;
    memcpy(peerInfo.peer_addr, _mac_addr, 6);
    ESP_ERROR_CHECK( esp_now_add_peer(&peerInfo));
}

void remote_now_send_data(char * data)
{
    printf("Data send is: %s\n", data);
    int buffer_len = strlen(data); 
    if (esp_now_send(_mac_addr, (uint8_t *)data, buffer_len) != ESP_OK) {
        printf("Send error!!!!!!!!!!\n");
    }
}