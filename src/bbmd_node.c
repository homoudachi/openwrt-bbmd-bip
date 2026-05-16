#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "bacnet/config.h"
#include "bacnet/apdu.h"
#include "bacnet/version.h"
#include "bacnet/basic/services.h"
#include "bacnet/datalink/dlenv.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/object/device.h"

#include "bbmd_config.h"
#include "bbmd_node.h"

static bool node_initialized = false;

static void init_service_handlers(void)
{
    Device_Init(NULL);

    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS, handler_who_has);
    apdu_set_unrecognized_service_handler_handler(
        handler_unrecognized_service);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROP_MULTIPLE, handler_read_property_multiple);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_WRITE_PROPERTY, handler_write_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_WRITE_PROP_MULTIPLE,
        handler_write_property_multiple);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_REINITIALIZE_DEVICE, handler_reinitialize_device);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION, handler_timesync_utc);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION, handler_timesync);
}

int bbmd_node_init(bbmd_config_t *config)
{
    if (!config || !config->node.enabled) {
        return -1;
    }

    setenv("BACNET_DATALINK", "BSC", 1);

    if (config->node.ca_cert) {
        setenv("BACNET_SC_ISSUER_1_CERTIFICATE_FILE",
               config->node.ca_cert, 1);
    }
    if (config->node.tls_cert) {
        setenv("BACNET_SC_OPERATIONAL_CERTIFICATE_FILE",
               config->node.tls_cert, 1);
    }
    if (config->node.tls_key) {
        setenv("BACNET_SC_OPERATIONAL_CERTIFICATE_PRIVATE_KEY_FILE",
               config->node.tls_key, 1);
    }

    if (config->node.primary_hub) {
        setenv("BACNET_SC_PRIMARY_HUB_URI", config->node.primary_hub, 1);
    }
    if (config->node.failover_count > 0 && config->node.failover_hubs && config->node.failover_hubs[0]) {
        setenv("BACNET_SC_FAILOVER_HUB_URI", config->node.failover_hubs[0], 1);
    }

    address_init();
    init_service_handlers();

    Device_Set_Object_Instance_Number(config->globals.device_instance);
    if (config->globals.device_name) {
        Device_Object_Name_ANSI_Init(config->globals.device_name);
    }
    Device_Set_Vendor_Identifier(config->globals.vendor_id);
    if (config->globals.model_name) {
        Device_Set_Model_Name(config->globals.model_name,
                              strlen(config->globals.model_name));
    }
    if (config->globals.firmware_rev) {
        Device_Set_Firmware_Revision(config->globals.firmware_rev,
                                     strlen(config->globals.firmware_rev));
    }

    dlenv_init();

    {
        uint8_t iam_buf[MAX_MPDU] = { 0 };
        Send_I_Am(iam_buf);
    }

    node_initialized = true;
    return 0;
}

int bbmd_node_run(void)
{
    BACNET_ADDRESS src = { 0 };
    uint8_t rx_buf[MAX_MPDU] = { 0 };
    uint16_t pdu_len;
    const unsigned delay_ms = 1;

    if (!node_initialized) {
        return 0;
    }

    pdu_len = datalink_receive(&src, rx_buf, MAX_MPDU, delay_ms);
    if (pdu_len) {
        npdu_handler(&src, rx_buf, pdu_len);
        return 1;
    }
    return 0;
}

void bbmd_node_maintenance(unsigned int seconds)
{
    if (!node_initialized) {
        return;
    }
    datalink_maintenance_timer((uint16_t)seconds);
}

void bbmd_node_stop(void)
{
    if (!node_initialized) {
        return;
    }
    datalink_cleanup();
    node_initialized = false;
}
