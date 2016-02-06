/*
 * IoTServer.cpp
 *
 *  Created on: 20 џэт. 2016 у.
 *      Author: complynx
 */

#include "IoTServer.h"
#include "IoTTasks.h"
#include "Config.h"
#include "routines.h"
#include "espmissingincludes.h"
extern "C"{
#include "ip_addr.h"
#include "driver/uart.h"
#include "osapi.h"
#include "user_interface.h"
}
#include "cJSON.h"

RequestProcessor::TaskList *RequestProcessor::tasks=(RequestProcessor::TaskList*)0;

ICACHE_FLASH_ATTR RequestProcessor::Task RequestProcessor::getTask(char*name){
	DPRINT("Looking for task: %s",name);
	TaskList *I;
	for(I=tasks;I;I=I->next)
		if(!os_strcmp(I->name,name)) return I->func;

	DPRINT("No task found.");
	return (Task)0;
}

ICACHE_FLASH_ATTR void RequestProcessor::registerTask(char* name,RequestProcessor::Task func){
	TaskList *I;
	DPRINT("Registering task: %s",name);
	for(I=tasks;I;I=I->next)
		if(!os_strcmp(I->name,name)) break;
	if(I){
		I->func=func;
	}else{
		I=new TaskList;
		I->func=func;
		I->name=cJSON_strdup(name);
		I->next=tasks;
		tasks=I;
	}
}

ICACHE_FLASH_ATTR void registerDefinedTasks(){
	DPRINT("Registering defined tasks");
#undef IOT_TASK_FUNCTION
#define IOT_TASK_FUNCTION(name,cb) RequestProcessor::registerTask((char*)name,cb);
#undef IOT_TASK
#define IOT_TASK(name) IOT_TASK_FUNCTION(#name,task_ ## name)
	IOT_TASKS
	IOT_TASK_FUNCTIONS
	DPRINT("Tasks registered");
}

ICACHE_FLASH_ATTR void task_whoAmI(RequestProcessor* p,cJSON*params){
	DPRINT("In the task whoAmI");
	if(!params){
		DPRINT("No parameters for the task");
	}else{
		char*s=cJSON_Print(params);
		DPRINT("%s",s);
		os_free(s);
	}
	p->whoAmI();
	PM;
}

ICACHE_FLASH_ATTR IoTServer::IoTServer() {
	q=new Queue(BUFFER_LENGTH);
	is_sending=0;
	_conn.proto.udp=new (esp_udp);
	_conn.type=ESPCONN_UDP;
	_conn.state = ESPCONN_NONE;
	_udp.local_port=GROUP_UDP_PORT;
	_udp.remote_port=GROUP_UDP_PORT;
	DPRINT("Server port: %d",_udp.local_port);
	IP4_ADDR_S(_udp.remote_ip,GROUP_IP_ADDR_1,GROUP_IP_ADDR_2,GROUP_IP_ADDR_3,GROUP_IP_ADDR_4);
	os_memcpy(_conn.proto.udp,&_udp,sizeof(_udp));
	_conn.reverse=0;
	espconn_regist_recvcb(&_conn,recv_cb);
	espconn_regist_sentcb(&_conn,sent_cb);
	espconn_create(&_conn);
	IP4_ADDR(&group_ip.ip, GROUP_IP_ADDR_1,GROUP_IP_ADDR_2,GROUP_IP_ADDR_3,GROUP_IP_ADDR_4);
	DPRINT("My name: %s",Config::I().name());
	DPRINT("Server started.");
	PM;
}

ICACHE_FLASH_ATTR void IoTServer::reset_udp(){
	os_memcpy(_conn.proto.udp->remote_ip,_udp.remote_ip,sizeof(_udp.remote_ip));
	_conn.proto.udp->remote_port=_udp.remote_port;
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

#define IS_FOR_ME 1
#define IS_NOT_FOR_ME 0
#define IS_FOR_ALL 2

static ICACHE_FLASH_ATTR int objectHasMyName(cJSON*obj){
	DPRINT("Is Request for me?");
	if(!obj) return IS_NOT_FOR_ME;
	cJSON*p;
	if(obj->type==cJSON_String){
		DPRINT("Name is: %s, my name is %s",obj->valuestring,Config::I().name());
		if(!os_strcmp(obj->valuestring,Config::I().name())) return IS_FOR_ME;
		DPRINT("Name is not me, maybe it's '*'");
		if(!os_strcmp(obj->valuestring,"*")) return IS_FOR_ALL;
	}else if(obj->type==cJSON_Array){
		DPRINT("There is an array of names");
		int i,L=cJSON_GetArraySize(obj);
		for(i=0;i<L;++i){
			p=cJSON_GetArrayItem(obj,i);
			if(p && p->type==cJSON_String) DPRINT("Next name is: %s",p->valuestring);
			if(p && p->type==cJSON_String && !os_strcmp(p->valuestring,Config::I().name())) return IS_FOR_ME;
		}
	}
	DPRINT("Not for me =(");
	return IS_NOT_FOR_ME;
}

ICACHE_FLASH_ATTR void RequestProcessor::whoAmI(){
	if(response){
		cJSON_AddItemToObject(response,"type",cJSON_CreateString(MY_TYPE));
	}
}

ICACHE_FLASH_ATTR void RequestProcessor::warn(char *warning){
	cJSON *obj=cJSON_GetObjectItem(response,"warnings");
	if(obj && obj->type!=cJSON_Array){
		cJSON_DeleteItemFromObject(response,"warnings");
		obj=(cJSON*)0;
	}
	if(!obj){
		obj=cJSON_CreateArray();
		cJSON_AddItemToObject(response,"warnings",obj);
	}
	cJSON_AddItemToArray(obj,cJSON_CreateString(warning));
}


ICACHE_FLASH_ATTR void RequestProcessor::taskNotFound(char *task){
	char*str=new char[os_strlen(task)+20];
	os_sprintf(str,"Task \"%s\" not found.",task);
	cJSON_AddItemToArray(response,cJSON_CreateString(str));
	delete str;
}

ICACHE_FLASH_ATTR void RequestProcessor::processTask(char *task,cJSON*params){
	Task t=getTask(task);
	if(t) t(this,params);
	else{
		taskNotFound(task);
	}
}

ICACHE_FLASH_ATTR void RequestProcessor::processTask(cJSON *task){
	cJSON* I;
	if(task->type == cJSON_Array && cJSON_GetArraySize(task)==2 && (I=cJSON_GetArrayItem(task,0))->type==cJSON_String){
		processTask(I->valuestring,cJSON_GetArrayItem(task,1));
	}else if(task->type == cJSON_Object && (I=cJSON_GetObjectItem(task,"task"))->type==cJSON_String){
		processTask(I->valuestring,cJSON_GetObjectItem(task,"parameters"));
	}
}

ICACHE_FLASH_ATTR void RequestProcessor::process(){
	DPRINT("Processing received tasks.");
	if(response && request && request->type==cJSON_Object){
		cJSON* tasks_obj,*I;
		if((tasks_obj=cJSON_GetObjectItem(request,"TT"))){
			DPRINT("There is several tasks.");
			if(tasks_obj->type == cJSON_Array){
				int i,L=cJSON_GetArraySize(tasks_obj);
				for(i=0;i<L;++i){
					I=cJSON_GetArrayItem(tasks_obj,i);
					processTask(I);
				}
			}else if(tasks_obj->type==cJSON_Object){
				for(I=tasks_obj->child;I;I=I->next){
					processTask(I->string,I);
				}
			}
		}else if((tasks_obj=cJSON_GetObjectItem(request,"task"))){
			DPRINT("There is a task.");
			if(tasks_obj->type==cJSON_String){
				processTask(tasks_obj->valuestring,cJSON_GetObjectItem(request,"parameters"));
			}else{
				processTask(tasks_obj);
			}
		}
	}
}

ICACHE_FLASH_ATTR void IoTServer::recv_cb(void*arg,char*data,unsigned short int len){
	struct espconn *pconn=(struct espconn *)arg;
	cJSON *root=cJSON_Parse(data),*obj;
	if(root){
		if(root->type==cJSON_Object){
			if(objectHasMyName(cJSON_GetObjectItem(root,"T"))==IS_FOR_ME){
				DPRINT("Creating Processor");
				RequestProcessor* r=new RequestProcessor(root);
				DPRINT("Processing");
				r->process();
				DPRINT("Sending the result");
				r->send();
				DPRINT("Removing processor");
				delete r;
			}else if(objectHasMyName(cJSON_GetObjectItem(root,"who"))){
				RequestProcessor* r=new RequestProcessor(root);
				r->whoAmI();
				r->send();
				delete r;
			}else
				cJSON_Delete(root);
		}else cJSON_Delete(root);
	}
	PM;
}

ICACHE_FLASH_ATTR Queue::Queue(u16 len){
	length=len;
	next=0;
	buffer=(char**)os_zalloc(sizeof(char*)*len);
}

ICACHE_FLASH_ATTR void Queue::push(char* r){
	if(!buffer[next]){
		buffer[next]=r;
		return;
	}else{
		for(int i=next+1;i<length;++i) if(!buffer[i]){
			buffer[i]=r;
			return;
		}
		for(int i=0;i<next;++i) if(!buffer[i]){
			buffer[i]=r;
			return;
		}
	}
	os_free(r);
}

ICACHE_FLASH_ATTR char* Queue::pop(){
	char*r=buffer[next];
	buffer[next++]=0;
	if(next==length) next=0;
	return r;
}

ICACHE_FLASH_ATTR Queue::~Queue(){
	for(int i=0;i<length;++i){
		if(buffer[i])
			delete buffer[i];
	}
	os_free(buffer);
}

ICACHE_FLASH_ATTR void IoTServer::sent_cb(void*arg){
	struct espconn *pconn=(struct espconn *)arg;
	os_free(srv->is_sending);
	srv->reset_udp();
	srv->is_sending=0;
	srv->send_current();
	PM;
}

ICACHE_FLASH_ATTR void IoTServer::send_current(){
	if(!is_sending){
		is_sending=q->pop();
		reset_udp();
		if(is_sending)
			espconn_send(&_conn,(u8*)is_sending,os_strlen(is_sending));
	}
}

ICACHE_FLASH_ATTR IoTServer& IoTServer::instance(){
	if(!srv)
		srv=new IoTServer();

	return *srv;
}

ICACHE_FLASH_ATTR IoTServer::~IoTServer() {
	// TODO Auto-generated destructor stub
}

ICACHE_FLASH_ATTR void RequestProcessor::send(){
	if(response){
		char* rc=cJSON_Print(response);
		srv->send(rc);
		cJSON_Delete(response);
		response=0;
	}
}

ICACHE_FLASH_ATTR void IoTServer::send(char*r){
	q->push(r);
	send_current();
}

ICACHE_FLASH_ATTR RequestProcessor::RequestProcessor(cJSON *req){
	if(!tasks) registerDefinedTasks();
	request=req;
	response=cJSON_CreateObject();
	cJSON*obj;
	cJSON_AddItemToObject(response,"F",cJSON_CreateString(Config::I().name()));
	if(request && request->type==cJSON_Object && (obj=cJSON_GetObjectItem(request,"F")) && (obj->type==cJSON_String)){
		cJSON_AddItemToObject(response,"T",cJSON_CreateString(obj->valuestring));
	}
}

ICACHE_FLASH_ATTR RequestProcessor::~RequestProcessor(){
	if(request) cJSON_Delete(request);
	if(response) cJSON_Delete(response);
}






