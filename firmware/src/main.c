#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "i2c_bus.h"
#include "max30102.h"
#include "mpu6050.h"
#include "ssd1306.h"
#include "w25q32.h"
#include "heart_rate.h"
#include "spo2.h"
#include "step_counter.h"
#include "calorie.h"
#include "fall_detector.h"
#include "motion_classifier.h"
#include "arrhythmia_screener.h"
#include "sleep_analyzer.h"
#include "ble.h"
#include "wifi_conn.h"
#include "deepseek_api.h"
#include "actuator.h"
#include "power_mgmt.h"

static const char *TAG = "MAIN";

#define STACK_SMALL   2048
#define STACK_MEDIUM  4096
#define STACK_LARGE   8192

#define PRIORITY_SENSOR     5
#define PRIORITY_ALGORITHM  4
#define PRIORITY_DISPLAY    3
#define PRIORITY_COMM       2
#define PRIORITY_POWER      1

typedef struct {
    float heart_rate;
    uint8_t spo2;
    uint32_t steps;
    float calories;
    motion_type_t motion;
    fall_state_t fall;
    float battery;
    bool fall_alert_sent;
} system_status_t;

static QueueHandle_t sensor_data_queue;
static QueueHandle_t display_update_queue;
static QueueHandle_t alert_queue;
static SemaphoreHandle_t i2c_mutex;
static SemaphoreHandle_t status_mutex;
static system_status_t sys_status;
static int64_t last_display_update;
static int64_t last_wifi_sync;
static bool wifi_enabled;

static void sensor_task(void *pvParameter)
{
    mpu6050_data_t mpu_data;
    max30102_sample_t max_data;
    heart_rate_result_t hr_result;
    spo2_result_t sp_result;
    step_counter_t step_result;

    static bool mpu_calibrated = false;
    if (!mpu_calibrated) {
        xSemaphoreTake(i2c_mutex, portMAX_DELAY);
        mpu6050_calibrate();
        xSemaphoreGive(i2c_mutex);
        mpu_calibrated = true;
    }

    while (1) {
        xSemaphoreTake(i2c_mutex, portMAX_DELAY);

        if (mpu6050_read_all(&mpu_data) == ESP_OK) {
            step_counter_feed_accel(mpu_data.accel_x, mpu_data.accel_y, mpu_data.accel_z);
            motion_classifier_feed_data(mpu_data.accel_x, mpu_data.accel_y, mpu_data.accel_z,
                                        mpu_data.gyro_x, mpu_data.gyro_y, mpu_data.gyro_z);
            fall_detector_feed_accel(mpu_data.accel_x, mpu_data.accel_y, mpu_data.accel_z);
        }

        if (max30102_read_fifo(&max_data) == ESP_OK && max_data.data_valid) {
            heart_rate_feed_sample(max_data.ir_raw, max_data.red_raw);
            spo2_feed_sample(max_data.ir_raw, max_data.red_raw);
        }

        xSemaphoreGive(i2c_mutex);

        heart_rate_get_result(&hr_result);
        spo2_get_result(&sp_result);
        step_counter_get_result(&step_result);

        if (hr_result.data_ready && hr_result.bpm > 0) {
            arrhythmia_screener_feed_rr_interval((uint32_t)(60000.0f / hr_result.bpm));
        }

        calorie_update_steps(step_result.total_steps);
        if (hr_result.data_ready) {
            calorie_update_heart_rate(hr_result.bpm);
        }

        motion_classifier_process();
        motion_type_t current_motion = motion_classifier_get_result();
        calorie_update_motion_type(current_motion);

        fall_detector_process();

        float calorie_total = 0.0f;
        calorie_get_total(&calorie_total);

        xSemaphoreTake(status_mutex, portMAX_DELAY);
        if (hr_result.data_ready) sys_status.heart_rate = hr_result.bpm;
        if (sp_result.data_ready) sys_status.spo2 = sp_result.spo2;
        sys_status.steps = step_result.total_steps;
        sys_status.calories = calorie_total;
        sys_status.motion = current_motion;
        sys_status.fall = fall_detector_get_state();
        sys_status.battery = power_mgmt_get_battery_percent();

        if (sys_status.fall == FALL_CONFIRMED && !sys_status.fall_alert_sent) {
            int alert = 1;
            xQueueSend(alert_queue, &alert, 0);
            sys_status.fall_alert_sent = true;
        }
        xSemaphoreGive(status_mutex);

        esp_task_wdt_reset();

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

static void display_task(void *pvParameter)
{
    static int page = 0;
    char line[64];

    while (1) {
        ssd1306_clear();

        xSemaphoreTake(status_mutex, portMAX_DELAY);
        switch (page) {
            case 0:
                snprintf(line, sizeof(line), "HR:%.0f SpO2:%d%%", sys_status.heart_rate, sys_status.spo2);
                ssd1306_draw_string(0, 0, line);
                snprintf(line, sizeof(line), "Steps:%lu Cal:%.1f", sys_status.steps, sys_status.calories);
                ssd1306_draw_string(0, 16, line);
                break;

            case 1:
                snprintf(line, sizeof(line), "Fall:%d Motion:%d", (int)sys_status.fall, (int)sys_status.motion);
                ssd1306_draw_string(0, 0, line);
                snprintf(line, sizeof(line), "Battery:%.0f%%", sys_status.battery);
                ssd1306_draw_string(0, 16, line);
                break;

            case 2:
                snprintf(line, sizeof(line), "WiFi:%s", wifi_enabled ? "ON" : "OFF");
                ssd1306_draw_string(0, 0, line);
                snprintf(line, sizeof(line), "Steps OK");
                ssd1306_draw_string(0, 16, line);
                break;

            default:
                break;
        }
        xSemaphoreGive(status_mutex);

        ssd1306_display();

        page = (page + 1) % 3;
        last_display_update = esp_timer_get_time();

        esp_task_wdt_reset();

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void comm_task(void *pvParameter)
{
    while (1) {
        xSemaphoreTake(status_mutex, portMAX_DELAY);
        if (ble_is_connected()) {
            ble_data_packet_t packet;
            packet.heart_rate = sys_status.heart_rate;
            packet.spo2 = sys_status.spo2;
            packet.steps = sys_status.steps;
            packet.calories = sys_status.calories;
            packet.motion_type = (uint8_t)sys_status.motion;
            packet.fall_detected = (sys_status.fall == FALL_CONFIRMED) ? 1 : 0;
            packet.battery_voltage = sys_status.battery;
            ble_send_data(&packet);
        }

        if (wifi_enabled) {
            int64_t now = esp_timer_get_time();
            if (now - last_wifi_sync > 3600000000LL) {
                daily_report_t report;
                memset(&report, 0, sizeof(report));
                report.total_steps = sys_status.steps;
                report.total_calories = sys_status.calories;
                report.avg_heart_rate = sys_status.heart_rate;
                report.avg_spo2 = sys_status.spo2;
                deepseek_api_upload_data(&report);
                last_wifi_sync = now;
            }
        }
        xSemaphoreGive(status_mutex);

        if (wifi_enabled && !wifi_conn_is_connected()) {
            wifi_conn_connect();
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void alert_task(void *pvParameter)
{
    int alert;

    while (1) {
        xQueueReceive(alert_queue, &alert, portMAX_DELAY);

        actuator_fall_alert();

        ssd1306_clear();
        ssd1306_draw_string(0, 0, "!!! FALL !!!");
        ssd1306_draw_string(0, 16, "Press to cancel");
        ssd1306_display();

        vTaskDelay(pdMS_TO_TICKS(3000));

        fall_detector_acknowledge();
        xSemaphoreTake(status_mutex, portMAX_DELAY);
        sys_status.fall_alert_sent = false;
        xSemaphoreGive(status_mutex);
    }
}

static void power_task(void *pvParameter)
{
    int64_t idle_start = 0;
    bool idle = false;

    while (1) {
        int64_t now = esp_timer_get_time();
        int64_t display_elapsed = now - last_display_update;

        if (sys_status.battery < 10.0f) {
            ESP_LOGW(TAG, "Battery critically low, entering deep sleep");
            ssd1306_clear();
            ssd1306_draw_string(0, 0, "Battery Low");
            ssd1306_draw_string(0, 16, "Shutting Down");
            ssd1306_display();
            vTaskDelay(pdMS_TO_TICKS(2000));
            power_mgmt_enter_deep_sleep();
        }

        if (sys_status.battery < 20.0f) {
            power_mgmt_set_mode(POWER_LIGHT_SLEEP);
        } else if (display_elapsed > 30000000LL) {
            if (!idle) {
                idle_start = now;
                idle = true;
            }
            if (now - idle_start > 120000000LL) {
                power_mgmt_set_mode(POWER_LIGHT_SLEEP);
            }
        } else {
            idle = false;
            power_mgmt_set_mode(POWER_ACTIVE);
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    i2c_mutex = xSemaphoreCreateMutex();
    status_mutex = xSemaphoreCreateMutex();
    sensor_data_queue = xQueueCreate(10, sizeof(int));
    display_update_queue = xQueueCreate(5, sizeof(int));
    alert_queue = xQueueCreate(5, sizeof(int));

    i2c_bus_init();
    i2c_bus_scan();

    mpu6050_init();
    max30102_init();
    ssd1306_init();
    w25q32_init();

    heart_rate_init();
    spo2_init();
    step_counter_init();
    calorie_init();
    fall_detector_init();
    motion_classifier_init();
    arrhythmia_screener_init();
    sleep_analyzer_init();

    ble_init();
    wifi_conn_init();
    deepseek_api_init();
    actuator_init();
    power_mgmt_init();

    wifi_enabled = false;
    last_display_update = esp_timer_get_time();
    last_wifi_sync = esp_timer_get_time();

    ssd1306_clear();
    ssd1306_draw_string(0, 0, "Smart Band");
    ssd1306_draw_string(0, 16, "Starting...");
    ssd1306_display();

    xTaskCreate(sensor_task, "sensor", STACK_LARGE, NULL, PRIORITY_SENSOR, NULL);
    xTaskCreate(display_task, "display", STACK_MEDIUM, NULL, PRIORITY_DISPLAY, NULL);
    xTaskCreate(comm_task, "comm", STACK_MEDIUM, NULL, PRIORITY_COMM, NULL);
    xTaskCreate(alert_task, "alert", STACK_SMALL, NULL, PRIORITY_ALGORITHM, NULL);
    xTaskCreate(power_task, "power", STACK_SMALL, NULL, PRIORITY_POWER, NULL);

    ESP_LOGI(TAG, "All tasks created, system ready");
}
