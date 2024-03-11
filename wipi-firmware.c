#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "boards/pico_w.h"

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"

#include "pico/util/queue.h"

#include "hardware/vreg.h"
#include "hardware/clocks.h"

#include "lwip/mem.h"
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/inet_chksum.h"

#include "commands.h"
#include "status.h"
#include "rxtx.h"

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif

/**
 * PING_DEBUG: Enable debugging for PING.
 */
#define PING_DEBUG     LWIP_DBG_ON
#define PING_TARGET   (netif_default?netif_default->gw:ip_addr_any)
#define PING_RCV_TIMEO 1000
#define PING_DELAY     1000
#define PING_ID        0xAFAF
#define PING_DATA_SIZE 32
#define PING_RESULT(ping_ok)

static u16_t ping_seq_num;
static u32_t ping_time;
static struct raw_pcb *ping_pcb;

static void
ping_prepare_echo( struct icmp_echo_hdr *iecho, u16_t len)
{
  size_t i;
  size_t data_len = len - sizeof(struct icmp_echo_hdr);

  ICMPH_TYPE_SET(iecho, ICMP_ECHO);
  ICMPH_CODE_SET(iecho, 0);
  iecho->chksum = 0;
  iecho->id     = PING_ID;
  iecho->seqno  = htons(++ping_seq_num);

  /* fill the additional data buffer with some data */
  for(i = 0; i < data_len; i++) {
    ((char*)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i;
  }

  iecho->chksum = inet_chksum(iecho, len);
}


static u8_t
ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr)
{
    struct icmp_echo_hdr *iecho;
    LWIP_UNUSED_ARG(arg);
    LWIP_UNUSED_ARG(pcb);
    LWIP_UNUSED_ARG(addr);
    LWIP_ASSERT("p != NULL", p != NULL);

    printf("static u8_t ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr)\n");
    printf("\tIP        = %08x\n",addr->addr);
    printf("\tFLAGS     = %08x\n",pcb->flags);
    printf("\tTTL       = %d\n",pcb->ttl);

    for (uint8_t i = 0; i < p->len; i++) {
        printf("%02x ", ((u8_t*)p->payload)[i]);
    }

    printf("\n");

    pbuf_free(p);
    return 1; /* eat the packet */
}

static void
ping_send(struct raw_pcb *pcb, ip_addr_t *addr)
{
    struct pbuf *p;
    struct icmp_echo_hdr *iecho;
    size_t ping_size = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;

    printf("static void ping_send(struct raw_pcb *raw, ip_addr_t *addr)\n");
    printf("\t%08x\n",pcb->ttl);
    printf("\t%08x\n",addr->addr);

    LWIP_ASSERT("ping_size <= 0xffff", ping_size <= 0xffff);

    p = pbuf_alloc(PBUF_IP, (u16_t)ping_size, PBUF_RAM);
    if (!p) {
        return;
    }
    if ((p->len == p->tot_len) && (p->next == NULL)) {
        iecho = (struct icmp_echo_hdr *)p->payload;

        ping_prepare_echo(iecho, (u16_t)ping_size);

        raw_sendto(pcb, p, addr);
        ping_time = sys_now();
    }
    pbuf_free(p);
}

static void
ping_raw_init(void)
{
  ping_pcb = raw_new(IP_PROTO_ICMP);
  LWIP_ASSERT("ping_pcb != NULL", ping_pcb != NULL);

  raw_recv(ping_pcb, ping_recv, NULL);
  raw_bind(ping_pcb, IP_ADDR_ANY);
}

typedef struct {
    uint8_t command;
    void* data;
    uint32_t data_size;
} queue_entry_t;

queue_t userport_queue;
queue_t wlan_queue;

void userport_core_main() {
    userport_init_gpio();
    userport_set_dir(GPIO_IN);

    char rx_block[256];
    char tx_block[256];

    while (true) {
        queue_entry_t userport_entry;
        queue_entry_t wlan_entry;

        rxtx_recv_mode();
        char rx_byte = rxtx_recv_byte();

        switch (rx_byte) {
            case command_wipi_status:
                rxtx_send_mode();
                userport_entry.command = command_wipi_status;
                userport_entry.data = NULL;
                userport_entry.data_size = 0;

                queue_add_blocking(&userport_queue, &userport_entry);
                queue_remove_blocking(&wlan_queue, &wlan_entry);

                wipi_status_t *status;
                status = (wipi_status_t*)wlan_entry.data;

                rxtx_send_byte(status->is_connected);
                rxtx_send_int(status->rssi);
                rxtx_send_byte(status->channel);
                rxtx_send_ip(status->ip);
                rxtx_send_ip(status->gateway);
                rxtx_send_ip(status->netmask);
                rxtx_send_block(status->ssid, 32);

                break;
                
            case command_connect:
                command_connect_data_t command_connect_data;

                userport_entry.command = command_connect;
                userport_entry.data = (void *)&command_connect_data;
                userport_entry.data_size = sizeof(command_connect_data);

                for (uint8_t i = 0; i < sizeof(command_connect_data.ssid); i++) {
                    command_connect_data.ssid[i] = rxtx_recv_byte();
                }

                for (uint8_t i = 0; i < sizeof(command_connect_data.password); i++) {
                    command_connect_data.password[i] = rxtx_recv_byte();
                }

                command_connect_data.timeout = rxtx_recv_byte();

                rxtx_send_mode();

                queue_add_blocking(&userport_queue, &userport_entry);
                queue_remove_blocking(&wlan_queue, &wlan_entry);

                rxtx_send_byte(((command_result_t*)wlan_entry.data)->result);
                printf("%d\n", ((command_result_t*)wlan_entry.data)->result);

                break;

            case command_disconnect:
                rxtx_send_mode();

                userport_entry.command = command_disconnect;
                userport_entry.data = NULL;
                userport_entry.data_size = 0;

                queue_add_blocking(&userport_queue, &userport_entry);
                queue_remove_blocking(&wlan_queue, &wlan_entry);

                rxtx_send_byte(((command_result_t*)wlan_entry.data)->result);
                printf("%d\n", ((command_result_t*)wlan_entry.data)->result);

                break;

            default:
                break;
        }
    }
}

int32_t cyw43_local_get_connection_rssi() {
        int32_t rssi;
        cyw43_ioctl(&cyw43_state, 127<<1, sizeof rssi, (uint8_t *)&rssi, CYW43_ITF_STA);
        return rssi < 0 ? rssi : -255;
}

int32_t cyw43_local_get_connection_channel() {
        int32_t channel;
        cyw43_ioctl(&cyw43_state, 29<<1, sizeof channel, (uint8_t *)&channel, CYW43_ITF_STA);
        return channel;
}

int main()
{
    stdio_init_all();

    queue_init(&userport_queue, sizeof(queue_entry_t), 16);
    queue_init(&wlan_queue, sizeof(queue_entry_t), 16);

    multicore_launch_core1(&userport_core_main);

    if (cyw43_arch_init_with_country(CYW43_COUNTRY_UK)) {
        debug(DEBUG_ERR,"Wi-Fi init failed");
        return -1;
    }

    bool led_status = true;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    netif_set_hostname(cyw43_state.netif, "bbc-master-wipi");
    netif_set_up(cyw43_state.netif);
    ping_raw_init();

    while (true) {
        while (!queue_is_empty(&userport_queue)) {
            queue_entry_t userport_entry;
            queue_entry_t wlan_entry;
            command_result_t command_result;

            queue_remove_blocking(&userport_queue, &userport_entry);

            //printf("WLAN command received: %d\n", userport_entry.command);

            switch (userport_entry.command) {
                case command_wipi_status:
                    cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);

                    wipi_status_t wipi_status;

                    wipi_status.is_connected = true;
                    wipi_status.rssi = cyw43_local_get_connection_rssi();
                    wipi_status.channel = cyw43_local_get_connection_channel();
                    memcpy(wipi_status.mac, cyw43_state.mac,sizeof(wipi_status.mac));
                    wipi_status.ip = cyw43_state.netif->ip_addr;
                    wipi_status.gateway = cyw43_state.netif->gw;
                    wipi_status.netmask = cyw43_state.netif->netmask;

                    wlan_entry.command = command_wipi_status;
                    wlan_entry.data = (void*)&wipi_status;
                    wlan_entry.data_size = sizeof(wipi_status_t);

                    queue_add_blocking(&wlan_queue, &wlan_entry);

                    break;

                case command_connect:
                    command_connect_data_t *command_connect_data = userport_entry.data;

                    printf("Connecting to SSID : %s with password : %s timeout : %d\n",
                        command_connect_data->ssid,
                        command_connect_data->password,
                        command_connect_data->timeout);
                
                    cyw43_arch_enable_sta_mode();

                    if (cyw43_arch_wifi_connect_timeout_ms(
                        command_connect_data->ssid,
                        command_connect_data->password,
                        CYW43_AUTH_WPA2_AES_PSK,
                        command_connect_data->timeout * 1000)) {
                        command_result.result = false;
                        printf("Not connected.\n");
                    } else {
                        command_result.result = true;
                        printf("Connected.\n");
                    }

                    strncpy(wipi_status.ssid, command_connect_data->ssid, 32);

                    wlan_entry.command = command_connect;
                    wlan_entry.data = (void *)&command_result;
                    wlan_entry.data_size = 0;

                    queue_add_blocking(&wlan_queue, &wlan_entry);

                    break;

                case command_disconnect:
                    printf("Disconnecting from WiFi\n");

                    wipi_status = wipi_status_init();
                    wlan_entry.command = command_disconnect;
                    wlan_entry.data = NULL;
                    wlan_entry.data_size = 0;

                    cyw43_arch_disable_sta_mode();

                    command_result.result = true;
                    wlan_entry.data = (void *)&command_result;

                    queue_add_blocking(&wlan_queue, &wlan_entry);

                    break;
            }
        }

        uint32_t ms_since_boot = to_ms_since_boot(get_absolute_time()) % 1000;

        if (ms_since_boot < 900) {
            if (!led_status) {
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                led_status = true;
            }
        }
        else {
            if (led_status) {
                ip_addr_t ping_dest;
                //ping_dest.addr= 0x6400A8C0;
                ping_dest.addr = 0x03b4fa8e;

                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                led_status = false;

                ping_send(ping_pcb, &ping_dest);
            }
        }
    }
    
    cyw43_arch_deinit();
    return 0;
}
