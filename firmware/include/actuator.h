#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

#define ACTUATOR_MOTOR_GPIO  4
#define ACTUATOR_BUZZER_GPIO 5

typedef enum {
    VIBRATE_SHORT  = 0,
    VIBRATE_MEDIUM = 1,
    VIBRATE_LONG   = 2,
    VIBRATE_PULSE  = 3
} vibrate_pattern_t;

typedef enum {
    BEEP_SHORT  = 0,
    BEEP_LONG   = 1,
    BEEP_DOUBLE = 2,
    BEEP_ALARM  = 3
} beep_pattern_t;

esp_err_t actuator_init(void);
esp_err_t actuator_vibrate(vibrate_pattern_t pattern);
esp_err_t actuator_buzzer(beep_pattern_t pattern);
esp_err_t actuator_vibrate_stop(void);
esp_err_t actuator_buzzer_stop(void);
esp_err_t actuator_fall_alert(void);
esp_err_t actuator_goal_celebrate(void);
