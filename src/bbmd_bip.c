#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <syslog.h>

#include "bacnet/config.h"
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacint.h"
#include "bacnet/bactext.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/iam.h"
#include "bacnet/dcc.h"
#include "bacnet/datalink/dlenv.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/bip.h"
#include "bacnet/datalink/bvlc.h"
#include "bacnet/basic/bbmd/h_bbmd.h"

#include "bbmd_config.h"
#include "bbmd_bip.h"

static bool bip_initialized;
static bool bip_stopped;
static uint8_t Rx_Buf[BIP_MPDU_MAX];

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

static void bip_npdu_handler(
    BACNET_ADDRESS *src, uint8_t *pdu, uint16_t pdu_len)
{
    int apdu_offset;
    BACNET_ADDRESS dest = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };

    if (!pdu || pdu[0] != BACNET_PROTOCOL_VERSION)
        return;

    apdu_offset = bacnet_npdu_decode(pdu, pdu_len, &dest, src, &npdu_data);
    if (apdu_offset <= 0)
        return;

    if (npdu_data.network_layer_message) {
        return;
    }

    if (apdu_offset > pdu_len)
        return;

    if (npdu_data.hop_count == 0)
        return;

    npdu_data.data_expecting_reply = false;
    apdu_handler(src, &pdu[apdu_offset],
                 (uint16_t)(pdu_len - apdu_offset));
}

int bbmd_bip_init(bbmd_config_t *config)
{
    char port_str[16];

    if (!config || !config->bip.enabled)
        return -1;

    setenv("BACNET_DATALINK", "BIP", 1);

    snprintf(port_str, sizeof(port_str), "%u", config->bip.port);
    setenv("BACNET_IP_PORT", port_str, 1);

    if (config->bip.lan_interface) {
        setenv("BACNET_IFACE", config->bip.lan_interface, 1);
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

    bip_set_port(config->bip.port);

    if (!bip_init(NULL)) {
        syslog(LOG_ERR, "Failed to initialize BIP datalink");
        return -1;
    }

    bvlc_init();

    bip_initialized = true;
    bip_stopped = false;
    syslog(LOG_INFO,
           "BIP-only BBMD mode started (port=%u interface=%s)",
           config->bip.port,
           config->bip.lan_interface ? config->bip.lan_interface : "(all)");
    return 0;
}

int bbmd_bip_run(void)
{
    BACNET_ADDRESS src = { 0 };
    uint16_t pdu_len;

    if (!bip_initialized || bip_stopped)
        return 0;

    pdu_len = bip_receive(&src, &Rx_Buf[0], sizeof(Rx_Buf), 1);
    if (pdu_len) {
        bip_npdu_handler(&src, &Rx_Buf[0], pdu_len);
        return 1;
    }
    return 0;
}

void bbmd_bip_maintenance(unsigned int seconds)
{
    if (!bip_initialized || bip_stopped)
        return;
    bvlc_maintenance_timer((uint16_t)seconds);
}

void bbmd_bip_stop(void)
{
    if (!bip_initialized || bip_stopped)
        return;
    bip_stopped = true;
    bip_cleanup();
    bip_initialized = false;
}
