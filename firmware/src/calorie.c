#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef enum {
    MOTION_STATIC = 0,
    MOTION_WALKING,
    MOTION_RUNNING,
    MOTION_CYCLING
} motion_type_t;

static const float MET_VALUES[] = {
    1.0f,
    3.5f,
    8.0f,
    6.0f
};

static float weight_kg;
static float total_calories;
static uint32_t last_steps;
static float last_hr;
static motion_type_t current_motion;

void calorie_init(float weight)
{
    weight_kg = weight;
    total_calories = 0.0f;
    last_steps = 0;
    last_hr = 60.0f;
    current_motion = MOTION_STATIC;
}

void calorie_update_steps(uint32_t steps)
{
    if (steps > last_steps) {
        uint32_t delta = steps - last_steps;
        total_calories += (float)delta * 0.04f * (weight_kg / 65.0f);
        last_steps = steps;
    }
}

void calorie_update_heart_rate(float hr)
{
    last_hr = hr;
}

void calorie_update_motion_type(int motion)
{
    if (motion >= MOTION_STATIC && motion <= MOTION_CYCLING) {
        current_motion = (motion_type_t)motion;
    }
}

float calorie_get_total(void)
{
    float met = MET_VALUES[current_motion];
    float base = met * weight_kg * 0.0175f;

    float hr_correction = 0.0f;
    if (last_hr > 60.0f) {
        hr_correction = (last_hr - 60.0f) * 0.015f;
    }

    return total_calories + base + hr_correction;
}
