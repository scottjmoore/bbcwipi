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

#include "status.h"
#include "rxtx.h"

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif

/*
GPIO Pinout

BBC Userport PB0 <-> GPIO0
BBC Userport PB1 <-> GPIO1
BBC Userport PB2 <-> GPIO2
BBC Userport PB3 <-> GPIO3
BBC Userport PB4 <-> GPIO4
BBC Userport PB5 <-> GPIO5
BBC Userport PB6 <-> GPIO6
BBC Userport PB7 <-> GPIO7
BBC Userport CB2 --> GPIO8
BBC Userport CB1 <-- GPIO9

74LVC245A(1) DIR <-- GPIO10 
*/

wipi_status_t status;

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
        rxtx_recv_mode();
        char rx_byte = rxtx_recv_byte();

        rxtx_send_mode();
        switch (rx_byte) {
            case 0x00:
                rxtx_send_byte(status.is_connected);
                rxtx_send_int(status.rssi);
                rxtx_send_byte(status.channel);
                rxtx_send_ip(status.ip);
                rxtx_send_ip(status.gateway);
                rxtx_send_ip(status.netmask);
                rxtx_send_block(status.ssid, 32);
                break;
            case 0x01:
                rxtx_send_byte(0x55);
                rxtx_send_byte(0xaa);
            default:
                break;
        }
    }
}

int32_t cyw43_local_get_connection_rssi() {
        int32_t rssi;
        cyw43_ioctl(&cyw43_state, 127<<1, sizeof rssi, (uint8_t *)&rssi, CYW43_ITF_STA);
        return rssi;
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

    status = wipi_status_init();

    if (cyw43_arch_init_with_country(CYW43_COUNTRY_UK)) {
        debug(DEBUG_ERR,"Wi-Fi init failed");
        return -1;
    }

    cyw43_arch_enable_sta_mode();
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    netif_set_hostname(cyw43_state.netif, "bbc-master-wipi");
    netif_set_up(cyw43_state.netif);

    printf("Attempting to connect to : %s\n", WIFI_SSID);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        debug(DEBUG_ERR,"Failed to connect.\n");
    } else {
        debug(DEBUG_LOG,"Connected.\n");
    }

    while (true) {
        cyw43_tcpip_link_status(&cyw43_state,CYW43_ITF_STA);

        status.is_connected = true;
        status.rssi = cyw43_local_get_connection_rssi();
        status.channel = cyw43_local_get_connection_channel();
        status.ip = cyw43_state.netif->ip_addr;
        status.gateway = cyw43_state.netif->gw;
        status.netmask = cyw43_state.netif->netmask;
        strncpy(status.ssid, WIFI_SSID, 32);
        status.hostname = (char*)(cyw43_state.netif->hostname);

        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(900);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(100);
    }
    
    cyw43_arch_deinit();
    return 0;
}