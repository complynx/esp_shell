#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#define ESP_PARAM_START_SEC 0x3D
#define PLATFORM_DEBUG		true
#define WIFI_CLIENTSSID "complynx_64"
#define WIFI_CLIENTPASS "ooyamaneko_1234"
#define UART_BITRATE BIT_RATE_115200
#define UDP_PORT 8266

// commas instead of dots for functions and macros
#define GROUP_IP_ADDR_1 238
#define GROUP_IP_ADDR_2 2
#define GROUP_IP_ADDR_3 6
#define GROUP_IP_ADDR_4 6

//#define FULL_HTTP_CODE_LIST 1


#ifdef PLATFORM_DEBUG
#define DPRINT(...)  do{ets_uart_printf("%s:%d: ",__FILE__,__LINE__);ets_uart_printf(__VA_ARGS__);ets_uart_printf("\r\n");}while(0)
#define PL ets_uart_printf("%s:%d\r\n",__FILE__,__LINE__)
#else
#define DPRINT(...)
#define PL
#endif

#endif
