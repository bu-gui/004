#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#define BLE_DEVICE_NAME "SmartBand"
#define BLE_PACKET_SIZE 16

typedef struct {
    uint8_t heart_rate;
    uint8_t spo2;
    uint16_t steps;
    uint16_t calories;
    uint8_t sleep_stage;
    uint8_t flags;
} __attribute__((packed)) ble_data_packet_t;

static uint16_t custom_svc_handle;
static uint16_t custom_chr_handle;
static bool ble_connected;

static const ble_uuid128_t custom_svc_uuid = {
    .u = {.type = BLE_UUID_TYPE_128},
    .value = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
              0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10}
};

static const ble_uuid128_t custom_chr_uuid = {
    .u = {.type = BLE_UUID_TYPE_128},
    .value = {0x11, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
              0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10}
};

static void ble_start_advertising(void);

static int ble_gap_event_cb(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                ble_connected = true;
            } else {
                ble_connected = false;
            }
            break;
        case BLE_GAP_EVENT_DISCONNECT:
            ble_connected = false;
            ble_start_advertising();
            break;
        case BLE_GAP_EVENT_ADV_COMPLETE:
            ble_start_advertising();
            break;
        default:
            break;
    }
    return 0;
}

void ble_start_advertising(void)
{
    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    struct ble_hs_adv_fields adv_fields;
    memset(&adv_fields, 0, sizeof(adv_fields));
    adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    adv_fields.name = (uint8_t *)BLE_DEVICE_NAME;
    adv_fields.name_len = strlen(BLE_DEVICE_NAME);
    adv_fields.name_is_complete = 1;

    ble_gap_adv_set_fields(&adv_fields);

    struct ble_hs_adv_fields rsp_fields;
    memset(&rsp_fields, 0, sizeof(rsp_fields));
    rsp_fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    ble_gap_adv_rsp_set_fields(&rsp_fields);

    ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params,
                      ble_gap_event_cb, NULL);
}

static int ble_svc_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                             struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    return 0;
}

static void ble_gatt_svc_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    switch (ctxt->op) {
        case BLE_GATT_REGISTER_OP_SVC:
            custom_svc_handle = ctxt->svc.handle;
            break;
        case BLE_GATT_REGISTER_OP_CHR:
            custom_chr_handle = ctxt->chr.def_handle;
            break;
        default:
            break;
    }
}

void ble_init(void)
{
    ble_connected = false;

    nimble_port_init();

    ble_svc_gap_init();
    ble_svc_gatt_init();

    ble_uuid128_t svc_uuid = custom_svc_uuid;

    struct ble_gatt_chr_def characteristic;
    memset(&characteristic, 0, sizeof(characteristic));
    characteristic.uuid = &custom_chr_uuid.u;
    characteristic.access_cb = ble_svc_access_cb;
    characteristic.flags = BLE_GATT_CHR_F_NOTIFY;
    characteristic.val_handle = &custom_chr_handle;

    struct ble_gatt_svc_def services[] = {
        {
            .type = BLE_GATT_SVC_TYPE_PRIMARY,
            .uuid = &svc_uuid.u,
            .characteristics = &characteristic,
        },
        { 0 },
    };

    ble_gatts_count_cfg(services);
    ble_gatts_add_svcs(services);

    ble_hs_cfg.gatts_register_cb = ble_gatt_svc_register_cb;

    ble_start_advertising();

    xTaskCreate((TaskFunction_t)nimble_port_run, "nimble", 4096, NULL, 5, NULL);
}

void ble_send_data(ble_data_packet_t *packet)
{
    if (!ble_connected) return;

    uint8_t data[BLE_PACKET_SIZE];
    memset(data, 0, BLE_PACKET_SIZE);
    memcpy(data, packet, sizeof(ble_data_packet_t));

    struct os_mbuf *om = ble_hs_mbuf_from_flat(data, BLE_PACKET_SIZE);
    if (om) {
        ble_gattc_notify_custom(0, custom_chr_handle, om);
    }
}

bool ble_is_connected(void)
{
    return ble_connected;
}
