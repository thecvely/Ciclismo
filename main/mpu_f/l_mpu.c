#include "driver/gptimer.h"
#include "driver/i2c.h"


#define I2C_TIMEOUT         1000        /* Tieempo en ms de timeout*/
#define MPU_DIR_1           0x68        /* Dirección del esclavo */
#define MPU_DIR_2           0x69        /* Dirección del esclavo */
#define MPU_PWR_REG         0x6B        /* Dirección de administración del sensor */
#define I2C_FREQ_HZ         400000      /*Frecuencia de reloj */

static const char *TAG_TIMER="TIMER";
static const char *TAG_I2C = "I2C";
float DT=4000.000000;                          /* Tiempo de muestreo en us, fs(1KHz MPU6050)*/ 
const float gyroScale   = (250.00000/32768)*0.002; //0.002 Derivada del tiempo  
float offset_accel[2][3]={{-0.0002029201, 0.0051225499, 0.0041498020}, {-0.0002029201, 0.0051225499, 0.0041498020}};// Dos sensores 3 ejes
static float angxy=0.000000;

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

/*Configuración de I2C master */
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
    i2c_param_config(CONFIG_I2C_PORT, &conf);
    return i2c_driver_install(CONFIG_I2C_PORT, conf.mode, 0, 0, 0);
}

/*Lectura de Registro en MPU */
static esp_err_t mpu_register_read(uint8_t mpu_dir, uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(CONFIG_I2C_PORT, mpu_dir, &reg_addr, 1, data, len, I2C_TIMEOUT / portTICK_PERIOD_MS);
}

/*Escritura de Registro en MPU */
static esp_err_t mpu_register_write(uint8_t mpu_dir, uint8_t reg_addr, uint8_t data)
{
    int ret;
    uint8_t write_buf[2] = {reg_addr, data};
    ret = i2c_master_write_to_device(CONFIG_I2C_PORT, mpu_dir, write_buf, sizeof(write_buf), I2C_TIMEOUT / portTICK_PERIOD_MS);
    return ret;
}

/*Tarea de cálculo de ángulos*/
void vTaskTimer( void * pvParameters )
{

ESP_LOGI(TAG_TIMER, "Configuración");
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
    ESP_LOGE(TAG_TIMER, "No se puede crear queue");
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

uint8_t buffer_g[2][6]; // 2 Sensores 6 buffer por sensor
uint8_t buffer_a[2][6]; // 2 Sensores 6 buffer por sensor
int16_t giro_int[2][3]; // 2 Sensores 3 ejes por sensor
int16_t acel_int[2][3]; // 2 Sensores 3 ejes por sensor
float giro_pos[2][3];   // 2 Sensores 3 e por sensor
float acel_pos[2][3];   // 2 Sensores 3 e por sensor

for (int i = 0; i<3; i++)
for (int j = 0; j<2; j++){
    giro_int[j][i]=0;
    acel_int[j][i]=0;
    giro_pos[j][i]=0.000000;
    //acel_pos[j][i]=0.000000; //Inicializar en caso de problemas
}

float cosxy=0.000000;


ESP_ERROR_CHECK(i2c_master_init());
ESP_LOGI(TAG_I2C, "I2C Inicializado");
ESP_ERROR_CHECK(mpu_register_write(MPU_DIR_1, MPU_PWR_REG, 0));
ESP_ERROR_CHECK(mpu_register_write(MPU_DIR_2, MPU_PWR_REG, 0));
vTaskDelay(100 / portTICK_PERIOD_MS);


  for(; ;)
  {
    if (xQueueReceive(queue, &timer_data, pdMS_TO_TICKS(1000))) {
        
        /*Lectura de datos del giroscopio*/
        ESP_ERROR_CHECK(mpu_register_read(MPU_DIR_1, 0x43, buffer_g[0], 6));
        ESP_ERROR_CHECK(mpu_register_read(MPU_DIR_2, 0x43, buffer_g[1], 6));

        /* Lectura de datos del acelerómetro */
        ESP_ERROR_CHECK(mpu_register_read(MPU_DIR_1, 0x3b, buffer_a[0], 6));
        ESP_ERROR_CHECK(mpu_register_read(MPU_DIR_2, 0x3b, buffer_a[1], 6));

        for (int f = 0; f<2; f++)
        for (int i=0, j=0; i<3;i++){
        acel_int[f][i]=buffer_a[f][j]<<8 | buffer_a[f][j+1];
        giro_int[f][i]=buffer_g[f][j]<<8 | buffer_g[f][j+1];
        j+=2;
        }
        
        for (int f = 0; f<2; f++)
        for (int i=0; i<3; i++){
            giro_pos[f][i]+=giro_int[f][i]*gyroScale-offset_accel[f][i];
            
        }

        cosxy=(acel_int[0][0]*acel_int[1][0] + acel_int[0][1]*acel_int[1][1])/(sqrt(pow(acel_int[0][0],2) + pow(acel_int[0][1],2))*sqrt(pow(acel_int[1][0],2) + pow(acel_int[1][1],2)));
        angxy=acos(cosxy);

        if(cosxy<0){
            angxy=angxy-180;
        }
        
    } else {
        ESP_LOGW(TAG_TIMER, "Missed one count event");
    }
  }

}


