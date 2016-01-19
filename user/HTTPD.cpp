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
char* newcpyn(char*str,u16 len);
void strToLower(char*);

inline char chrToLower(const char c){
	if(c>='A' && c<='Z')
		return (c-'A')+'a';

	return c;
};
extern "C"
ICACHE_FLASH_ATTR int stricmp(const char*a,const char*b){
	const char*i,*j;
	int ret=0;
	for(i=a,j=b;*i && *j;++i,++j)
		if((ret=(chrToLower(*i)-chrToLower(*j)))!=0)
			return ret;
	return 0;
}

ICACHE_FLASH_ATTR char* Args::toString(){
	return val;
}

ICACHE_FLASH_ATTR void Args::setName(const char* s){
	name=new char[os_strlen(s)+1];
	strToLower(name);
}

ICACHE_FLASH_ATTR Args::~Args(){
	if(val) delete[] val;
	if(name) delete [] name;
	if(next) delete [] next;
}


ICACHE_FLASH_ATTR u8 fromHex(char c){
	if(c>='0' && c<='9') return c-'0';
	if(c>='A' && c<='F') return c-'A'+0xA;
	if(c>='a' && c<='f') return c-'a'+0xA;
	return 0x10;//err
}

ICACHE_FLASH_ATTR u16 decodeURIcomponent(char*str){
	char*i,*j,c;
	for(i=j=str;*i;++i){
		if(*i=='%' && *(i+1) && *(i+2)){
			c=fromHex(*(++i))*0x10;
			c+=fromHex(*(++i));
			*(j++)=c;
		}else{
			*(j++)=*i;
		}
	}
	*j=*i;
	return j-str;
}

ICACHE_FLASH_ATTR HTTPD::HTTPD() {
    conn.type = ESPCONN_TCP;
    conn.state = ESPCONN_NONE;
    conn.proto.tcp = &tcps;
    conn.proto.tcp->local_port = 80;
    espconn_regist_connectcb(&conn, listen);

    espconn_accept(&conn);

    DPRINT("webserver started");
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

    DPRINT("webserver's %d.%d.%d.%d:%d got data, length: %d", pesp_conn->proto.tcp->remote_ip[0],
        		pesp_conn->proto.tcp->remote_ip[1],pesp_conn->proto.tcp->remote_ip[2],
        		pesp_conn->proto.tcp->remote_ip[3],pesp_conn->proto.tcp->remote_port,length);
}

ICACHE_FLASH_ATTR void Response::addHeader(Args* h){
	Args* Ai;
	for(Ai=headers;Ai->next;Ai=Ai->next);
	Ai->next=h;
}

ICACHE_FLASH_ATTR u16 Request::parse_starting_line(){
	char *pos,*pos2;
	size_t i;
	for(i=length,pos=data;i;--i,++pos)
		if(*pos == ' ')
			break;
	if(!i) return send_err(400);
#define M(meth) \
	if(!os_strncmp((char*)# meth ,data,(pos-data))){\
		method=meth;\
		DPRINT("Got method %s",(const char*)# meth);\
	}else

	METHODS_ALLOWED method=UNKNOWN;
#undef M

	if(!method)return send_err(501);
	if(method==OPTIONS){
		Response r(conn,version?version:10);
		r.code=200;
		r.addHeader(HTTPD::Allow());
		u16 ret=r.send();
		response_sent=true;
		return ret;
	}

	--i;++pos;
	if(!i)return send_err(400);

	for(pos2=pos;i;--i,++pos)
		if(*pos == ' ')
			break;
	if(!i)return send_err(505);//http 0.9

	parse_URI(pos2,pos-pos2);

	if((i-=10)<0) return send_err(400);//" HTTP/1.x\r\n"
	pos+=6;
	if(*pos!='1') return send_err(505);
	pos+=2;
	if(*pos=='1') version=11;
	else if(*pos=='0') version=10;
	else return send_err(505);

	DPRINT("Got version 1.%d",version%10);

	if(pos[1]!='\r' || pos[2]!='\n')
		 return send_err(400);

	return 0;
}

ICACHE_FLASH_ATTR char* extractURIpart(char* start,u16 len){
	char* tmp=new char[len+1];
	os_strncpy(tmp,start,len);
	tmp[len]=0;
	u16 l=decodeURIcomponent(tmp);
	char*ret=new char[l+1];
	os_strcpy(ret,tmp);
	delete [] tmp;
	return ret;
}

ICACHE_FLASH_ATTR u16 Request::parse_URI(char* str,u16 len){
	char *i,*k;
	u16 j;
	for(i=str,j=len;j;--j,++i)
		if(*i=='?')
			break;

	URI=extractURIpart(str,i-str);

	DPRINT("Extracted URL: %s",URI);

	Args*last=arguments;
	if(last) for(;last->next;last=last->next);
	while(j){
		Args *a=new Args;

		++i,--j;//last char was '?' if this is the first entrance, or '&'
		for(k=i;j;--j,++i)
			if(*i=='&' || *i=='=')
				break;

		a->name=extractURIpart(k,i-k);

		if(j && *i=='='){
			++i,--j;
			for(k=i;j;--j,++i)
				if(*i=='&')
					break;

			a->val=extractURIpart(k,i-k);
		}else{
			a->val=new char[1];
			a->val[0]=0;
		}

		if(last){
			last->next=a;
		}else{
			arguments=a;
		}
		last=a;
		DPRINT("Extracted param: \"%s\" = \"%s\"",last->name,last->val);
	}

	return 0;
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

	//6xx -- internal server codes,
	E(600,"Internal error");
	E(601,"Response already sent");

#undef E
	return (char*)"";
}

ICACHE_FLASH_ATTR void strToLower(char*s){
	for(char*i=s;*i;++i)
		*i=chrToLower(*i);
}

ICACHE_FLASH_ATTR u16 Response::send_err(u16 errnum){
	code=errnum;
	contents=get_error_str(code);
	DPRINT("Sending error %d %s",code,contents);
	u16 ret=send();
	contents=(char*)0;//not to delete static hardcoded data on ~Response
	return ret;
}

ICACHE_FLASH_ATTR u16 Response::send(){
	if(is_sent) return 601;

	char*buf,*pos;
	u16 length=0;
	Args *Ai;

	DPRINT("Sending response");

	if(code>=600) code=500;

	char*errstr=get_error_str(code);

	//HTTP/1.x COD str\r\n -- 13+strlen(str)+2
	length=15+os_strlen(errstr);

	DPRINT("HTTP/1.%d %d %s",(version%10),code,errstr);

	if(code == 405 || code == 501){
		addHeader(HTTPD::Allow());
	}

	if(contents){
		DPRINT("Adding content length");
		for(Ai=headers;Ai->next;Ai=Ai->next);
		Ai->next=new Args();
		Ai=Ai->next;
		char* tmp=new char[7];
		os_sprintf(tmp,"%d",os_strlen(contents));
		Ai->name=newcpy((char*)"content-length");
		Ai->val=tmp;

		//HTTP/1.x COD str\r\n
		//[headers: vals\r\n]
		//\r\n 						-- +2ch
		//contents 					-- +strlen(contents)
		length+=2+os_strlen(contents);
	}

	if(code<400){
		DPRINT("Determining content expiration info");
		//there is real content, not errcode, set it's expiration
		for(Ai=headers;Ai->next || !os_strcmp("expires",Ai->name);Ai=Ai->next);
		if(os_strcmp("expires",Ai->name)){
			DPRINT("Adding content expiration info");
			Ai=Ai->next=new Args;
			Ai->name=newcpy((char*)"expires");
			Ai->val=newcpy((char*)"Fri, 10 Apr 2008 14:00:00 GMT");
			for(Ai=headers;Ai->next || (!os_strcmp("pragma",Ai->name) && !os_strcmp("no-cache",Ai->val));Ai=Ai->next);
			if(os_strcmp("pragma",Ai->name)){
				Ai=Ai->next=new Args;
				Ai->name=newcpy((char*)"pragma");
				Ai->val=newcpy((char*)"no-cache");
			}

		}
	}

	for(Ai=headers;Ai;Ai=Ai->next){
		//name: val\r\n 	-- strlen(name)+strlen(val)+4
		length+=os_strlen(Ai->name)+os_strlen(Ai->toString())+4;
	}

	buf=new char[length];
	pos=buf;
	os_sprintf(pos,"HTTP/1.%d %d %s\r\n",(version%10),code,errstr);
	pos=pos+os_strlen(pos);

	for(Ai=headers;Ai;Ai=Ai->next){
		DPRINT("%s: %s",Ai->name,Ai->toString());
		os_sprintf(pos,"%s: %s\r\n",Ai->name,Ai->toString());
		pos=pos+os_strlen(pos);
	}
	DPRINT("");

	if(contents){
		*(pos++)='\r';
		*(pos++)='\n';

		os_strncpy(pos,contents,os_strlen(contents));
	}

	DPRINT("Sending... length = %d",length);

	espconn_send(conn, (unsigned char*)buf, length);
	DPRINT("Sent...? Disconnecting");
	espconn_disconnect(conn);
	DPRINT("Finished.");

	is_sent=true;
	return code;
}

ICACHE_FLASH_ATTR Args* HTTPD::Allow(){
	Args *ret=new Args;
#define M(meth) # meth ","
	ret->val=newcpy((char*)METHODS_ALLOWED "OPTIONS");
#undef M
	ret->name=newcpy((char*)"allow");
	return ret;
}

ICACHE_FLASH_ATTR Response::~Response(){
	DPRINT("Deleting Response struct");
	if(headers) delete headers;
	if(contents) delete [] contents;
}

ICACHE_FLASH_ATTR char* newcpy(char*str,u16 pluslen){
	char* ret=new char[os_strlen(str)+pluslen];
	os_strcpy(ret,str);
	return ret;
}
ICACHE_FLASH_ATTR char* newcpyn(char*str,u16 len){
	char* ret=new char[len+1];
	os_strncpy(ret,str,len);
	ret[len]=0;
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
	i->name=newcpy((char*)"server");
	i->val=newcpy((char*)"lwIoTSrv/1.0");
//	i=i->next=new Args;
//	i->name=newcpy("Server");
//	i->val=newcpy("lwIoTSrv/1.0");
}

ICACHE_FLASH_ATTR u16 Request::send_err(u16 errnum){
	Response r(conn,version?version:10);
	u16 ret=r.send_err(errnum);
	response_sent=true;
	return ret;
}

ICACHE_FLASH_ATTR char* pastNextRN(char* I,u16 i){
	for(--i/*one ahead*/;i;--i,++I)
		if(I[0]=='\r' && I[1]=='\n')
			break;
	if(!i) return I+1;
	return I+2;
}

ICACHE_FLASH_ATTR char* newstrcat(const char* str1,const char*str2){
	char *ret=new char[os_strlen(str1)+os_strlen(str2)+1];
	os_strcpy(ret,str1);
	os_strcpy(ret+os_strlen(str1),str2);
	return ret;
}

ICACHE_FLASH_ATTR u16 Request::parse_headers(){
	char *I=pastNextRN(data,length),*J;
	u16 i=length-(I-data);

	if(!i) return send_err(400);

	Args*last=headers;
	if(last) for(;last->next;last=last->next);
	while(true){
		J=pastNextRN(I,i);

		if(J<=I+2) break;

		char *tmp=newcpyn(J,J-I-2),*ts;
		DPRINT("\"%s\"",tmp);

		PL;if(*tmp==' '){
			PL;ts=last->val;
			PL;last->val=newstrcat(ts,tmp);
			PL;delete[]ts;
		}else{
			PL;
			if(last) last=last->next=new Args;
			else last=headers=new Args;
			PL;for(ts=tmp;*ts;++ts)
				if(*ts==':')
					break;

			PL;last->name=newcpyn(tmp,ts-tmp);
			PL;strToLower(last->name);
			PL;if(*ts){
				PL;last->val=newcpy(ts+1);
			}else{
				PL;last->val=newcpy((char*)"");
			}

			DPRINT("%s: \"%s\"",last->name,last->val);
		}
		PL;delete [] tmp;

		PL;I=J;
	}

	return 0;
}

ICACHE_FLASH_ATTR u16 Request::parse(){
    DPRINT("Parsing request");
	u16 ret=parse_starting_line();
	if(!ret) ret=parse_headers();
	if(!ret) return send_err(200);
	return ret;
}

ICACHE_FLASH_ATTR Request::~Request(){
	DPRINT("Deleting Request struct");
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

    DPRINT("webserver's %d.%d.%d.%d:%d err %d reconnect", pesp_conn->proto.tcp->remote_ip[0],
    		pesp_conn->proto.tcp->remote_ip[1],pesp_conn->proto.tcp->remote_ip[2],
    		pesp_conn->proto.tcp->remote_ip[3],pesp_conn->proto.tcp->remote_port, err);
}

ICACHE_FLASH_ATTR
void HTTPD::discon(void *arg)
{
    struct espconn *pesp_conn = (struct espconn *)arg;

    DPRINT("webserver's %d.%d.%d.%d:%d disconnect", pesp_conn->proto.tcp->remote_ip[0],
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

