#ifndef MQTT_COMM_H
#define MQTT_COMM_H
#include <stdint.h>
#include <stddef.h>

void mqtt_setup(const char *client_id, const char *broker_ip, const char *user, const char *pass);
void mqtt_setup_psk(const char *client_id, const char *broker_ip,
                    const char *user, const char *pass,
                    const unsigned char *psk, size_t psk_len,
                    const char *psk_identity);
void mqtt_comm_publish(const char *topic, const uint8_t *data, size_t len);
void mqtt_comm_publish_retained(const char *topic, const uint8_t *data, size_t len);
#endif