/*
	The hello world c++ demo
*/

#include <ets_sys.h>
#include "osapi.h"
#include <os_type.h>
#include <gpio.h>
extern "C"{
#	include "driver/uart.h"
#	include "user_interface.h"
#	include "user_config.h"
#	include "espconn.h"
#   include "sntp.h"
}
#include "routines.h"
#include "espmissingincludes.h"
#include "Config.h"
#include "IoTServer.h"
#include "cJSON.h"
#include "RGBLED.h"

extern "C" void ICACHE_FLASH_ATTR user_rf_pre_init(void)
{
}



void ICACHE_FLASH_ATTR reset_wifi_configs(){
	DPRINT("Resetting WiFi config...");
	wifi_set_opmode(STATION_MODE);
	struct station_config stconfig;
	wifi_station_disconnect();
	wifi_station_dhcpc_stop();
	if(wifi_station_get_config(&stconfig))
	{
		DPRINT("Setting config...");
		os_memset(stconfig.ssid, 0, sizeof(stconfig.ssid));
		os_memset(stconfig.password, 0, sizeof(stconfig.password));
		os_sprintf((char*)stconfig.ssid, "%s", WIFI_CLIENTSSID);
		os_sprintf((char*)stconfig.password, "%s", WIFI_CLIENTPASS);

		Config::I().wifi_configured(wifi_station_set_config(&stconfig));
		DPRINT("Config setting %s",Config::I().wifi_configured()?"success":"failed");
	}
	wifi_station_connect();
	wifi_station_dhcpc_start();
	wifi_station_set_auto_connect(1);
	DPRINT("ESP8266 in STA mode configured.");
}

static ETSTimer WiFiCheck;

static void ICACHE_FLASH_ATTR wifi_check_ip(void *arg)
{
	os_timer_disarm(&WiFiCheck);

	switch(wifi_station_get_connect_status())
	{
		case STATION_GOT_IP:
			struct ip_info LocalIP;
			wifi_get_ip_info(STATION_IF, &LocalIP);
			if(LocalIP.ip.addr != 0) {
				DPRINT("WiFi connected");
				Config::I().errno(ERRNO_OK);

				IoTServer::instance().setIP(LocalIP);
				return;
			}
			break;
		case STATION_WRONG_PASSWORD:
		case STATION_CONNECT_FAIL:
		case STATION_NO_AP_FOUND:
			Config::I().errno(STATION_ERROR_TO_ERRNO(wifi_station_get_connect_status()));
			DPRINT("WiFi error #%d, see <user_interface.h> for info.",wifi_station_get_connect_status());
			break;
		default:
			DPRINT("WiFi connecting...");
	}
	os_timer_setfn(&WiFiCheck, (os_timer_func_t *)wifi_check_ip, NULL);
	os_timer_arm(&WiFiCheck, 1000, 0);
}
#define TEST_MEM 4096
#define MAX_SEC 0x80
void ICACHE_FLASH_ATTR test_flash(){
	char mem[TEST_MEM];
	ets_uart_printf("Clean sectors:\r\n");
	for(unsigned int i=0;i<MAX_SEC;++i){
		spi_flash_read(i*SPI_FLASH_SEC_SIZE, (unsigned int*)mem,TEST_MEM);
		char b=mem[0];
		bool c=false;
		for(unsigned int j=1;j<TEST_MEM;++j){
			if(mem[j]!=b){
				c=true;
				break;
			}
		}
		if(!c)
			ets_uart_printf("%x ",i);
	}
	ets_uart_printf("\r\n");
}

extern "C" void ICACHE_FLASH_ATTR user_init(void)
{
	do_global_ctors();
	// Configure the UART
	uart_init(UART_BITRATE, UART_BITRATE);

	DPRINT("ESP8266 platform starting...");
//	Config::instance().zero();
	DPRINT("My name: %s",Config::I().name());
	if(!strlen(Config::I().name())){
		Config::I().reset();
		DPRINT("My name: %s",Config::I().name());
	}

	Config::I().errno(ERRNO_OK);


	if(!Config::I().wifi_configured())
		reset_wifi_configs();

//	startudp();

	os_timer_disarm(&WiFiCheck);
	Config::I().errno(ERRNO_UNDEFINED);
	os_timer_setfn(&WiFiCheck, (os_timer_func_t *)wifi_check_ip, NULL);
	os_timer_arm(&WiFiCheck, 1000, 0);

	RGBLED::I();
	test_flash();

	PM;
//	HTTPD::instance();
}
