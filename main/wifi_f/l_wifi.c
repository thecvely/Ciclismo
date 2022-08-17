#include <string.h>
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "socket_f/l_socket.h"
#include "mem_f/l_mem.h"



static const char *TAG_AP = "Access Point";
static const char *TAG_STA = "Estación";
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;


static void ip_event_ap(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){

    if(event_base == IP_EVENT && event_id==IP_EVENT_AP_STAIPASSIGNED){

        ip_event_ap_staipassigned_t* event = ( ip_event_ap_staipassigned_t*) event_data;
        ESP_LOGI(TAG_STA, "Estación conectada:" IPSTR, IP2STR(&event->ip));

        static char ip_str[15]="";
        sprintf(ip_str,IPSTR, IP2STR(&event->ip));
        xTaskCreate(tcp_client_task, "tcp_client", 4096, &ip_str, 5, NULL);
    
    }

}

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
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_ap,NULL,NULL));
    
    char *ssid=storage_read_sta("ap_ssid");
    char *pass=storage_read_sta("ap_pass");

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "",
            .ssid_len = 0,
            .channel = CONFIG_AP_CHANNEL,
            .password = CONFIG_AP_PASSWORD,
            .max_connection = CONFIG_AP_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .pmf_cfg = {
                    .required = false,
            },
        },
    };


    strlcpy((char *) wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid));
    strlcpy((char *) wifi_config.ap.password, pass, sizeof(wifi_config.ap.password));
    wifi_config.ap.ssid_len=strlen(ssid);

    if (strlen(CONFIG_AP_PASSWORD) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGW(TAG_AP, "Configuración de AP finalizada. SSID:%s password:%s channel:%d",
             wifi_config.ap.ssid, wifi_config.ap.password, wifi_config.ap.channel);
}



static void wifi_event_sta(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < CONFIG_STA_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGW(TAG_STA, "Reconectando con AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, BIT1);
            ESP_LOGE(TAG_STA, "Error al conectar a:%s", CONFIG_STA_SSID);
            wifi_init_ap();
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG_STA, "Estación conectada:" IPSTR, IP2STR(&event->ip_info.gw));
        
        static char ip_str[15]="";
        sprintf(ip_str,IPSTR, IP2STR(&event->ip_info.gw));
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


    char *ssid=storage_read_sta("sta_ssid");
        ESP_LOGI("SSID:","%s",ssid);
    char *pass=storage_read_sta("sta_pass");
        ESP_LOGI("PASS:","%s",pass);
    
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
	     //.threshold.authmode = WIFI_AUTH_WPA2_WPA3_PSK,
        },
    };

    strlcpy((char *) wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strlcpy((char *) wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));


    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG_STA, "Configuración finalizada");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, BIT0 | BIT1, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & BIT0) {
        ESP_LOGI(TAG_STA, "Conectado a :%s con password%s", wifi_config.sta.ssid, wifi_config.sta.password);
    } else if (bits & BIT1) {
        ESP_LOGW(TAG_STA, "Error al conectar a:%s con password%s", wifi_config.sta.ssid, wifi_config.sta.password);

    } else {
        ESP_LOGE(TAG_STA, "Evento inesperado");
    }

}
