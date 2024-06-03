/* @file  BLE_OTA_ESP32.c
   @brief this code involves ESP32 in BLE peripheral mode and as a GATT server,
          with OTA service for firmware update.On selecting a .bin file by the client,
          the particular file gets written in packets to the OTA partition currently 
          available on the device.
   @author bheesma-10
*/

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_system.h"

#include "esp_mac.h"


#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_common_api.h"

#include "BLE_Peripheral.h"

#include "esp_ota_ops.h"


static const char *TAG = "Bluedroid-Peripheral";

#define DEVICE_NAME         "ESP32"

#define PROFILE_NUM         1                              //Number of Application Profiles
#define PROFILE_A_APP_ID    0                              //Application Profile ID

#define INFO_INST_ID        0                              //Device Info service id
#define OTA_INST_ID         1                              //OTA service id

#define ADV_CONFIG_FLAG                           (1 << 0)
#define SCAN_RSP_CONFIG_FLAG                      (1 << 1)

/*variables*/
#define GATTS_CHAR_VAL_LEN_MAX 100
#define OTA_VAL_LEN_MAX 256
#define CHAR_DECLARATION_SIZE       (sizeof(uint8_t))
#define REBOOT_DEEP_SLEEP_TIMEOUT 1000

/*ota variables*/
const esp_partition_t *update_partition;
esp_ota_handle_t update_handle;
bool updating = false;
uint16_t num_pkgs_received = 0;
uint16_t packet_size = 0;

#define PREPARE_BUF_MAX_SIZE        1024
typedef struct {
    uint8_t                 *prepare_buf;
    int                     prepare_len;
} prepare_type_env_t;
 
static prepare_type_env_t prepare_write_env;

/*
***************************************************************************************************************
GAP Profile 
***************************************************************************************************************
*/
static uint8_t adv_config_done = 0;

uint16_t conn_id=0;
uint8_t ota_ctrl_val=0;
uint16_t ota_flag=0;

typedef struct {

    uint8_t company_id[2];         //Espressif Semicoductors ID - 0x02E5
    uint8_t mac_address[6];
}manufacturer_data;

manufacturer_data device_data = {.company_id = {0xE5,0x02}, .mac_address = {0}};

static uint8_t custom_manufacturer_data[sizeof(device_data)];

/*128 bits ota service uuid*/
static uint8_t ota_service_uuid[16] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    0xd2, 0xd0, 0x52, 0x4f, 0xa4, 0x74, 0x43, 0xf3, 0x94, 0xb5, 0xb2, 0x97, 0xf3, 0x42, 0x97, 0x6f
};

/*config primary adv data*/
static esp_ble_adv_data_t esp32_ble_adv_config = {
    .set_scan_rsp = false,
    .include_name = false,
    .include_txpower = true,
    .min_interval = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = 0x00,
    .manufacturer_len = sizeof(custom_manufacturer_data), 
    .p_manufacturer_data =  custom_manufacturer_data, 
    .service_data_len = 0,
    .p_service_data = NULL,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

/*config scan response data*/
static esp_ble_adv_data_t esp32_ble_scan_rsp_config = {
    .set_scan_rsp = true,
    .include_name = false,
    .service_uuid_len = sizeof(ota_service_uuid),
    .p_service_uuid = ota_service_uuid,
};

/*configure esp32 advertising connection parameters*/
static esp_ble_adv_params_t esp32_ble_adv_params = {
    .adv_int_min        = 0x0020,
    .adv_int_max        = 0x00A0,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};




/*
 ****************************************************************************************
 GATT Profile
 ****************************************************************************************
 */

/*Attribute table variables*/
uint16_t ble_device_info[IDX_NB_INFO];
uint16_t ble_ota[IDX_NB_OTA];

/*variable holding firmare file data chunks*/
uint8_t ble_ota_data[OTA_VAL_LEN_MAX] = {0};

/*characteristics id's*/
static const uint16_t CTRL_UUID                    = 0xEE01;
static const uint16_t DATA_UUID                    = 0xEE02;

/*primary uuid's of service ,characteristics and CCCD descriptor*/
static const uint16_t primary_service_uuid         = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid   = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

static const uint16_t device_info_service_uuid     = ESP_GATT_UUID_DEVICE_INFO_SVC;
static const uint16_t manuf_name_character_uuid    = ESP_GATT_UUID_MANU_NAME;
static const uint16_t model_num_character_uuid     = ESP_GATT_UUID_MODEL_NUMBER_STR;

/*properties of characteristics*/
static const uint8_t char_prop_read_write_notify   = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t char_prop_write               = ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t char_prop_read                = ESP_GATT_CHAR_PROP_BIT_READ;

/*descriptor and readable data of characteristics*/
static const uint8_t ota_ctrl[] = "ota char";
uint8_t ota_cccd[] = {0x00, 0x00};
#define manuf_name "Espressif"
#define model_num "esp-wroom 32"

/*structure responsible for application profile attributes*/
struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};



static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                        esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);


 // One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT 
static struct gatts_profile_inst profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /*means that the Application Profile is not linked to any client yet*/
    },

};


/* Full Database Description - Used to add attributes into the database */
static esp_gatts_attr_db_t ble_gatt_info_db[IDX_NB_INFO] = {
    [IDX_SVC_INFO] = {{.auto_rsp = ESP_GATT_AUTO_RSP},{.uuid_length = ESP_UUID_LEN_16,.uuid_p = (uint8_t*)&primary_service_uuid,
      .perm = ESP_GATT_PERM_READ,.max_length = sizeof(device_info_service_uuid),.length = sizeof(device_info_service_uuid),.value = (uint8_t*)&device_info_service_uuid}},

    [IDX_CHAR_MANUF_NAME] = {{.auto_rsp = ESP_GATT_AUTO_RSP},{.uuid_length = ESP_UUID_LEN_16,.uuid_p = (uint8_t*)&character_declaration_uuid,
      .perm = ESP_GATT_PERM_READ,.max_length = CHAR_DECLARATION_SIZE,.length = CHAR_DECLARATION_SIZE,.value = (uint8_t*)&char_prop_read}},

    [IDX_CHAR_MANUF_NAME_VAL] = {{.auto_rsp = ESP_GATT_AUTO_RSP},{.uuid_length = ESP_UUID_LEN_16,.uuid_p = (uint8_t*)&manuf_name_character_uuid,
      .perm = ESP_GATT_PERM_READ,.max_length = sizeof(manuf_name),.length = sizeof(manuf_name),.value = (uint8_t*)manuf_name}},

    [IDX_CHAR_MODEL_NUM] = {{.auto_rsp = ESP_GATT_AUTO_RSP},{.uuid_length = ESP_UUID_LEN_16,.uuid_p = (uint8_t*)&character_declaration_uuid,
      .perm = ESP_GATT_PERM_READ,.max_length = CHAR_DECLARATION_SIZE,.length = CHAR_DECLARATION_SIZE,.value = (uint8_t*)&char_prop_read}},

    [IDX_CHAR_MODEL_NUM_VAL] = {{.auto_rsp = ESP_GATT_AUTO_RSP},{.uuid_length = ESP_UUID_LEN_16,.uuid_p = (uint8_t*)&model_num_character_uuid,
      .perm = ESP_GATT_PERM_READ,.max_length = sizeof(model_num),.length = sizeof(model_num),.value = (uint8_t*)model_num}},
}; 


static esp_gatts_attr_db_t ble_gatt_ota_db[IDX_NB_OTA] = {
    [IDX_SVC_OTA] = {{.auto_rsp = ESP_GATT_AUTO_RSP},{.uuid_length = ESP_UUID_LEN_16,.uuid_p = (uint8_t*)&primary_service_uuid,
      .perm = ESP_GATT_PERM_READ,.max_length = sizeof(ota_service_uuid),.length = sizeof(ota_service_uuid),.value = (uint8_t*)ota_service_uuid}},

    [IDX_CHAR_CTRL] = {{.auto_rsp = ESP_GATT_AUTO_RSP},{.uuid_length = ESP_UUID_LEN_16,.uuid_p = (uint8_t*)&character_declaration_uuid,
      .perm = ESP_GATT_PERM_READ,.max_length = CHAR_DECLARATION_SIZE,.length = CHAR_DECLARATION_SIZE,.value = (uint8_t*)&char_prop_read_write_notify}},

    [IDX_CHAR_VAL_CTRL] = {{.auto_rsp = ESP_GATT_AUTO_RSP},{.uuid_length = ESP_UUID_LEN_16,.uuid_p = (uint8_t*)&CTRL_UUID,
      .perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,.max_length = GATTS_CHAR_VAL_LEN_MAX,.length = sizeof(ota_ctrl),.value = (uint8_t*)&ota_ctrl}},

    [IDX_CHAR_CFG_CTRL] = {{.auto_rsp = ESP_GATT_AUTO_RSP},{.uuid_length = ESP_UUID_LEN_16,.uuid_p = (uint8_t*)&character_client_config_uuid,
      .perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,.max_length = sizeof(uint16_t),.length = sizeof(ota_cccd),.value = (uint8_t*)&ota_cccd}},

    [IDX_CHAR_DATA] = {{.auto_rsp = ESP_GATT_AUTO_RSP},{.uuid_length = ESP_UUID_LEN_16,.uuid_p = (uint8_t*)&character_declaration_uuid,
      .perm = ESP_GATT_PERM_READ ,.max_length = CHAR_DECLARATION_SIZE,.length = CHAR_DECLARATION_SIZE,.value = (uint8_t*)&char_prop_write}},

    [IDX_CHAR_VAL_DATA] = {{.auto_rsp = ESP_GATT_AUTO_RSP},{.uuid_length = ESP_UUID_LEN_16,.uuid_p = (uint8_t*)&DATA_UUID,
      .perm = ESP_GATT_PERM_WRITE,.max_length = OTA_VAL_LEN_MAX,.length = sizeof(ble_ota_data),.value = (uint8_t*)ble_ota_data}},
};




static char *esp_key_type_to_str(esp_ble_key_type_t key_type)
{
   char *key_str = NULL;
   switch(key_type) {
    case ESP_LE_KEY_NONE:
        key_str = "ESP_LE_KEY_NONE";
        break;
    case ESP_LE_KEY_PENC:
        key_str = "ESP_LE_KEY_PENC";
        break;
    case ESP_LE_KEY_PID:
        key_str = "ESP_LE_KEY_PID";
        break;
    case ESP_LE_KEY_PCSRK:
        key_str = "ESP_LE_KEY_PCSRK";
        break;
    case ESP_LE_KEY_PLK:
        key_str = "ESP_LE_KEY_PLK";
        break;
    case ESP_LE_KEY_LLK:
        key_str = "ESP_LE_KEY_LLK";
        break;
    case ESP_LE_KEY_LENC:
        key_str = "ESP_LE_KEY_LENC";
        break;
    case ESP_LE_KEY_LID:
        key_str = "ESP_LE_KEY_LID";
        break;
    case ESP_LE_KEY_LCSRK:
        key_str = "ESP_LE_KEY_LCSRK";
        break;
    default:
        key_str = "INVALID BLE KEY TYPE";
        break;

   }

   return key_str;
}

static char *esp_auth_req_to_str(esp_ble_auth_req_t auth_req)
{
   char *auth_str = NULL;
   switch(auth_req) {
    case ESP_LE_AUTH_NO_BOND:
        auth_str = "ESP_LE_AUTH_NO_BOND";
        break;
    case ESP_LE_AUTH_BOND:
        auth_str = "ESP_LE_AUTH_BOND";
        break;
    case ESP_LE_AUTH_REQ_MITM:
        auth_str = "ESP_LE_AUTH_REQ_MITM";
        break;
    case ESP_LE_AUTH_REQ_BOND_MITM:
        auth_str = "ESP_LE_AUTH_REQ_BOND_MITM";
        break;
    case ESP_LE_AUTH_REQ_SC_ONLY:
        auth_str = "ESP_LE_AUTH_REQ_SC_ONLY";
        break;
    case ESP_LE_AUTH_REQ_SC_BOND:
        auth_str = "ESP_LE_AUTH_REQ_SC_BOND";
        break;
    case ESP_LE_AUTH_REQ_SC_MITM:
        auth_str = "ESP_LE_AUTH_REQ_SC_MITM";
        break;
    case ESP_LE_AUTH_REQ_SC_MITM_BOND:
        auth_str = "ESP_LE_AUTH_REQ_SC_MITM_BOND";
        break;
    default:
        auth_str = "INVALID BLE AUTH REQ";
        break;
   }

   return auth_str;
}

/**
 * @brief Lists all bonded devices(devices with shared pairing keys..........)
 */

static void show_bonded_devices(void)
{
    int dev_num = esp_ble_get_bond_device_num();

    esp_ble_bond_dev_t *dev_list = (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
    esp_ble_get_bond_device_list(&dev_num, dev_list);
    // ESP_LOGI(TAG, "Bonded devices number : %d\n", dev_num);

    // ESP_LOGI(TAG, "Bonded devices list : %d\n", dev_num);
    for (int i = 0; i < dev_num; i++) {
        esp_log_buffer_hex(TAG, (void *)dev_list[i].bd_addr, sizeof(esp_bd_addr_t));
    }

    free(dev_list);
}

/**
 * @brief removes all bonded devices(devices with shared pairing keys..........)
 */
static void __attribute__((unused)) remove_all_bonded_devices(void)
{
    int dev_num = esp_ble_get_bond_device_num();

    esp_ble_bond_dev_t *dev_list = (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
    esp_ble_get_bond_device_list(&dev_num, dev_list);
    for (int i = 0; i < dev_num; i++) {
        esp_ble_remove_bond_device(dev_list[i].bd_addr);
    }

    free(dev_list);
}

/**
 * @brief GAP event handler
 * @param event : Event type
 * @param param : Point to callback parameter, currently is union type
 */

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    ESP_LOGV(TAG, "GAP_EVT, event %d\n", event);

    switch (event) {
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
        if (adv_config_done == 0){
            esp_ble_gap_start_advertising(&esp32_ble_adv_params);
        }
        break;
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~ADV_CONFIG_FLAG);
        if (adv_config_done == 0){
            esp_ble_gap_start_advertising(&esp32_ble_adv_params);
        }
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        //advertising start complete event to indicate advertising start successfully or failed
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "advertising start failed, error status = %x", param->adv_start_cmpl.status);
            break;
        }
        // ESP_LOGI(TAG, "advertising start success");
        break;
    case ESP_GAP_BLE_PASSKEY_REQ_EVT:                           /* passkey request event */
        // ESP_LOGI(TAG, "ESP_GAP_BLE_PASSKEY_REQ_EVT");
        /* Call the following function to input the passkey which is displayed on the remote device */
        //esp_ble_passkey_reply(heart_rate_profile_tab[HEART_PROFILE_APP_IDX].remote_bda, true, 0x00);
        break;
    case ESP_GAP_BLE_OOB_REQ_EVT: {
        // ESP_LOGI(TAG, "ESP_GAP_BLE_OOB_REQ_EVT");
        uint8_t tk[16] = {1}; //If you paired with OOB, both devices need to use the same tk
        esp_ble_oob_req_reply(param->ble_security.ble_req.bd_addr, tk, sizeof(tk));
        break;
    }
    case ESP_GAP_BLE_LOCAL_IR_EVT:                               /* BLE local IR event */
        // ESP_LOGI(TAG, "ESP_GAP_BLE_LOCAL_IR_EVT");
        break;
    case ESP_GAP_BLE_LOCAL_ER_EVT:                               /* BLE local ER event */
        // ESP_LOGI(TAG, "ESP_GAP_BLE_LOCAL_ER_EVT");
        break;
    case ESP_GAP_BLE_NC_REQ_EVT:
        /* The app will receive this evt when the IO has DisplayYesNO capability and the peer device IO also has DisplayYesNo capability.
        show the passkey number to the user to confirm it with the number displayed by peer device. */
        esp_ble_confirm_reply(param->ble_security.ble_req.bd_addr, true);
        // ESP_LOGI(TAG, "ESP_GAP_BLE_NC_REQ_EVT, the passkey Notify number:%d", param->ble_security.key_notif.passkey);
        break;
    case ESP_GAP_BLE_SEC_REQ_EVT:
        /* send the positive(true) security response to the peer device to accept the security request.
        If not accept the security request, should send the security response with negative(false) accept value*/
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;
    case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:  ///the app will receive this evt when the IO  has Output capability and the peer device IO has Input capability.
        ///show the passkey number to the user to input it in the peer device.
        // ESP_LOGI(TAG, "The passkey Notify number:%06d", param->ble_security.key_notif.passkey);
        break;
    case ESP_GAP_BLE_KEY_EVT:
        //shows the ble key info share with peer device to the user.
        // ESP_LOGI(TAG, "key type = %s", esp_key_type_to_str(param->ble_security.ble_key.key_type));
        break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT: {
        esp_bd_addr_t bd_addr;
        memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
       
        ESP_LOGI(TAG, "remote BD_ADDR: %08x%04x",\
                (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
                (bd_addr[4] << 8) + bd_addr[5]);
    
        // ESP_LOGI(TAG, "address type = %d", param->ble_security.auth_cmpl.addr_type);
        // ESP_LOGI(TAG, "pair status = %s",param->ble_security.auth_cmpl.success ? "success" : "fail");
        if(!param->ble_security.auth_cmpl.success) {
            // ESP_LOGI(TAG, "fail reason = 0x%x",param->ble_security.auth_cmpl.fail_reason);
        } else {
            // ESP_LOGI(TAG, "auth mode = %s",esp_auth_req_to_str(param->ble_security.auth_cmpl.auth_mode));
        }
        show_bonded_devices();
        break;
    }
    case ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT: {
        ESP_LOGD(TAG, "ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT status = %d", param->remove_bond_dev_cmpl.status);
        // ESP_LOGI(TAG, "ESP_GAP_BLE_REMOVE_BOND_DEV");
        // ESP_LOGI(TAG, "-----ESP_GAP_BLE_REMOVE_BOND_DEV----");
        esp_log_buffer_hex(TAG, (void *)param->remove_bond_dev_cmpl.bd_addr, sizeof(esp_bd_addr_t));
        // ESP_LOGI(TAG, "------------------------------------");
        break;
    }
    case ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT:
        if (param->local_privacy_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(TAG, "config local privacy failed, error status = %x", param->local_privacy_cmpl.status);
            break;
        }

        /*******************************MAC Address*******************************/
        
        ESP_ERROR_CHECK(esp_efuse_mac_get_default(device_data.mac_address));
        // ESP_LOGI(TAG,"Base Mac address");

        for(int mac_size=0;mac_size<sizeof(device_data.mac_address);mac_size++){
            // ESP_LOGI(TAG,"%.2x:",device_data.mac_address[mac_size]);
        }


        /****************************prepare Manufacturer data for ADV_PDU***********************************/
        int data_index=0;

                    /*company id*/
        custom_manufacturer_data[data_index]=device_data.company_id[0];
        data_index++;
        custom_manufacturer_data[data_index]=device_data.company_id[1];
        data_index++;

                   /*MAC Address*/
        for(int manuf_data=0;manuf_data<sizeof(device_data.mac_address);manuf_data++,data_index++){
            custom_manufacturer_data[data_index]=device_data.mac_address[manuf_data];
        }

        esp_err_t ret = esp_ble_gap_config_adv_data(&esp32_ble_adv_config);
        if (ret){
            ESP_LOGE(TAG, "config adv data failed, error code = %x", ret);
        }else{
            adv_config_done |= ADV_CONFIG_FLAG;
        }

        ret = esp_ble_gap_config_adv_data(&esp32_ble_scan_rsp_config);
        if (ret){
            ESP_LOGE(TAG, "config adv data failed, error code = %x", ret);
        }else{
            adv_config_done |= SCAN_RSP_CONFIG_FLAG;
        }

        break;
    default:
        break;
    }
}


void prepare_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    // ESP_LOGI(TAG, "prepare write, handle = %d, value len = %d", param->write.handle, param->write.len);
    esp_gatt_status_t status = ESP_GATT_OK;
    if (prepare_write_env->prepare_buf == NULL) {
        prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE * sizeof(uint8_t));
        prepare_write_env->prepare_len = 0;
        if (prepare_write_env->prepare_buf == NULL) {
            ESP_LOGE(TAG, "%s, Gatt_server prep no mem", __func__);
            status = ESP_GATT_NO_RESOURCES;
        }
    } else {
        if(param->write.offset > PREPARE_BUF_MAX_SIZE) {
            status = ESP_GATT_INVALID_OFFSET;
        } else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE) {
            status = ESP_GATT_INVALID_ATTR_LEN;
        }
    }
    /*send response when param->write.need_rsp is true */
    if (param->write.need_rsp){
        esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
        if (gatt_rsp != NULL){
            gatt_rsp->attr_value.len = param->write.len;
            gatt_rsp->attr_value.handle = param->write.handle;
            gatt_rsp->attr_value.offset = param->write.offset;
            gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
            esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
            if (response_err != ESP_OK){
               ESP_LOGE(TAG, "Send response error");
            }
            free(gatt_rsp);
        }else{
            ESP_LOGE(TAG, "%s, malloc failed", __func__);
        }
    }
    if (status != ESP_GATT_OK){
        return;
    }
    memcpy(prepare_write_env->prepare_buf + param->write.offset,
           param->write.value,
           param->write.len);
    prepare_write_env->prepare_len += param->write.len;

}



/**
 * @brief GATT profile event handler(gets called for every individual profiles.....)
 * @param event : Event type
 * @param gatts_if : GATT server access interface, normally
 *                   different gatts_if correspond to different profile
 * @param param : Point to callback parameter, currently is union type
 */

static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                        esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    ESP_LOGV(TAG, "event = %x\n",event);
    switch (event) {
        case ESP_GATTS_REG_EVT:
            esp_ble_gap_set_device_name(DEVICE_NAME);
            esp_ble_gap_config_local_privacy(true);
            /*create device info attribute table first*/
            esp_ble_gatts_create_attr_tab(ble_gatt_info_db,gatts_if,IDX_NB_INFO,INFO_INST_ID);            break;
        case ESP_GATTS_READ_EVT:
            // ESP_LOGI(TAG,"Read Event");
            break;
        case ESP_GATTS_WRITE_EVT:
            if (!param->write.is_prep){

                // ESP_LOGI(TAG,"attribute handle:%d, len:%d",param->write.handle,param->write.len);

                if(param->write.handle == ble_ota[IDX_CHAR_CFG_CTRL] && param->write.len==2){
                    esp_log_buffer_hex(TAG,param->write.value,2);
                    ota_flag = param->write.value[0] | (param->write.value[1]<<8);
                }
                else if(param->write.handle == ble_ota[IDX_CHAR_VAL_CTRL] && param->write.len==1){
                    esp_log_buffer_hex(TAG,param->write.value,1);
                    ota_ctrl_val = param->write.value[0];
                }
                else if(param->write.handle == ble_ota[IDX_CHAR_VAL_DATA] && updating==false){
                    esp_log_buffer_hex(TAG,param->write.value,2);
                    packet_size = param->write.value[0] | (param->write.value[1]<<8);
                }
                else if(param->write.handle == ble_ota[IDX_CHAR_VAL_DATA]){
                    memset(ble_ota_data,0,sizeof(ble_ota_data));
                    memcpy(ble_ota_data,param->write.value,packet_size);
                    if(updating==true){
                      //write received data to partition
                      esp_err_t err = esp_ota_write(update_handle, (const void *)ble_ota_data,packet_size);
                      if (err != ESP_OK) {
                         ESP_LOGE(TAG, "esp_ota_write failed (%s)!",
                         esp_err_to_name(err));
                      }

                      num_pkgs_received++;
                      // ESP_LOGI(TAG, "Received packet %d", num_pkgs_received);
                    }

                }
                else{
                    //nop
                }
                
            }
            else{
                /* handle prepare write */
                prepare_write_event_env(gatts_if, &prepare_write_env, param);
            }

            break;
        case ESP_GATTS_EXEC_WRITE_EVT:
            break;
        case ESP_GATTS_MTU_EVT:
        /*
         *************************************************************************************************
         *********event triggered after client request for updating MTU(set max to 350 for now)***********
         *************************************************************************************************
        */
            // ESP_LOGI(TAG,"for connection id:%u",param->mtu.conn_id);
            // ESP_LOGI(TAG,"requested MTU size from client:%u",param->mtu.mtu);
            break;
        case ESP_GATTS_CONF_EVT:
            break;
        case ESP_GATTS_UNREG_EVT:
            break;
        case ESP_GATTS_DELETE_EVT:
            break;
        case ESP_GATTS_START_EVT:
            break;
        case ESP_GATTS_STOP_EVT:
            break;

        case ESP_GATTS_CONNECT_EVT:
            // ESP_LOGI(TAG, "ESP_GATTS_CONNECT_EVT");
             //start security connect with peer device when receive the connect event sent by the master 
            // esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);
            break;
        case ESP_GATTS_DISCONNECT_EVT:
            // ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x", param->disconnect.reason);
            /* start advertising again when missing the connect */
            esp_ble_gap_start_advertising(&esp32_ble_adv_params);
            break;
        case ESP_GATTS_OPEN_EVT:
            break;
        case ESP_GATTS_CANCEL_OPEN_EVT:
            break;
        case ESP_GATTS_CLOSE_EVT:
            break;
        case ESP_GATTS_LISTEN_EVT:
            break;
        case ESP_GATTS_CONGEST_EVT:
            break;
        case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
            // ESP_LOGI(TAG, "The number handle = %x",param->add_attr_tab.num_handle);
            if (param->create.status == ESP_GATT_OK){
                if(param->add_attr_tab.num_handle == IDX_NB_INFO) {
                    memcpy(ble_device_info, param->add_attr_tab.handles,sizeof(ble_device_info));
                    esp_ble_gatts_start_service(ble_device_info[IDX_SVC_INFO]);

                    for(int idx=0;idx<IDX_NB_INFO;idx++){
                        // ESP_LOGI(TAG,"attribute table:%d",ble_device_info[idx]);
                    }

                    /*create ota attribute table next*/
                    esp_ble_gatts_create_attr_tab(ble_gatt_ota_db, gatts_if,IDX_NB_OTA, OTA_INST_ID);
                    
                }

                if(param->add_attr_tab.num_handle == IDX_NB_OTA){
                    memcpy(ble_ota,param->add_attr_tab.handles,sizeof(ble_ota));
                    esp_ble_gatts_start_service(ble_ota[IDX_SVC_OTA]);

                    for(int idx=0;idx<IDX_NB_OTA;idx++){
                        // ESP_LOGI(TAG,"attribute table:%d",ble_ota[idx]);
                    }

                }
                

            }else{
                ESP_LOGE(TAG, " Create attribute table failed, error code = %x", param->create.status);
            }
        break;
    }

        default:
           break;
    }
}


/**
 * @brief GATTS event handler for all application profiles
 * @param event : Event type
 * @param gatts_if : GATT server access interface, normally
 *                   different gatts_if correspond to different profile
 * @param param : Point to callback parameter, currently is union type
 */

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                                esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            profile_tab[PROFILE_A_APP_ID].gatts_if = gatts_if;
        } else {
            // ESP_LOGI(TAG, "Reg app failed, app_id %04x, status %d\n",
                    // param->reg.app_id,
                    // param->reg.status;
            return;
        }
    }

    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gatts_if == profile_tab[idx].gatts_if) {
                if (profile_tab[idx].gatts_cb) {
                    profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);

}


/*
*******************************************************************************************************
@brief task for OTA characteristic value check and taking respective action
@note  checks for next available partition to write too and sets that partition to boot from after restart. 
*******************************************************************************************************
*/
void ota_main(void *arg){

    esp_err_t err,ack;

    while(1){

        /*check ota control characteristics descriptor*/
        if(ota_flag == 0x0001){
            
            uint8_t ack_val=0;
            if(ota_ctrl_val == OTA_REQUEST){
                    // get the next free OTA partition
                    update_partition = esp_ota_get_next_update_partition(NULL);
                    // start the ota update
                    err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES,&update_handle);
                    if (err != ESP_OK) {
                       ESP_LOGE(TAG, "esp_ota_begin failed (%s)",
                       esp_err_to_name(err));
                       esp_ota_abort(update_handle);
                       ack_val = OTA_REQUEST_NAK;
                    } 
                    else {
                       ack_val = OTA_REQUEST_ACK;
                       updating = true;

                       // retrieve the packet size from OTA data
                       // ESP_LOGI(TAG, "Packet size is: %d", packet_size);

                       num_pkgs_received = 0;
                    }

                    ack = esp_ble_gatts_send_indicate(profile_tab[PROFILE_A_APP_ID].gatts_if,conn_id,ble_ota[IDX_CHAR_VAL_CTRL],sizeof(ack_val),(uint8_t*)&ack_val,false);
                    if(ack==0){
                       // ESP_LOGI(TAG,"sent data:%d",ack);
                       // ESP_LOGI(TAG,"conn_id:%d",conn_id);
                    }

                    // ESP_LOGI(TAG, "OTA request acknowledgement has been sent.");

                    ota_ctrl_val = 0xff;

            }
            else if(ota_ctrl_val == OTA_DONE){
                    updating = false;

                    // end the OTA and start validation
                    err = esp_ota_end(update_handle);
                    if (err != ESP_OK) {
                          if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
                             ESP_LOGE(TAG,"Image validation failed, image is corrupted!");
                          } 
                          else {
                             ESP_LOGE(TAG, "esp_ota_end failed (%s)!",
                             esp_err_to_name(err));
                          }
                    } 
                    else {
                          // select the new partition for the next boot
                          err = esp_ota_set_boot_partition(update_partition);
                          if (err != ESP_OK) {
                             ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!",
                             esp_err_to_name(err));
                          }
                    }

                    // set the control value
                    if (err != ESP_OK) {
                       ack_val = OTA_DONE_NAK;
                    } else {
                       ack_val = OTA_DONE_ACK;
                    }

                    ack = esp_ble_gatts_send_indicate(profile_tab[PROFILE_A_APP_ID].gatts_if,conn_id,ble_ota[IDX_CHAR_VAL_CTRL],sizeof(ack_val),(uint8_t*)&ack_val,false);
                    if(ack==0){
                       // ESP_LOGI(TAG,"sent data:%d",ack);
                       // ESP_LOGI(TAG,"conn_id:%d",conn_id);
                    }

                    // ESP_LOGI(TAG, "OTA DONE acknowledgement has been sent.");

                    // restart the ESP to finish the OTA
                    if (err == ESP_OK) {
                       // ESP_LOGI(TAG, "Preparing to restart!");
                       vTaskDelay(pdMS_TO_TICKS(REBOOT_DEEP_SLEEP_TIMEOUT));
                       esp_restart();
                    }

            }  
        }
        else{
            //nop
        }

        vTaskDelay(100/portTICK_PERIOD_MS);

    }
}


/*
**********************************************************************************************
@brief main application code
**********************************************************************************************
*/
void app_main(void)
{
/*
***********************************************************************************************
**********************************firmware image handling**************************************
***********************************************************************************************
*/

/*get current running partition*/
    esp_partition_t* app_partition = esp_ota_get_running_partition();

    switch(app_partition->address){
       // case 0x10000: ESP_LOGI(TAG,"running factory app..............................................");
                     break;
       // case 0x110000:ESP_LOGI(TAG,"running ota_0 app................................................");
                     break;
       // case 0x210000:ESP_LOGI(TAG,"running ota_1 app................................................");
                     break;
       default:      
                     ESP_LOGI(TAG,"running unknown app..............................................");
                     break;
    }

   /*tag image as valid on first boot and indicate tag on further reboots*/
    esp_err_t ret;
    esp_ota_img_states_t ota_app_state;
    ret = esp_ota_get_state_partition(app_partition,&ota_app_state);
    if(ret==ESP_OK){
        if(ota_app_state==ESP_OTA_IMG_UNDEFINED){   //state is set by esp_ota_set_boot_partition() function if CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE option is not enabled.
            if( esp_ota_mark_app_valid_cancel_rollback()==ESP_OK){
                ESP_LOGI(TAG,"application is valid and running............................");
            }
            else{
                esp_ota_mark_app_invalid_rollback_and_reboot();
                ESP_LOGI(TAG,"application invalid,rolling back to last valid application.............................");
            }
        }
        else if(ota_app_state==ESP_OTA_IMG_VALID){
            ESP_LOGI(TAG,"running image already verified...........................");
        }
        else{
            //nop
        }

    }
    

/*
***********************************************************************************************
**************************BLE initialization and security setting******************************
***********************************************************************************************
*/

    

    // Initialize NVS.
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg); 
    if (ret) {
        // ESP_LOGE(TAG, "%s init controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        // ESP_LOGE(TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    /* Bluetooth stack should be up and running after below commands*/
    // ESP_LOGI(TAG, "%s init bluetooth", __func__);
    ret = esp_bluedroid_init_with_cfg(&bluedroid_cfg);
    if (ret) {
        // ESP_LOGE(TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        // ESP_LOGE(TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    /*The functions gatts_event_handler() and gap_event_handler() handle all the events that are 
    pushed to the application from the BLE stack.*/
    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret){
        // ESP_LOGE(TAG, "gatts register error, error code = %x", ret);
        return;
    }
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret){
        // ESP_LOGE(TAG, "gap register error, error code = %x", ret);
        return;
    }

    ret = esp_ble_gatts_app_register(PROFILE_A_APP_ID);
    if (ret){
        // ESP_LOGE(TAG, "gatts app register error, error code = %x", ret);
        return;
    }

    /* set the security iocap & auth_req & key size & init key response key parameters to the stack*/
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;     //bonding with peer device after authentication
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;           //set the IO capability to No output No input
    uint8_t key_size = 16;      //the key size should be 7~16 bytes
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    //set static passkey
    uint32_t passkey = 123456;
    uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_DISABLE;
    uint8_t oob_support = ESP_BLE_OOB_DISABLE;
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey, sizeof(uint32_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_OOB_SUPPORT, &oob_support, sizeof(uint8_t));
    /* If your BLE device acts as a Slave, the init_key means you hope which types of key of the master should distribute to you,
    and the response key means which key you can distribute to the master;
    If your BLE device acts as a master, the response key means you hope which types of key of the slave should distribute to you,
    and the init key means which key you can distribute to the slave. */
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

     // Just show how to clear all the bonded devices
     // * Delay 30s, clear all the bonded devices
     // *
     // vTaskDelay(30000 / portTICK_PERIOD_MS);
     // remove_all_bonded_devices();



    /*create task for ota*/
    xTaskCreate(&ota_main,"OTA over BLE",10000,NULL,0,NULL);

    
}
