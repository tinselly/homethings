
#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/socket.h>
#include <zephyr/random/rand32.h>
#include <zephyr/sys/printk.h>

#include "certs/certs.h"
#include "local_config.h"

/******************************************************************************/

#define DEVICE_TOPIC "homethings/led/" MQTT_CLIENTID

/******************************************************************************/

LOG_MODULE_REGISTER(mqtt_azure, LOG_LEVEL_DBG);

/******************************************************************************/

/* Buffers for MQTT client. */
static uint8_t s_rx_buffer[APP_MQTT_BUFFER_SIZE];
static uint8_t s_tx_buffer[APP_MQTT_BUFFER_SIZE];

/* The mqtt client struct */
static struct mqtt_client s_client_ctx;

/* MQTT Broker details. */
static struct sockaddr_storage s_broker;

/* Socket Poll */
static struct zsock_pollfd s_fds[1];
static int s_nfds = 0;

static bool s_mqtt_connected = false;

#if defined(CONFIG_NET_DHCPV4)
static struct k_work_delayable s_check_network_conn;

/* Network Management events */
#define L4_EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)

static struct net_mgmt_event_callback s_l4_mgmt_cb;
#endif

#if defined(CONFIG_DNS_RESOLVER)
static struct zsock_addrinfo s_dns_hints;
static struct zsock_addrinfo* s_dns_haddr;
#endif

static K_SEM_DEFINE(s_mqtt_start, 0, 1);

static sec_tag_t s_sec_tags[] = { APP_CA_CERT_TAG, APP_DEV_CERT_TAG, APP_DEV_KEY_TAG };

static uint8_t topic[] = "homethings/led/" MQTT_CLIENTID;
static struct mqtt_topic s_subs_topic;
static struct mqtt_subscription_list s_subs_list;

static struct mqtt_topic s_will_topic = { MQTT_UTF8_LITERAL(DEVICE_TOPIC), 1 };
static struct mqtt_utf8 s_will_message = MQTT_UTF8_LITERAL("Offline");

/******************************************************************************/

static void mqtt_event_handler(struct mqtt_client* const client, const struct mqtt_evt* evt);

static void prepare_fds(struct mqtt_client* client) {
    if (client->transport.type == MQTT_TRANSPORT_SECURE) {
        s_fds[0].fd = client->transport.tls.sock;
    }

    s_fds[0].events = ZSOCK_POLLIN;
    s_nfds = 1;
}

static void clear_fds(void) {
    s_nfds = 0;
}

static int wait(int timeout) {
    int rc = -EINVAL;

    if (s_nfds <= 0) {
        return rc;
    }

    rc = zsock_poll(s_fds, s_nfds, timeout);
    if (rc < 0) {
        LOG_ERR("poll error: %d", errno);
        return -errno;
    }

    return rc;
}

static void broker_init(void) {
    struct sockaddr_in* broker4 = (struct sockaddr_in*)&s_broker;

    broker4->sin_family = AF_INET;
    broker4->sin_port = htons(SERVER_PORT);

#if defined(CONFIG_DNS_RESOLVER)
    net_ipaddr_copy(&broker4->sin_addr, &net_sin(s_dns_haddr->ai_addr)->sin_addr);
#else
    zsock_inet_pton(AF_INET, SERVER_ADDR, &broker4->sin_addr);
#endif
}

static void client_init(struct mqtt_client* client) {

    struct mqtt_sec_config* tls_config = NULL;

    mqtt_client_init(client);

    broker_init();

    /* MQTT client configuration */
    client->broker = &s_broker;
    client->evt_cb = mqtt_event_handler;

    client->client_id.utf8 = (const uint8_t*)MQTT_CLIENTID;
    client->client_id.size = strlen(MQTT_CLIENTID);
    client->password = NULL;
    client->user_name = NULL;

    client->protocol_version = MQTT_VERSION_3_1_1;

    /* MQTT buffers configuration */
    client->rx_buf = s_rx_buffer;
    client->rx_buf_size = sizeof(s_rx_buffer);
    client->tx_buf = s_tx_buffer;
    client->tx_buf_size = sizeof(s_tx_buffer);

    client->will_topic = &s_will_topic;
    client->will_message = &s_will_message;

    /* MQTT transport configuration */
    client->transport.type = MQTT_TRANSPORT_SECURE;

    tls_config = &client->transport.tls.config;
    tls_config->peer_verify = TLS_PEER_VERIFY_NONE;
    tls_config->cipher_list = NULL;
    tls_config->sec_tag_list = s_sec_tags;
    tls_config->sec_tag_count = ARRAY_SIZE(s_sec_tags);
}

static void mqtt_event_handler(struct mqtt_client* const client, const struct mqtt_evt* evt) {
    struct mqtt_puback_param puback;
    uint8_t data[33];
    int len;
    int bytes_read;

    switch (evt->type) {
        case MQTT_EVT_SUBACK:
            LOG_DBG("SUBACK packet id: %u", evt->param.suback.message_id);
            break;

        case MQTT_EVT_UNSUBACK:
            LOG_DBG("UNSUBACK packet id: %u", evt->param.suback.message_id);
            break;

        case MQTT_EVT_CONNACK:
            if (evt->result) {
                LOG_ERR("MQTT connect failed %d", evt->result);
                break;
            }

            s_mqtt_connected = true;
            LOG_INF("MQTT client connected!");
            break;

        case MQTT_EVT_DISCONNECT:
            LOG_WRN("MQTT client disconnected %d", evt->result);

            s_mqtt_connected = false;
            clear_fds();
            break;

        case MQTT_EVT_PUBACK:
            if (evt->result) {
                LOG_ERR("MQTT PUBACK error %d", evt->result);
                break;
            }

            LOG_DBG("PUBACK packet id: %u\n", evt->param.puback.message_id);
            break;

        case MQTT_EVT_PUBLISH:
            len = evt->param.publish.message.payload.len;

            LOG_INF("MQTT publish received %d, %d bytes", evt->result, len);
            LOG_INF(" id: %d, qos: %d",
                    evt->param.publish.message_id,
                    evt->param.publish.message.topic.qos);

            while (len) {
                bytes_read = mqtt_read_publish_payload(
                    &s_client_ctx, data, len >= sizeof(data) - 1 ? sizeof(data) - 1 : len);
                if (bytes_read < 0 && bytes_read != -EAGAIN) {
                    LOG_ERR("failure to read payload");
                    break;
                }

                data[bytes_read] = '\0';
                LOG_INF("   payload: %s", data);
                len -= bytes_read;
            }

            puback.message_id = evt->param.publish.message_id;
            mqtt_publish_qos1_ack(&s_client_ctx, &puback);
            break;

        default:
            LOG_DBG("Unhandled MQTT event %d", evt->type);
            break;
    }
}

static void subscribe(struct mqtt_client* client) {
    int rc = 0;

    /* subscribe */
    subs_topic.topic.utf8 = topic;
    subs_topic.topic.size = strlen(topic);
    subs_list.list = &subs_topic;
    subs_list.list_count = 1U;
    subs_list.message_id = 1U;

    rc = mqtt_subscribe(client, &subs_list);
    if (rc != 0) {
        LOG_ERR("Failed on topic %s", topic);
    }
}

static void poll_mqtt() {

    uint32_t last_time = 0;
    while (s_mqtt_connected) {

        const int rc = wait(100);
        if (rc > 0) {
            mqtt_input(&s_client_ctx);
        }

        if (k_uptime_get_32() - last_time > (CONFIG_MQTT_KEEPALIVE / 2)) {
            last_time = k_uptime_get_32();
            mqtt_live(&s_client_ctx);
        }
    }
}

static int publish_boot_content(struct mqtt_client* client) {

    struct mqtt_publish_param param;

    param.message.topic.qos = 1;
    param.message.topic.topic = MQTT_UTF8_LITERAL(DEVICE_TOPIC);

    param.message.payload.data = "Online";
    param.message.payload.len = strlen(param.message.payload.data);

    param.message_id = sys_rand32_get();
    param.dup_flag = 0U;
    param.retain_flag = 1U;

    return mqtt_publish(client, &param);
}

static int try_to_connect(struct mqtt_client* client) {
    uint8_t retries = 3U;
    int rc;

    LOG_DBG("attempting to connect...");

    while (retries--) {
        client_init(client);

        rc = mqtt_connect(client);
        if (rc) {
            LOG_ERR("mqtt_connect failed %d", rc);
            k_sleep(K_SECONDS(3));
            continue;
        }

        prepare_fds(client);

        rc = wait(APP_SLEEP_MSECS);
        if (rc < 0) {
            mqtt_abort(client);
            return rc;
        }

        mqtt_input(client);

        if (s_mqtt_connected) {
            subscribe(client);
            publish_boot_content(client);
            return 0;
        }

        mqtt_abort(client);

        wait(10 * MSEC_PER_SEC);
    }

    return -EINVAL;
}

#if defined(CONFIG_DNS_RESOLVER)
static int get_mqtt_broker_addrinfo(void) {
    int retries = 3;
    int rc = -EINVAL;

    while (retries--) {
        s_dns_hints.ai_family = AF_INET;
        s_dns_hints.ai_socktype = SOCK_STREAM;
        s_dns_hints.ai_protocol = 0;

        rc = zsock_getaddrinfo(SERVER_ADDR, "1883", &s_dns_hints, &s_dns_haddr);
        if (rc == 0) {
            LOG_INF("DNS resolved for %s:%d",
                    CONFIG_SAMPLE_CLOUD_AZURE_HOSTNAME,
                    CONFIG_SAMPLE_CLOUD_AZURE_SERVER_PORT);

            return 0;
        }

        LOG_ERR("DNS not resolved for %s:%d, retrying",
                CONFIG_SAMPLE_CLOUD_AZURE_HOSTNAME,
                CONFIG_SAMPLE_CLOUD_AZURE_SERVER_PORT);
    }

    return rc;
}
#endif

/* DHCP tries to renew the address after interface is down and up.
 * If DHCPv4 address renewal is success, then it doesn't generate
 * any event. We have to monitor this way.
 * If DHCPv4 attempts exceeds maximum number, it will delete iface
 * address and attempts for new request. In this case we can rely
 * on IPV4_ADDR_ADD event.
 */
#if defined(CONFIG_NET_DHCPV4)
static void check_network_connection(struct k_work* work) {

    if (s_mqtt_connected) {
        return;
    }

    struct net_if* const iface = net_if_get_default();
    if (!iface) {
        goto end;
    }

    if (iface->config.dhcpv4.state == NET_DHCPV4_BOUND) {
        k_sem_give(&s_mqtt_start);
        return;
    }

    LOG_INF("waiting for DHCP to acquire addr");

end:
    k_work_reschedule(&s_check_network_conn, K_SECONDS(3));
}
#endif

#if defined(CONFIG_NET_DHCPV4)
static void abort_mqtt_connection(void) {
    if (s_mqtt_connected) {
        s_mqtt_connected = false;
        mqtt_abort(&s_client_ctx);
    }
}

static void
l4_event_handler(struct net_mgmt_event_callback* cb, uint32_t mgmt_event, struct net_if* iface) {
    if ((mgmt_event & L4_EVENT_MASK) != mgmt_event) {
        return;
    }

    if (mgmt_event == NET_EVENT_L4_CONNECTED) {
        /* Wait for DHCP to be back in BOUND state */
        k_work_reschedule(&s_check_network_conn, K_SECONDS(3));

        return;
    }

    if (mgmt_event == NET_EVENT_L4_DISCONNECTED) {
        abort_mqtt_connection();
        k_work_cancel_delayable(&s_check_network_conn);

        return;
    }
}
#endif

static void mqtt_thread(void* unused1, void* unused2, void* unused3) {

    ARG_UNUSED(unused1);
    ARG_UNUSED(unused2);
    ARG_UNUSED(unused3);

    LOG_DBG("Waiting for network to setup...");

#if defined(CONFIG_NET_DHCPV4)
    k_work_init_delayable(&s_check_network_conn, check_network_connection);

    net_mgmt_init_event_callback(&s_l4_mgmt_cb, l4_event_handler, L4_EVENT_MASK);
    net_mgmt_add_event_callback(&s_l4_mgmt_cb);
#endif

    while (true) {
        int rc = 0;

        k_sem_take(&s_mqtt_start, K_FOREVER);

#if defined(CONFIG_DNS_RESOLVER)
        rc = get_mqtt_broker_addrinfo();
        if (rc) {
            k_sleep(K_SECONDS(20));
            continue;
        }
#endif
        rc = try_to_connect(&s_client_ctx);
        if (rc != 0) {
            k_sleep(K_SECONDS(20));
            continue;
        }

        poll_mqtt();
    }
}

/******************************************************************************/

K_THREAD_DEFINE(s_mqtt_thread, 4096, mqtt_thread, NULL, NULL, NULL, 5, 0, 0);

/******************************************************************************/
