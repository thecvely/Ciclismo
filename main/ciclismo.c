#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2c.h"
#include "nvs_flash.h"
#include "math.h"
#include "wifi_f/l_wifi.h"
#include "mpu_f/l_mpu.h"

void app_main(void)
{
ESP_LOGI(TAG_TIMER, "CICLISMO");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    //wifi_init_ap();
    wifi_init_sta();

static uint8_t ucParameterToPass;
TaskHandle_t xHandle = NULL;

xTaskCreate( vTaskTimer, "MPU_TIMER", 8196, &ucParameterToPass, tskIDLE_PRIORITY, &xHandle );
configASSERT( xHandle );

ESP_LOGW(TAG_TIMER, "FIN DE TAREA");


}