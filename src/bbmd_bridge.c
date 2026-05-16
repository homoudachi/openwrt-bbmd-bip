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
#include "bacnet/datalink/bsc/bsc-datalink.h"

#include "bbmd_config.h"
#include "bbmd_bridge.h"

static bool bridge_initialized;
static bool bridge_stopped;
static uint16_t BIP_Net;
static uint16_t BSC_Net;

typedef struct dnet_entry {
    uint8_t mac[MAX_MAC_LEN];
    uint8_t mac_len;
    uint16_t net;
    bool enabled;
    struct dnet_entry *next;
} DNET_ENTRY;

static DNET_ENTRY *Router_Table;

static uint8_t BIP_Rx_Buf[BIP_MPDU_MAX];
static uint8_t BSC_Rx_Buf[BVLC_SC_NPDU_SIZE_CONF];

#define BRIDGE_TX_BUF_SIZE ((BIP_MPDU_MAX > BVLC_SC_NPDU_SIZE_CONF) ? BIP_MPDU_MAX : BVLC_SC_NPDU_SIZE_CONF)
static uint8_t Tx_Buf[BRIDGE_TX_BUF_SIZE];

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

static void bridge_get_broadcast_address(BACNET_ADDRESS *dest)
{
    if (dest) {
        dest->mac_len = 0;
        dest->net = BACNET_BROADCAST_NETWORK;
        dest->len = 0;
    }
}

static DNET_ENTRY *dnet_find(uint16_t net)
{
    DNET_ENTRY *port = Router_Table;
    while (port) {
        if (net == port->net)
            return port;
        port = port->next;
    }
    return NULL;
}

static bool port_find(uint16_t snet, BACNET_ADDRESS *addr)
{
    DNET_ENTRY *port = Router_Table;
    while (port) {
        if (port->net == snet) {
            if (addr) {
                addr->mac_len = port->mac_len;
                memcpy(addr->mac, port->mac, MAX_MAC_LEN);
            }
            return true;
        }
        port = port->next;
    }
    return false;
}

static void port_add(uint16_t snet, const BACNET_ADDRESS *addr)
{
    DNET_ENTRY *port = Router_Table;
    DNET_ENTRY *last = NULL;

    if (dnet_find(snet))
        return;

    while (port) {
        last = port;
        port = port->next;
    }

    port = calloc(1, sizeof(DNET_ENTRY));
    if (!port)
        return;
    port->net = snet;
    port->enabled = true;
    if (addr) {
        port->mac_len = addr->mac_len;
        memcpy(port->mac, addr->mac, MAX_MAC_LEN);
    }

    if (last)
        last->next = port;
    else
        Router_Table = port;
}

static void dnet_cleanup(void)
{
    DNET_ENTRY *port = Router_Table;
    while (port) {
        DNET_ENTRY *next = port->next;
        free(port);
        port = next;
    }
    Router_Table = NULL;
}

static int bridge_datalink_send(
    uint16_t snet,
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{
    if (snet == BSC_Net) {
        return bsc_send_pdu(dest, npdu_data, pdu, pdu_len);
    }
    return bip_send_pdu(dest, npdu_data, pdu, pdu_len);
}

static void send_i_am_router_to_network(uint16_t snet, uint16_t net)
{
    BACNET_ADDRESS dest;
    bool data_expecting_reply = false;
    BACNET_NPDU_DATA npdu_data;
    int pdu_len = 0;
    int len = 0;
    DNET_ENTRY *port = NULL;

    bridge_get_broadcast_address(&dest);
    npdu_encode_npdu_network(
        &npdu_data, NETWORK_MESSAGE_I_AM_ROUTER_TO_NETWORK,
        data_expecting_reply, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(&Tx_Buf[0], &dest, NULL, &npdu_data);
    if (net) {
        len = encode_unsigned16(&Tx_Buf[pdu_len], net);
        pdu_len += len;
    } else {
        port = Router_Table;
        while (port) {
            if (port->net != snet) {
                len = encode_unsigned16(&Tx_Buf[pdu_len], port->net);
                pdu_len += len;
            }
            port = port->next;
        }
    }
    bridge_datalink_send(snet, &dest, &npdu_data, &Tx_Buf[0], pdu_len);
}

static void send_who_is_router_to_network(uint16_t snet, uint16_t dnet)
{
    BACNET_ADDRESS dest;
    bool data_expecting_reply = false;
    BACNET_NPDU_DATA npdu_data;
    int pdu_len = 0;
    int len = 0;

    bridge_get_broadcast_address(&dest);
    npdu_encode_npdu_network(
        &npdu_data, NETWORK_MESSAGE_WHO_IS_ROUTER_TO_NETWORK,
        data_expecting_reply, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(&Tx_Buf[0], &dest, NULL, &npdu_data);
    if (dnet) {
        len = encode_unsigned16(&Tx_Buf[pdu_len], dnet);
        pdu_len += len;
    }
    bridge_datalink_send(snet, &dest, &npdu_data, &Tx_Buf[0], pdu_len);
}

static void send_reject_message_to_network(
    uint16_t snet, const BACNET_ADDRESS *dst, uint8_t reject_reason, uint16_t dnet)
{
    BACNET_ADDRESS dest;
    bool data_expecting_reply = false;
    BACNET_NPDU_DATA npdu_data;
    int pdu_len = 0;
    int len = 0;

    if (dst) {
        bacnet_address_copy(&dest, dst);
    } else {
        bridge_get_broadcast_address(&dest);
    }
    npdu_encode_npdu_network(
        &npdu_data, NETWORK_MESSAGE_REJECT_MESSAGE_TO_NETWORK,
        data_expecting_reply, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(&Tx_Buf[0], &dest, NULL, &npdu_data);
    Tx_Buf[pdu_len] = reject_reason;
    pdu_len++;
    if (dnet) {
        len = encode_unsigned16(&Tx_Buf[pdu_len], dnet);
        pdu_len += len;
    }
    bridge_datalink_send(snet, &dest, &npdu_data, &Tx_Buf[0], pdu_len);
}

static void send_initialize_routing_table_ack(uint16_t snet, const BACNET_ADDRESS *dst)
{
    BACNET_ADDRESS dest;
    bool data_expecting_reply = false;
    BACNET_NPDU_DATA npdu_data;
    int pdu_len = 0;
    int len = 0;
    uint8_t count = 0;
    uint8_t port_id = 1;
    DNET_ENTRY *port = NULL;

    if (dst) {
        bacnet_address_copy(&dest, dst);
    } else {
        bridge_get_broadcast_address(&dest);
    }
    npdu_encode_npdu_network(
        &npdu_data, NETWORK_MESSAGE_INIT_RT_TABLE_ACK, data_expecting_reply,
        MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(&Tx_Buf[0], &dest, NULL, &npdu_data);

    port = Router_Table;
    while (port) {
        count++;
        port = port->next;
    }
    Tx_Buf[pdu_len] = count;
    pdu_len++;

    if (count > 0) {
        port = Router_Table;
        while (port) {
            len = encode_unsigned16(&Tx_Buf[pdu_len], port->net);
            pdu_len += len;
            Tx_Buf[pdu_len] = port_id;
            pdu_len++;
            port_id++;
            Tx_Buf[pdu_len] = 0;
            pdu_len++;
            port = port->next;
        }
    }
    bridge_datalink_send(snet, &dest, &npdu_data, &Tx_Buf[0], pdu_len);
}

static void who_is_router_to_network_handler(
    uint16_t snet, const BACNET_NPDU_DATA *npdu_data,
    const uint8_t *npdu, uint16_t npdu_len)
{
    DNET_ENTRY *port = NULL;
    uint16_t network = 0;
    uint16_t len = 0;

    (void)npdu_data;
    if (npdu) {
        if (npdu_len >= 2) {
            len += decode_unsigned16(&npdu[len], &network);
            port = dnet_find(network);
            if (port) {
                if (port->net != snet) {
                    send_i_am_router_to_network(snet, network);
                }
            } else {
                port = Router_Table;
                while (port) {
                    if (port->net != snet) {
                        send_who_is_router_to_network(port->net, network);
                    }
                    port = port->next;
                }
            }
        } else {
            send_i_am_router_to_network(snet, 0);
        }
    }
}

static void network_control_handler(
    uint16_t snet,
    BACNET_ADDRESS *src,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *npdu,
    uint16_t npdu_len)
{
    uint16_t npdu_offset = 0;
    uint16_t dnet = 0;
    uint16_t len = 0;

    switch (npdu_data->network_message_type) {
    case NETWORK_MESSAGE_WHO_IS_ROUTER_TO_NETWORK:
        who_is_router_to_network_handler(snet, npdu_data, npdu, npdu_len);
        break;
    case NETWORK_MESSAGE_I_AM_ROUTER_TO_NETWORK:
        len = 2;
        while (npdu_len >= len) {
            len = decode_unsigned16(&npdu[npdu_offset], &dnet);
            npdu_len -= len;
            npdu_offset += len;
        }
        break;
    case NETWORK_MESSAGE_I_COULD_BE_ROUTER_TO_NETWORK:
        break;
    case NETWORK_MESSAGE_REJECT_MESSAGE_TO_NETWORK:
        break;
    case NETWORK_MESSAGE_ROUTER_BUSY_TO_NETWORK:
    case NETWORK_MESSAGE_ROUTER_AVAILABLE_TO_NETWORK:
        break;
    case NETWORK_MESSAGE_INIT_RT_TABLE:
        if (npdu_len > 0) {
            if (npdu[0] == 0) {
                send_initialize_routing_table_ack(snet, NULL);
            } else {
                int net_count = npdu[0];
                int i = 1;
                while (net_count--) {
                    decode_unsigned16(&npdu[i], &dnet);
                    i += 4;
                }
                send_initialize_routing_table_ack(snet, NULL);
            }
        }
        break;
    case NETWORK_MESSAGE_INIT_RT_TABLE_ACK:
        break;
    case NETWORK_MESSAGE_ESTABLISH_CONNECTION_TO_NETWORK:
    case NETWORK_MESSAGE_DISCONNECT_CONNECTION_TO_NETWORK:
        break;
    default:
        send_reject_message_to_network(
            snet, src, NETWORK_REJECT_UNKNOWN_MESSAGE_TYPE, 0);
        break;
    }
}

static void routed_src_address(
    BACNET_ADDRESS *router_src, uint16_t snet, const BACNET_ADDRESS *src)
{
    unsigned int i = 0;

    if (router_src && src) {
        if (port_find(snet, router_src)) {
            if (src->net) {
                router_src->net = src->net;
                router_src->len = src->len;
                for (i = 0; i < MAX_MAC_LEN; i++) {
                    router_src->adr[i] = src->adr[i];
                }
            } else {
                router_src->net = snet;
                router_src->len = src->mac_len;
                for (i = 0; i < MAX_MAC_LEN; i++) {
                    router_src->adr[i] = src->mac[i];
                }
            }
        }
    }
}

static void routed_apdu_handler(
    uint16_t snet,
    BACNET_NPDU_DATA *npdu,
    BACNET_ADDRESS *src,
    BACNET_ADDRESS *dest,
    uint8_t *apdu,
    uint16_t apdu_len)
{
    int npdu_len = 0;
    BACNET_ADDRESS local_dest;
    BACNET_ADDRESS router_src = { 0 };
    DNET_ENTRY *port = NULL;

    if (dest->net == BACNET_BROADCAST_NETWORK) {
        bridge_get_broadcast_address(&local_dest);
        npdu->hop_count--;
        routed_src_address(&router_src, snet, src);
        npdu_len = npdu_encode_pdu(&Tx_Buf[0], &local_dest, &router_src, npdu);
        memmove(&Tx_Buf[npdu_len], apdu, apdu_len);

        port = Router_Table;
        while (port) {
            if (port->net != snet) {
                bridge_datalink_send(
                    port->net, &local_dest, npdu,
                    &Tx_Buf[0], npdu_len + apdu_len);
            }
            port = port->next;
        }
        return;
    }

    if ((dest->net == BIP_Net && snet != BIP_Net) ||
        (dest->net == BSC_Net && snet != BSC_Net)) {
        bridge_get_broadcast_address(&local_dest);
        npdu->hop_count--;
        routed_src_address(&router_src, snet, src);
        npdu_len = npdu_encode_pdu(&Tx_Buf[0], &local_dest, &router_src, npdu);
        memmove(&Tx_Buf[npdu_len], apdu, apdu_len);
        bridge_datalink_send(
            dest->net, &local_dest, npdu,
            &Tx_Buf[0], npdu_len + apdu_len);
        return;
    }
}

static void bridge_npdu_handler(
    uint16_t snet, BACNET_ADDRESS *src, uint8_t *pdu, uint16_t pdu_len)
{
    int apdu_offset = 0;
    BACNET_ADDRESS dest = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    bool is_local, is_broadcast, is_remote;

    if (!pdu || pdu[0] != BACNET_PROTOCOL_VERSION)
        return;

    apdu_offset = bacnet_npdu_decode(pdu, pdu_len, &dest, src, &npdu_data);
    if (apdu_offset <= 0)
        return;

    if (npdu_data.network_layer_message) {
        if (dest.net == 0 || dest.net == BACNET_BROADCAST_NETWORK) {
            network_control_handler(
                snet, src, &npdu_data, &pdu[apdu_offset],
                (uint16_t)(pdu_len - apdu_offset));
        }
        return;
    }

    if (apdu_offset > pdu_len)
        return;

    if (npdu_data.hop_count == 0)
        return;

    is_local = (dest.net == 0 || dest.net == snet);
    is_broadcast = (dest.net == BACNET_BROADCAST_NETWORK);
    is_remote = (!is_local && !is_broadcast &&
                 (dest.net == BIP_Net || dest.net == BSC_Net));

    if (!is_local && !is_broadcast && !is_remote)
        return;

    if (is_broadcast &&
        ((pdu[apdu_offset] & 0xF0) == PDU_TYPE_CONFIRMED_SERVICE_REQUEST))
        return;

    routed_apdu_handler(
        snet, &npdu_data, src, &dest, &pdu[apdu_offset],
        (uint16_t)(pdu_len - apdu_offset));

    if (is_local || is_broadcast) {
        npdu_data.data_expecting_reply = false;
        apdu_handler(src, &pdu[apdu_offset],
                     (uint16_t)(pdu_len - apdu_offset));
    }
}

static int send_i_am_on_bsc(void)
{
    BACNET_ADDRESS dest;
    BACNET_ADDRESS my_address;
    BACNET_NPDU_DATA npdu_data;
    int pdu_len = 0;
    int len = 0;

    bsc_get_broadcast_address(&dest);
    bsc_get_my_address(&my_address);

    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(&Tx_Buf[0], &dest, &my_address, &npdu_data);

    len = iam_encode_apdu(
        &Tx_Buf[pdu_len], Device_Object_Instance_Number(), MAX_APDU,
        Device_Segmentation_Supported(), Device_Vendor_Identifier());
    pdu_len += len;

    return bsc_send_pdu(&dest, &npdu_data, &Tx_Buf[0], (unsigned)pdu_len);
}

int bbmd_bridge_init(bbmd_config_t *config)
{
    char port_str[16];
    BACNET_ADDRESS my_address = { 0 };

    if (!config || !config->bridge.enabled)
        return -1;

    BIP_Net = config->globals.network_number;
    if (BIP_Net == 0)
        BIP_Net = 1;
    BSC_Net = BIP_Net + 1;

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

    snprintf(port_str, sizeof(port_str), "%u", config->bridge.port);
    setenv("BACNET_IP_PORT", port_str, 1);
    if (config->bridge.lan_interface) {
        setenv("BACNET_IFACE", config->bridge.lan_interface, 1);
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

    bip_set_port(config->bridge.port);

    if (!bip_init(NULL)) {
        syslog(LOG_ERR, "Failed to initialize BIP datalink");
        return -1;
    }

    bvlc_init();

    bip_get_my_address(&my_address);
    port_add(BIP_Net, &my_address);

    bsc_get_my_address(&my_address);
    port_add(BSC_Net, &my_address);

    send_i_am_router_to_network(BIP_Net, 0);
    send_i_am_router_to_network(BSC_Net, 0);

    send_i_am_on_bsc();

    bridge_initialized = true;
    bridge_stopped = false;
    syslog(LOG_INFO,
           "BACnet/IP to BACnet/SC bridge started "
           "(BIP net=%u port=%u BSC net=%u hub_port=%u)",
           BIP_Net, config->bridge.port,
           BSC_Net, config->hub.port);
    return 0;
}

int bbmd_bridge_run(void)
{
    BACNET_ADDRESS src = { 0 };
    uint16_t pdu_len;

    if (!bridge_initialized || bridge_stopped)
        return 0;

    pdu_len = bip_receive(&src, &BIP_Rx_Buf[0], sizeof(BIP_Rx_Buf), 1);
    if (pdu_len) {
        bridge_npdu_handler(BIP_Net, &src, &BIP_Rx_Buf[0], pdu_len);
        return 1;
    }

    pdu_len = bsc_receive(&src, &BSC_Rx_Buf[0], sizeof(BSC_Rx_Buf), 1);
    if (pdu_len) {
        bridge_npdu_handler(BSC_Net, &src, &BSC_Rx_Buf[0], pdu_len);
        return 1;
    }

    return 0;
}

void bbmd_bridge_maintenance(unsigned int seconds)
{
    if (!bridge_initialized || bridge_stopped)
        return;

    bsc_maintenance_timer((uint16_t)seconds);
    bvlc_maintenance_timer((uint16_t)seconds);
}

void bbmd_bridge_stop(void)
{
    if (!bridge_initialized || bridge_stopped)
        return;

    bridge_stopped = true;
    bsc_cleanup();
    bip_cleanup();
    dnet_cleanup();
    bridge_initialized = false;
}
