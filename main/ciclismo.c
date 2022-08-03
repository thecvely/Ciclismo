#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2c.h"
#include "nvs_flash.h"
#include "math.h"
#include "wifi_f/l_wifi.h"
#include "analog_f/l_analog.h"

#include "https_f/l_https.h"



void app_main(void)
{
ESP_LOGI("MAIN", "CICLISMO");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    //wifi_init_ap();

    static httpd_handle_t server = NULL;
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_START, &connect_handler, &server));


    wifi_init_sta();
    

static uint8_t ucParameterToPass;
TaskHandle_t xHandle = NULL;

xTaskCreate( vtaskAnalog, "MPU_TIMER", 8196, &ucParameterToPass, tskIDLE_PRIORITY, &xHandle );
configASSERT( xHandle );

ESP_LOGW("MAIN", "FIN DE TAREA");


}