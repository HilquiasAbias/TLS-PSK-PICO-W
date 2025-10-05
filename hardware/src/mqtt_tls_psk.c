#include "include/mqtt_tls_psk.h"
#include "lwip/altcp_tls.h"
#include "lwip/apps/altcp_tls_mbedtls_opts.h"
#include "lwip/apps/mqtt.h"
#include "mbedtls/ssl.h"
#include "mbedtls/error.h"
#include <string.h>
#include <stdio.h>

static struct {
    const unsigned char *psk;
    size_t psk_len;
    const char *psk_identity;
} psk_config;

err_t mqtt_tls_psk_setup(struct altcp_tls_config *conf,
                         const unsigned char *psk, size_t psk_len,
                         const char *psk_identity)
{
    if (conf == NULL || psk == NULL || psk_identity == NULL) {
        return ERR_ARG;
    }

    psk_config.psk = psk;
    psk_config.psk_len = psk_len;
    psk_config.psk_identity = psk_identity;

    mbedtls_ssl_config *ssl_conf = (mbedtls_ssl_config *)conf;
    
    int ret = mbedtls_ssl_conf_psk(ssl_conf,
                                    psk,
                                    psk_len,
                                    (const unsigned char *)psk_identity,
                                    strlen(psk_identity));
    
    if (ret != 0) {
        char error_buf[100];
        mbedtls_strerror(ret, error_buf, sizeof(error_buf));
        printf("mbedtls_ssl_conf_psk falhou: -0x%04x - %s\n", -ret, error_buf);
        return ERR_VAL;
    }

    printf("PSK configurado: identity='%s', key_len=%d\n", psk_identity, psk_len);
    return ERR_OK;
}

struct altcp_tls_config *mqtt_tls_create_config_client_psk(
    const unsigned char *psk, size_t psk_len,
    const char *psk_identity)
{
    struct altcp_tls_config *conf = altcp_tls_create_config_client(NULL, 0);
    
    if (conf == NULL) {
        printf("Erro ao criar config TLS\n");
        return NULL;
    }

    err_t err = mqtt_tls_psk_setup(conf, psk, psk_len, psk_identity);
    if (err != ERR_OK) {
        altcp_tls_free_config(conf);
        return NULL;
    }

    return conf;
}
