/*
 * IoTServer.h
 *
 *  Created on: 20 џэт. 2016 у.
 *      Author: complynx
 */

#ifndef USER_IOTSERVER_H_
#define USER_IOTSERVER_H_

#include "user_config.h"
extern "C"{
#include "espconn.h"
}

class IoTServer {
private:
	IoTServer();
	IoTServer(IoTServer const&);
	void operator=(IoTServer const&);
	static void recv_cb(void*arg,char*data,unsigned short int len);
	struct espconn _conn;
	esp_udp _udp;
	struct ip_info group_ip;
	struct ip_info my_ip;
public:
	static IoTServer&instance();
	void setIP(struct ip_info&);
	~IoTServer();
};

#endif /* USER_IOTSERVER_H_ */
