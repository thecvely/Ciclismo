#include <esp_https_server.h>
#include "esp_tls.h"
#include "sdkconfig.h"
#include "mem_f/l_mem.h"

static const char *TAG = "Webserver";

extern const uint8_t https_style_start[]   asm("_binary_pure_css_start");
extern const uint8_t https_style_end[]   asm("_binary_pure_css_end");
extern const uint8_t https_root_start[]   asm("_binary_root_html_start");
extern const uint8_t https_root_end[]   asm("_binary_root_html_end");
extern const uint8_t https_sta_start[] asm("_binary_sta_html_start");
extern const uint8_t https_sta_end[] asm("_binary_sta_html_end");
extern const uint8_t https_ap_start[] asm("_binary_ap_html_start");
extern const uint8_t https_ap_end[]   asm("_binary_ap_html_end");


/*
static esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, "<h1>Hello Secure World!</h1>", HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

*/


static esp_err_t root_get_handler(httpd_req_t *req)
{
    
printf("Cliente en root");
char* data_file=(char*)https_root_start;

int lnt=https_root_end - https_root_start;
printf("Longitud: %d",lnt);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, data_file, https_root_end - https_root_start);
    return ESP_OK;
}


static esp_err_t css_get_handler(httpd_req_t *req)
{
    
printf("LEYENDO DATOS DE ARCHIVO....***");
char* data_file=(char*)https_style_start;

//int lnt=sizeof(data_file);
//printf("Longitud: %d",lnt);
    
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, data_file, https_style_end-https_style_start);
    return ESP_OK;
}

static esp_err_t sta_post_handler(httpd_req_t *req){

    char * data_file = (char*)https_sta_start;
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, data_file,https_sta_end-https_sta_start);
    return ESP_OK;

}

static esp_err_t ap_post_handler(httpd_req_t *req){

    char * data_file = (char*)https_ap_start;
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, data_file,https_ap_end-https_ap_start);
    return ESP_OK;

}

static esp_err_t save_post_handler(httpd_req_t *req){
    char content[100];
    size_t recv_size=MIN(req->content_len, sizeof(content));
    int ret=httpd_req_recv(req,content,recv_size);
    if(ret <=0){
        if(ret==HTTPD_SOCK_ERR_TIMEOUT){
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    const char resp[]="URI POST Response";
    httpd_resp_send(req,resp,HTTPD_RESP_USE_STRLEN);

    ESP_LOGI(TAG,"Datos: %s",content);

    char ssid[50];
    char pass[50];
    char redt[50];
    
  char * pch;
  ESP_LOGI (TAG, "Splitting string \"%s\" into tokens:\n",content);
  pch = strtok (content," &=");
  uint8_t i=0;
  while (pch != NULL)
  {


    

    switch (i)
    {
    case 1:
        sprintf(ssid,pch,sizeof(pch));
        ESP_LOGI (TAG, "%s\n",ssid);
        break;
    case 3:
        sprintf(pass,pch,sizeof(pch));
        ESP_LOGI (TAG, "%s\n",pass);
        break;
    case 5:
        sprintf(redt,pch,sizeof(pch));
        ESP_LOGI (TAG, "%s\n",redt);
        break;
    
    default:
        break;
    }
    pch = strtok (NULL, " &=");
    i++;
  }

    ESP_LOGI(TAG, "\nTipo: %s",redt);

    if(redt[0]=='0'){
        storage_write_sta("sta_ssid",ssid);
        storage_write_sta("sta_ssid",pass);
    }else if(redt[0]=='1'){
        storage_write_sta("ap_ssid",ssid);
        storage_write_sta("ap_ssid",pass);
    }

    esp_restart();
    return ESP_OK;
    
}



static const httpd_uri_t url_root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_get_handler
};

static const httpd_uri_t url_css = {
    .uri       = "/style",
    .method    = HTTP_GET,
    .handler   = css_get_handler
};

static const httpd_uri_t url_sta={
    .uri       = "/sta",
    .method    = HTTP_POST,
    .handler   = sta_post_handler
};


static const httpd_uri_t url_ap={
    .uri       = "/ap",
    .method    = HTTP_POST,
    .handler   = ap_post_handler
};

static const httpd_uri_t url_save={
    .uri = "/save",
    .method= HTTP_POST,
    .handler   =save_post_handler
};


static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    ESP_LOGI(TAG, "Iniciando Webserver");

    httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();

    extern const unsigned char servercert_start[] asm("_binary_hcert_pem_start");
    extern const unsigned char servercert_end[]   asm("_binary_hcert_pem_end");
    conf.servercert = servercert_start;
    conf.servercert_len = servercert_end - servercert_start;

    extern const unsigned char prvtkey_pem_start[] asm("_binary_hkey_pem_start");
    extern const unsigned char prvtkey_pem_end[]   asm("_binary_hkey_pem_end");
    conf.prvtkey_pem = prvtkey_pem_start;
    conf.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

    
    
    
    esp_err_t ret = httpd_ssl_start(&server, &conf);
    if (ESP_OK != ret) {
        ESP_LOGI(TAG, "Error iniciando Webserver");
        return NULL;
    }

    // Set URI handlers
    ESP_LOGI(TAG, "Registering URI handlers");
    httpd_register_uri_handler(server, &url_root);
    httpd_register_uri_handler(server, &url_css);
    httpd_register_uri_handler(server, &url_sta);
    httpd_register_uri_handler(server, &url_ap);
    httpd_register_uri_handler(server, &url_save);
    return server;
}

static esp_err_t stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    return httpd_ssl_stop(server);
}

static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        if (stop_webserver(*server) == ESP_OK) {
            *server = NULL;
        } else {
            ESP_LOGE(TAG, "Failed to stop https server");
        }
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        *server = start_webserver();
    }
}


