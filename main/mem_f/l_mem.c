#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"


void storage_write_sta(char *name, char *value){

    ESP_LOGI("Flash","Escribiendo datos name: %s valor: %s",name,value);
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE("Flash","Error (%s) de inicialización!\n", esp_err_to_name(err));
    } else {
        
        err = nvs_set_str(my_handle, name, value);
        printf((err != ESP_OK) ? "Error en set!\n" : "Configurado\n");
        err = nvs_commit(my_handle);
        printf((err != ESP_OK) ? "Error al guardar!\n" : "Guardado\n");
        nvs_close(my_handle);

}
}

 char* storage_read_sta(char* name){

     char *value="";
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE("Flash","Error (%s) de inicialización!\n", esp_err_to_name(err));
    } else {
                    
        size_t required_size;
        err = nvs_get_str(my_handle, name, NULL,&required_size);
        value=malloc(required_size);
        err =nvs_get_str(my_handle, name,value,&required_size);

        switch (err) {
            case ESP_OK:
                ESP_LOGI("Flash", "Lectura de datos: name=%s value=%s",name,value);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                ESP_LOGE("Flash","El valor no está iniciado!\n");
                    if(strcmp(name,"sta_ssid")==0){
                        value=CONFIG_STA_SSID;
                        storage_write_sta(name,CONFIG_STA_SSID);
                    }else if(strcmp(name,"sta_pass")==0){
                        value=CONFIG_STA_PASSWORD;
                        storage_write_sta(name,CONFIG_STA_PASSWORD);
                    }else if(strcmp(name,"ap_ssid")==0){
                        value=CONFIG_AP_SSID;
                        storage_write_sta(name,CONFIG_AP_SSID);
                    }else if (strcmp(name,"ap_pass")==0){
                        value=CONFIG_AP_PASSWORD;
                        storage_write_sta(name,CONFIG_AP_PASSWORD);
                    }
                    
                break;
            default :
                ESP_LOGE("Flash","Error (%s) en lectura!\n", esp_err_to_name(err));
        }
        nvs_close(my_handle);
}

return value;
}


void memory_reset(void){

gpio_set_direction(GPIO_NUM_14, GPIO_MODE_INPUT);
gpio_pullup_en(GPIO_NUM_14);


uint8_t count=0;
for (; ;)
{
    if(!gpio_get_level(GPIO_NUM_14)){
    ESP_LOGI("Reset", "Reset activado");
    count++;
    if(count==5){
        ESP_LOGE("Reset", "Reset ejecutado");
        storage_write_sta("sta_ssid",CONFIG_STA_SSID);
        storage_write_sta("sta_pass",CONFIG_STA_PASSWORD);
        storage_write_sta("ap_ssid",CONFIG_AP_SSID);
        storage_write_sta("ap_pass",CONFIG_AP_PASSWORD);
        esp_restart();
    }
    }else
    ESP_LOGI("Reset", "Reset desactivado");
    vTaskDelay(pdMS_TO_TICKS(1000));

}


}

void storage_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );    
}

