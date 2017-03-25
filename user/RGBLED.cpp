/*
 * RGBLED.cpp
 *
 *  Created on: 2 февр. 2016 г.
 *      Author: complynx
 */

#include "user_config.h"
#include "RGBLED.h"
#include "Config.h"
extern "C"{
#	include "driver/uart.h"
#include <pwm.h>
#include <ctype.h>
#include <sntp.h>

	typedef long time_t;
	struct tm
	{
	  int	tm_sec;
	  int	tm_min;
	  int	tm_hour;
	  int	tm_mday;
	  int	tm_mon;
	  int	tm_year;
	  int	tm_wday;
	  int	tm_yday;
	  int	tm_isdst;
	};
	struct tm * sntp_localtime(const time_t * tim_p);
}
#include "cJSON.h"
#include "IoTServer.h"

#define DUTY_MUL 1000
#define DUTY_DIV 45

static RGBLED* led;

static u32 ICACHE_STORE_ATTR ICACHE_RODATA_ATTR duty_tab[] = {
		0,1,2,3,4,5,6,7,8,9,10,11,12,13,15,18,21,25,29,33,38,
		43,48,54,60,66,73,81,88,96,105,114,123,133,144,155,166,178,190,203,216,
		230,244,259,274,290,307,324,341,359,378,397,417,437,458,480,502,524,548,572,596,
		621,647,674,701,728,757,786,816,846,877,909,941,974,1008,1042,1077,1113,1149,1187,1225,
		1263,1303,1343,1383,1425,1467,1510,1554,1599,1644,1690,1737,1785,1833,1882,1932,1983,2034,2086,2140,
		2193,2248,2304,2360,2417,2475,2534,2594,2654,2715,2778,2841,2904,2969,3035,3101,3168,3236,3305,3375,
		3446,3518,3590,3664,3738,3813,3889,3966,4044,4123,4203,4284,4365,4448,4531,4616,4701,4787,4874,4963,
		5052,5142,5233,5325,5418,5512,5606,5702,5799,5897,5996,6095,6196,6298,6401,6504,6609,6715,6822,6929,
		7038,7148,7259,7371,7484,7598,7713,7828,7946,8064,8183,8303,8424,8546,8670,8794,8920,9046,9174,9302,
		9432,9563,9695,9828,9962,10097,10233,10371,10509,10649,10789,10931,11074,11218,11363,11509,11657,11805,11955,12106,
		12258,12411,12565,12720,12877,13034,13193,13353,13514,13676,13840,14004,14170,14337,14505,14674,14845,15016,15189,15363,
		15538,15715,15892,16071,16251,16432,16614,16798,16983,17169,17356,17544,17734,17925,18117,18311,18505,18701,18898,19096,
		19296,19497,19699,19902,20107,20312,20519,20728,20937,21148,21360,21574,21788,22222
};

RGBLED& ICACHE_FLASH_ATTR RGBLED::I(){
	if(!led) led = new RGBLED();
	return *led;
}

u32 secondsoftheday(u32 cts){
	tm* t=sntp_localtime((time_t*)&cts);
	return t->tm_hour*3600+t->tm_min*60+t->tm_sec;
}
u8 shiftcolor(u8 s,u8 e,float shift){
	float c=(float)e-(float)s;
	c*=shift;
	c+=(float)s;
	if(c<0) c=0;
	else if(c>255) c=255;
	return (u8)c;
}


#define TICK 20 //msec
#define TIMER_TO_ABSOLUTE 100000 //10000 seconds

void ICACHE_FLASH_ATTR RGBLED::switch_sequence(){
	if(program && current_control->has_operations){
		DPRINT("Switching");
		DPRINT("Setting color to last color: %lx",(u32)current_operation->color);
		color(current_operation->color);
		if(current_operation->next_is_operation){
			current_operation+=1;
			DPRINT("Next is Op");
		}else{
			DPRINT("Next is Control");
			current_control=(ControlByte*)(current_operation+1);
			if(current_control->reverse){
				DPRINT("Reversing");
				current_control=(ControlByte*)program;
			}

			if(current_control->has_operations){
				DPRINT("Switching to operation.");
				current_operation=(OperationSequence*)(current_control+1);
			}else{
				DPRINT("This is stop byte.");
			}
		}

		init_sequence();
	}
}

void ICACHE_FLASH_ATTR RGBLED::init_sequence(){
	u32 cts=sntp_get_current_timestamp();
	if(program && current_control->has_operations){
		DPRINT("Setting start color. %lx %lx", (u32)_color_start, (u32)_color);
		_color_start=_color;
		DPRINT("Start color. %lx", (u32)_color_start);
		timer=current_operation->time;
		DPRINT("Timer: %ld", timer);
		if(!current_operation->absolute){
			if(timer>TIMER_TO_ABSOLUTE){
				if(!cts) cts=1;
				timer/=10;//0.1 sec -> sec
				timer+=cts;
				starttime=cts;
				DPRINT("Timer is relative and too big, switching to absolute mode");
			}else{
				starttime=0;
				timer*=100;//0.1 sec -> msec
				DPRINT("Timer is relative");
			}
		}else{
			if(!cts) cts=1;
			starttime=cts;
			timer+=cts-secondsoftheday(cts);
			DPRINT("Timer is absolute");
		}
		DPRINT("Timer: %ld, Starttime: %ld",timer,starttime);
		DPRINT("target color: %lx",(u32)current_operation->color);

		if(!current_operation->smooth){
			color(current_operation->color);
			DPRINT("Color is not smooth, new color: %lx",(u32)_color);
		}
	}
}

static u32 ticker=0;
#define RESET_CYCLE 10000
static ETSTimer ledcycle;
void cycle_tick(){
	u32 cts=sntp_get_current_timestamp();
	++ticker;
	if(ticker>RESET_CYCLE){
		IoTServer::instance().maybe_reset();
		ticker = 0;
	}
	if(led->program && led->current_control->has_operations && led->enabled){//if there is a program and we are not at the end
		if(cts || !led->starttime){
			if(led->starttime==1){
				led->init_sequence();
			}
			bool shiftprog;
			do{
				shiftprog=false;
				if(led->starttime){
					if(cts>led->timer) 		shiftprog=true;
				}else{
					if(led->timer<=TICK)	shiftprog=true;
					else					led->timer-=TICK;//shiftprog = false here, so we will exit this.
				}

				if(shiftprog){
					led->switch_sequence();
					if(!led->current_control->has_operations)
						return;
					if(!led->starttime) 	led->timer+=TICK;//neutralize next cycle
				}
			}while(shiftprog);

			if(led->current_operation->smooth){
				float td;
				if(led->starttime){
					td=(double)(cts-led->starttime)/(double)(led->timer-led->starttime);
				}else{
					td=(double)(led->timer)/(double)(led->current_operation->time*100);
					td=1.-td;
				}
				led->color(
						shiftcolor(led->_color_start.R(),led->current_operation->color.R(),td),
						shiftcolor(led->_color_start.G(),led->current_operation->color.G(),td),
						shiftcolor(led->_color_start.B(),led->current_operation->color.B(),td));
			}
		}
	}
}

ICACHE_FLASH_ATTR RGBLED::RGBLED() {
	u32 io_info[][3] =
	{{PWM_R_OUT_IO_MUX,PWM_R_OUT_IO_FUNC,PWM_R_OUT_IO_NUM},
	{PWM_G_OUT_IO_MUX,PWM_G_OUT_IO_FUNC,PWM_G_OUT_IO_NUM},
	{PWM_B_OUT_IO_MUX,PWM_B_OUT_IO_FUNC,PWM_B_OUT_IO_NUM}};
	enabled = true;
	program=0;

	DPRINT("Pins: R=%d, G=%d, B=%d",PWM_R_OUT_IO_NUM,PWM_G_OUT_IO_NUM,PWM_B_OUT_IO_NUM);

	uint32 pwm_duty_init[3] = {0};

	pwm_init(PWM_PERIOD,pwm_duty_init,3,io_info);
//	color(Config::I().led_color());
	color(0);

	sntp_setservername(0, (char*)SNTP_SERVER_0); // set server 0 by domain name
	sntp_setservername(1, (char*)SNTP_SERVER_1); // set server 1 by domain name
	sntp_set_timezone(SNTP_TZ);
	sntp_init();

	os_timer_disarm(&ledcycle);
	os_timer_setfn(&ledcycle, (os_timer_func_t *)cycle_tick, NULL);
	os_timer_arm(&ledcycle, TICK, 1);
}

u8 ICACHE_FLASH_ATTR RGBLED::color_n(u8 i,u8 c){
	if(i>2) return 0;
	i=2-i;//reversed byte order
	u8* col=(u8*)(&_color);
	col[i]=c;
	pwm_set_duty(toDuty(col[i]),i);
	pwm_start();
	return col[i];
}


void ICACHE_FLASH_ATTR RGBLED::set_check_bit(OperationSequence* s){
	if(!s) return;
	s->check_bit=s->absolute xor s->next_is_operation xor s->smooth;
}
void ICACHE_FLASH_ATTR RGBLED::set_check_bit(ControlByte* s){
	if(!s) return;
	s->check_bit=s->fire xor s->has_operations xor s->reverse;
}
bool ICACHE_FLASH_ATTR RGBLED::test_check_bit(OperationSequence* s){
	if(!s) return false;
	return s->check_bit==(s->absolute xor s->next_is_operation xor s->smooth);
}
bool ICACHE_FLASH_ATTR RGBLED::test_check_bit(ControlByte* s){
	if(!s) return false;
	return s->check_bit==(s->fire xor s->has_operations xor s->reverse);
}

#define MAX_PROGRAM_LENGTH 4095
u32 ICACHE_FLASH_ATTR RGBLED::program_length(void* s){
	if(!s) return 0;
	ControlByte*cb=(ControlByte*)s;
	bool check_control=true;
	u32 len=0;
	OperationSequence* os;
	do{
		if(len>MAX_PROGRAM_LENGTH) return 0;
		if(check_control){
			if(!test_check_bit(cb)) return 0;
			len+=sizeof(ControlByte);
			if(cb->reverse) return ((void*)cb!=(void*)s)?len:0;
			if(cb->has_operations) os=(OperationSequence*)(cb+1);
			else return len;
			check_control=false;
		}else{
			if(!test_check_bit(os)) return 0;
			len+=sizeof(OperationSequence);
			if(os->next_is_operation) os+=1;
			else{
				cb=(ControlByte*)(os+1);
				check_control=true;
			}
		}
	}while(true);
	return len;
}

void ICACHE_FLASH_ATTR RGBLED::smooth_transfer(const Color& c,int dsec){
	void *p=os_malloc(2*sizeof(ControlByte)+sizeof(OperationSequence));
	ControlByte*cb=(ControlByte*)p;
	cb->has_operations=1;
	cb->reverse=0;
	cb->tail=0xd;
	cb->fire=0;
	set_check_bit(cb);
	OperationSequence*os=(OperationSequence*)(cb+1);
	os->absolute=0;
	os->next_is_operation=0;
	os->smooth=dsec>0?1:0;
	os->time=dsec;//20 sec
	os->color=c;
	set_check_bit(os);
	cb=(ControlByte*)(os+1);
	cb->has_operations=0;
	cb->reverse=0;
	cb->fire=0;
	cb->tail=0xd;
	set_check_bit(cb);

	if(program) os_free(program);
	program=p;

	current_control=(ControlByte*)program;
	current_operation=(OperationSequence*)(current_control+1);

	init_sequence();
}

int ICACHE_FLASH_ATTR RGBLED::set_program(void* s){
	u32 len=program_length(s);
	if(!len) return 1;
	void *old_prog=program;
	program=os_malloc(len);
	os_memcpy(program,s,len);
	if(old_prog) os_free(old_prog);
	current_control=(ControlByte*)program;
	current_operation=(OperationSequence*)(current_control+1);

	init_sequence();
	return 0;
}

u8 ICACHE_FLASH_ATTR RGBLED::toByte(u32 c){
	u8 ret=255;
	for(;ret;--ret)
		if(duty_tab[ret]<=c) break;
	return ret;
}
u32 ICACHE_FLASH_ATTR RGBLED::toDuty(u8 c){
	return (u32)(duty_tab[c]);
}
bool ICACHE_FLASH_ATTR RGBLED::toggle(bool mode){
	enabled = mode;

	if(enabled){
		color(_color);
	}else{
		pwm_set_duty(0,0);
		pwm_set_duty(0,1);
		pwm_set_duty(0,2);
		pwm_start();
	}
	return enabled;
}

ICACHE_FLASH_ATTR RGBLED::~RGBLED() {
}

Color ICACHE_FLASH_ATTR RGBLED::color(Color c){
	//DPRINT("%lx %lx",(u32)_color,(u32)c);
	_color=c;
	if(enabled){
		//Config::I().led_color(COLOR(_R,_G,_B));
		pwm_set_duty(toDuty(_color.components.r),0);
		pwm_set_duty(toDuty(_color.components.g),1);
		pwm_set_duty(toDuty(_color.components.b),2);
		pwm_start();
	}
	//DPRINT("%lx",(u32)_color);
	return _color;
}

Color ICACHE_FLASH_ATTR RGBLED::color(u8 r,u8 g,u8 b){
	Color col(r,g,b);
	return color(col);
}

char hexify(u8 d){
	if(d<10) return d+'0';
	if(d<16) return d-10+'A';
	return '0';
}
u8 dehexify(char d){
	if(d>='0' && d<='9') return d-'0';
	d=tolower(d);
	if(d>='a' && d<='f') return d+10-'a';
	return 255;
}

char* ICACHE_FLASH_ATTR RGBLED::toString(Color c){
	char* ret=new char[8];
	ret[0]='#';
	ret[1]=hexify((c.components.r>>1)&0xF);
	ret[2]=hexify(c.components.r&0xF);
	ret[3]=hexify((c.components.g>>1)&0xF);
	ret[4]=hexify(c.components.g&0xF);
	ret[5]=hexify((c.components.b>>1)&0xF);
	ret[6]=hexify(c.components.b&0xF);
	ret[7]=0;
	return ret;
}

char* ICACHE_FLASH_ATTR skipspaces(char*str){
	--str;
	while(*(++str)){
		if(*str == '\r') continue;
		if(*str == '\n') continue;
		if(*str == '\t') continue;
		if(*str == ' ') continue;
		break;
	}
	return str;
}

Color ICACHE_FLASH_ATTR RGBLED::fromString(cJSON* str){
	if(str && str->type==cJSON_String){
		return fromString(str->valuestring);
	}
	return 0;
}

Color ICACHE_FLASH_ATTR RGBLED::fromString(char* str){
	if(!str) return 0;
	u32 ret=0;
	u8 c;
	str=skipspaces(str);
	if(*str=='#'){
		c=dehexify(*(++str));if(c==255) return (Color)0;		ret+=c;//R
		c=dehexify(*(++str));if(c==255) return (Color)0;ret<<=4;ret+=c;//R
		c=dehexify(*(++str));if(c==255) return (Color)0;ret<<=4;ret+=c;//G
		c=dehexify(*(++str));if(c==255) return (Color)0;ret<<=4;ret+=c;//G
		c=dehexify(*(++str));if(c==255) return (Color)0;ret<<=4;ret+=c;//B
		c=dehexify(*(++str));if(c==255) return (Color)0;ret<<=4;ret+=c;//B
		return (Color)ret;
	}else if(tolower(*str)=='r' && *(++str) && tolower(*str)=='g' && *(++str) && tolower(*str)=='b'){
		if(*(++str) && tolower(*str)=='a') ++str;
		str=skipspaces(str);
		if(*str!='(') return (Color)0;
		str=skipspaces(str+1);
		u8 c=0,pos=0;
		while(*str){
			str=skipspaces(str);
			if(*str==',' || *str==')'){
				str=skipspaces(str+1);
				ret<<=8;
				++pos;
				ret+=c;
				if(pos>2){
					return (Color)ret;
				}
				c=0;
				continue;
			}
			if(*str>='0' && *str<='9'){
				c*=10;
				c+=(*str-'0');
				++str;
				continue;
			}
			return (Color)ret;
		}
		return (Color)ret;

	}
	return (Color)ret;
}

// tasks


ICACHE_FLASH_ATTR void task_getcolor(RequestProcessor* p,cJSON*params){
	if(p->response){
		cJSON *r=cJSON_GetObjectItem(p->response,"color");
		if(r) cJSON_Delete(r);
		char* str=RGBLED::toString(led->color());
		r=cJSON_CreateString(str);
		os_free(str);
		cJSON_AddItemToObject(p->response,"color",r);
	}
}

ICACHE_FLASH_ATTR void task_setcolor(RequestProcessor* p,cJSON*params){
	Color c;
	u32 timer=STANDARD_TRANSFER_TIME;
	cJSON *col=0,*tim=0;
	if(params){
		if(params->type==cJSON_String){
			col=params;
		}else if(params->type==cJSON_Array){
			if(cJSON_GetArraySize(params)>1){
				tim=cJSON_GetArrayItem(params,1);
			}
			if(cJSON_GetArraySize(params)>0){
				col=cJSON_GetArrayItem(params,0);
			}
		}else if(params->type==cJSON_Object){
			col=cJSON_GetObjectItem(params,"color");
			tim=cJSON_GetObjectItem(params,"duration");
		}
		if(tim && tim->type==cJSON_Number){
			timer=(tim->valuedouble*10.);
		}
		if(col && col->type==cJSON_String){
			c=RGBLED::fromString(col->valuestring);
			led->smooth_transfer(c,timer);
		}
	}
	task_getcolor(p,params);
}

ICACHE_FLASH_ATTR void task_is_enabled(RequestProcessor* p,cJSON*params){
	if(p->response){
		cJSON *r=cJSON_GetObjectItem(p->response,"enabled");
		if(r) cJSON_Delete(r);
		r=cJSON_CreateBool(led->is_enabled());
		cJSON_AddItemToObject(p->response,"enabled",r);
	}
}

ICACHE_FLASH_ATTR void task_toggle(RequestProcessor* p,cJSON*params){
	bool en = !led->is_enabled();
	if(params){
		if(params->type == cJSON_NULL || params->type == cJSON_False) en = false;
		else if(params->type == cJSON_Number){
			en = !!(params->valueint);
		}else if(params->type == cJSON_String){
			en = !!strlen(params->string);
		}else{
			en = true;
		}
	}
	en = led->toggle(en);
	task_is_enabled(p,params);
}

ICACHE_FLASH_ATTR cJSON* program_to_json(void*p){
	u32 len=RGBLED::program_length(p);
	if(!len) return 0;
	ControlByte*cb=(ControlByte*)p;
	bool in_control=true;
	OperationSequence* os;
	cJSON *prog=cJSON_CreateArray();
	cJSON *last=0;
	do{
		if(in_control){
			if(cb->has_operations) os=(OperationSequence*)(cb+1);
			else{
				if(!last){
					cJSON_Delete(prog);
					return 0;
				}
				if(cb->reverse && last){
					cJSON_AddItemToObject(last,"reverse",cJSON_CreateBool(true));
				}
				return prog;
			}
			in_control=false;
		}else{
			last=cJSON_CreateObject();
			cJSON_AddItemToArray(prog,last);
			char *buf=RGBLED::toString(os->color);
			cJSON_AddItemToObject(last,"color",cJSON_CreateString(buf));
			os_free(buf);
			if(os->absolute){
				buf=new char[9];//hh:mm:ss\0 -- 9 symbols
				os_sprintf(buf,"%d:%d:%d",(u32)(os->time/3600),(u32)((os->time/60)%60),(u32)((os->time)%60));
				cJSON_AddItemToObject(last,"time",cJSON_CreateString(buf));
			}else{
				buf=new char[16];
				os_sprintf(buf,"%ld.%d",(u32)(os->time/10),(u32)(os->time%10));
				cJSON_AddItemToObject(last,"duration",cJSON_CreateConverted(buf));
			}
			if(os->smooth){
				if(cb->fire)
					cJSON_AddItemToObject(last,"fire",cJSON_CreateBool(true));
			}else{
				cJSON_AddItemToObject(last,"step",cJSON_CreateBool(true));
			}
			if(os->next_is_operation) os+=1;
			else{
				cb=(ControlByte*)(os+1);
				in_control=true;
			}
		}
	}while(true);
	return 0;
}

struct prog_interim_sequence{
	prog_interim_sequence*next;
	u8 type;
	char op[sizeof(OperationSequence)];
};
ICACHE_FLASH_ATTR void free_program_interim(prog_interim_sequence* start){
	prog_interim_sequence *i=start,*prev;
	DPRINT("Cleaning interim");
	int len=0;
	while(i){
		prev=i;
		i=i->next;
		os_free(prev);
		if(i==start) break;//loop prevent
	}
}

ICACHE_FLASH_ATTR void* program_from_interim(prog_interim_sequence* start){
	prog_interim_sequence *i=start;
	char*prog=0;
	int len=0;
	OperationSequence *os=0;
	ControlByte*cb=0;
	for(;i;i=i->next){
		if(len>MAX_PROGRAM_LENGTH || i->next==start){
			free_program_interim(start);
			return 0;
		}
		len+=(i->type==0)?sizeof(ControlByte):sizeof(OperationSequence);
	}
	DPRINT("Got program len: %d",len);
	if(len){
		prog=(char*)os_malloc(len);
		len=0;
		for(i=start;i;i=i->next){
			if(i->type==0){
				DPRINT("This is cb");
				if(os){
					os->next_is_operation=0;
					RGBLED::set_check_bit(os);
				}
				os_memcpy(prog+len,(i->op),sizeof(ControlByte));
				cb=(ControlByte*)(prog+len);
				cb->has_operations=1;
				RGBLED::set_check_bit(cb);
				len+=sizeof(ControlByte);
			}else{
				DPRINT("This is op");
				os_memcpy(prog+len,(i->op),sizeof(OperationSequence));
				os=(OperationSequence*)(prog+len);
				os->next_is_operation=1;
				RGBLED::set_check_bit(os);
				len+=sizeof(OperationSequence);
			}
		}
		if(cb){
			DPRINT("Set no-ops flag in last cb");
			cb->has_operations=0;
			RGBLED::set_check_bit(cb);
		}
		DPRINT("Is new program good?");
		if(!RGBLED::program_length(prog)){
			DPRINT("Nope =(");
			os_free(prog);
			prog=0;
		}
	}
	free_program_interim(start);
	return prog;
}

ICACHE_FLASH_ATTR bool object_isTrue(cJSON*p){
	if(!p) return false;
	if(p->type==cJSON_True) return true;
	if(p->type==cJSON_Number && p->valuedouble!=0) return true;
	return false;
}

ICACHE_FLASH_ATTR u32 strpart_to_uint(char**s){
	if(!s || !*s) return 0;
	char *p=*s;
	u32 num=0;
	p=skipspaces(p);
	while(*p<='9' && *p>='0'){num*=10;num+=*p-'0';++p;}
	p=skipspaces(p);
	*s=p;
	return num;
}

ICACHE_FLASH_ATTR u32 str_to_time(char*i){
	if(!i) return 0;
	u32 t=0,part=0;
	DPRINT("timestr: \"%s\"",i);
	t=strpart_to_uint(&i)*60;
	DPRINT("timestr: \"%s\"",i);
	if(*i==':'){ t+=strpart_to_uint(&(++i));}
	t*=60;
	DPRINT("timestr: \"%s\"",i);
	if(*i==':'){ t+=strpart_to_uint(&(++i));}
	return t;
}

ICACHE_FLASH_ATTR void* program_from_json(cJSON*p){
	if(!p || p->type!=cJSON_Array) return 0;
	u32 i,len=cJSON_GetArraySize(p);
	if(len<1) return 0;
	DPRINT("Got program len: %d",len);
	ControlByte*cb;
	OperationSequence* os;
	prog_interim_sequence *start,*I,*prev;
	start=new prog_interim_sequence;
	start->next=0;
	start->type=0;
	cb=(ControlByte*)(start->op);
	cb->reverse=0;
	cJSON *el=cJSON_GetArrayItem(p,0),*prop,*color,*duration;
	bool fire;
	cb->fire=(el->type==cJSON_Object && object_isTrue(cJSON_GetObjectItem(el,"fire")));
	DPRINT("First fire: %d",cb->fire);
	for(i=0,I=start;i<len;++i){
		prev=I;
		I=prev->next=new prog_interim_sequence;
		I->next=0;
		I->type=1;
		os=(OperationSequence*)(I->op);
		os->smooth=1;
		os->absolute=0;
		el=cJSON_GetArrayItem(p,i);
		DPRINT("Got el: %d",i);
		fire=(el->type==cJSON_Object && object_isTrue(cJSON_GetObjectItem(el,"fire")));
		DPRINT("Fire?: %d",fire?1:0);
		if(fire xor cb->fire){
			if(prev->type==1){
				DPRINT("Creating new CB with Fire flag");
				prev->next=new prog_interim_sequence;
				prev->next->next=I;
				prev->next->type=0;
				cb=(ControlByte*)(prev->next->op);
				cb->fire=fire?1:0;
				cb->reverse=0;
				prev=prev->next;
			}
			cb->fire=fire?1:0;
			DPRINT("Fire? %d",cb->fire);
		}
		if(el->type==cJSON_String){
			DPRINT("El is str %s",el->valuestring);
			color=el;
			duration=0;
		}else if(el->type==cJSON_Array){
			DPRINT("El is array");
			color=cJSON_GetArrayItem(el,0);
			duration=cJSON_GetArrayItem(el,1);
		}
		if(el->type==cJSON_Object){
			DPRINT("El is object");
			os->smooth=!object_isTrue(cJSON_GetObjectItem(el,"step"));
			os->color=RGBLED::fromString(cJSON_GetObjectItem(el,"color"));
			if(prop=cJSON_GetObjectItem(el,"duration")){
				DPRINT("Has prop duration");
				if(prop->type==cJSON_Number) os->time=prop->valuedouble*10.;
				else os->time=STANDARD_TRANSFER_TIME;
			}else if(prop=cJSON_GetObjectItem(el,"time")){
				DPRINT("Has prop time");
				os->absolute=1;
				if(prop->type==cJSON_Number) os->time=prop->valuedouble;
				else if(prop->type==cJSON_String){
					os->time=str_to_time(prop->valuestring);
				}else{
					os->time=0;
				}
			}else os->time=STANDARD_TRANSFER_TIME;

			if(object_isTrue(cJSON_GetObjectItem(el,"reverse"))){
				DPRINT("Color is %lx",(u32)os->color);
				DPRINT("time is %ld",os->time);
				DPRINT("We add reverse CB");
				I=I->next=new prog_interim_sequence;
				I->next=0;
				I->type=0;
				cb=(ControlByte*)(I->op);
				cb->reverse=1;
				break;
			}
		}else{
			if(color && color->type==cJSON_String){
				os->color=RGBLED::fromString(color->valuestring);
			}else{
				os->color=0;
			}
			if(duration && duration->type==cJSON_Number){
				os->time=duration->valuedouble*10.;
			}else{
				os->time=STANDARD_TRANSFER_TIME;
			}
		}
		DPRINT("Abs? %d",os->absolute);
		DPRINT("Smooth? %d",os->smooth);
		DPRINT("Color is %lx",(u32)os->color);
		DPRINT("time is %ld",os->time);
	}
	if(I->type==1){
		DPRINT("Creating new CB at the end");
		I=I->next=new prog_interim_sequence;
		I->next=0;
		I->type=0;
		cb=(ControlByte*)(I->op);
		cb->fire=0;
		cb->reverse=0;
	}
	DPRINT("Convert from interim");
	return program_from_interim(start);
}

ICACHE_FLASH_ATTR void task_setprog(RequestProcessor* p,cJSON*params){
	void*prog=program_from_json(params);
	if(prog)
		RGBLED::I().set_program(prog);
	os_free(prog);
}

ICACHE_FLASH_ATTR void task_getprog(RequestProcessor* p,cJSON*params){
	if(p->response){
		cJSON*prog=program_to_json(led->get_program());
		if(prog)
			cJSON_AddItemToObject(p->response,"program",prog);
		else
			cJSON_AddItemToObject(p->response,"program",cJSON_CreateNull());
	}
}






