#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "actuator.h"

#define ACTUATOR_MOTOR_GPIO   4
#define ACTUATOR_BUZZER_GPIO  5

#define MOTOR_PWM_CHANNEL  LEDC_CHANNEL_0
#define BUZZER_PWM_CHANNEL LEDC_CHANNEL_1

#define MOTOR_TIMER        LEDC_TIMER_0
#define BUZZER_TIMER       LEDC_TIMER_1

#define PWM_DUTY_ON        200
#define PWM_DUTY_OFF       0

static const char *TAG = "ACTUATOR";

static void pwm_init(int gpio, ledc_channel_t channel, ledc_timer_t timer, int freq)
{
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = timer,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = freq,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t channel_conf = {
        .gpio_num = gpio,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = channel,
        .timer_sel = timer,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&channel_conf);
}

static void pwm_set(ledc_channel_t channel, uint32_t duty)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
}

esp_err_t actuator_init(void)
{
    gpio_reset_pin(ACTUATOR_MOTOR_GPIO);
    gpio_reset_pin(ACTUATOR_BUZZER_GPIO);

    pwm_init(ACTUATOR_MOTOR_GPIO, MOTOR_PWM_CHANNEL, MOTOR_TIMER, 1000);
    pwm_init(ACTUATOR_BUZZER_GPIO, BUZZER_PWM_CHANNEL, BUZZER_TIMER, 2000);

    pwm_set(MOTOR_PWM_CHANNEL, PWM_DUTY_OFF);
    pwm_set(BUZZER_PWM_CHANNEL, PWM_DUTY_OFF);

    return ESP_OK;
}

esp_err_t actuator_vibrate(vibrate_pattern_t pattern)
{
    switch (pattern) {
        case VIBRATE_SHORT:
            pwm_set(MOTOR_PWM_CHANNEL, PWM_DUTY_ON);
            vTaskDelay(pdMS_TO_TICKS(200));
            pwm_set(MOTOR_PWM_CHANNEL, PWM_DUTY_OFF);
            break;

        case VIBRATE_MEDIUM:
            pwm_set(MOTOR_PWM_CHANNEL, PWM_DUTY_ON);
            vTaskDelay(pdMS_TO_TICKS(500));
            pwm_set(MOTOR_PWM_CHANNEL, PWM_DUTY_OFF);
            break;

        case VIBRATE_LONG:
            pwm_set(MOTOR_PWM_CHANNEL, PWM_DUTY_ON);
            vTaskDelay(pdMS_TO_TICKS(1000));
            pwm_set(MOTOR_PWM_CHANNEL, PWM_DUTY_OFF);
            break;

        case VIBRATE_PULSE:
            for (int i = 0; i < 3; i++) {
                pwm_set(MOTOR_PWM_CHANNEL, PWM_DUTY_ON);
                vTaskDelay(pdMS_TO_TICKS(100));
                pwm_set(MOTOR_PWM_CHANNEL, PWM_DUTY_OFF);
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            break;

        default:
            break;
    }
    return ESP_OK;
}

esp_err_t actuator_buzzer(beep_pattern_t pattern)
{
    switch (pattern) {
        case BEEP_SHORT:
            pwm_set(BUZZER_PWM_CHANNEL, PWM_DUTY_ON);
            vTaskDelay(pdMS_TO_TICKS(100));
            pwm_set(BUZZER_PWM_CHANNEL, PWM_DUTY_OFF);
            break;

        case BEEP_LONG:
            pwm_set(BUZZER_PWM_CHANNEL, PWM_DUTY_ON);
            vTaskDelay(pdMS_TO_TICKS(500));
            pwm_set(BUZZER_PWM_CHANNEL, PWM_DUTY_OFF);
            break;

        case BEEP_DOUBLE:
            for (int i = 0; i < 2; i++) {
                pwm_set(BUZZER_PWM_CHANNEL, PWM_DUTY_ON);
                vTaskDelay(pdMS_TO_TICKS(100));
                pwm_set(BUZZER_PWM_CHANNEL, PWM_DUTY_OFF);
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            break;

        case BEEP_ALARM:
            for (int i = 0; i < 5; i++) {
                pwm_set(BUZZER_PWM_CHANNEL, PWM_DUTY_ON);
                vTaskDelay(pdMS_TO_TICKS(200));
                pwm_set(BUZZER_PWM_CHANNEL, PWM_DUTY_OFF);
                vTaskDelay(pdMS_TO_TICKS(200));
            }
            break;

        default:
            break;
    }
    return ESP_OK;
}

esp_err_t actuator_vibrate_stop(void)
{
    pwm_set(MOTOR_PWM_CHANNEL, PWM_DUTY_OFF);
    return ESP_OK;
}

esp_err_t actuator_buzzer_stop(void)
{
    pwm_set(BUZZER_PWM_CHANNEL, PWM_DUTY_OFF);
    return ESP_OK;
}

esp_err_t actuator_fall_alert(void)
{
    actuator_buzzer(BEEP_ALARM);
    actuator_vibrate(VIBRATE_PULSE);
    return ESP_OK;
}

esp_err_t actuator_goal_celebrate(void)
{
    actuator_vibrate(VIBRATE_SHORT);
    actuator_buzzer(BEEP_DOUBLE);
    return ESP_OK;
}
