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


static ETSTimer ledcycle;
static int msec=20,state=0, i=0;

static void ICACHE_FLASH_ATTR cycle_tick(){
	os_timer_disarm(&ledcycle);
	RGBLED* l=&RGBLED::I();
	u8 r=l->R(),g=l->G(),b=l->B();
	switch(state){
	case 0://dim to black
		if(!r && !g && !b){
			++state;
		}else{
			if(r) --r;
			if(g) --g;
			if(b) --b;
		}
		break;
	case 6:
	case 1://light up red;
		if(r==255)	++state;
		else		++r;
		break;
	case 2://light up green;
		if(g==255)	++state;
		else		++g;
		break;
	case 3://light down red;
		if(!r)	++state;
		else	--r;
		break;
	case 4://light up blue;
		if(b==255)	++state;
		else		++b;
		break;
	case 5://light down green;
		if(!g)	++state;
		else	--g;
		break;
	case 7://light down blue;
		if(!b)	++state;
		else	--b;
		break;
	case 8://light up all;
		if(b==255 && r==255 && g==255)	++state;
		else{
			if(b<255) ++b;
			if(g<255) ++g;
			if(r<255) ++r;
		}
		break;
	default:
//		DPRINT("Full cycle passed");
		state=0;
	}

	l->color(r,g,b);
	++i;
	if(i>100){
		uint32 current_stamp;
		current_stamp = sntp_get_current_timestamp();
		if(current_stamp){
			DPRINT("sntp: %d, %s",current_stamp,
			sntp_get_real_time(current_stamp));
			PM;
		}else{
			DPRINT("no sntp info");
		}
		i=0;
		DPRINT("RGB: %d %d %d, state: %d",r,g,b,state);
	}

	os_timer_setfn(&ledcycle, (os_timer_func_t *)cycle_tick, NULL);
	os_timer_arm(&ledcycle, msec, 0);
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

	RGBLED::I().color(0x00FFFFFF);

	sntp_setservername(0, (char*)"ru.pool.ntp.org"); // set server 0 by domain name
	sntp_setservername(1, (char*)"ntp4.stratum2.ru"); // set server 1 by domain name
	sntp_set_timezone(3);
	sntp_init();

	os_timer_disarm(&ledcycle);
	os_timer_setfn(&ledcycle, (os_timer_func_t *)cycle_tick, NULL);
	os_timer_arm(&ledcycle, 5000, 0);


	PM;
//	HTTPD::instance();
}
