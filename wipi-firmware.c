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

#include "lwip/dns.h"
#include "lwip/icmp.h"
#include "lwip/inet_chksum.h"
#include "lwip/mem.h"
#include "lwip/netif.h"
#include "lwip/raw.h"
#include "lwip/sys.h"

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

wipi_status_t wipi_status;

bool dns_inprogress;
uint32_t dns_inprogress_ip;

typedef struct {
    uint8_t command;
    void* data;
    uint32_t data_size;
} queue_entry_t;

queue_t userport_queue;
queue_t wlan_queue;

typedef struct {
    void* data;
    uint32_t data_size;
} ip_queue_entry_t;

queue_t ip_icmp_queue;
queue_t ip_tcp_queue;
queue_t ip_udp_queue;

typedef struct {
    uint8_t     version_ihl;
    uint8_t     dscp_ecn;
    uint16_t    total_length;
    uint16_t    identification;
    uint16_t    flags_fragment;
    uint8_t     ttl;
    uint8_t     protocol;
    uint16_t    header_checksum;
    uint32_t    source_ip;
    uint32_t    destination_ip;
} ip_header_t, *ip_header_ptr;

typedef struct {
    uint8_t     type;
    uint8_t     code;
    uint16_t    checksum;
} icmp_header_t, *icmp_header_ptr;

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

static void dns_callback(const char *name, const ip_addr_t *ipaddr, void *arg) {
    debug(DEBUG_LOG,"DNS callback: %s -> %08x\n", name, ipaddr->addr);

    command_dns_lookup_result_ptr command_dns_lookup_result = arg;

    command_dns_lookup_result->ip = ipaddr->addr;
    command_dns_lookup_result->result = true;
}

static uint8_t raw_icmp_recv_callback(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr)
{
    LWIP_ASSERT("p != NULL", p != NULL);

    ip_queue_entry_t ip_icmp_entry;

    uint8_t protocol = (pcb)->protocol;
    uint32_t source = ((uint32_t*)addr)[0];

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

            ip_icmp_entry.data = p;
            ip_icmp_entry.data_size = p->len;

            if (queue_try_add(&ip_icmp_queue, &ip_icmp_entry)) {
                debug(DEBUG_LOG,"ICMP Packet added to queue %08x\n\n",p->payload);
                return 1;
            }

            break;
    }

    debug(DEBUG_LOG,"\n");
    debug(DEBUG_LOG,"Packet not accepted\n\n");

    return 0; // pass the packet on to rest of the network stack
}

static uint8_t raw_udp_recv_callback(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr)
{
    return 0; // pass the packet on to rest of the network stack
}

void userport_core_main() {
    userport_init_gpio();
    userport_set_dir(GPIO_IN);

    // while (true) {
    //     sleep_us(1);
    //     if (gpio_get(USERPORT_CB2) == 1) {
    //         gpio_put(USERPORT_CB1,0);
    //         sleep_us(1);
    //         gpio_put(USERPORT_CB1,1);
    //     }
    //     uint16_t last_test;
    //     uint16_t test = (gpio_get(USERPORT_CB1) << 9) |
    //         (gpio_get(USERPORT_CB2) << 8) |
    //         (gpio_get(USERPORT_PB7) << 7) |
    //         (gpio_get(USERPORT_PB6) << 6) |
    //         (gpio_get(USERPORT_PB5) << 5) |
    //         (gpio_get(USERPORT_PB4) << 4) |
    //         (gpio_get(USERPORT_PB3) << 3) |
    //         (gpio_get(USERPORT_PB2) << 2) |
    //         (gpio_get(USERPORT_PB1) << 1) |
    //         (gpio_get(USERPORT_PB0) << 0);

    //     if (test != last_test) {
    //         debug(DEBUG_LOG,"%010x\n",test);
    //         last_test = test;
    //     }
    // }
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

                wipi_status_t *status = (wipi_status_t*)wlan_entry.data;
                //command_status_data_ptr command_status_data;

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

            case command_dns_lookup:
                command_dns_lookup_data_t command_dns_lookup_data;

                userport_entry.command = command_dns_lookup;
                userport_entry.data = (void *)&command_dns_lookup_data;
                userport_entry.data_size = sizeof(command_dns_lookup_data);

                rxtx_recv_string(command_dns_lookup_data.hostname, 127);
                
                rxtx_send_mode();

                queue_add_blocking(&userport_queue, &userport_entry);
                queue_remove_blocking(&wlan_queue, &wlan_entry);

                rxtx_send_byte(((command_dns_lookup_result_ptr)wlan_entry.data)->result);
                rxtx_send_dword(((command_dns_lookup_result_ptr)wlan_entry.data)->ip);
                break;

            case command_send_icmp:
                command_send_icmp_data_t command_send_icmp_data;

                userport_entry.command = command_send_icmp;
                userport_entry.data = (void *)&command_send_icmp_data;
                userport_entry.data_size = sizeof(command_send_icmp_data);

                command_send_icmp_data.destination_ip = rxtx_recv_dword();
                command_send_icmp_data.type = rxtx_recv_byte();
                command_send_icmp_data.code = rxtx_recv_byte();
                command_send_icmp_data.packet_size = rxtx_recv_word();
            
                command_send_icmp_data.packet_data = mem_malloc(command_send_icmp_data.packet_size);
                for (uint16_t i = 0; i < command_send_icmp_data.packet_size; i++) {
                    command_send_icmp_data.packet_data[i] = rxtx_recv_byte();
                }

                rxtx_send_mode();

                queue_add_blocking(&userport_queue, &userport_entry);
                queue_remove_blocking(&wlan_queue, &wlan_entry);

                rxtx_send_byte(((command_result_t*)wlan_entry.data)->result);
                break;

            case command_receive_icmp:
                userport_entry.command = command_receive_icmp;
                userport_entry.data = NULL;
                userport_entry.data_size = 0;

                rxtx_send_mode();

                queue_add_blocking(&userport_queue, &userport_entry);
                queue_remove_blocking(&wlan_queue, &wlan_entry);

                command_receive_icmp_data_ptr command_receive_icmp_data = wlan_entry.data;

                rxtx_send_dword(command_receive_icmp_data->source_ip);
                rxtx_send_byte(command_receive_icmp_data->type);
                rxtx_send_byte(command_receive_icmp_data->code);
                rxtx_send_word(command_receive_icmp_data->packet_size);

                if (command_receive_icmp_data->packet_size > 0) {
                    for (uint16_t i = 0; i < command_receive_icmp_data->packet_size; i++) {
                        rxtx_send_byte(command_receive_icmp_data->packet_data[i]);
                    }
                mem_free(command_receive_icmp_data->packet_data);
                }
                break;

            case command_send_udp:
                command_send_udp_data_t command_send_udp_data;

                userport_entry.command = command_send_udp;
                userport_entry.data = (void *)&command_send_udp_data;
                userport_entry.data_size = sizeof(command_send_udp_data);

                command_send_udp_data.destination_ip = rxtx_recv_dword();
                command_send_udp_data.destination_port = rxtx_recv_word();
                command_send_udp_data.source_port = rxtx_recv_word();
                command_send_udp_data.packet_size = rxtx_recv_word();
                
                command_send_udp_data.packet_data = mem_malloc(command_send_udp_data.packet_size);
                for (uint16_t i = 0; i < command_send_udp_data.packet_size; i++) {
                    debug(DEBUG_INFO,"%d,%d\n",i,command_send_udp_data.packet_size);
                    command_send_udp_data.packet_data[i] = rxtx_recv_byte();
                }

                rxtx_send_mode();

                queue_add_blocking(&userport_queue, &userport_entry);
                queue_remove_blocking(&wlan_queue, &wlan_entry);

                rxtx_send_byte(((command_result_t*)wlan_entry.data)->result);
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

    queue_init(&userport_queue, sizeof(queue_entry_t), 2);
    queue_init(&wlan_queue, sizeof(queue_entry_t), 2);
    queue_init(&ip_icmp_queue, sizeof(queue_entry_t), 8);
    queue_init(&ip_tcp_queue, sizeof(queue_entry_t), 8);
    queue_init(&ip_udp_queue, sizeof(queue_entry_t), 8);

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

    raw_recv(icmp_pcb, raw_icmp_recv_callback, NULL);
    raw_bind(icmp_pcb, IP_ADDR_ANY);
    //raw_recv(tcp_pcb, raw_recv_callback, NULL);
    //raw_bind(tcp_pcb, IP_ADDR_ANY);
    raw_recv(udp_pcb, raw_udp_recv_callback, NULL);
    raw_bind(udp_pcb, IP_ADDR_ANY);

    while (true) {
        while (!queue_is_empty(&userport_queue)) {
            queue_entry_t userport_entry;
            queue_entry_t wlan_entry;
            command_result_t command_result;

            queue_remove_blocking(&userport_queue, &userport_entry);

            debug(DEBUG_INFO,"WLAN command received: %d\n", userport_entry.command);

            switch (userport_entry.command) {
                case command_wipi_status:
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
                    command_connect_data_ptr command_connect_data = userport_entry.data;

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

                case command_dns_lookup:
                    command_dns_lookup_data_ptr command_dns_lookup_data = userport_entry.data;

                    debug(DEBUG_LOG,"DNS Lookup : %s\n",command_dns_lookup_data->hostname);

                    command_dns_lookup_result_t command_dns_lookup_result;
                    wlan_entry.data = (void *)&command_dns_lookup_result;
                    wlan_entry.data_size = sizeof(command_dns_lookup_result);

                    command_dns_lookup_result.ip = 0xffffffff;
                    command_dns_lookup_result.result = false;

                    ip_addr_t   dns_lookup_ip;
                    err_t      dns_lookup_res = dns_gethostbyname(command_dns_lookup_data->hostname, &dns_lookup_ip, &dns_callback, &command_dns_lookup_result);

                    switch (dns_lookup_res) {
                        case ERR_OK:
                            debug(DEBUG_LOG,"DNS lookup success %08x\n",dns_lookup_ip.addr);
                            command_dns_lookup_result.ip=(uint32_t)(dns_lookup_ip.addr);
                            command_dns_lookup_result.result=true;
                            queue_add_blocking(&wlan_queue, &wlan_entry);                            
                            break;
                        case ERR_INPROGRESS:
                            while (command_dns_lookup_result.result == false) {
                                sleep_ms(100);
                            }
                            queue_add_blocking(&wlan_queue, &wlan_entry);                                                        
                            break;
                        case ERR_ARG:
                            debug(DEBUG_LOG,"DNS lookup failed\n");
                            command_dns_lookup_result.ip=0x00000000;
                            command_dns_lookup_result.result=false;
                            queue_add_blocking(&wlan_queue, &wlan_entry);                            
                            break;
                    }

                    break;

                case command_send_icmp:
                    command_send_icmp_data_ptr command_send_icmp_data = userport_entry.data;

                    debug(DEBUG_LOG,"Sending ICMP packet\n");

                    debug(DEBUG_LOG,"\tDestination IP   = 0x%08x\n",command_send_icmp_data->destination_ip);
                    debug(DEBUG_LOG,"\tType             = %d\n",command_send_icmp_data->type);
                    debug(DEBUG_LOG,"\tCode             = %d\n",command_send_icmp_data->code);
                    debug(DEBUG_LOG,"\tPacket Size      = %d\n",command_send_icmp_data->packet_size);

                    if (command_send_icmp_data->packet_size > 0) {
                        debug(DEBUG_LOG,"\tPacket           = ");

                        for (uint16_t i = 0; i<command_send_icmp_data->packet_size; i++) {
                            uint8_t c = command_send_icmp_data->packet_data[i];
                            debug(DEBUG_LOG,"%c",(c>31)&&(c<127)?c:'.');
                        }
                        debug(DEBUG_LOG,"\n\n");
                    }

                    wlan_entry.command = command_send_icmp;
                    wlan_entry.data = NULL;
                    wlan_entry.data_size = 0;

                    struct pbuf *icmp_p;
                    struct icmp_hdr *icmp;
                    
                    size_t icmp_size = 4 + command_send_icmp_data->packet_size;

                    icmp_p = pbuf_alloc(PBUF_IP, (u16_t)icmp_size, PBUF_RAM);
                    if (!icmp_p) {
                        command_result.result = false;
                        wlan_entry.data = (void *)&command_result;
                        queue_add_blocking(&wlan_queue, &wlan_entry);
                        break;
                    }
                    if ((icmp_p->len == icmp_p->tot_len) && (icmp_p->next == NULL)) {
                        icmp = (struct icmp_hdr *)icmp_p->payload;
                        icmp->chksum = 0;
                        icmp->type = command_send_icmp_data->type;
                        icmp->code = command_send_icmp_data->code;

                        for(uint16_t i = 0; i < command_send_icmp_data->packet_size; i++) {
                            ((uint8_t*)icmp)[4 + i] = command_send_icmp_data->packet_data[i];
                        }
                        mem_free(command_send_icmp_data->packet_data);

                        icmp->chksum = inet_chksum(icmp, icmp_size);
                        ip_addr_t dest = IPADDR4_INIT(command_send_icmp_data->destination_ip);
                        raw_sendto(icmp_pcb, icmp_p, &dest);
                    }
                    
                    pbuf_free(icmp_p);
                    command_result.result = true;
                    wlan_entry.data = (void *)&command_result;
                    queue_add_blocking(&wlan_queue, &wlan_entry);
                    break;

                case command_receive_icmp:
                    command_receive_icmp_data_t command_receive_icmp_data;

                    debug(DEBUG_LOG,"Receiving ICMP packet\n");

                    wlan_entry.command = command_receive_icmp;
                    wlan_entry.data = &command_receive_icmp_data;
                    wlan_entry.data_size = sizeof(command_receive_icmp_data_t);

                    ip_queue_entry_t ip_icmp_entry;
                    bool res;

                    res=queue_try_peek(&ip_icmp_queue, &ip_icmp_entry);
                    debug(DEBUG_LOG,"queue_try_peek(&ip_icmp_queue, &ip_icmp_entry)=%d\n",res);

                    if (!res) {
                        command_receive_icmp_data.source_ip = 0x00000000;
                        command_receive_icmp_data.code = 0;
                        command_receive_icmp_data.type = 0;
                        command_receive_icmp_data.packet_size = 0;
                    } else
                    {
                        if (queue_try_remove(&ip_icmp_queue, &ip_icmp_entry)) {
                            struct pbuf *p = ip_icmp_entry.data;

                            debug(DEBUG_LOG,"ICMP Packet removed from queue %08x\n\n",p->payload);
                            ip_header_t ip_header;
                            icmp_header_t icmp_header;

                            memcpy(&ip_header, p->payload, sizeof(ip_header_t));
                            memcpy(&icmp_header,((uint8_t*)(p->payload))+sizeof(ip_header_t),sizeof(icmp_header_t));
                            debug(DEBUG_LOG,"Source IP = %08x (%d)\n",ip_header.source_ip,sizeof(ip_header_t));
                            debug(DEBUG_LOG,"Type      = %02x\n",icmp_header.type);
                            debug(DEBUG_LOG,"Code      = %02x\n",icmp_header.code);
                            debug(DEBUG_LOG,"%d, %d, %d\n",p->len,sizeof(ip_header_t), sizeof(icmp_header_t));
                            command_receive_icmp_data.source_ip = ip_header.source_ip;
                            command_receive_icmp_data.type = icmp_header.type;
                            command_receive_icmp_data.code = icmp_header.code;
                            command_receive_icmp_data.packet_size = (p->len)-sizeof(ip_header_t)-sizeof(icmp_header_t);
                            
                            debug(DEBUG_LOG,"Source IP = %08x\n",command_receive_icmp_data.source_ip);
                            debug(DEBUG_LOG,"Type      = %02x\n",command_receive_icmp_data.type);
                            debug(DEBUG_LOG,"Code      = %02x\n",command_receive_icmp_data.code);
                            debug(DEBUG_LOG,"Size      = %d\n",command_receive_icmp_data.packet_size);

                            command_receive_icmp_data.packet_data = mem_malloc(command_receive_icmp_data.packet_size);
                            for (uint16_t i = 0; i < command_receive_icmp_data.packet_size; i++) {
                                command_receive_icmp_data.packet_data[i] = ((uint8_t*)(p->payload))[sizeof(ip_header_t)+sizeof(icmp_header_t)+i];
                            }
                            
                            pbuf_free(p);
                        }
                    }

                    queue_add_blocking(&wlan_queue, &wlan_entry);
                    break;

                case command_send_udp:
                    command_send_udp_data_ptr command_send_udp_data = userport_entry.data;

                    debug(DEBUG_LOG,"Sending UDP packet\n");

                    debug(DEBUG_LOG,"\tDestination IP   = 0x%08x\n",command_send_udp_data->destination_ip);
                    debug(DEBUG_LOG,"\tDestination Port = %d\n",command_send_udp_data->destination_port);
                    debug(DEBUG_LOG,"\tSource Port      = %d\n",command_send_udp_data->source_port);
                    debug(DEBUG_LOG,"\tPacket Size      = %d\n",command_send_udp_data->packet_size);

                    if (command_send_udp_data->packet_size > 0) {
                        debug(DEBUG_LOG,"\tPacket           = ");

                        for (uint16_t i = 0; i<command_send_udp_data->packet_size; i++) {
                            uint8_t c = command_send_udp_data->packet_data[i];
                            debug(DEBUG_LOG,"%c",(c>31)&&(c<127)?c:'.');
                        }
                        debug(DEBUG_LOG,"\n\n");
                    }

                    wlan_entry.command = command_send_udp;
                    wlan_entry.data = NULL;
                    wlan_entry.data_size = 0;

                    struct pbuf *udp_p;
                    struct udp_hdr *udp;
                    
                    size_t udp_size = 8 + command_send_udp_data->packet_size;

                    udp_p = pbuf_alloc(PBUF_IP, (u16_t)udp_size, PBUF_RAM);
                    if (!udp_p) {
                        command_result.result = false;
                        wlan_entry.data = (void *)&command_result;
                        queue_add_blocking(&wlan_queue, &wlan_entry);
                        break;
                    }
                    if ((udp_p->len == udp_p->tot_len) && (udp_p->next == NULL)) {
                        udp = (struct udp_hdr *)udp_p->payload;
                        udp->chksum = 0;
                        udp->dest = htons(command_send_udp_data->destination_port);
                        udp->src = htons(command_send_udp_data->source_port);
                        udp->len = htons(command_send_udp_data->packet_size+8);

                        for(uint16_t i = 0; i < command_send_udp_data->packet_size; i++) {
                            ((uint8_t*)udp)[8 + i] = command_send_udp_data->packet_data[i];
                        }
                        mem_free(command_send_udp_data->packet_data);

                        udp->chksum = inet_chksum(udp, udp_size);
                        ip_addr_t dest = IPADDR4_INIT(command_send_udp_data->destination_ip);
                        raw_sendto(udp_pcb, udp_p, &dest);
                    }
                    
                    pbuf_free(udp_p);
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
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                led_status = false;
            }
        }
    }
    
    cyw43_arch_deinit();
    return 0;
}
