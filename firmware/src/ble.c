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
static bool ble_synced;
static bool ble_adv_active;

static const ble_uuid16_t custom_svc_uuid = {
    .u = {.type = BLE_UUID_TYPE_16},
    .value = 0xFFE0
};

static const ble_uuid16_t custom_chr_uuid = {
    .u = {.type = BLE_UUID_TYPE_16},
    .value = 0xFFE1
};

static struct ble_gatt_chr_def characteristic;
static struct ble_gatt_svc_def services[2];
static bool notify_enabled;

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
        case BLE_GATT_ACCESS_OP_WRITE_DSC:
            /* CCCD read/write handled by NimBLE internally via ble_gatts_notify_custom */
            return 0;

        default:
            return 0;
    }
}

static int ble_gap_event_cb(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            ble_adv_active = false;
            if (event->connect.status == 0) {
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
            ble_adv_active = false;
            ble_conn_handle = BLE_HS_CONN_HANDLE_NONE;
            ble_connected = false;
            notify_enabled = false;
            ESP_LOGI(TAG, "Disconnected reason=%d", event->disconnect.reason);
            ble_start_advertising();
            break;

        case BLE_GAP_EVENT_SUBSCRIBE:
            notify_enabled = event->subscribe.cur_notify;
            ESP_LOGI(TAG, "CCCD subscribe changed: notify=%d indicate=%d",
                     event->subscribe.cur_notify, event->subscribe.cur_indicate);
            break;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            ble_adv_active = false;
            ESP_LOGD(TAG, "Adv complete, restarting");
            ble_start_advertising();
            break;

        default:
            break;
    }
    return 0;
}

/* NimBLE host sync callback: controller is ready, safe to start advertising */
static void ble_on_sync(void)
{
    ESP_LOGI(TAG, "NimBLE host synced with controller");
    ble_synced = true;
    ble_start_advertising();
}

/* Reset callback for when host resets */
static void ble_on_reset(int reason)
{
    ESP_LOGW(TAG, "NimBLE host reset reason=%d", reason);
    ble_synced = false;
    ble_adv_active = false;
    notify_enabled = false;
}

esp_err_t ble_start_advertising(void)
{
    if (!ble_synced) {
        ESP_LOGW(TAG, "BLE not synced, skip advertising");
        return ESP_ERR_INVALID_STATE;
    }

    if (ble_adv_active) {
        return ESP_OK;
    }

    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    struct ble_hs_adv_fields adv_fields;
    memset(&adv_fields, 0, sizeof(adv_fields));
    adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    adv_fields.num_uuids16 = 1;
    adv_fields.uuids16 = (ble_uuid16_t *)&custom_svc_uuid;

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

    ble_adv_active = true;
    ESP_LOGI(TAG, "Advertising started");
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
    ble_synced = false;
    ble_adv_active = false;
    notify_enabled = false;

    /* BLE8: Check nimble_port_init return value */
    rc = nimble_port_init();
    if (rc != 0) {
        ESP_LOGE(TAG, "nimble_port_init failed rc=%d", rc);
        return ESP_FAIL;
    }

    ble_svc_gap_init();
    ble_svc_gatt_init();

    memset(&characteristic, 0, sizeof(characteristic));
    characteristic.uuid = &custom_chr_uuid.u;
    characteristic.access_cb = ble_svc_access_cb;
    characteristic.flags = BLE_GATT_CHR_F_NOTIFY;
    characteristic.val_handle = &custom_chr_val_handle;

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

    /* Set sync callback: advertising will start AFTER host syncs with controller */
    ble_hs_cfg.sync_cb = ble_on_sync;
    ble_hs_cfg.reset_cb = ble_on_reset;
    ble_hs_cfg.gatts_register_cb = ble_gatt_svc_register_cb;

    /* Start NimBLE host task (this will trigger ble_on_sync when ready) */
    nimble_port_freertos_init(nimble_port_run);

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

    if (!notify_enabled) {
        ESP_LOGD(TAG, "Notification not enabled, skipping send");
        return ESP_OK;
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
