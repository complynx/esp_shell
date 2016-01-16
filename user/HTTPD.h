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

class Args{
public:
	Args():name(0),val(0),next(0){}
	char* name;
	char* val;
	Args* next;
	~Args(){
		if(name) delete [] name;
		if(val) delete [] val;
		if(next) delete next;
	}
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
	void send();
	void send_err(u16 code);
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
		UNKNOWN=0,GET,POST,HEAD
	} method;
	char* URI;
	Args* headers;
	Args* arguments;
	char* body;

	Request(struct espconn*, char *, unsigned short);
	void parse();
	void parse_starting_line();
	void parse_headers();
	void parse_body();
	void send_err(u16 errno);
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
	static HTTPD& instance();
	virtual ~HTTPD();
};

#endif /* USER_HTTPD_H_ */
