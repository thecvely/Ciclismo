#include <string.h>
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "socket_f/l_socket.h"



static const char *TAG_AP = "Wifi Access Point";
static const char *TAG_STA = "wifi station";
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

/*Código del Access Point*/
static void wifi_event_ap(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{


    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG_AP, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);

    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG_AP, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }


}

void wifi_init_ap(void)
{

    ESP_LOGI(TAG_AP, "Iniciando Access Point");
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

    ESP_LOGI(TAG_AP, "Configuración de AP finalizada. SSID:%s password:%s channel:%d",
             CONFIG_AP_SSID, CONFIG_AP_PASSWORD, CONFIG_AP_CHANNEL);
}



static void wifi_event_sta(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < CONFIG_STA_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG_STA, "Reconectando con AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, BIT1);
        }
        ESP_LOGI(TAG_STA,"No se puede conectar");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG_STA, "Estación conectada:" IPSTR, IP2STR(&event->ip_info.gw));
        
        static char ip_str[15]="";
        sprintf(ip_str,IPSTR, IP2STR(&event->ip_info.gw));
        //String ucParameterToPass;
        xTaskCreate(tcp_client_task, "tcp_client", 4096, &ip_str, 5, NULL);
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, BIT0);
    }

}


void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

   
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_sta,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_sta,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_STA_SSID,
            .password = CONFIG_STA_PASSWORD,
	     //.threshold.authmode = WIFI_AUTH_WPA2_WPA3_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG_STA, "Configuración de estación Wifi finalizada");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, BIT0 | BIT1, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & BIT0) {
        ESP_LOGI(TAG_STA, "Conectado a :%s", CONFIG_STA_SSID);
    } else if (bits & BIT1) {
        ESP_LOGI(TAG_STA, "Error al conectar a:%s", CONFIG_STA_SSID);
        wifi_init_ap();

    } else {
        ESP_LOGE(TAG_STA, "Evento inesperado");
    }

}


static void event_ap_sta(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{


}

void wifi_init_ap_sta(void)
{

}