#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "esp_eth_enc28j60.h"
#include "driver/spi_master.h"



esp_eth_mac_t* enc28j60_new_mac( spi_device_handle_t *spi_handle, int INT_GPIO )
{
    eth_enc28j60_config_t enc28j60_config = ETH_ENC28J60_DEFAULT_CONFIG( *spi_handle );
    enc28j60_config.int_gpio_num = INT_GPIO;

    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    mac_config.rx_task_stack_size = 4048;
    mac_config.smi_mdc_gpio_num  = -1; // ENC28J60 doesn't have SMI interface
    mac_config.smi_mdio_gpio_num = -1;
    // mac_config.rx_task_prio      = 1;
    return esp_eth_mac_new_enc28j60( &enc28j60_config, &mac_config );
}


esp_eth_mac_t* enc28j60_begin(int miso_gpio, int mosi_gpio, int sclk_gpio, int cs_gpio, int int_gpio, int spi_clock_mhz, int spi_host)
{
    if(ESP_OK !=gpio_install_isr_service(0)) return NULL;
    /* ENC28J60 ethernet driver is based on spi driver */
    spi_bus_config_t buscfg =
    {
        .miso_io_num   = miso_gpio,
        .mosi_io_num   = mosi_gpio,
        .sclk_io_num   = sclk_gpio,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    if( ESP_OK != spi_bus_initialize( spi_host, &buscfg, SPI_DMA_CH_AUTO )) return NULL;

    spi_device_interface_config_t devcfg =
    {
        .command_bits     = 3,
        .address_bits     = 5,
        .mode             = 0,
        .clock_speed_hz   = spi_clock_mhz * 1000 * 1000,
        .spics_io_num     = cs_gpio,
        .queue_size       = 1,
        .cs_ena_posttrans = enc28j60_cal_spi_cs_hold_time(spi_clock_mhz),
    };

    spi_device_handle_t spi_handle = NULL;
    if(ESP_OK != spi_bus_add_device( spi_host, &devcfg, &spi_handle )) return NULL;

    return enc28j60_new_mac( &spi_handle, int_gpio );
}
