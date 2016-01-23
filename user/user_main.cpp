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
}
#include "routines.h"
#include "espmissingincludes.h"
#include "Config.h"
#include "IoTServer.h"
#include "cJSON.h"

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
		if(!wifi_station_set_config(&stconfig))
		{
			DPRINT("Config setting failed");
			Config::I().wifi_configured(false);
		}else{
			DPRINT("Config setting success");
			Config::I().wifi_configured(true);
		}
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

				IoTServer::instance().setIP(LocalIP);
				return;
			}
			break;
		case STATION_WRONG_PASSWORD:
			DPRINT("WiFi connecting error, wrong password");
			break;
		case STATION_NO_AP_FOUND:
			DPRINT("WiFi connecting error, ap not found");
			break;
		case STATION_CONNECT_FAIL:
			DPRINT("WiFi connecting fail\r\n");
			break;
		default:
			DPRINT("WiFi connecting...");
	}
	os_timer_setfn(&WiFiCheck, (os_timer_func_t *)wifi_check_ip, NULL);
	os_timer_arm(&WiFiCheck, 1000, 0);
}

extern "C" void ICACHE_FLASH_ATTR user_init(void)
{
	do_global_ctors();
	// Configure the UART
	uart_init(UART_BITRATE, UART_BITRATE);

	DPRINT("ESP8266 platform starting...");
//	Config::instance().zero();

	if(!Config::I().wifi_configured())
		reset_wifi_configs();

//	startudp();

	os_timer_disarm(&WiFiCheck);
	os_timer_setfn(&WiFiCheck, (os_timer_func_t *)wifi_check_ip, NULL);
	os_timer_arm(&WiFiCheck, 1000, 0);

	cJSON *root,*format;
	int framerate;
	char* rendered;
	root = cJSON_Parse("{\n    \"name\": \"Jack (\\\"Bee\\\") Nimble\",\n    \"format\": {\n        \"type\":       \"rect\",\n        \"width\":      1920, \n        \"height\":     1080, \n        \"interlace\":  false, \n        \"frame rate\": 24\n    }\n}");
	format = cJSON_GetObjectItem(root,"format");
	framerate = cJSON_GetObjectItem(format,"frame rate")->valueint;
	DPRINT("%d",framerate);
	cJSON_SetIntValue(cJSON_GetObjectItem(format,"frame rate"),25);
	rendered = cJSON_Print(root);
	DPRINT("%s",rendered);
	cJSON_Delete(root);
	os_free(rendered);


	PM;
//	HTTPD::instance();
}
