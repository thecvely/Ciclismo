#include <string.h>
#include <sys/param.h>
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"

#include "../mpu_f/l_mpu.h"


static const char *TAG_SK = "Cliente Socket";
float sk_output= 0.000000;

static void tcp_client_task(void *pvParameters)
{
    
    ESP_LOGI(TAG_SK, "Valor recibido %s", pvParameters);
    char rx_buffer[128];
    //char host_ip[] = CONFIG_IPV4_SOCKET_SERVER;
    char host_ip[15];
    sprintf(host_ip,"%s",pvParameters);
    int addr_family = 0;
    int ip_protocol = 0;

    while (1) {

        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(host_ip);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(CONFIG_PORT_SOCKET_SERVER);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;

        
        

        int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
            if (sock < 0) {
                ESP_LOGE(TAG_SK, "Unable to create socket: errno %d", errno);
                break;
            
            }
            ESP_LOGI(TAG_SK, "Socket created, connecting to %s:%d", host_ip, CONFIG_PORT_SOCKET_SERVER);

            int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
            if (err != 0) {
                ESP_LOGE(TAG_SK, "Socket unable to connect: errno %d", errno);
                break;
            }
            ESP_LOGI(TAG_SK, "Successfully connected");


        int size=0;
        while (1) {

            sk_output=angxy;
            if(sk_output==0)
            size=9;
            else if(sk_output>0)
            size=(int)log10(sk_output)+9;
            else
            size=(int)log10(abs(sk_output))+10;
            
            char buffer[size];

            sprintf(buffer, "%.6f\n", angxy);
            
            ESP_LOGI(TAG_SK,"Valor de buffer Socket %s", buffer);
            
            
            err = send(sock, buffer, size, 0);
            if (err < 0) {
                ESP_LOGE(TAG_SK, "Error occurred during sending: errno %d", errno);
                break;
            }
            //ESP_LOGI(TAG_SK, "Mensaje enviado, esperando respuesta");
            //shutdown(sock, 0);
            //close(sock);
            /*int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            // Error occurred during receiving
            if (len < 0) {
                ESP_LOGE(TAG_SK, "recv failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                ESP_LOGI(TAG_SK, "Received %d bytes from %s:", len, host_ip);
                ESP_LOGI(TAG_SK, "%s", rx_buffer);
            }*/
            sk_output++; 

            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }

        if (sock != -1) {
            ESP_LOGE(TAG_SK, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}