#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"


void storage_write_sta(char *name, char *value){

    printf("Escribiendo datos");
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Done\n");
        // Escribir
        printf("Actualizando datos");
        
        err = nvs_set_str(my_handle, name, value);
        printf((err != ESP_OK) ? "Error!\n" : "Hecho\n");

        printf("Guardando...");
        err = nvs_commit(my_handle);
        printf((err != ESP_OK) ? "Error!\n" : "Hecho\n");
        nvs_close(my_handle);

}
}

 char* storage_read_sta(char* name){

     char *value="";
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
                    
        size_t required_size;
        err = nvs_get_str(my_handle, name, NULL,&required_size);
        value=malloc(required_size);
        err =nvs_get_str(my_handle, name,value,&required_size);

        switch (err) {
            case ESP_OK:
                printf("Done\n");
                printf("Restart counter = %s\n", value);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                ESP_LOGE("MEM","El valor no está iniciado!\n");
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
                printf("Error B (%s) reading!\n", esp_err_to_name(err));
        }

        nvs_close(my_handle);


}

return value;
}



void storage_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    // Open
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGW("Flash","Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {

        // Read
        int32_t restart_counter = 0; // value will default to 0, if not set yet in NVS
        err = nvs_get_i32(my_handle, "restart_counter", &restart_counter);
        switch (err) {
            case ESP_OK:
                printf("Restart counter = %d\n", restart_counter);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("The value is not initialized yet!\n");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(err));
        }

        // Write
        printf("Updating restart counter in NVS ... ");
        restart_counter++;
        err = nvs_set_i32(my_handle, "restart_counter", restart_counter);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Commit written value.
        // After setting any values, nvs_commit() must be called to ensure changes are written
        // to flash storage. Implementations may write to storage at other times,
        // but this is not guaranteed.
        printf("Committing updates in NVS ... ");
        err = nvs_commit(my_handle);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Close
        nvs_close(my_handle);
    }
    
    
}

