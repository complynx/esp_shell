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
#include "cJSON.h"


class RequestProcessor{
public:
	typedef void (*Task)(RequestProcessor*processor,cJSON*params);
	struct TaskList{
		char*name;
		Task func;
		TaskList *next;
	};
	cJSON*request;
	cJSON*response;
	static TaskList *tasks;
	RequestProcessor(cJSON*req);
	void send();
	static void registerTask(char*,Task);
	static Task getTask(char*);
	void process();
	void processTask(char*task,cJSON*params);
	void processTask(cJSON*task);
	void taskNotFound(char*task);
	void warn(char*task);
	void whoAmI();
	virtual ~RequestProcessor();
};

class Queue{
public:
	char**buffer;
	u16 next;
	u16 length;
	Queue(u16 len);
	virtual ~Queue();
	char* pop();
	void push(char*);
};

class IoTServer {
private:
	IoTServer();
	Queue*q;
	IoTServer(IoTServer const&);
	void operator=(IoTServer const&);
	static void recv_cb(void*arg,char*data,unsigned short int len);
	static void sent_cb(void*arg);
	void reset_udp();
	char* is_sending;
	struct espconn _conn;
	esp_udp _udp;
	struct ip_info group_ip;
	struct ip_info my_ip;
	void send_current();
public:
	void maybe_reset();
	void send(char*);
	static IoTServer&instance();
	void setIP(struct ip_info&);
	~IoTServer();
};


#endif /* USER_IOTSERVER_H_ */
