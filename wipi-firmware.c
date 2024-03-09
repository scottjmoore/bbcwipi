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

static const uint32_t command_wipi_status = 0x00;

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

        rxtx_send_mode();
        switch (rx_byte) {
            case command_wipi_status:
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

    if (cyw43_arch_init_with_country(CYW43_COUNTRY_UK)) {
        debug(DEBUG_ERR,"Wi-Fi init failed");
        return -1;
    }

    cyw43_arch_enable_sta_mode();

    bool led_status = true;
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
        while (!queue_is_empty(&userport_queue)) {
            queue_entry_t userport_entry;
            queue_entry_t wlan_entry;
            
            queue_remove_blocking(&userport_queue, &userport_entry);

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
                    strncpy(wipi_status.ssid, WIFI_SSID, 32);

                    wlan_entry.command = command_wipi_status;
                    wlan_entry.data = (void*)&wipi_status;
                    wlan_entry.data_size = sizeof(wipi_status_t);

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