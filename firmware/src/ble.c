#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "ble.h"

#define BLE_DEVICE_NAME "SmartBand"

static const char *TAG = "ble";

static uint16_t custom_svc_handle;
static uint16_t custom_chr_val_handle;
static uint16_t ble_conn_handle = BLE_HS_CONN_HANDLE_NONE;
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

/* BLE7: Static GATT definitions (not on stack) */
static struct ble_gatt_dsc_def cccd_dsc;
static struct ble_gatt_chr_def characteristic;
static struct ble_gatt_svc_def services[2];

esp_err_t ble_start_advertising(void);

/* BLE11: Access callback supporting CCCD and characteristic read/write */
static int ble_svc_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                             struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            ESP_LOGD(TAG, "CHR read conn_handle=%d", conn_handle);
            /* Notification-only characteristic; return zero-length OK */
            return 0;

        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            ESP_LOGD(TAG, "CHR write conn_handle=%d len=%d",
                     conn_handle, OS_MBUF_PKTLEN(ctxt->om));
            return 0;

        case BLE_GATT_ACCESS_OP_READ_DSC:
            /* BLE3: CCCD read - NimBLE handles value internally */
            return 0;

        case BLE_GATT_ACCESS_OP_WRITE_DSC:
            /* BLE3: CCCD write - NimBLE handles subscription state internally */
            return 0;

        default:
            return 0;
    }
}

static int ble_gap_event_cb(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                /* BLE9: Track real connection handle */
                ble_conn_handle = event->connect.conn_handle;
                ble_connected = true;
                ESP_LOGI(TAG, "Connected conn_handle=%d", ble_conn_handle);
            } else {
                ble_conn_handle = BLE_HS_CONN_HANDLE_NONE;
                ble_connected = false;
                ESP_LOGE(TAG, "Connect failed status=%d", event->connect.status);
            }
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            ble_conn_handle = BLE_HS_CONN_HANDLE_NONE;
            ble_connected = false;
            ESP_LOGI(TAG, "Disconnected reason=%d", event->disconnect.reason);
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

esp_err_t ble_start_advertising(void)
{
    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    struct ble_hs_adv_fields adv_fields;
    memset(&adv_fields, 0, sizeof(adv_fields));
    adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    /* BLE4: Advertise service UUID */
    adv_fields.num_uuids128 = 1;
    adv_fields.uuids128 = &custom_svc_uuid;

    adv_fields.name = (uint8_t *)BLE_DEVICE_NAME;
    adv_fields.name_len = strlen(BLE_DEVICE_NAME);
    adv_fields.name_is_complete = 1;

    int rc = ble_gap_adv_set_fields(&adv_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "adv_set_fields failed rc=%d", rc);
        return ESP_FAIL;
    }

    struct ble_hs_adv_fields rsp_fields;
    memset(&rsp_fields, 0, sizeof(rsp_fields));
    rsp_fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "adv_rsp_set_fields failed rc=%d", rc);
        return ESP_FAIL;
    }

    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                           &adv_params, ble_gap_event_cb, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "adv_start failed rc=%d", rc);
        return ESP_FAIL;
    }

    return ESP_OK;
}

static void ble_gatt_svc_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    switch (ctxt->op) {
        case BLE_GATT_REGISTER_OP_SVC:
            custom_svc_handle = ctxt->svc.handle;
            ESP_LOGI(TAG, "Service registered handle=%d", custom_svc_handle);
            break;
        case BLE_GATT_REGISTER_OP_CHR:
            custom_chr_val_handle = ctxt->chr.val_handle;
            ESP_LOGI(TAG, "Characteristic registered val_handle=%d def_handle=%d",
                     custom_chr_val_handle, ctxt->chr.def_handle);
            break;
        default:
            break;
    }
}

esp_err_t ble_init(void)
{
    int rc;

    ble_connected = false;
    ble_conn_handle = BLE_HS_CONN_HANDLE_NONE;

    /* BLE8: Check nimble_port_init return value */
    rc = nimble_port_init();
    if (rc != 0) {
        ESP_LOGE(TAG, "nimble_port_init failed rc=%d", rc);
        return ESP_FAIL;
    }

    ble_svc_gap_init();
    ble_svc_gatt_init();

    /* BLE3: CCCD descriptor for notification subscription */
    memset(&cccd_dsc, 0, sizeof(cccd_dsc));
    cccd_dsc.uuid = BLE_UUID16_DECLARE(0x2902);
    cccd_dsc.att_flags = BLE_ATT_F_READ | BLE_ATT_F_WRITE;
    cccd_dsc.access_cb = ble_svc_access_cb;

    /* BLE7: Static characteristic definition */
    memset(&characteristic, 0, sizeof(characteristic));
    characteristic.uuid = &custom_chr_uuid.u;
    characteristic.access_cb = ble_svc_access_cb;
    characteristic.flags = BLE_GATT_CHR_F_NOTIFY;
    characteristic.val_handle = &custom_chr_val_handle;
    characteristic.descriptors = &cccd_dsc;

    /* BLE7: Static service definition */
    memset(&services, 0, sizeof(services));
    services[0].type = BLE_GATT_SVC_TYPE_PRIMARY;
    services[0].uuid = &custom_svc_uuid.u;
    services[0].characteristics = &characteristic;
    /* services[1] is zero-initialized -> end marker */

    /* BLE8: Check return values */
    rc = ble_gatts_count_cfg(services);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gatts_count_cfg failed rc=%d", rc);
        return ESP_FAIL;
    }

    rc = ble_gatts_add_svcs(services);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gatts_add_svcs failed rc=%d", rc);
        return ESP_FAIL;
    }

    ble_hs_cfg.gatts_register_cb = ble_gatt_svc_register_cb;

    rc = ble_start_advertising();
    if (rc != 0) {
        ESP_LOGW(TAG, "Initial advertising failed rc=%d (will retry on disconnect)", rc);
        /* Non-fatal: will retry on disconnect event */
    }

    xTaskCreate((TaskFunction_t)nimble_port_run, "nimble", 4096, NULL, 5, NULL);

    return ESP_OK;
}

esp_err_t ble_send_data(const ble_data_packet_t *packet)
{
    if (!ble_connected) {
        return ESP_ERR_INVALID_STATE;
    }
    if (packet == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* BLE9: Use sizeof(ble_data_packet_t) instead of BLE_PACKET_SIZE */
    struct os_mbuf *om = ble_hs_mbuf_from_flat(packet, sizeof(ble_data_packet_t));
    if (om == NULL) {
        ESP_LOGE(TAG, "mbuf alloc failed");
        return ESP_ERR_NO_MEM;
    }

    /* BLE2: Use ble_gatts_notify_custom (NimBLE server API) */
    /* BLE9: Use tracked conn_handle instead of hardcoded 0 */
    int rc = ble_gatts_notify_custom(ble_conn_handle, custom_chr_val_handle, om);
    if (rc != 0) {
        ESP_LOGE(TAG, "notify failed rc=%d", rc);
        return ESP_FAIL;
    }

    return ESP_OK;
}

bool ble_is_connected(void)
{
    return ble_connected;
}
