
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "box_wifi.h"
#include "box_now.h"
#include "mqtt_lib.h"
#include "box_json_parse.h"


/*0 là off, 1 là on, 2 là up, 3 là down, 4 là stop*/
typedef enum 
{
    DOOR_OFF = 0, 
    DOOR_ON = 1, 
    DOOR_UP = 2,
    DOOR_DOWN = 3, 
    DOOR_STOP = 4,
}door_state_t; 

#define TAG "MAIN_TAG"
#define PUBLISH_TAG "hung_boxremote"

#define BOX_ID 1234
#define REMOTE_ID 9421

#define vDelay(x) vTaskDelay(x/portTICK_RATE_MS)

static void json_generate(char * json_string, int control_data, int fire_s, int id_remote);
static void box_now_cb_handle(char * data, int len); 
static void box_mqtt_cb_handle(char *data, int len); 
void door_update_status(void *arg); 

static void door_io_init(void); 
static void door_open(void); 
static void door_close(void); 
static void door_stop(void); 
static door_state_t door_get_state(void); 
static void door_handle(door_state_t door_cmd); 

TaskHandle_t door_update_handle; 
void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_LOGI(TAG, "WIFI BOX STA MQTT NEW");
    door_io_init(); 
    box_wifi_init("TOTOLINK_A810R", "hung2000ubqn"); 
    box_now_init(); 
    box_now_set_cb(box_now_cb_handle);

    mqtt_set_callback(box_mqtt_cb_handle); 
    mqtt_app_init(); 
	mqtt_app_start(); 

    //vTaskDelay(10000 / portTICK_RATE_MS);
    xTaskCreate(door_update_status, "door", 10000, NULL, 10, &door_update_handle); 
}

static void box_now_cb_handle(char * data, int len)
{
    char mdata[255] = {0}; 
	for(int i = 0; i < len; i++)
	{
		mdata[i] = data[i]; 
	}
	printf("data now: %s\n", mdata); 

    if(box_json_parse_start(mdata) != BOX_JSON_P_PASS)
    {
        printf("Parse Fail\n");
        box_json_parse_end(); 
        return; 
    }
    int id_remote, control_data = 0; 

    if(box_json_get_int_val("id_remote", &id_remote) == BOX_JSON_P_PASS)
    {
        printf("id_remote is: %d\n", id_remote); 
    }

    if(box_json_get_int_val("control_data", &control_data) == BOX_JSON_P_PASS)
    {
        printf("control_data is: %d\n", control_data); 
    }
    box_json_parse_end(); 

    if(id_remote == 942134)
    {
        door_handle(control_data); 
    }
}

static void box_mqtt_cb_handle(char *data, int len)
{
    char mdata[255] = {0}; 
	for(int i = 0; i < len; i++)
	{
		mdata[i] = data[i]; 
	}
	printf("data mqtt: %s\n", mdata); 

    if(box_json_parse_start(mdata) != BOX_JSON_P_PASS)
    {
        printf("Parse Fail\n");
        box_json_parse_end(); 
        return; 
    }
    int id, id_remote, control_data = 0; 
    
    if(box_json_get_int_val("id", &id) == BOX_JSON_P_PASS)
    {
        printf("ID is: %d\n", id); 
    }

    if(box_json_get_int_val("id_remote", &id_remote) == BOX_JSON_P_PASS)
    {
        printf("id_remote is: %d\n", id_remote); 
    }

    if(box_json_get_int_val("control_data", &control_data) == BOX_JSON_P_PASS)
    {
        printf("control_data is: %d\n", control_data); 
    }
    box_json_parse_end(); 

    if(id_remote == 942134 && id == 123456) //942134" "id" : 123456,
    {
        door_handle(control_data); 
    }
}


void door_update_status(void *arg)
{
    printf("Door Init\n"); 
    while (1)
    {     
        int _door_state = door_get_state(); 
        char _mqtt_buffer[255] = {0};

        json_generate(_mqtt_buffer, door_get_state(), 0, 942134); 
        printf("mqtt push %s\n", _mqtt_buffer); 
        mqtt_pub(PUBLISH_TAG, _mqtt_buffer, strlen(_mqtt_buffer));
        vTaskDelay(5000 / portTICK_RATE_MS); 
    }
}

#define RELAY_2 GPIO_NUM_21
#define RELAY_3 GPIO_NUM_19
#define RELAY_4 GPIO_NUM_18

#define SENSOR_HEADER GPIO_NUM_0
#define SENSOR_FOOTER GPIO_NUM_15

door_state_t door_status = DOOR_OFF; 

typedef enum{
    LO_TO_HI = 1,
    HI_TO_LO = 2,
    ANY_EDLE = 3
}   interrupt_type_edle_t;

static void IRAM_ATTR gpio_input_handler(void* arg)  {
    int gpio_num = (uint32_t) arg;
    door_stop(); 
}

static door_state_t door_get_state(void)
{
    return door_status; 
}

static void door_io_init(void)
{
    gpio_pad_select_gpio(RELAY_2); 
    gpio_set_direction(RELAY_2, GPIO_MODE_INPUT_OUTPUT); 

    gpio_pad_select_gpio(RELAY_3); 
    gpio_set_direction(RELAY_3, GPIO_MODE_INPUT_OUTPUT); 

    gpio_pad_select_gpio(RELAY_4); 
    gpio_set_direction(RELAY_4, GPIO_MODE_INPUT_OUTPUT); 

    door_stop(); 

    gpio_pad_select_gpio(SENSOR_HEADER);
    gpio_set_direction(SENSOR_HEADER, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SENSOR_HEADER, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(SENSOR_HEADER, HI_TO_LO);

    gpio_pad_select_gpio(SENSOR_FOOTER);
    gpio_set_direction(SENSOR_FOOTER, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SENSOR_FOOTER, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(SENSOR_FOOTER, HI_TO_LO);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(SENSOR_HEADER, gpio_input_handler, (void*) SENSOR_HEADER);  
    gpio_isr_handler_add(SENSOR_FOOTER, gpio_input_handler, (void*) SENSOR_FOOTER);  
}
/*
    4 dây: 2 dây chung, 1 dây lên, 1 dây xuống
    3 relay: relay 2, relay 3, relay 4
    relay 2 (thưởng mở) up: 
    relay 3 (thường mở) down:
    relay 4 (thường đóng) stop: 
    relay nguồn: ..
    Để up, relay 4 off relay 2 on.
    Để downn, relay 4 off relay 3 on.
    Để dừng, relay 4 on. 
*/
#define RELAY_ON 1
#define RELAY_OFF 0
/* Để up, relay 4 off relay 2 on */
static void door_open(void)
{
    door_status = DOOR_UP; 
    gpio_set_level(RELAY_2, RELAY_ON);
    //vDelay(1000);
    //gpio_set_level(RELAY_2, RELAY_OFF);
    gpio_set_level(RELAY_3, RELAY_OFF); 
    gpio_set_level(RELAY_4, RELAY_OFF);
}
static void door_close(void)
{
    door_status = DOOR_DOWN; 
    gpio_set_level(RELAY_2, RELAY_OFF);
    gpio_set_level(RELAY_3, RELAY_ON);
    //vDelay(1000);
    //gpio_set_level(RELAY_3, RELAY_OFF); 
    gpio_set_level(RELAY_4, RELAY_OFF);
}
static void door_stop(void)
{
    door_status = DOOR_STOP; 
    gpio_set_level(RELAY_2, RELAY_OFF);
    gpio_set_level(RELAY_3, RELAY_OFF); 
    gpio_set_level(RELAY_4, RELAY_ON);  
    //vDelay(1000);
    //gpio_set_level(RELAY_4, RELAY_OFF); 
}


static void door_handle(door_state_t door_cmd)
{
    door_stop(); 
    vDelay(1000); 
    switch (door_cmd)
    {
    case DOOR_OFF:
        break;
    case DOOR_ON:
        break;
    case DOOR_UP:
        printf("Open door!!!\n");
        door_open(); 
        break;
    case DOOR_DOWN:
        printf("Close door!!!\n");
        door_close(); 
        break;
    case DOOR_STOP:
        printf("Stop door!!!\n");
        door_stop(); 
        break;
    }
}

/*
json form topic 2
{
  "id" : 123456,
  "control_data": 1,
  "id_remote": 942134,
}
json form topic 1
{
  "control_data": 1,
  "fire_s": 0,
  "id_remote": 942134,
}

id: là mã của box để nhận dạng giữa app và box (kiểu String) để 6 kí tự 
id_remote: là mã của remote thêm vào khi send now giữa remote và box (kiểu String) để 6 kí tự
control_data: là lệnh điều khiển cửa, 0 là off, 1 là on, 2 là up, 3 là down, 4 là stop (kiểu int)
fire_s: có giá trị 0 or 1 cảnh báo cháy từ sensor( kiểu int)

box sẽ publish lên topic 1 và subcribe topic 2 (truyền dữ liệu lên topic 1, và nhận dữ liệu về từ topic 2)
Truyền lên thì lấy id_remote từ esp now bên remote, fire_s để default = 0 (nếu dư time e sẽ thêm cảm biến sửa sau)
control_data lấy từ remote hoặc từ app về.

box subcribe lấy json từ topic 2 về. So sánh id trùng vs id của box thì sẽ nhận control_data để xử lí
và id_remote nếu null thì thôi còn nếu khác null thì so sánh với với id_remote sẵn có nếu khác thì thay đổi 
(lưu ý thay đổi thì remote sẽ k điều khiển đc nữa) 

*/

/*
json form topic 1
{
  "control_data": 1,
  "fire_s": 0,
  "id_remote": 942134,
}
*/
static void json_generate(char * json_string, int control_data, int fire_s, int id_remote)
{
    char json_data[255] = {0}; 
    sprintf(json_data, "{\"control_data\":%d,\"fire_s\":%d,\"id_remote\":\"%d\"}", control_data, fire_s, id_remote); 
    for(int i = 0; i < strlen(json_data); i++)
    {
        json_string[i] = json_data[i]; 
    }
}