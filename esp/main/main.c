#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/uart.h"
#include "led_strip.h"
#include "mqtt_client.h"
#include "esp_log.h"

static const char *TAG = "bridge";

/* ======================== WiFi 配置 ======================== */
#define WIFI_SSID      "3030"
#define WIFI_PASS      "3030303030"

/* ======================== UART 配置 ======================== */
#define UART_NUM       UART_NUM_2
#define UART_TX_PIN    16
#define UART_RX_PIN    17
#define UART_BUF_SIZE  512

/* ======================== OneNET MQTT 配置 ======================== */
#define ONENET_PRODUCT_ID   "6176tD4mc7"
#define ONENET_DEVICE_NAME  "esp32"
#define ONENET_TOKEN        "version=2018-10-31&res=products%2F6176tD4mc7%2Fdevices%2Fesp32&et=4780830839&method=md5&sign=OI4Qcd0SiZ6BF67b6odgBQ%3D%3D"

#define TOPIC_DATA     "$sys/" ONENET_PRODUCT_ID "/" ONENET_DEVICE_NAME "/thing/property/post"
#define TOPIC_CMD      "$sys/" ONENET_PRODUCT_ID "/" ONENET_DEVICE_NAME "/thing/property/set"
#define TOPIC_SET_REPLY "$sys/" ONENET_PRODUCT_ID "/" ONENET_DEVICE_NAME "/thing/property/set_reply"

static esp_mqtt_client_handle_t mqtt_client;
static EventGroupHandle_t wifi_evt_group;
#define WIFI_CONNECTED_BIT BIT0

/* ======================== NVS 阈值存取 ======================== */
#define NVS_NS "threshold"

/* 可写的属性列表（与物模型对应） */
static const char *prop_names[] = {
    "maxtemp_set", "minitemp_set", "maxhum_set",
    "minihum_set", "minlight_set", "neardist_set",
    "led_switch", "servo_switch", "motor_switch"     /* 执行器控制 */
};
static const int PROP_COUNT = sizeof(prop_names) / sizeof(prop_names[0]);

/* 判断是否为执行器属性（不存 NVS，只转发 actuator 命令） */
static int is_actuator_prop(const char *key)
{
    return (strcmp(key, "led_switch") == 0 ||
            strcmp(key, "servo_switch") == 0 ||
            strcmp(key, "motor_switch") == 0);
}

/* 执行器属性名 → 简写 act 名 */
static const char *actuator_act_name(const char *prop)
{
    if (strcmp(prop, "led_switch") == 0)   return "led";
    if (strcmp(prop, "servo_switch") == 0) return "servo";
    if (strcmp(prop, "motor_switch") == 0) return "motor";
    return "";
}

/* 判断 key 是否应存为整数（湿度/光照阈值无小数） */
static int is_int_key(const char *key)
{
    return (strcmp(key, "maxhum_set") == 0 ||
            strcmp(key, "minihum_set") == 0 ||
            strcmp(key, "minlight_set") == 0);
}

static void nvs_save_one(const char *key, float val)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
        if (is_int_key(key)) {
            nvs_set_i32(h, key, (int32_t)val);
        } else {
            /* maxtemp_set, minitemp_set, neardist_set → float */
            nvs_set_blob(h, key, &val, sizeof(float));
        }
        nvs_commit(h);
        nvs_close(h);
    }
}

static float nvs_load_one(const char *key, float def_val)
{
    nvs_handle_t h;
    float val = def_val;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) == ESP_OK) {
        if (is_int_key(key)) {
            int32_t iv = (int32_t)def_val;
            nvs_get_i32(h, key, &iv);
            val = (float)iv;
        } else {
            size_t sz = sizeof(float);
            nvs_get_blob(h, key, &val, &sz);
        }
        nvs_close(h);
    }
    return val;
}

/* 发送所有阈值给 STM32（初始化） */
static void send_init_thresholds(void)
{
    float vals[6];
    vals[0] = nvs_load_one("maxtemp_set", 35.0f);
    vals[1] = nvs_load_one("minitemp_set", 0.0f);
    vals[2] = nvs_load_one("maxhum_set", 80.0f);
    vals[3] = nvs_load_one("minihum_set", 20.0f);
    vals[4] = nvs_load_one("minlight_set", 20.0f);
    vals[5] = nvs_load_one("neardist_set", 10.0f);

    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "{\"cmd\":\"init\",\"thr\":{"
        "\"maxtemp_set\":%.1f,"
        "\"minitemp_set\":%.1f,"
        "\"maxhum_set\":%.0f,"
        "\"minihum_set\":%.0f,"
        "\"minlight_set\":%.0f,"
        "\"neardist_set\":%.1f"
        "}}\n",
        vals[0], vals[1], vals[2], vals[3], vals[4], vals[5]);
    uart_write_bytes(UART_NUM, cmd, strlen(cmd));
    ESP_LOGI(TAG, "[NVS->UART] Sent init thresholds");
}

/* ======================== WiFi ======================== */
static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WiFi Disconnected, retrying...");
        xEventGroupClearBits(wifi_evt_group, WIFI_CONNECTED_BIT);
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *evt = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "WiFi Connected, IP: " IPSTR, IP2STR(&evt->ip_info.ip));
        xEventGroupSetBits(wifi_evt_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init(void)
{
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_evt_group = xEventGroupCreate();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);

    wifi_config_t wifi_config = {
        .sta = {.ssid = WIFI_SSID, .password = WIFI_PASS},
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}

static void uart_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = 115200, .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE, .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM, UART_BUF_SIZE, 0, 0, NULL, 0);
}

/* ======================== MQTT ======================== */

/* 云端下发属性设置 → 解析/转发/存NVS */
static void handle_cloud_set(const char *payload, int payload_len)
{
    char buf[512];
    int copy_len = payload_len < (int)(sizeof(buf) - 1) ? payload_len : (int)(sizeof(buf) - 1);
    memcpy(buf, payload, copy_len);
    buf[copy_len] = '\0';

    char req_id[64] = "0";
    char *idp = strstr(buf, "\"id\":\"");
    if (idp) {
        idp += 6;
        int k = 0;
        while (*idp && *idp != '\"' && k < 62) req_id[k++] = *idp++;
        req_id[k] = '\0';
    }

    bool found_any = false;
    for (int i = 0; i < PROP_COUNT; i++) {
        char key[64];
        snprintf(key, sizeof(key), "\"%s\":{\"value\":", prop_names[i]);
        char *pos = strstr(buf, key);
        int nested = 1;
        if (!pos) {
            snprintf(key, sizeof(key), "\"%s\":", prop_names[i]);
            pos = strstr(buf, key);
            nested = 0;
        }
        if (pos) {
            pos += strlen(key);
            char value_str[32] = {0};
            int j = 0;
            char end_char = nested ? '}' : ',';
            while (*pos && *pos != end_char && *pos != '}' && j < 31)
                value_str[j++] = *pos++;
            value_str[j] = '\0';

            float val = 0;
            /* 支持 JSON boolean: "true"/"false" 与数字 */
            if (strncmp(value_str, "true", 4) == 0)
                val = 1.0f;
            else if (strncmp(value_str, "false", 5) == 0)
                val = 0.0f;
            else
                sscanf(value_str, "%f", &val);

            /* 执行器属性：发 actuator 命令，不存 NVS */
            if (is_actuator_prop(prop_names[i])) {
                char cmd[64];
                snprintf(cmd, sizeof(cmd),
                    "{\"cmd\":\"actuator\",\"act\":\"%s\",\"val\":%.0f}\n",
                    actuator_act_name(prop_names[i]), val);
                uart_write_bytes(UART_NUM, cmd, strlen(cmd));
                ESP_LOGI(TAG, "[MQTT->UART] %s", cmd);
                found_any = true;
                continue;
            }

            /* 转发给 STM32 */
            char cmd[256];
            if (strchr(value_str, '.'))
                snprintf(cmd, sizeof(cmd),
                    "{\"cmd\":\"set\",\"prop\":\"%s\",\"value\":%.1f}\n",
                    prop_names[i], val);
            else
                snprintf(cmd, sizeof(cmd),
                    "{\"cmd\":\"set\",\"prop\":\"%s\",\"value\":%.0f}\n",
                    prop_names[i], val);
            uart_write_bytes(UART_NUM, cmd, strlen(cmd));
            ESP_LOGI(TAG, "[MQTT->UART] %s", cmd);

            /* 保存到 NVS */
            nvs_save_one(prop_names[i], val);

            found_any = true;
        }
    }

    if (found_any) {
        char reply[256];
        snprintf(reply, sizeof(reply),
            "{\"id\":\"%s\",\"code\":200,\"msg\":\"success\"}", req_id);
        esp_mqtt_client_publish(mqtt_client, TOPIC_SET_REPLY, reply, 0, 1, 0);
        ESP_LOGI(TAG, "[MQTT->REPLY] %s", reply);
    }
}

static void mqtt_event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    esp_mqtt_event_handle_t evt = (esp_mqtt_event_handle_t)data;
    switch (event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT Connected");
        esp_mqtt_client_subscribe(mqtt_client, TOPIC_CMD, 0);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "CMD: %.*s", evt->data_len, evt->data);
        handle_cloud_set(evt->data, evt->data_len);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT Error");
        break;
    }
}

static void mqtt_init(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://studio-mqtt.heclouds.com:1883",
        .credentials = {
            .username = ONENET_PRODUCT_ID,
            .authentication.password = ONENET_TOKEN,
            .client_id = ONENET_DEVICE_NAME,
        },
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

/* ======================== UART ↔ MQTT 主任务 ======================== */

static void handle_stm32_msg(const char *json)
{
    /* STM32 OLED 阈值保存 → 存 NVS */
    if (strstr(json, "\"cmd\":\"report\"")) {
        for (int i = 0; i < 6; i++) {
            char key[64];
            snprintf(key, sizeof(key), "\"%s\":", prop_names[i]);
            char *pos = strstr(json, key);
            if (pos) {
                pos += strlen(key);
                char val_str[32] = {0};
                int j = 0;
                while (*pos && *pos != ',' && *pos != '}' && j < 31)
                    val_str[j++] = *pos++;
                val_str[j] = '\0';
                float val = atof(val_str);
                nvs_save_one(prop_names[i], val);
            }
        }
        ESP_LOGI(TAG, "[UART->NVS] Thresholds synced from STM32");
    }
}

static void uart_to_mqtt_task(void *arg)
{
    char buf[256];
    int wait_ok = 0;
    while (1) {
        if (!wait_ok) {
            ESP_LOGI(TAG, "Init WiFi...");
            wifi_init();
            ESP_LOGI(TAG, "Waiting for WiFi...");
            xEventGroupWaitBits(wifi_evt_group, WIFI_CONNECTED_BIT,
                                pdFALSE, pdTRUE, portMAX_DELAY);
            ESP_LOGI(TAG, "WiFi Got IP, starting services...");
            uart_init();
            ESP_LOGI(TAG, "UART Ready (GPIO%d=RX, GPIO%d=TX)", UART_RX_PIN, UART_TX_PIN);
            mqtt_init();
            ESP_LOGI(TAG, "MQTT Ready");

            /* 从 NVS 加载阈值发给 STM32 恢复 */
            vTaskDelay(pdMS_TO_TICKS(1000));
            send_init_thresholds();

            wait_ok = 1;
        }

        int len = uart_read_bytes(UART_NUM, (uint8_t *)buf, sizeof(buf)-1, pdMS_TO_TICKS(100));
        if (len > 0) {
            buf[len] = '\0';
            char *start = strchr(buf, '{');
            char *end   = strchr(buf, '}');
            if (start && end && end > start) {
                end[1] = '\0';

                /* 判断消息类型 */
                if (strstr(start, "\"cmd\":\"report\"")) {
                    handle_stm32_msg(start);
                } else if (strstr(start, "\"t\":") && strstr(start, "\"h\":")) {
                    /* 传感器数据 → 转 OneJSON 发布 */
                    float t = 0, h = 0;
                    int l = 0;
                    float d = 0;
                    sscanf(start, "{\"t\":%f,\"h\":%f,\"l\":%d,\"d\":%f}",
                           &t, &h, &l, &d);

                    char onejson[256];
                    snprintf(onejson, sizeof(onejson),
                        "{\"id\":\"%ld\","
                        "\"version\":\"1.0\","
                        "\"params\":{"
                        "\"temp_value\":{\"value\":%.1f},"
                        "\"humidity_value\":{\"value\":%.0f},"
                        "\"light_value\":{\"value\":%d},"
                        "\"dist_value\":{\"value\":%.1f}"
                        "}}",
                        (long)xTaskGetTickCount(), t, h, l, d);

                    int msg_id = esp_mqtt_client_publish(mqtt_client, TOPIC_DATA, onejson, 0, 1, 0);
                    ESP_LOGI(TAG, "[UART->MQTT] %s (msg_id=%d)", onejson, msg_id);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void)
{
    /* 关闭板载 WS2812 RGB LED (GPIO48) */
    led_strip_handle_t led;
    led_strip_config_t strip_cfg = {
        .strip_gpio_num = 48,
        .max_leds = 1,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };
    led_strip_rmt_config_t rmt_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_cfg, &rmt_cfg, &led));
    led_strip_set_pixel(led, 0, 0, 0, 0);
    led_strip_refresh(led);

    ESP_LOGI(TAG, "ESP32-S3 Bridge Started");
    xTaskCreate(uart_to_mqtt_task, "bridge", 4096, NULL, 5, NULL);
}
