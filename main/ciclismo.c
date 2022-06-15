#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gptimer.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "math.h"


#define I2C_PORT            0
#define I2C_TIMEOUT         1000        /* Tieempo en ms de timeout*/
#define MPU_DIR             0x68        /* Dirección del esclavo */
#define MPU_PWR_REG         0x6B        /* Dirección de administración del sensor */
#define I2C_FREQ_HZ         400000      /*Frecuencia de reloj */

static const char *TAG_TIMER="TIMER";
static const char *TAG_I2C = "I2C";
float DT=10000.000000;                          /* Tiempo de muestreo en us, fs(1KHz MPU6050)*/
const float accScale    = 2.00000*9.81/32768; 
const float gyroScale   = 250.00000/32768; 
const float A=0.98000, B=0.02000;

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





static float getPosition(float ang_prev, float ang_giro, float ang_acel){
float angulo=0;

angulo=A*(ang_prev- ang_giro)+B*ang_acel;

return A;
}


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
int16_t gi1[3]={0,0,0};
int16_t ai1[3]={0,0,0};
float gf1[3]={0.00,0.00,0.00};
float af1[3]={0.00,0.00,0.00};
float grados[6]={0.000000,0.000000,0.000000,0.000000,0.000000,0.000000};//Posisicón final 0-2(Aceleración | 3-5 Giroscopio)


ESP_ERROR_CHECK(i2c_master_init());
ESP_LOGI(TAG_I2C, "I2C Inicializado");
ESP_ERROR_CHECK(mpu_register_write(MPU_PWR_REG, 0));
vTaskDelay(100 / portTICK_PERIOD_MS);




uint64_t cr=0, ciclos=0, cout=0;
  for(; ;)
  {
    //ESP_LOGI(TAG_TIMER, "EJECUCIÓN DE TAREA");  // Task code goes here.

    if (xQueueReceive(queue, &timer_data, pdMS_TO_TICKS(1000))) {
        //ESP_LOGI(TAG_TIMER, "Timer reloaded, count=%llu", timer_data.count);
        //ESP_LOGI(TAG_TIMER, "Valor %llu", timer_data.value);

        /*Lectura de datos del giroscopio*/
        ESP_ERROR_CHECK(mpu_register_read(0x43, buffer_g1, 6));
        /* Lectura de datos del acelerómetro */
        ESP_ERROR_CHECK(mpu_register_read(0x3b, buffer_a1, 6));

        for (int i=0, j=0; i<3;i++){
        ai1[i]=buffer_a1[j]<<8 | buffer_a1[j+1];
        af1[i]=ai1[i]*accScale;
        gi1[i]=buffer_g1[j]<<8 | buffer_g1[j+1];
        gf1[i]=gi1[i]*gyroScale;
        j+=2;
        }

        grados[0]=atan(af1[0]/sqrt(pow(af1[1], 2.0)+pow(af1[2],2)))*180/3.1416;
        grados[1]=atan(af1[1]/sqrt(pow(af1[0], 2.0)+pow(af1[2],2)))*180/3.1416;
        grados[2]=atan(af1[2]/sqrt(pow(af1[1], 2.0)+pow(af1[0],2)))*180/3.1416;
        
        grados[3]+=(gf1[0]*DT)/1000000;
        grados[4]+=(gf1[1]*DT)/1000000;
        grados[5]+=(gf1[2]*DT)/1000000;
        
        //ESP_LOGI(TAG_I2C, "Datos= G [%.5f x] - [%.5f y] - [%.5f z] ** ciclos:%llu", grados[3], grados[4], grados[5],ciclos);
        
       if(ciclos==100){
            //ESP_LOGI(TAG_I2C, "Datos= A [%.5f x] - [%.5f y] - [%.5f z]", grados[0], grados[1], grados[2]);
            ESP_LOGI(TAG_I2C, "Datos= G [%.5f x] - [%.5f y] - [%.5f z] ***** ciclos:%llu | dt:%6.f |out:%llu", grados[3], grados[4], grados[5], ciclos, DT,cout);
            ciclos=0;
            cout++;
        }
        ciclos++;
        
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
xTaskCreate( vTaskTimer, "MPU_TIMER", 8196, &ucParameterToPass, tskIDLE_PRIORITY, &xHandle );
configASSERT( xHandle );

ESP_LOGW(TAG_TIMER, "FIN DE TAREA");


}