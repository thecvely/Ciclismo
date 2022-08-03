#ifndef _L_HTTPS
#define _L_HTTPS

static void disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

#include  "l_https.c"
#endif
