#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "fall_detector.h"

#define WINDOW_SIZE 200
#define STEP_SIZE 50
#define FEATURE_DIM 6
#define HIDDEN1 16
#define HIDDEN2 8
#define CONFIRM_THRESH 3

static float accel_buffer[WINDOW_SIZE][3];
static int buffer_head;
static int buffer_count;
static int window_ready;
static int confirm_counter;
static fall_state_t fall_state;

static const float w1[FEATURE_DIM * HIDDEN1] = {
    0.42f, -0.13f, 0.27f, -0.35f, 0.18f, 0.09f,
    -0.28f, 0.33f, -0.21f, 0.15f, -0.42f, 0.11f,
    0.19f, -0.31f, 0.24f, -0.17f, 0.36f, -0.08f,
    0.13f, 0.29f, -0.26f, 0.22f, -0.14f, 0.37f,
    -0.19f, 0.08f, -0.33f, 0.41f, -0.12f, 0.25f,
    0.16f, -0.29f, 0.34f, -0.22f, 0.11f, -0.38f,
    -0.24f, 0.18f, -0.15f, 0.32f, -0.27f, 0.13f,
    0.21f, -0.36f, 0.09f, -0.28f, 0.31f, -0.17f,
    -0.11f, 0.26f, -0.34f, 0.19f, -0.23f, 0.14f,
    0.38f, -0.09f, 0.28f, -0.31f, 0.17f, -0.25f,
    0.12f, -0.37f, 0.22f, -0.16f, 0.33f, -0.19f,
    0.08f, 0.41f, -0.13f, 0.29f, -0.24f, 0.15f,
    -0.32f, 0.18f, -0.27f, 0.11f, -0.38f, 0.23f,
    0.26f, -0.14f, 0.31f, -0.09f, 0.37f, -0.22f,
    -0.18f, 0.34f, -0.11f, 0.28f, -0.33f, 0.16f,
    0.14f, -0.26f, 0.38f, -0.17f, 0.21f, -0.35f,
    0.09f, -0.29f, 0.24f, -0.13f, 0.32f, -0.27f,
    0.06f, 0.17f, -0.39f, 0.22f, -0.14f, 0.31f,
    -0.28f, 0.11f, -0.35f, 0.18f, -0.26f, 0.33f,
    0.19f, -0.12f, 0.29f, -0.37f, 0.15f, -0.23f
};

static const float b1[HIDDEN1] = {
    0.12f, -0.08f, 0.15f, -0.11f, 0.09f, -0.14f, 0.07f, -0.13f,
    0.11f, -0.06f, 0.16f, -0.09f, 0.13f, -0.10f, 0.08f, -0.12f
};

static const float w2[HIDDEN1 * HIDDEN2] = {
    0.31f, -0.18f, 0.24f, -0.27f, 0.15f, -0.33f, 0.21f, -0.19f,
    -0.14f, 0.29f, -0.22f, 0.17f, -0.31f, 0.12f, -0.26f, 0.23f,
    0.18f, -0.25f, 0.33f, -0.16f, 0.22f, -0.29f, 0.14f, -0.21f,
    -0.27f, 0.16f, -0.19f, 0.34f, -0.12f, 0.28f, -0.23f, 0.15f,
    0.22f, -0.31f, 0.18f, -0.24f, 0.29f, -0.15f, 0.26f, -0.17f,
    -0.19f, 0.14f, -0.33f, 0.21f, -0.28f, 0.16f, -0.12f, 0.32f,
    0.25f, -0.22f, 0.17f, -0.29f, 0.13f, -0.34f, 0.19f, -0.16f,
    -0.23f, 0.31f, -0.14f, 0.27f, -0.18f, 0.24f, -0.32f, 0.11f,
    0.16f, -0.28f, 0.22f, -0.13f, 0.34f, -0.19f, 0.25f, -0.21f,
    -0.31f, 0.17f, -0.26f, 0.14f, -0.23f, 0.32f, -0.15f, 0.29f,
    0.13f, -0.24f, 0.31f, -0.18f, 0.27f, -0.22f, 0.16f, -0.34f,
    -0.28f, 0.19f, -0.15f, 0.33f, -0.11f, 0.26f, -0.29f, 0.18f,
    0.27f, -0.16f, 0.23f, -0.31f, 0.14f, -0.25f, 0.33f, -0.18f,
    -0.22f, 0.34f, -0.12f, 0.28f, -0.19f, 0.15f, -0.27f, 0.21f,
    0.19f, -0.33f, 0.14f, -0.26f, 0.31f, -0.17f, 0.24f, -0.22f,
    -0.15f, 0.28f, -0.32f, 0.13f, -0.24f, 0.33f, -0.18f, 0.26f,
    0.11f, -0.29f, 0.32f, -0.14f, 0.23f, -0.27f, 0.18f, -0.21f,
    -0.34f, 0.15f, -0.25f, 0.19f, -0.31f, 0.28f, -0.16f, 0.22f,
    0.24f, -0.13f, 0.29f, -0.32f, 0.17f, -0.22f, 0.31f, -0.15f,
    -0.27f, 0.33f, -0.18f, 0.14f, -0.26f, 0.21f, -0.34f, 0.12f,
    0.28f, -0.19f, 0.16f, -0.33f, 0.24f, -0.11f, 0.29f, -0.23f,
    -0.16f, 0.31f, -0.27f, 0.18f, -0.14f, 0.34f, -0.22f, 0.25f,
    0.14f, -0.32f, 0.21f, -0.28f, 0.16f, -0.24f, 0.33f, -0.17f,
    -0.29f, 0.23f, -0.15f, 0.31f, -0.19f, 0.12f, -0.26f, 0.34f,
    0.21f, -0.17f, 0.34f, -0.23f, 0.28f, -0.14f, 0.19f, -0.31f,
    -0.25f, 0.11f, -0.29f, 0.32f, -0.16f, 0.27f, -0.13f, 0.24f,
    0.32f, -0.21f, 0.17f, -0.34f, 0.13f, -0.28f, 0.25f, -0.16f,
    -0.18f, 0.14f, -0.31f, 0.23f, -0.27f, 0.33f, -0.11f, 0.29f,
    0.15f, -0.33f, 0.28f, -0.19f, 0.31f, -0.24f, 0.17f, -0.22f,
    -0.32f, 0.26f, -0.14f, 0.29f, -0.21f, 0.13f, -0.34f, 0.18f,
    0.26f, -0.15f, 0.22f, -0.31f, 0.19f, -0.33f, 0.14f, -0.28f,
    -0.17f, 0.34f, -0.23f, 0.11f, -0.29f, 0.32f, -0.18f, 0.25f
};

static const float b2[HIDDEN2] = {
    0.09f, -0.11f, 0.13f, -0.07f, 0.14f, -0.09f, 0.11f, -0.12f
};

static const float w3[HIDDEN2 * 2] = {
    0.45f, -0.45f,
    -0.38f, 0.38f,
    0.42f, -0.42f,
    -0.35f, 0.35f,
    0.48f, -0.48f,
    -0.41f, 0.41f,
    0.39f, -0.39f,
    -0.44f, 0.44f
};

static const float b3[2] = {0.15f, -0.15f};

static float features[FEATURE_DIM];

esp_err_t fall_detector_init(void)
{
    memset(accel_buffer, 0, sizeof(accel_buffer));
    buffer_head = 0;
    buffer_count = 0;
    window_ready = 0;
    confirm_counter = 0;
    fall_state = FALL_NONE;
    memset(features, 0, sizeof(features));
    return ESP_OK;
}

esp_err_t fall_detector_feed_accel(float ax, float ay, float az)
{
    int idx = buffer_head % WINDOW_SIZE;
    accel_buffer[idx][0] = ax;
    accel_buffer[idx][1] = ay;
    accel_buffer[idx][2] = az;
    buffer_head++;
    if (buffer_count < WINDOW_SIZE) buffer_count++;
    if (buffer_count >= WINDOW_SIZE) window_ready = 1;
    return ESP_OK;
}

static void extract_features(void)
{
    if (!window_ready) return;

    float smv_sum = 0.0f;
    float smv_max = 0.0f;
    float smv_min = 1e10f;
    float ax_max = -1e10f;
    float az_sum = 0.0f;

    for (int i = 0; i < WINDOW_SIZE; i++) {
        int idx = (buffer_head - WINDOW_SIZE + i) % WINDOW_SIZE;
        float ax = accel_buffer[idx][0];
        float ay = accel_buffer[idx][1];
        float az = accel_buffer[idx][2];
        float smv = sqrtf(ax * ax + ay * ay + az * az);

        smv_sum += smv;
        if (smv > smv_max) smv_max = smv;
        if (smv < smv_min) smv_min = smv;
        if (ax > ax_max) ax_max = ax;
        az_sum += az;
    }

    float smv_mean = smv_sum / WINDOW_SIZE;
    float smv_var = 0.0f;
    for (int i = 0; i < WINDOW_SIZE; i++) {
        int idx = (buffer_head - WINDOW_SIZE + i) % WINDOW_SIZE;
        float ax = accel_buffer[idx][0];
        float ay = accel_buffer[idx][1];
        float az = accel_buffer[idx][2];
        float smv = sqrtf(ax * ax + ay * ay + az * az);
        float d = smv - smv_mean;
        smv_var += d * d;
    }
    float smv_std = sqrtf(smv_var / WINDOW_SIZE);

    float mean_az = az_sum / WINDOW_SIZE;
    float peak_to_peak = smv_max - smv_min;

    features[0] = smv_max / 20.0f;
    features[1] = smv_mean / 10.0f;
    features[2] = smv_std / 5.0f;
    features[3] = ax_max / 10.0f;
    features[4] = fabsf(mean_az - 9.8f) / 5.0f;
    features[5] = peak_to_peak / 15.0f;
}

static float relu(float x)
{
    return x > 0.0f ? x : 0.0f;
}

static void softmax(float *x, int n)
{
    float max = x[0];
    for (int i = 1; i < n; i++) {
        if (x[i] > max) max = x[i];
    }
    float sum = 0.0f;
    for (int i = 0; i < n; i++) {
        x[i] = expf(x[i] - max);
        sum += x[i];
    }
    if (sum > 0.0f) {
        for (int i = 0; i < n; i++) {
            x[i] /= sum;
        }
    }
}

static float mlp_inference(void)
{
    float h1[HIDDEN1];
    for (int i = 0; i < HIDDEN1; i++) {
        float sum = b1[i];
        for (int j = 0; j < FEATURE_DIM; j++) {
            sum += features[j] * w1[j * HIDDEN1 + i];
        }
        h1[i] = relu(sum);
    }

    float h2[HIDDEN2];
    for (int i = 0; i < HIDDEN2; i++) {
        float sum = b2[i];
        for (int j = 0; j < HIDDEN1; j++) {
            sum += h1[j] * w2[j * HIDDEN2 + i];
        }
        h2[i] = relu(sum);
    }

    float output[2];
    for (int i = 0; i < 2; i++) {
        output[i] = b3[i];
        for (int j = 0; j < HIDDEN2; j++) {
            output[i] += h2[j] * w3[j * 2 + i];
        }
    }

    softmax(output, 2);
    return output[1];
}

int fall_detector_process(void)
{
    if (!window_ready) return FALL_NONE;

    extract_features();
    float prob = mlp_inference();

    if (prob > 0.7f) {
        confirm_counter++;
        if (confirm_counter >= CONFIRM_THRESH) {
            fall_state = FALL_CONFIRMED;
            return FALL_CONFIRMED;
        }
    } else {
        confirm_counter = 0;
    }

    return FALL_NONE;
}

esp_err_t fall_detector_acknowledge(void)
{
    fall_state = FALL_NONE;
    confirm_counter = 0;
    return ESP_OK;
}

fall_state_t fall_detector_get_state(void)
{
    return fall_state;
}

bool fall_detector_is_falling(void)
{
    return fall_state != FALL_NONE;
}
