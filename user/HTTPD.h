/*
 * HTTPD.h
 *
 *  Created on: 16 џэт. 2016 у.
 *      Author: complynx
 */

#ifndef USER_HTTPD_H_
#define USER_HTTPD_H_
#include "user_config.h"

extern "C"{
#include "espconn.h"
#include "c_types.h"
}

#define METHODS_ALLOWED \
	M(GET)\
	M(POST)\
	M(OPTIONS)\
	M(HEAD)

class Args{
public:
	Args():name(0),val(0),next(0){}
	char* name;
	char* val;
	Args* next;
	virtual char* toString();
	virtual void setName(const char*);
	virtual ~Args();
};

class Strings{
public:
	Strings():str(0),next(0){}
	char* str;
	Strings* next;
	virtual ~Strings(){
		if(str) delete [] str;
		if(next) delete next;
	}
};

class Headers:public Args{
public:
	Headers():Args(),previous(0){}
	Strings *previous;
//	virtual ~Headers();
};

class Response{
private:
public:
	struct espconn* conn;
	Args* headers;
	u8 version;
	u16 code;
	bool is_sent;
	char* contents;
	Response(struct espconn*,u8 version=10);
	u16 send();
	void addHeader(Args*);
	u16 send_err(u16 code);
	static char* get_error_str(u16 code);
	virtual ~Response();
};

class Request{
private:
public:
	struct espconn* conn;
	char* data;
	unsigned short length;
	bool response_sent;
	u8 version;
	enum Method{
#define M(meth) , meth
		UNKNOWN=0 METHODS_ALLOWED
#undef M
	} method;
	char* URI;
	Args* headers;
	Args* arguments;
	char* body;

	Request(struct espconn*, char *, unsigned short);
	u16 parse();
	u16 parse_starting_line();
	u16 parse_URI(char*,u16);
	u16 parse_headers();
	u16 parse_body();
	u16 send_err(u16 errno);
	virtual ~Request();
};

class HTTPD {
private:
    struct espconn conn;
    esp_tcp tcps;
    static void listen(void*);
    static void recon(void*, sint8);
    static void recv(void*, char *, unsigned short);
    static void discon(void*);
    void recv_nonstatic(struct espconn*, char *, unsigned short);
	HTTPD();
	HTTPD(HTTPD const&);
	void operator=(HTTPD const&);
public:
    static Args* Allow();
	static HTTPD& instance();
	virtual ~HTTPD();
};

#endif /* USER_HTTPD_H_ */
