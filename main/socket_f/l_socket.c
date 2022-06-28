#include <string.h>
#include <sys/param.h>
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"


static const char *TAG_SK = "Cliente Socket";
int sk_output= 1;

static void tcp_client_task(void *pvParameters)
{
    char rx_buffer[128];
    char host_ip[] = CONFIG_IPV4_SOCKET_SERVER;
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

            if(sk_output==0)
            size=2;
            else if(sk_output>0)
            size=(int)log10(sk_output)+2;
            else
            size=(int)log10(abs(sk_output))+3;
            
            char buffer[size];

            sprintf(buffer, "%d\n", sk_output);
            
            ESP_LOGI(TAG_SK,"Valor de buffer: %s | tamaño: %d | %d", buffer, sizeof(sk_output), size);
            
            
            
            err = send(sock, buffer, size, 0);
            if (err < 0) {
                ESP_LOGE(TAG_SK, "Error occurred during sending: errno %d", errno);
                break;
            }
            ESP_LOGI(TAG_SK, "Mensaje enviado, esperando respuesta");
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

            vTaskDelay(500 / portTICK_PERIOD_MS);
        }

        if (sock != -1) {
            ESP_LOGE(TAG_SK, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}