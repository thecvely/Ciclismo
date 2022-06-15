#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gptimer.h"
#include "esp_log.h"
#include "driver/i2c.h"


static const char *TAG= "TASK";
void vTaskCode( void * pvParameters )
{

ESP_LOGI(TAG, "EJECUCIÓN DE TAREA");
  for( ;; )
  {
    ESP_LOGI(TAG, "EJECUCIÓN DE TAREA");  // Task code goes here.
  }

}

typedef struct 
{
    uint64_t count;
    uint64_t value;
} timer_data_t;


static bool IRAM_ATTR eventMPUTimer(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx){
    BaseType_t high_task_on=pdFALSE;
    QueueHandle_t queue=(QueueHandle_t)user_ctx;
    timer_data_t timer_data={
        .count=edata->count_value,
        .value=edata->alarm_value,
    };
    xQueueSendFromISR(queue, &timer_data, &high_task_on);
    return(high_task_on==pdTRUE);

}

void timerSensor(void)
{

ESP_LOGI(TAG, "vOtherFunction");


//1.- Creación de timer
gptimer_handle_t mputimer;
gptimer_config_t timer_config={
    .clk_src=GPTIMER_CLK_SRC_DEFAULT,
    .direction=GPTIMER_COUNT_UP,
    .resolution_hz=1000000,
};
ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &mputimer));

//2.- Configurar alarma

gptimer_alarm_config_t alarm_config={
    .alarm_count=1000000,
    .reload_count=0,
    .flags.auto_reload_on_alarm=true,
};
ESP_ERROR_CHECK(gptimer_set_alarm_action(mputimer, &alarm_config));

//3.- Registro de callbacks 
QueueHandle_t queue=xQueueCreate(10, sizeof(timer_data_t));
if (!queue){
    ESP_LOGE(TAG, "Failed to create queue");
    return;
}
gptimer_event_callbacks_t cbs={
    .on_alarm=eventMPUTimer,
};
ESP_ERROR_CHECK(gptimer_register_event_callbacks(mputimer, &cbs, queue));

//4.- Habilitación de timer
ESP_ERROR_CHECK(gptimer_enable(mputimer));
ESP_LOGI(TAG, "Timer habilitado");

//5.- Iniciar el timer
ESP_ERROR_CHECK(gptimer_start(mputimer));



static uint8_t ucParameterToPass;
TaskHandle_t xHandle = NULL;

  xTaskCreate( vTaskCode, "NAME", 2048, &ucParameterToPass, tskIDLE_PRIORITY, &xHandle );
  configASSERT( xHandle );

ESP_LOGW(TAG, "FIN DE TAREA");
}

void app_main(void)
{
ESP_LOGI(TAG, "MUNDO");

timerSensor();

}