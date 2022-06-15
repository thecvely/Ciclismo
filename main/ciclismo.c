#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gptimer.h"
#include "esp_log.h"
#include "driver/i2c.h"


#define I2C_PORT            0
#define I2C_TIMEOUT         1000        /* Tieempo en ms de timeout*/
#define MPU_DIR             0x68        /* Dirección del esclavo */
#define MPU_PWR_REG         0x6B        /* Dirección de administración del sensor */
#define DT                  2000000     /* Tiempo de muestreo en us, fs(1KHz MPU6050)*/
#define I2C_FREQ_HZ         100000      /*Frecuencia de reloj */

typedef struct 
{
    uint64_t count;
    uint64_t value;
} timer_data_t;

static const char *TAG_TIMER="TIMER";
static const char *TAG_I2C = "I2C";


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




//******************************FUNCIONES DE I2C******************
//****************************************************************
/*Inicialización de I2C*/
static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = CONFIG_I2C_SDA,
        .scl_io_num = CONFIG_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
    };
    i2c_param_config(I2C_PORT, &conf);
    return i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);
}


/*Lectura de Registro en MPU */
static esp_err_t mpu_register_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_PORT, MPU_DIR, &reg_addr, 1, data, len, I2C_TIMEOUT / portTICK_PERIOD_MS);
}

/*Escritura de Registro en MPU */
static esp_err_t mpu_register_write(uint8_t reg_addr, uint8_t data)
{
    int ret;
    uint8_t write_buf[2] = {reg_addr, data};
    ret = i2c_master_write_to_device(I2C_PORT, MPU_DIR, write_buf, sizeof(write_buf), I2C_TIMEOUT / portTICK_PERIOD_MS);
    return ret;
}

//******************************FIN FUNCIONES DE I2C******************
//********************************************************************

void vTaskTimer( void * pvParameters )
{

//*******************************************************************************
//********************************CONFIGURACIÓN DE TIMER*************************
ESP_LOGI(TAG_TIMER, "EJECUCIÓN DE TAREA");
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
    .alarm_count=DT,
    .reload_count=0,
    .flags.auto_reload_on_alarm=true,
};
ESP_ERROR_CHECK(gptimer_set_alarm_action(mputimer, &alarm_config));
//3.- Registro de callbacks 
QueueHandle_t queue=xQueueCreate(10, sizeof(timer_data_t));
if (!queue){
    ESP_LOGE(TAG_TIMER, "Failed to create queue");
    return;
}
gptimer_event_callbacks_t cbs={
    .on_alarm=eventMPUTimer,
};
ESP_ERROR_CHECK(gptimer_register_event_callbacks(mputimer, &cbs, queue));
//4.- Habilitación de timer
ESP_ERROR_CHECK(gptimer_enable(mputimer));
ESP_LOGI(TAG_TIMER, "Timer habilitado");
//5.- Iniciar el timer
ESP_ERROR_CHECK(gptimer_start(mputimer));
timer_data_t timer_data;



//*******************************************************************************
//********************************CONFIGURACIÓN DE I2C_MPU*************************

uint8_t buffer_g1[6];
uint8_t buffer_a1[6];
int16_t g1[3]={0,0,0};
int16_t a1[3]={0,0,0};

ESP_ERROR_CHECK(i2c_master_init());
ESP_LOGI(TAG_I2C, "I2C Inicializado");
ESP_ERROR_CHECK(mpu_register_write(MPU_PWR_REG, 0));
vTaskDelay(100 / portTICK_PERIOD_MS);



  for( ;; )
  {
    ESP_LOGI(TAG_TIMER, "EJECUCIÓN DE TAREA");  // Task code goes here.
    //vTaskDelay(500 / portTICK_PERIOD_MS);

    if (xQueueReceive(queue, &timer_data, pdMS_TO_TICKS(5000))) {
        ESP_LOGI(TAG_TIMER, "Timer reloaded, count=%llu", timer_data.count);
        ESP_LOGI(TAG_TIMER, "Valor %llu", timer_data.value);

        /*Lectura de datos del giroscopio*/
        ESP_ERROR_CHECK(mpu_register_read(0x43, buffer_g1, 6));
        /* Lectura de datos del acelerómetro */
        ESP_ERROR_CHECK(mpu_register_read(0x3b, buffer_a1, 6));


        

        for (int i=0, j=0; i<3;i++){
        a1[i]=buffer_a1[j]<<8 | buffer_a1[j+1];
        g1[i]=buffer_g1[j]<<8 | buffer_g1[j+1];
        j+=2;
        }

        ESP_LOGI(TAG_I2C, "Datos= A [%d x] - [%d y] - [%d z]", a1[0], a1[1], a1[2]);
        ESP_LOGI(TAG_I2C, "Datos= G [%d x] - [%d y] - [%d z]", g1[0], g1[1], g1[2]);







    } else {
        ESP_LOGW(TAG_TIMER, "Missed one count event");
    }



  }

}

void app_main(void)
{
ESP_LOGI(TAG_TIMER, "MUNDO");

static uint8_t ucParameterToPass;
TaskHandle_t xHandle = NULL;
xTaskCreate( vTaskTimer, "NAME", 2048, &ucParameterToPass, tskIDLE_PRIORITY, &xHandle );
configASSERT( xHandle );

ESP_LOGW(TAG_TIMER, "FIN DE TAREA");


}