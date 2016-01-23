/*
 * IoTServer.cpp
 *
 *  Created on: 20 џэт. 2016 у.
 *      Author: complynx
 */

#include "IoTServer.h"
#include "Config.h"
#include "routines.h"
#include "espmissingincludes.h"
extern "C"{
#include "ip_addr.h"
#include "driver/uart.h"
#include "osapi.h"
#include "user_interface.h"
}

ICACHE_FLASH_ATTR IoTServer::IoTServer() {
	_conn.proto.udp=&_udp;
	_conn.type=ESPCONN_UDP;
	_conn.state = ESPCONN_NONE;
	_udp.local_port=Config::instance().port();
	_udp.remote_port=Config::instance().port();
	DPRINT("Server port: %d",_udp.local_port);
	IP4_ADDR_S(_udp.remote_ip,GROUP_IP_ADDR_1,GROUP_IP_ADDR_2,GROUP_IP_ADDR_3,GROUP_IP_ADDR_4);
	_conn.reverse=0;
	espconn_regist_recvcb(&_conn,recv_cb);
	espconn_create(&_conn);
	IP4_ADDR(&group_ip.ip, GROUP_IP_ADDR_1,GROUP_IP_ADDR_2,GROUP_IP_ADDR_3,GROUP_IP_ADDR_4);
	DPRINT("Server started.");
	PM;
}

ICACHE_FLASH_ATTR void IoTServer::setIP(struct ip_info&ip){
	os_memcpy(&my_ip,&ip,sizeof(my_ip));
	DPRINT("My IP: %d.%d.%d.%d",my_ip.ip.addr&0xff,(my_ip.ip.addr>>8)&0xff,(my_ip.ip.addr>>16)&0xff,(my_ip.ip.addr>>24)&0xff);
	DPRINT("Group IP: %d.%d.%d.%d",group_ip.ip.addr&0xff,(group_ip.ip.addr>>8)&0xff,(group_ip.ip.addr>>16)&0xff,(group_ip.ip.addr>>24)&0xff);
	espconn_igmp_join(&my_ip.ip, &group_ip.ip);
	DPRINT("Joined multicast group.");
	PM;
}

static IoTServer* srv=0;

ICACHE_FLASH_ATTR void IoTServer::recv_cb(void*arg,char*data,unsigned short int len){
	struct espconn *pconn=(struct espconn *)arg;
	char c=data[len-1];
	data[len-1]=0;
	DPRINT("%s%c",data,c);
	data[len-1]=c;
	PM;
}

ICACHE_FLASH_ATTR IoTServer& IoTServer::instance(){
	if(!srv)
		srv=new IoTServer();

	return *srv;
}

ICACHE_FLASH_ATTR IoTServer::~IoTServer() {
	// TODO Auto-generated destructor stub
}

