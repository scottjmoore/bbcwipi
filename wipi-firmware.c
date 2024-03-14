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

static struct raw_pcb *icmp_pcb;
static struct raw_pcb *tcp_pcb;
static struct raw_pcb *udp_pcb;

static const uint8_t* ip_ptos(uint8_t protocol) {
    static const uint8_t* p_1  = "ICMP";
    static const uint8_t* p_2  = "IGMP";
    static const uint8_t* p_6  = "TCP";
    static const uint8_t* p_17 = "UDP";
    static const uint8_t* p_default = "UNKNOWN";

    switch (protocol) {
        case 1:
            return p_1;
        case 2:
            return p_2;
        case 6:
            return p_6;
        case 17:
            return p_17;
        default:
            return p_default;
    }
}

static uint8_t raw_recv_callback(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr)
{
    LWIP_ASSERT("p != NULL", p != NULL);

    uint8_t protocol = (pcb)->protocol;
    uint32_t source = ((uint32_t*)addr)[0];

    if (protocol != 1) {
        return false;
    }
    
    debug(DEBUG_LOG,"static uint8_t raw_recv_callback(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr)\n");
    debug(DEBUG_LOG,"\targ         = %08x\n",arg);
    debug(DEBUG_LOG,"\tpcb         = %08x\n",pcb);
    debug(DEBUG_LOG,"\tp           = %08x\n",p);
    debug(DEBUG_LOG,"\taddr        = %08x\n\n",addr);

    debug(DEBUG_LOG,"\tProtocol    = %d (%s)\n",protocol,ip_ptos(protocol));
    debug(DEBUG_LOG,"\tPayload     = %d byte(s)\n",p->len);
    debug(DEBUG_LOG,"\tSource      = %d.%d.%d.%d\n\n",
        (source>>0)&0xff,
        (source>>8)&0xff,
        (source>>16)&0xff,
        (source>>24)&0xff);

    
    debug(DEBUG_LOG,"\tIP Header   = ");
    for (uint8_t i = 0; i < 20; i++) {
        debug(DEBUG_LOG,"%02x ", ((uint8_t*)p->payload)[i]);
    }
    debug(DEBUG_LOG,"\n");

    uint8_t c = 0;

    switch (protocol) {
        case 1:
            debug(DEBUG_LOG,"\tICMP Header = ");
            for (uint16_t i = 0; i < 4; i++) {
                debug(DEBUG_LOG,"%02x ", ((uint8_t*)p->payload)[20+i]);
            }
            debug(DEBUG_LOG,"\n");
            debug(DEBUG_LOG,"\tICMP Data   = ");
            for (uint16_t i = 0; i < ((p->len)-24); i++) {
                uint8_t data = ((uint8_t*)p->payload)[24+i];
                debug(DEBUG_LOG,"%02x ", data);
                if (++c >= 16) {
                    c = 0;
                    for (int16_t j = -15; j <= 0; j++) {
                        uint8_t data = ((uint8_t*)p->payload)[24+i+j];
                        debug(DEBUG_LOG,"%c", data>31&&data<127?data:'.');
                    }
                    debug(DEBUG_LOG,"\n\t              ");
                }
            }
            if (c > 0) {
                for (uint16_t j = 0; j < (16-c); j++) {
                    debug(DEBUG_LOG,".. ");
                }
                for (int16_t k = -c; k < 0; k++) {
                    uint8_t data = ((uint8_t*)p->payload)[(p->len)+k];
                    debug(DEBUG_LOG,"%c", data>31&&data<127?data:'.');
                }
            }
            debug(DEBUG_LOG,"\n");
            break;

        case 6:
            debug(DEBUG_LOG,"\tTCP Header  = ");
            for (uint16_t i = 0; i < 20; i++) {
                debug(DEBUG_LOG,"%02x ", ((uint8_t*)p->payload)[20+i]);
            }
            debug(DEBUG_LOG,"\n");
            debug(DEBUG_LOG,"\tTCP Data    = ");
            for (uint16_t i = 0; i < ((p->len)-24); i++) {
                uint8_t data = ((uint8_t*)p->payload)[24+i];
                debug(DEBUG_LOG,"%02x ", data);
                if (++c >= 16) {
                    c = 0;
                    for (int16_t j = -15; j <= 0; j++) {
                        uint8_t data = ((uint8_t*)p->payload)[24+i+j];
                        debug(DEBUG_LOG,"%c", data>31&&data<127?data:'.');
                    }
                    debug(DEBUG_LOG,"\n\t              ");
                }
            }
            if (c > 0) {
                for (uint16_t j = 0; j < (16-c); j++) {
                    debug(DEBUG_LOG,".. ");
                }
                for (int16_t k = -c; k < 0; k++) {
                    uint8_t data = ((uint8_t*)p->payload)[(p->len)+k];
                    debug(DEBUG_LOG,"%c", data>31&&data<127?data:'.');
                }
            }
            debug(DEBUG_LOG,"\n");
            break;

        case 17:
            debug(DEBUG_LOG,"\tUDP Header  = ");
            for (uint16_t i = 0; i < 8; i++) {
                debug(DEBUG_LOG,"%02x ", ((uint8_t*)p->payload)[20+i]);
            }
            debug(DEBUG_LOG,"\n");
            debug(DEBUG_LOG,"\tUDP Data    = ");
            for (uint16_t i = 0; i < ((p->len)-24); i++) {
                uint8_t data = ((uint8_t*)p->payload)[24+i];
                debug(DEBUG_LOG,"%02x ", data);
                if (++c >= 16) {
                    c = 0;
                    for (int16_t j = -15; j <= 0; j++) {
                        uint8_t data = ((uint8_t*)p->payload)[24+i+j];
                        debug(DEBUG_LOG,"%c", data>31&&data<127?data:'.');
                    }
                    debug(DEBUG_LOG,"\n\t              ");
                }
            }
            if (c > 0) {
                for (uint16_t j = 0; j < (16-c); j++) {
                    debug(DEBUG_LOG,".. ");
                }
                for (int16_t k = -c; k < 0; k++) {
                    uint8_t data = ((uint8_t*)p->payload)[(p->len)+k];
                    debug(DEBUG_LOG,"%c", data>31&&data<127?data:'.');
                }
            }
            debug(DEBUG_LOG,"\n");
            break;
    }

    debug(DEBUG_LOG,"\n");

    return 0;

    //pbuf_free(p);
    //return 1; /* eat the packet */
}

typedef struct {
    uint8_t command;
    void* data;
    uint32_t data_size;
} queue_entry_t;

queue_t userport_queue;
queue_t wlan_queue;

typedef struct {
    struct pbuf *p;
    uint32_t data_size;
} ip_queue_entry_t;

queue_t ip_icmp_queue;
queue_t ip_tcp_queue;
queue_t ip_udp_queue;

void userport_core_main() {
    userport_init_gpio();
    userport_set_dir(GPIO_IN);

    char rx_block[256];
    char tx_block[256];

    while (true) {
        queue_entry_t userport_entry;
        queue_entry_t wlan_entry;
        ip_queue_entry_t ip_icmp_entry;

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
                rxtx_send_dword(status->rssi);
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

                break;

            case command_disconnect:
                rxtx_send_mode();

                userport_entry.command = command_disconnect;
                userport_entry.data = NULL;
                userport_entry.data_size = 0;

                queue_add_blocking(&userport_queue, &userport_entry);
                queue_remove_blocking(&wlan_queue, &wlan_entry);

                rxtx_send_byte(((command_result_t*)wlan_entry.data)->result);

                break;

            case command_send_icmp:
                command_send_icmp_data_t send_icmp_data;

                userport_entry.command = command_send_icmp;
                userport_entry.data = (void *)&send_icmp_data;
                userport_entry.data_size = sizeof(send_icmp_data);

                send_icmp_data.destination_ip = rxtx_recv_dword();
                send_icmp_data.type = rxtx_recv_byte();
                send_icmp_data.code = rxtx_recv_byte();
                send_icmp_data.packet_size = rxtx_recv_byte();
            
                for (uint8_t i = 0; i < send_icmp_data.packet_size; i++) {
                    send_icmp_data.packet[i] = rxtx_recv_byte();
                }

                rxtx_send_mode();

                queue_add_blocking(&userport_queue, &userport_entry);
                queue_remove_blocking(&wlan_queue, &wlan_entry);

                rxtx_send_byte(((command_result_t*)wlan_entry.data)->result);

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

    icmp_pcb = raw_new(IP_PROTO_ICMP);
    tcp_pcb = raw_new(IP_PROTO_TCP);
    udp_pcb = raw_new(IP_PROTO_UDP);

    LWIP_ASSERT("icmp_pcb != NULL", icmp_pcb != NULL);
    LWIP_ASSERT("tcp_pcb != NULL", tcp_pcb != NULL);
    LWIP_ASSERT("udp_pcb != NULL", udp_pcb != NULL);

    raw_recv(icmp_pcb, raw_recv_callback, NULL);
    raw_bind(icmp_pcb, IP_ADDR_ANY);
    raw_recv(tcp_pcb, raw_recv_callback, NULL);
    raw_bind(tcp_pcb, IP_ADDR_ANY);
    raw_recv(udp_pcb, raw_recv_callback, NULL);
    raw_bind(udp_pcb, IP_ADDR_ANY);

    while (true) {
        while (!queue_is_empty(&userport_queue)) {
            queue_entry_t userport_entry;
            queue_entry_t wlan_entry;
            ip_queue_entry_t ip_icmp_entry;
            command_result_t command_result;

            queue_remove_blocking(&userport_queue, &userport_entry);

            debug(DEBUG_INFO,"WLAN command received: %d\n", userport_entry.command);

            switch (userport_entry.command) {
                case command_wipi_status:
                    wipi_status_t wipi_status;

                    //debug(DEBUG_LOG,"WLAN status requested\n");

                    cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);

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

                    debug(DEBUG_LOG,"Connecting to SSID : %s (timeout = %d)\n",
                        command_connect_data->ssid,
                        command_connect_data->timeout);
                
                    cyw43_arch_enable_sta_mode();

                    if (cyw43_arch_wifi_connect_timeout_ms(
                        command_connect_data->ssid,
                        command_connect_data->password,
                        CYW43_AUTH_WPA2_AES_PSK,
                        command_connect_data->timeout * 1000)) {
                        command_result.result = false;
                        debug(DEBUG_LOG,"Not connected\n");
                    } else {
                        command_result.result = true;
                        debug(DEBUG_LOG,"Connected\n");
                    }

                    strncpy(wipi_status.ssid, command_connect_data->ssid, 32);

                    wlan_entry.command = command_connect;
                    wlan_entry.data = (void *)&command_result;
                    wlan_entry.data_size = 0;

                    queue_add_blocking(&wlan_queue, &wlan_entry);

                    break;

                case command_disconnect:
                    debug(DEBUG_LOG,"Disconnecting from WiFi\n");

                    wipi_status = wipi_status_init();
                    wlan_entry.command = command_disconnect;
                    wlan_entry.data = NULL;
                    wlan_entry.data_size = 0;

                    cyw43_arch_disable_sta_mode();

                    command_result.result = true;
                    wlan_entry.data = (void *)&command_result;

                    queue_add_blocking(&wlan_queue, &wlan_entry);

                    break;

                case command_send_icmp:
                    command_send_icmp_data_t *command_send_icmp_data = userport_entry.data;

                    debug(DEBUG_LOG,"Sending ICMP packet\n");

                    debug(DEBUG_LOG,"\tDestination IP   = 0x%08x\n",command_send_icmp_data->destination_ip);
                    debug(DEBUG_LOG,"\tType             = %d\n",command_send_icmp_data->type);
                    debug(DEBUG_LOG,"\tCode             = %d\n",command_send_icmp_data->code);
                    debug(DEBUG_LOG,"\tPacket Size      = %d\n",command_send_icmp_data->packet_size);

                    if (command_send_icmp_data->packet_size > 0) {
                        debug(DEBUG_LOG,"\tPacket           = ",command_send_icmp_data->packet_size);

                        for (uint8_t i = 0; i<command_send_icmp_data->packet_size; i++) {
                            uint8_t c = command_send_icmp_data->packet[i];
                            debug(DEBUG_LOG,"%c",(c>31)&&(c<127)?c:'.');
                        }
                        debug(DEBUG_LOG,"\n\n");
                    }

                    wlan_entry.command = command_send_icmp;
                    wlan_entry.data = NULL;
                    wlan_entry.data_size = 0;

                    struct pbuf *p;

                    struct icmp_hdr *icmp;
    
                    size_t icmp_size = sizeof(struct icmp_hdr) + command_send_icmp_data->packet_size;

                    p = pbuf_alloc(PBUF_IP, (u16_t)icmp_size, PBUF_RAM);
                    if (!p) {
                        command_result.result = false;
                        wlan_entry.data = (void *)&command_result;
                        queue_add_blocking(&wlan_queue, &wlan_entry);
                    }
                    if ((p->len == p->tot_len) && (p->next == NULL)) {
                        icmp = (struct icmp_hdr *)p->payload;
                        icmp->chksum = 0;
                        icmp->type = command_send_icmp_data->type;
                        icmp->code = command_send_icmp_data->code;

                        for(uint32_t i = 0; i < command_send_icmp_data->packet_size; i++) {
                            ((char*)icmp)[sizeof(struct icmp_hdr) + i] = command_send_icmp_data->packet[i];
                        }

                        icmp->chksum = inet_chksum(icmp, icmp_size);
                        ip_addr_t dest = IPADDR4_INIT(command_send_icmp_data->destination_ip);
                        raw_sendto(icmp_pcb, p, &dest);
                    }
                    
                    pbuf_free(p);
                    command_result.result = true;
                    wlan_entry.data = (void *)&command_result;
                    queue_add_blocking(&wlan_queue, &wlan_entry);
                    //queue_add_blocking(&ip_icmp_queue, &ip_icmp_entry);
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
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                led_status = false;

            }
        }
    }
    
    cyw43_arch_deinit();
    return 0;
}
