#include <string.h>
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"


static const char *TAG_AP = "Wifi Access Point";

/*Código del Access Point*/
static void wifi_event_ap(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{


    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG_AP, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG_AP, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }


}

void wifi_init_ap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_ap, NULL, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = CONFIG_AP_SSID,
            .ssid_len = strlen(CONFIG_AP_SSID),
            .channel = CONFIG_AP_CHANNEL,
            .password = CONFIG_AP_PASSWORD,
            .max_connection = CONFIG_AP_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .pmf_cfg = {
                    .required = false,
            },
        },
    };

    if (strlen(CONFIG_AP_PASSWORD) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_AP, "COnfiguración de AP finalizada. SSID:%s password:%s channel:%d",
             CONFIG_AP_SSID, CONFIG_AP_PASSWORD, CONFIG_AP_CHANNEL);
}


/*Código de la Estación Wifi*/
static void event_sta(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{

}


void wifi_init_sta(void)
{

}

/*Código de la Función Dual AP+Estación*/
static void event_ap_sta(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{


}

void wifi_init_ap_sta(void)
{

}