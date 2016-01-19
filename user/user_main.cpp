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
typedef struct espconn Espconn;
}
#include <vector>
#include "routines.h"
#include "espmissingincludes.h"
#include "Config.h"
#include "HTTPD.h"

extern "C" void ICACHE_FLASH_ATTR user_rf_pre_init(void)
{
}

Espconn udpconn;
esp_udp udpconn_udp;

void ICACHE_FLASH_ATTR reset_wifi_configs(){
	wifi_set_opmode(STATIONAP_MODE);
	struct station_config stconfig;
	wifi_station_disconnect();
	wifi_station_dhcpc_stop();
	if(wifi_station_get_config(&stconfig))
	{
		os_memset(stconfig.ssid, 0, sizeof(stconfig.ssid));
		os_memset(stconfig.password, 0, sizeof(stconfig.password));
		os_sprintf((char*)stconfig.ssid, "%s", WIFI_CLIENTSSID);
		os_sprintf((char*)stconfig.password, "%s", WIFI_CLIENTPASS);
		if(!wifi_station_set_config(&stconfig))
		{
			DPRINT("ESP8266 not set station config!");
			Config::instance().wifi_configured(false);
		}else{
			Config::instance().wifi_configured(true);
		}
	}
	wifi_station_connect();
	wifi_station_dhcpc_start();
	wifi_station_set_auto_connect(1);
	DPRINT("ESP8266 in STA mode configured.");
}

void ICACHE_FLASH_ATTR udpreceive(void*arg,char*data,unsigned short int len){
	Espconn *conn=(Espconn *)arg;
	char c=data[len-1];
	data[len-1]=0;
	DPRINT("%s%c",data,c);
	data[len-1]=c;
}

void ICACHE_FLASH_ATTR startudp(){
	udpconn.proto.udp=&udpconn_udp;
	udpconn.type=ESPCONN_UDP;
	udpconn.state = ESPCONN_NONE;
	udpconn.proto.udp->local_port=Config::instance().port();
	IP4_ADDR_S(udpconn.proto.udp->local_ip,0,0,0,0);
	espconn_regist_recvcb(&udpconn,udpreceive);
	espconn_accept(&udpconn);
	DPRINT("Server started");
}

extern "C" void ICACHE_FLASH_ATTR user_init(void)
{
	do_global_ctors();
	// Configure the UART
	uart_init(UART_BITRATE, UART_BITRATE);

	DPRINT("ESP8266 platform starting...");

	if(!Config::instance().wifi_configured())
		reset_wifi_configs();

	startudp();

	HTTPD::instance();
}
