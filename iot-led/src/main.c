
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/types.h>

#include <zephyr/net/net_event.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>

#include "local_config.h"

#include "certs/certs.h"

/******************************************************************************/

LOG_MODULE_REGISTER(main);

/******************************************************************************/

static int tls_init() {
    int rc = 0;

    rc = tls_credential_add(
        APP_CA_CERT_TAG, TLS_CREDENTIAL_CA_CERTIFICATE, ca_cert_pem, sizeof(ca_cert_pem));
    if (rc < 0) {
        LOG_ERR("Failed to register ca certificate: %d", rc);
        return rc;
    }

    rc = tls_credential_add(
        APP_DEV_CERT_TAG, TLS_CREDENTIAL_SERVER_CERTIFICATE, dev_cert_pem, sizeof(dev_cert_pem));
    if (rc < 0) {
        LOG_ERR("Failed to register server certificate: %d", rc);
        return rc;
    }

    rc = tls_credential_add(
        APP_DEV_KEY_TAG, TLS_CREDENTIAL_PRIVATE_KEY, dev_key_pem, sizeof(dev_key_pem));
    if (rc < 0) {
        LOG_ERR("Failed to register private key: %d", rc);
        return rc;
    }

    LOG_INF("TLS credentials were installed");

    return rc;
}

static int main_init(const struct device* dev) {
    int rc = 0;

    rc = tls_init();

    return rc;
}

/******************************************************************************/

void main() {
    struct net_if* const iface = net_if_get_default();

    static struct wifi_connect_req_params wifi_conn_params;
    {
        wifi_conn_params.ssid = WIFI_SSID;
        wifi_conn_params.ssid_length = strlen(WIFI_SSID);
        wifi_conn_params.channel = WIFI_CHANNEL_ANY;
        wifi_conn_params.psk = WIFI_PASS;
        wifi_conn_params.psk_length = strlen(WIFI_PASS);
        wifi_conn_params.security = WIFI_SECURITY_TYPE_PSK;
        wifi_conn_params.mfp = WIFI_MFP_OPTIONAL;
    }

    if (net_mgmt(NET_REQUEST_WIFI_CONNECT,
                 iface,
                 &wifi_conn_params,
                 sizeof(struct wifi_connect_req_params))) {
        LOG_ERR("Failed to connect wifi");
    }

    while (1) {
        k_sleep(K_SECONDS(1));
    }
}

/******************************************************************************/

SYS_INIT(main_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

/******************************************************************************/
