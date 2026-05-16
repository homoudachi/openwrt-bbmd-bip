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
#include "bbmd_hub.h"

static bool hub_initialized = false;

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

int bbmd_hub_init(bbmd_config_t *config)
{
    char port_str[16];

    if (!config || !config->hub.enabled) {
        return -1;
    }

    setenv("BACNET_DATALINK", "BSC", 1);

    if (config->hub.ca_cert) {
        setenv("BACNET_SC_ISSUER_1_CERTIFICATE_FILE",
               config->hub.ca_cert, 1);
    }
    if (config->hub.tls_cert) {
        setenv("BACNET_SC_OPERATIONAL_CERTIFICATE_FILE",
               config->hub.tls_cert, 1);
    }
    if (config->hub.tls_key) {
        setenv("BACNET_SC_OPERATIONAL_CERTIFICATE_PRIVATE_KEY_FILE",
               config->hub.tls_key, 1);
    }

    snprintf(port_str, sizeof(port_str), "%u", config->hub.port);
    setenv("BACNET_SC_HUB_FUNCTION_BINDING", port_str, 1);

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

    hub_initialized = true;
    return 0;
}

int bbmd_hub_run(void)
{
    BACNET_ADDRESS src = { 0 };
    uint8_t rx_buf[MAX_MPDU] = { 0 };
    uint16_t pdu_len;
    const unsigned delay_ms = 1;

    if (!hub_initialized) {
        return 0;
    }

    pdu_len = datalink_receive(&src, rx_buf, MAX_MPDU, delay_ms);
    if (pdu_len) {
        npdu_handler(&src, rx_buf, pdu_len);
        return 1;
    }
    return 0;
}

void bbmd_hub_maintenance(unsigned int seconds)
{
    if (!hub_initialized) {
        return;
    }
    datalink_maintenance_timer((uint16_t)seconds);
}

void bbmd_hub_stop(void)
{
    if (!hub_initialized) {
        return;
    }
    datalink_cleanup();
    hub_initialized = false;
}
