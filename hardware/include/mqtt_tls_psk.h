#ifndef MQTT_TLS_PSK_H
#define MQTT_TLS_PSK_H

#include "lwip/err.h"
#include <stddef.h>

struct altcp_tls_config;

err_t mqtt_tls_psk_setup(struct altcp_tls_config *conf,
                         const unsigned char *psk, size_t psk_len,
                         const char *psk_identity);

struct altcp_tls_config *mqtt_tls_create_config_client_psk(
    const unsigned char *psk, size_t psk_len,
    const char *psk_identity);

#endif
