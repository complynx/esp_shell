/*
 * HTTPD.cpp
 *
 *  Created on: 16 џэт. 2016 у.
 *      Author: complynx
 */

#include "HTTPD.h"
#include "espmissingincludes.h"
extern "C"{
#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"
#include "c_types.h"
#include "ip_addr.h"
#include "osapi.h"
#include "espconn.h"
#include "driver/uart.h"
}
char* newcpy(char*str,u16 pluslen=1);

ICACHE_FLASH_ATTR HTTPD::HTTPD() {
    conn.type = ESPCONN_TCP;
    conn.state = ESPCONN_NONE;
    conn.proto.tcp = &tcps;
    conn.proto.tcp->local_port = 80;
    espconn_regist_connectcb(&conn, listen);

    espconn_accept(&conn);

    DPRINT("webserver started\n");
}
static HTTPD* server=(HTTPD*)0;


void ICACHE_FLASH_ATTR
HTTPD::recv(void *arg, char *pusrdata, unsigned short length)
{
    struct espconn *pesp_conn = (struct espconn *)arg;
    Request r(pesp_conn,pusrdata,length);
    r.parse();
}

ICACHE_FLASH_ATTR Request::Request(struct espconn *pesp_conn, char *pusrdata, unsigned short len){
	conn=pesp_conn;
	data=pusrdata;
	length=len;
	response_sent=false;
	version=0;
	method=UNKNOWN;
	URI=(char*)0;
	headers=(Args*)0;
	arguments=(Args*)0;
	body=(char*)0;

    DPRINT("webserver's %d.%d.%d.%d:%d got data, length: %d\n", pesp_conn->proto.tcp->remote_ip[0],
        		pesp_conn->proto.tcp->remote_ip[1],pesp_conn->proto.tcp->remote_ip[2],
        		pesp_conn->proto.tcp->remote_ip[3],pesp_conn->proto.tcp->remote_port,length);
}

ICACHE_FLASH_ATTR void Request::parse_starting_line(){
	char *pos,*pos2;
	size_t i;
	for(i=length,pos=data;i;i--,pos++)
		if(*pos == ' ')
			break;
	if(i){
#define M(meth) \
		if(!os_strncmp((char*)# meth ,data,(pos-data))){\
			method=meth;\
			DPRINT("Got method %s\n",(const char*)# meth);\
		}else

		M(GET)M(POST)M(HEAD) method=UNKNOWN;
#undef M

		if(!method){
			send_err(501);
		}else{
		}
	}
}

ICACHE_FLASH_ATTR char* Response::get_error_str(u16 code){
#define E(num,str) if(num==code) return (char*)str

#ifdef FULL_HTTP_CODE_LIST
	E(100,"Continue");
	E(101,"Switching Protocols");
	E(102,"Processing");
#endif

	E(200,"OK");
#ifdef FULL_HTTP_CODE_LIST
	E(201,"Created");
	E(202,"Accepted");
	E(203,"Non-Authoritative Information");
	E(204,"No Content");
	E(205,"Reset Content");
	E(206,"Partial Content");
	E(207,"Multi-Status");
	E(226,"IM Used");
#endif

	E(300,"Multiple Choices");
	E(301,"Moved Permanently");
	E(302,"Found");
	E(303,"See Other");
	E(304,"Not Modified");
#ifdef FULL_HTTP_CODE_LIST
	E(305,"Use Proxy");
#endif
	//E(306,"Reserved");
	E(307,"Temporary Redirect");

	E(400,"Bad Request");
	E(401,"Unauthorized");
#ifdef FULL_HTTP_CODE_LIST
	E(402,"Payment Required");
#endif
	E(403,"Forbidden");
	E(404,"Not Found");
	E(405,"Method Not Allowed");
	E(406,"Not Acceptable");
#ifdef FULL_HTTP_CODE_LIST
	E(407,"Proxy Authentication Required");
	E(408,"Request Timeout");
	E(409,"Conflict");
	E(410,"Gone");
	E(411,"Length Required");
	E(412,"Precondition Failed");
#endif
	E(413,"Request Entity Too Large");
	E(414,"Request-URI Too Large");
	E(415,"Unsupported Media Type");
#ifdef FULL_HTTP_CODE_LIST
	E(416,"Requested Range Not Satisfiable");
	E(417,"Expectation Failed");
	E(422,"Unprocessable Entity");
	E(423,"Locked");
	E(424,"Failed Dependency");
	E(425,"Unordered Collection");
	E(426,"Upgrade Required");
	E(428,"Precondition Required");
	E(429,"Too Many Requests");
#endif
	E(431,"Request Header Fields Too Large");
#ifdef FULL_HTTP_CODE_LIST
	E(434,"Requested host unavailable");
	E(449,"Retry With");
	E(451,"Unavailable For Legal Reasons");
#endif

	E(500,"Internal Server Error");
	E(501,"Not Implemented");
#ifdef FULL_HTTP_CODE_LIST
	E(502,"Bad Gateway");
	E(503,"Service Unavailable");
	E(504,"Gateway Timeout");
#endif
	E(505,"HTTP Version Not Supported");
	E(506,"Variant Also Negotiates");
	E(507,"Insufficient Storage");
#ifdef FULL_HTTP_CODE_LIST
	E(508,"Loop Detected");
	E(509,"Bandwidth Limit Exceeded");
	E(510,"Not Extended");
	E(511,"Network Authentication Required");
#endif

#undef E
	return (char*)"";
}

ICACHE_FLASH_ATTR void Response::send_err(u16 errno){
	code=errno;
	contents=get_error_str(errno);
	DPRINT("Sending error %d %s\n",errno,contents);
	send();
	contents=(char*)0;//not to delete static hardcoded data on ~Response
}

ICACHE_FLASH_ATTR void Response::send(){
	if(is_sent) return;

	char*buf,*pos;
	u16 length=0;
	Args *Ai;

	DPRINT("Sending response\n");

	char*errstr=get_error_str(code);

	//HTTP/1.x COD str\r\n -- 13+strlen(str)+2
	length=15+os_strlen(errstr);

	DPRINT("HTTP/1.%d %d %s\n",(version%10),code,errstr);

	if(contents){
		DPRINT("Adding content length\n");
		for(Ai=headers;Ai->next;Ai=Ai->next);
		Ai=Ai->next=new Args;
		Ai->name=newcpy((char*)"Content-Length");
		Ai->val=new char[7];
		os_sprintf(Ai->val,"%d",os_strlen(contents));

		//HTTP/1.x COD str\r\n
		//[headers: vals\r\n]
		//\r\n 						-- +2ch
		//contents 					-- +strlen(contents)
		length+=2+os_strlen(contents);
	}

	if(code<400){
		DPRINT("Determining content expiration info\n");
		//there is real content, not errcode, set it's expiration
		for(Ai=headers;Ai->next || !os_strcmp("Expires",Ai->name);Ai=Ai->next);
		if(os_strcmp("Expires",Ai->name)){
			DPRINT("Adding content expiration info\n");
			Ai=Ai->next=new Args;
			Ai->name=newcpy((char*)"Expires");
			Ai->val=newcpy((char*)"Fri, 10 Apr 2008 14:00:00 GMT");
			for(Ai=headers;Ai->next || (!os_strcmp("Pragma",Ai->name) && !os_strcmp("no-cache",Ai->val));Ai=Ai->next);
			if(os_strcmp("Pragma",Ai->name)){
				Ai=Ai->next=new Args;
				Ai->name=newcpy((char*)"Pragma");
				Ai->val=newcpy((char*)"no-cache");
			}
		}
	}

	for(Ai=headers;Ai;Ai=Ai->next){
		//name: val\r\n 	-- strlen(name)+strlen(val)+4
		length+=os_strlen(Ai->name)+os_strlen(Ai->val)+4;
	}

	buf=new char[length];
	pos=buf;
	os_sprintf(pos,"HTTP/1.%d %d %s\r\n",(version%10),code,errstr);
	pos=pos+os_strlen(pos);

	for(Ai=headers;Ai;Ai=Ai->next){
		DPRINT("%s: %s\r\n",Ai->name,Ai->val);
		os_sprintf(pos,"%s: %s\r\n",Ai->name,Ai->val);
		pos=pos+os_strlen(pos);
	}
	DPRINT("\n");

	if(contents){
		*(pos++)='\r';
		*(pos++)='\n';

		os_strncpy(pos,contents,os_strlen(contents));
	}

	DPRINT("Sending... length = %d\n",length);

	espconn_sent(conn, (unsigned char*)buf, length);

	DPRINT("Sent...?\n");
	is_sent=true;
}

ICACHE_FLASH_ATTR Response::~Response(){
	if(headers) delete headers;
	if(contents) delete [] contents;
}

ICACHE_FLASH_ATTR char* newcpy(char*str,u16 pluslen){
	char* ret=new char[os_strlen(str)+pluslen];
	os_strcpy(ret,str);
	return ret;
}

ICACHE_FLASH_ATTR Response::Response(espconn* c,u8 ver){
	conn=c;
	version=ver;

	headers=(Args*)0;
	code=200;
	is_sent=false;
	contents=(char*)0;

	Args *i;
	headers=i=new Args;

//	i=i->next=new Args;
//	i->name=newcpy("Server");
//	i->val=newcpy("lwIoTSrv/1.0");
}

ICACHE_FLASH_ATTR void Request::send_err(u16 errno){
	Response r(conn,version?version:10);
	r.send_err(errno);
	response_sent=true;
}

ICACHE_FLASH_ATTR void Request::parse(){
    DPRINT("Parsing request\n");
	parse_starting_line();
}

ICACHE_FLASH_ATTR Request::~Request(){
	if(headers) delete headers;
	if(arguments) delete arguments;
}

void ICACHE_FLASH_ATTR HTTPD::listen(void *arg){
    struct espconn *pesp_conn = (struct espconn *)arg;

    espconn_regist_recvcb(pesp_conn, recv);
    espconn_regist_reconcb(pesp_conn, recon);
    espconn_regist_disconcb(pesp_conn, discon);
}

ICACHE_FLASH_ATTR
void HTTPD::recon(void *arg, sint8 err)
{
    struct espconn *pesp_conn = (struct espconn *)arg;

    DPRINT("webserver's %d.%d.%d.%d:%d err %d reconnect\n", pesp_conn->proto.tcp->remote_ip[0],
    		pesp_conn->proto.tcp->remote_ip[1],pesp_conn->proto.tcp->remote_ip[2],
    		pesp_conn->proto.tcp->remote_ip[3],pesp_conn->proto.tcp->remote_port, err);
}

ICACHE_FLASH_ATTR
void HTTPD::discon(void *arg)
{
    struct espconn *pesp_conn = (struct espconn *)arg;

    DPRINT("webserver's %d.%d.%d.%d:%d disconnect\n", pesp_conn->proto.tcp->remote_ip[0],
        		pesp_conn->proto.tcp->remote_ip[1],pesp_conn->proto.tcp->remote_ip[2],
        		pesp_conn->proto.tcp->remote_ip[3],pesp_conn->proto.tcp->remote_port);
}

HTTPD& ICACHE_FLASH_ATTR HTTPD::instance(){
	if(!server)
		server=new HTTPD();
	return *server;
}

ICACHE_FLASH_ATTR HTTPD::~HTTPD() {
	// TODO Auto-generated destructor stub
}

