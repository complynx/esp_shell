#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#define USE_OPTIMIZE_PRINTF

#define ESP_PARAM_SEC_1 0x0e
#define ESP_PARAM_SEC_2 0x0f
#define PLATFORM_DEBUG		true
#define WIFI_CLIENTSSID "ssid"
#define WIFI_CLIENTPASS "pass"
#define UART_BITRATE BIT_RATE_115200
#define GROUP_UDP_PORT 8266
#define MY_TYPE "RGB LED"
#define BUFFER_LENGTH 16
#define PWM_PERIOD 1000
#define MAX_DUTY PWM_PERIOD*1000/45

#define SNTP_SERVER_0 "ru.pool.ntp.org"
#define SNTP_SERVER_1 "ntp4.stratum2.ru"
#define SNTP_TZ 3//MSK

// commas instead of dots for functions and macros
#define GROUP_IP_ADDR_1 238
#define GROUP_IP_ADDR_2 2
#define GROUP_IP_ADDR_3 6
#define GROUP_IP_ADDR_4 6

#define ICACHE_STORE_ATTR __attribute__((aligned(4)))
//#define FULL_HTTP_CODE_LIST 1


#ifdef PLATFORM_DEBUG
#define DPRINT(...)  do{ets_uart_printf("%s:%d: ",__FILE__,__LINE__);ets_uart_printf(__VA_ARGS__);ets_uart_printf("\r\n");}while(0)
#define PL ets_uart_printf("%s:%d\r\n",__FILE__,__LINE__)
#define PM ets_uart_printf("%s:%d, heap: %d\r\n",__FILE__,__LINE__,system_get_free_heap_size())
#else
#define DPRINT(...)
#define PL
#define PM
#endif

#endif
