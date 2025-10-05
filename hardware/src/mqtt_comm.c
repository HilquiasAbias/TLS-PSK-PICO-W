#include "lwip/apps/mqtt.h"
#include "lwip/altcp_tls.h"
#include "include/mqtt_comm.h"
#include "include/mqtt_tls_psk.h"
#include "lwipopts.h"
#include "lwip/dns.h"
#include <string.h>

static mqtt_client_t *client;
static struct altcp_tls_config *tls_config = NULL;

static struct
{
    const char *client_id;
    const char *user;
    const char *pass;
    const unsigned char *psk;
    size_t psk_len;
    const char *psk_identity;
    u16_t port;
} mqtt_config;

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
    if (status == MQTT_CONNECT_ACCEPTED)
    {
        printf("Conectado ao broker MQTT%s!\n", 
               mqtt_config.psk ? " com TLS-PSK" : "");
    }
    else
    {
        printf("Falha ao conectar ao broker, código: %d\n", status);
    }
}

static void dns_found_cb(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
    if (ipaddr == NULL)
    {
        printf("Erro na resolução DNS\n");
        return;
    }

    printf("DNS resolvido: %s -> %s\n", name, ipaddr_ntoa(ipaddr));

    client = mqtt_client_new();
    if (client == NULL)
    {
        printf("Falha ao criar o cliente MQTT\n");
        return;
    }

    struct mqtt_connect_client_info_t ci;
    memset(&ci, 0, sizeof(ci));
    ci.client_id = mqtt_config.client_id;
    ci.client_user = mqtt_config.user;
    ci.client_pass = mqtt_config.pass;
    ci.keep_alive = 60;

#if LWIP_ALTCP && LWIP_ALTCP_TLS
    if (mqtt_config.psk != NULL)
    {
        printf("Criando conexão MQTT com TLS-PSK...\n");
        
        tls_config = mqtt_tls_create_config_client_psk(
            mqtt_config.psk,
            mqtt_config.psk_len,
            mqtt_config.psk_identity
        );

        if (tls_config == NULL)
        {
            printf("Erro ao configurar TLS-PSK\n");
            mqtt_client_free(client);
            return;
        }

        ci.tls_config = tls_config;
        printf("Conectando MQTT com TLS-PSK na porta %d...\n", mqtt_config.port);
    }
    else
#endif
    {
        printf("Conectando MQTT sem TLS na porta %d...\n", mqtt_config.port);
    }

    err_t err = mqtt_client_connect(client, ipaddr, mqtt_config.port,
                                      mqtt_connection_cb, NULL, &ci);
    if (err != ERR_OK)
    {
        printf("Erro ao conectar MQTT: %d\n", err);
    }
}

void mqtt_setup_psk(const char *client_id, const char *broker_ip,
                    const char *user, const char *pass,
                    const unsigned char *psk, size_t psk_len,
                    const char *psk_identity)
{
    ip_addr_t broker_addr;

    mqtt_config.client_id = client_id;
    mqtt_config.user = user;
    mqtt_config.pass = pass;
    mqtt_config.psk = psk;
    mqtt_config.psk_len = psk_len;
    mqtt_config.psk_identity = psk_identity;
    mqtt_config.port = (psk != NULL) ? 8883 : 1883;

    printf("Configurando MQTT para %s:%d\n", broker_ip, mqtt_config.port);

    if (ip4addr_aton(broker_ip, &broker_addr))
    {
        dns_found_cb(broker_ip, &broker_addr, NULL);
    }
    else
    {
        printf("Resolvendo DNS: %s\n", broker_ip);
        err_t err = dns_gethostbyname(broker_ip, &broker_addr, dns_found_cb, NULL);
        if (err == ERR_OK)
        {
            dns_found_cb(broker_ip, &broker_addr, NULL);
        }
        else if (err != ERR_INPROGRESS)
        {
            printf("Erro ao iniciar resolução DNS: %d\n", err);
        }
    }
}

void mqtt_setup(const char *client_id, const char *broker_ip, const char *user, const char *pass)
{
    mqtt_setup_psk(client_id, broker_ip, user, pass, NULL, 0, NULL);
}

static void mqtt_pub_request_cb(void *arg, err_t result)
{
    if (result == ERR_OK)
    {
        printf("Publicação MQTT enviada com sucesso!\n");
    }
    else
    {
        printf("Erro ao publicar via MQTT: %d\n", result);
    }
}

void mqtt_comm_publish(const char *topic, const uint8_t *data, size_t len)
{
    if (client == NULL)
    {
        printf("Cliente MQTT não inicializado\n");
        return;
    }

    err_t status = mqtt_publish(
        client,
        topic,
        data,
        len,
        0,
        0,
        mqtt_pub_request_cb,
        NULL
    );

    if (status != ERR_OK)
    {
        printf("mqtt_publish falhou: %d\n", status);
    }
}

void mqtt_comm_publish_retained(const char *topic, const uint8_t *data, size_t len)
{
    if (client == NULL)
    {
        printf("Cliente MQTT não inicializado\n");
        return;
    }

    err_t status = mqtt_publish(
        client,
        topic,
        data,
        len,
        0,
        1,
        mqtt_pub_request_cb,
        NULL
    );

    if (status != ERR_OK)
    {
        printf("mqtt_publish_retained falhou: %d\n", status);
    }
}
