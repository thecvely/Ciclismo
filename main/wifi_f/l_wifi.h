#ifndef _L_WIFI
#define _L_WIFI
//static void event_ap(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

void wifi_init_ap(void);

//static void event_sta(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

void wifi_init_sta(void);

//static void event_ap_sta(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void wifi_init_ap_sta(void);


#include  "l_wifi.c"
#endif
