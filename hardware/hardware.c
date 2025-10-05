#include <string.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"

#include "include/wifi_conn.h"
#include "include/mqtt_comm.h"

#define LED_GREEN_PIN 11
#define BUTTON_PIN 5

#define PUBLISH_INTERVAL_MS 5000
#define LED_DURATION_MS 2000

static uint32_t last_publish_time = 0;
static uint32_t led_start_time = 0;
static bool led_active = false;

void led_on()
{
    gpio_put(LED_GREEN_PIN, 1);
    led_active = true;
    led_start_time = to_ms_since_boot(get_absolute_time());
    printf("LED verde ligado\n");
}

void led_off()
{
    gpio_put(LED_GREEN_PIN, 0);
    led_active = false;
    printf("LED verde desligado\n");
}

void update_led()
{
    if (led_active)
    {
        if (to_ms_since_boot(get_absolute_time()) - led_start_time >= LED_DURATION_MS)
        {
            led_off();
        }
    }
}

void setup_gpio()
{
    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_put(LED_GREEN_PIN, 0);

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
}

void setup_adc()
{
    adc_init();
    adc_set_temp_sensor_enabled(true);
}

float read_temperature()
{
    adc_select_input(4);
    uint16_t raw = adc_read();
    const float conversion_factor = 3.3f / (1 << 12);
    float voltage = raw * conversion_factor;
    float temperature = 27.0f - (voltage - 0.706f) / 0.001721f;
    return temperature;
}

bool read_button()
{
    return !gpio_get(BUTTON_PIN);
}

void publish_sensor_data(float temperature, bool button_pressed)
{
    char json_buffer[512];
    char ip_str[16];

    ip4addr_ntoa_r(netif_ip4_addr(netif_default), ip_str, sizeof(ip_str));

    snprintf(json_buffer, sizeof(json_buffer),
             "{"
             "\"team\":\"aluno35\","
             "\"device\":\"bitdoglab_aluno35\","
             "\"ip\":\"%s\","
             "\"data\":{"
             "\"temperature\":%.2f,"
             "\"button\":%s"
             "}"
             "}",
             ip_str,
             temperature,
             button_pressed ? "true" : "false");

    mqtt_comm_publish("/aluno35/pub/bitdoglab/", (uint8_t *)json_buffer, strlen(json_buffer));
    printf("Dados publicados:\n%s\n", json_buffer);
}

int main()
{
    stdio_init_all();
    sleep_ms(5000);

    setup_gpio();
    setup_adc();

    printf("Conectando ao WiFi...\n");
    connect_to_wifi("Fonseca 2.4G", "Mel238511");

    printf("Configurando MQTT com TLS-PSK...\n");

    const unsigned char psk[] = {0x41, 0x42, 0x43, 0x44, 0x58, 0x58,
                                 0x45, 0x46, 0x31, 0x32, 0x33, 0x34};

    mqtt_setup_psk("aluno35", "192.168.1.132", "aluno35", "aluno35",
                   psk, sizeof(psk), "aluno35");

    sleep_ms(2000);

    while (true)
    {
        update_led();

        uint32_t now = to_ms_since_boot(get_absolute_time());

        if (now - last_publish_time >= PUBLISH_INTERVAL_MS)
        {
            float temperature = read_temperature();
            bool button_pressed = read_button();

            led_on();
            publish_sensor_data(temperature, button_pressed);
            last_publish_time = now;
        }

        sleep_ms(100);
    }

    return 0;
}
