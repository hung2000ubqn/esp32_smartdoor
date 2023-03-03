
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"

#include "remote_now.h"
#include "remote_wifi.h"

#define FLAG0 BIT1

//EC:62:60:1C:59:A4
static uint8_t mac_addr[] =  {0xEC, 0x62, 0x60, 0x1C, 0x59, 0xA4}; 

#define UP_BUTTON GPIO_NUM_18
#define DOWN_BUTTON GPIO_NUM_19
#define STOP_BUTTON GPIO_NUM_21
#define PW_BUTTON GPIO_NUM_20

EventGroupHandle_t xCreatedEventGroup;
TaskHandle_t myTaskHandle; 

void myNowTask(void * arg);
typedef enum{
    LO_TO_HI = 1,
    HI_TO_LO = 2,
    ANY_EDLE = 3
}   interrupt_type_edle_t;

static const char * TAG = "MAIN_LOG: "; 
static void button_init(); 
void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    remote_wifi_init();
    remote_now_set_macadr(mac_addr);  
    remote_now_init();

    button_init();
    xCreatedEventGroup = xEventGroupCreate();
    xTaskCreate(myNowTask, "now task", 10000, NULL, 10, myTaskHandle); 

}
int gpio_num = 0; 
void myNowTask(void * arg)
{
    while (1)
    {
        EventBits_t uxBits = xEventGroupWaitBits(
            xCreatedEventGroup,   /* The event group being tested. */
            FLAG0, /* The bits within the event group to wait for. */
            pdTRUE,        /* BIT_0 & BIT_4 should be cleared before returning. */
            pdFALSE,       /* Don't wait for both bits, either bit will do. */
            0xFFFFFFFF );/* Wait a maximum of 100ms for either bit to be set. */

        if( ( uxBits & FLAG0 ) == ( FLAG0 ) )
        {
            printf("Button\n"); 
            int id_remote = 942134; 
            int control_data = 0; 

            if(gpio_num == UP_BUTTON)
            {   
                control_data = 2; 
            }
            else if(gpio_num == DOWN_BUTTON)
            {
                control_data = 3; 
            }
            else if(gpio_num == STOP_BUTTON)
            {
                control_data = 4; 
            }
            char now_buffer[255] = {0}; 
            sprintf(now_buffer, "{\"control_data\":%d,\"id_remote\":%d}", control_data, id_remote); 
            ets_printf("Now send: %s\n", now_buffer); 
            remote_now_send_data(now_buffer); 
        }
    }
    
}
/*
int id_remote = 942134; 
    int control_data = 0; 

    if(gpio_num == UP_BUTTON)
    {   
        control_data = 2; 
    }
    else if(gpio_num == DOWN_BUTTON)
    {
        control_data = 3; 
    }
    else if(gpio_num == STOP_BUTTON)
    {
        control_data = 4; 
    }
    char now_buffer[255] = {0}; 
    sprintf(now_buffer, "{\"control_data\":%d,\"id_remote\":%d}", control_data, id_remote); 
    ets_printf("Now send: %s\n", now_buffer); 
    remote_now_send_data(now_buffer); 
}*/
uint32_t lTime = 0; 
static void IRAM_ATTR gpio_input_handler(void* arg)  {
    gpio_num = (uint32_t) arg;
    ets_printf("PIN: %d\n", gpio_num); 
    uint32_t cTime = xTaskGetTickCountFromISR();  
    uint32_t tickms = cTime - lTime; 
    if(tickms * portTICK_RATE_MS > 300)
    {
        BaseType_t xHigherPriorityTaskWoken;
        xEventGroupSetBitsFromISR(xCreatedEventGroup, FLAG0, &xHigherPriorityTaskWoken);
    }
    lTime = cTime;
}
static void button_init()
{
    gpio_pad_select_gpio(UP_BUTTON);
    gpio_set_direction(UP_BUTTON, GPIO_MODE_INPUT);
    gpio_set_pull_mode(UP_BUTTON, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(UP_BUTTON, HI_TO_LO);

    gpio_pad_select_gpio(DOWN_BUTTON);
    gpio_set_direction(DOWN_BUTTON, GPIO_MODE_INPUT);
    gpio_set_pull_mode(DOWN_BUTTON, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(DOWN_BUTTON, HI_TO_LO);

    gpio_pad_select_gpio(STOP_BUTTON);
    gpio_set_direction(STOP_BUTTON, GPIO_MODE_INPUT);
    gpio_set_pull_mode(STOP_BUTTON, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(STOP_BUTTON, HI_TO_LO);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(DOWN_BUTTON, gpio_input_handler, (void*) DOWN_BUTTON);  
    gpio_isr_handler_add(UP_BUTTON, gpio_input_handler, (void*) UP_BUTTON);     
    gpio_isr_handler_add(STOP_BUTTON, gpio_input_handler, (void*) STOP_BUTTON);     
}


