/*
 * RGBLED.cpp
 *
 *  Created on: 2 ����. 2016 �.
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
}

#define DUTY_MUL 1000
#define DUTY_DIV 45

static RGBLED* led;

RGBLED& ICACHE_FLASH_ATTR RGBLED::I(){
	if(!led) led = new RGBLED();
	return *led;
}


#define TICK 2000 //msec
static ETSTimer ledcycle;
void cycle_tick(){
	if(led->program){//if there is a program
		if(led->timer_relative){

		}
	}
	u32 cts=sntp_get_current_timestamp();
	if(cts){
		DPRINT("%s",sntp_get_real_time(cts));
	}
}

ICACHE_FLASH_ATTR RGBLED::RGBLED() {
	u32 io_info[][3] =
	{{PWM_R_OUT_IO_MUX,PWM_R_OUT_IO_FUNC,PWM_R_OUT_IO_NUM},
	{PWM_G_OUT_IO_MUX,PWM_G_OUT_IO_FUNC,PWM_G_OUT_IO_NUM},
	{PWM_B_OUT_IO_MUX,PWM_B_OUT_IO_FUNC,PWM_B_OUT_IO_NUM}};
	program=0;

	DPRINT("Pins: R=%d, G=%d, B=%d",PWM_R_OUT_IO_NUM,PWM_G_OUT_IO_NUM,PWM_B_OUT_IO_NUM);

	uint32 pwm_duty_init[3] = {0};

	pwm_init(PWM_PERIOD,pwm_duty_init,3,io_info);
	color(Config::I().led_color());

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
	col[i] = toByte(pwm_get_duty(i));
	return col[i];
}

u8 ICACHE_FLASH_ATTR RGBLED::toByte(u32 c){
	c+=MAX_DUTY/512;//eliminate rounding errors
	c*=255;
	c/=MAX_DUTY;
	if(c>255) c=255;
	return c & 0xFF;
}
u32 ICACHE_FLASH_ATTR RGBLED::toDuty(u8 c){
	u32 ret=c;
	ret*=MAX_DUTY;
	ret/=255;
	if(ret>MAX_DUTY) ret=MAX_DUTY;
	return ret;
}

ICACHE_FLASH_ATTR RGBLED::~RGBLED() {
	// TODO Auto-generated destructor stub
}

Color ICACHE_FLASH_ATTR RGBLED::color(Color c){
	DPRINT("%lx %lx",(u32)_color,(u32)c);
	_color=c;
	//Config::I().led_color(COLOR(_R,_G,_B));
	pwm_set_duty(toDuty(_color.components.r),0);
	pwm_set_duty(toDuty(_color.components.g),1);
	pwm_set_duty(toDuty(_color.components.b),2);
	pwm_start();
	_color.components.r=toByte(pwm_get_duty(0));
	_color.components.g=toByte(pwm_get_duty(1));
	_color.components.b=toByte(pwm_get_duty(2));
	DPRINT("%lx",(u32)_color);
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

char* ICACHE_FLASH_ATTR RGBLED::toString(){
	char* ret=new char[8];
	ret[0]='#';
	ret[1]=hexify((_color.components.r>>1)&0xF);
	ret[2]=hexify(_color.components.r&0xF);
	ret[3]=hexify((_color.components.g>>1)&0xF);
	ret[4]=hexify(_color.components.g&0xF);
	ret[5]=hexify((_color.components.b>>1)&0xF);
	ret[6]=hexify(_color.components.b&0xF);
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

Color ICACHE_FLASH_ATTR RGBLED::fromString(char* str){
	u32 ret=0;
	u8 c;
	str=skipspaces(str);
	if(*str=='#'){
		c=dehexify(*(++str));if(c==255) return (Color)0;		ret+=c;//R
		c=dehexify(*(++str));if(c==255) return (Color)0;ret<<=1;ret+=c;//R
		c=dehexify(*(++str));if(c==255) return (Color)0;ret<<=1;ret+=c;//G
		c=dehexify(*(++str));if(c==255) return (Color)0;ret<<=1;ret+=c;//G
		c=dehexify(*(++str));if(c==255) return (Color)0;ret<<=1;ret+=c;//B
		c=dehexify(*(++str));if(c==255) return (Color)0;ret<<=1;ret+=c;//B
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


