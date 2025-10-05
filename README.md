# Raspberry Pi Pico W e Mosquitto broker com autenticação TLS-PSK (Pre-Shared Key).
```
tarefa/
├── hardware/          # Código para Raspberry Pi Pico W
│   ├── hardware.c     # Aplicação principal (publica temperatura + botão)
│   ├── src/
│   │   ├── mqtt_comm.c      # Cliente MQTT com suporte TLS
│   │   ├── mqtt_tls_psk.c   # Configuração TLS-PSK
│   │   └── wifi_conn.c      # Conexão WiFi
│   ├── lwipopts.h          # Configuração lwIP (ALTCP_TLS habilitado)
│   └── mbedtls_config.h    # Configuração mbedTLS (PSK habilitado)
│
└── servidor/          # Configuração do broker Mosquitto
    ├── mosquitto.conf # Broker com dual listener (TLS-PSK + plaintext)
    ├── psk.txt        # Chaves PSK
    ├── passwd         # Senhas hash
    └── acl.conf       # Controle de acesso por tópicos
```
### Credenciais
- **Identity/Usuário:** aluno35
- **Senha:** aluno35  
- **Chave PSK (texto):** ABCDXXEF1234
- **Chave PSK (hex):** 414243445858454631323334

### Tópicos MQTT (ACL)
- `/aluno35/pub/#` - Publicação
- `/aluno35/sub/#` - Subscrição
- `/aluno35/bitdoglab/#` - Publicação e subscrição

### Servidor Mosquitto

```bash
cd servidor
mosquitto -c mosquitto.conf -v
```
### Pico W (ou utilizando os comandos da extensão no VSCODE)

```bash
cd hardware/build
cmake -G "Unix Makefiles" ..
make

# Pressione BOOTSEL no Pico W e conecte ao USB
cp hardware.uf2 /media/$USER/RPI-RP2/
```

### Altere para utilizar

```c
// Linha 113: WiFi
connect_to_wifi("SEU_SSID", "SUA_SENHA");

// Linha 119: IP do broker
mqtt_setup_psk("aluno35", "127.0.0.1", ...);  // TROQUE O IP!
```

### Teste

**Terminal 1 - Subscrever:**
```bash
mosquitto_sub -h localhost -p 8883 \
  --psk "414243445858454631323334" \
  --psk-identity aluno35 \
  --ciphers "PSK-AES128-CBC-SHA256" \
  -t '/aluno35/sub/#' \
  -v
```

**Terminal 2 - Publicar:**
```bash
mosquitto_pub -h localhost -p 8883 \
  --psk "414243445858454631323334" \
  --psk-identity aluno35 \
  --ciphers "PSK-AES128-CBC-SHA256" \
  -t '/aluno35/pub/' \
  -m 'Teste com TLS-PSK'
```

### Pico W - Cliente MQTT com TLS-PSK

O código implementa TLS-PSK usando:

1. **lwIP MQTT Client** com suporte ALTCP
2. **mbedTLS** para criptografia TLS
3. **Função `mqtt_tls_create_config_client_psk()`** customizada que:
   - Cria config TLS com `altcp_tls_create_config_client()`
   - Configura PSK com `mbedtls_ssl_conf_psk()`
   - Retorna config pronto para `mqtt_connect_client_info_t.tls_config`

### Fluxo de Conexão TLS-PSK

```
1. WiFi Connect
2. DNS Resolve (broker IP)
3. mqtt_tls_create_config_client_psk()
   ├─> altcp_tls_create_config_client()
   └─> mbedtls_ssl_conf_psk(PSK, identity)
4. mqtt_client_info.tls_config = config
5. mqtt_client_connect() 
   └─> lwIP cria altcp_tls_new() internamente
6. TLS Handshake com PSK
7. MQTT CONNECT
```
### Como utilizar TLS-PSK na pico w

**lwipopts.h:**
```c
#define LWIP_ALTCP 1
#define LWIP_ALTCP_TLS 1
#define LWIP_ALTCP_TLS_MBEDTLS 1
#define MEM_SIZE 16000  // Memória aumentada para TLS
```

**mbedtls_config.h:**
```c
#define MBEDTLS_KEY_EXCHANGE_PSK_ENABLED  // Habilita PSK
```

1. **lwIP MQTT JÁ tem suporte ALTCP/TLS** - basta habilitar com `LWIP_ALTCP_TLS`
2. **Campo `tls_config` existe** em `mqtt_connect_client_info_t` (condicional a `#if LWIP_ALTCP && LWIP_ALTCP_TLS`)
3. **mbedTLS no Pico SDK** já está integrado com lwIP via `pico_lwip_mbedtls`
4. **É crucial:** Habilitar PSK (`MBEDTLS_KEY_EXCHANGE_PSK_ENABLED`) e configurar corretamente