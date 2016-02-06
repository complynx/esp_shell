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
}

#define DUTY_MUL 1000
#define DUTY_DIV 45

static RGBLED* led;

RGBLED& ICACHE_FLASH_ATTR RGBLED::I(){
	if(!led) led = new RGBLED();
	return *led;
}

ICACHE_FLASH_ATTR RGBLED::RGBLED() {
	u32 io_info[][3] =
	{{PWM_R_OUT_IO_MUX,PWM_R_OUT_IO_FUNC,PWM_R_OUT_IO_NUM},
	{PWM_G_OUT_IO_MUX,PWM_G_OUT_IO_FUNC,PWM_G_OUT_IO_NUM},
	{PWM_B_OUT_IO_MUX,PWM_B_OUT_IO_FUNC,PWM_B_OUT_IO_NUM}};

	DPRINT("Pins: R=%d, G=%d, B=%d",PWM_R_OUT_IO_NUM,PWM_G_OUT_IO_NUM,PWM_B_OUT_IO_NUM);

	uint32 pwm_duty_init[3] = {0};

	pwm_init(PWM_PERIOD,pwm_duty_init,3,io_info);
	color(Config::I().led_color());
}

u8 ICACHE_FLASH_ATTR RGBLED::R(){return _color[0];}
u8 ICACHE_FLASH_ATTR RGBLED::G(){return _color[1];}
u8 ICACHE_FLASH_ATTR RGBLED::B(){return _color[2];}

u8 ICACHE_FLASH_ATTR RGBLED::R(u8 r){return color_n(r,0);}
u8 ICACHE_FLASH_ATTR RGBLED::G(u8 r){return color_n(r,1);}
u8 ICACHE_FLASH_ATTR RGBLED::B(u8 r){return color_n(r,2);}

u8 ICACHE_FLASH_ATTR RGBLED::color_n(u8 i,u8 c){
	if(i>2) return 0;
	pwm_set_duty(toDuty(_color[i]),i);
	pwm_start();
	_color[i] = toByte(pwm_get_duty(i));
	return _color[i];
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

u32 ICACHE_FLASH_ATTR RGBLED::color(){
	return COLOR_A(_color);
}

u32 ICACHE_FLASH_ATTR RGBLED::color(u32 c){
	SPLIT_COLOR_A(_color,c);
	//Config::I().led_color(COLOR(_R,_G,_B));
	pwm_set_duty(toDuty(_color[0]),0);
	pwm_set_duty(toDuty(_color[1]),1);
	pwm_set_duty(toDuty(_color[2]),2);
	pwm_start();
	_color[0]=toByte(pwm_get_duty(0));
	_color[1]=toByte(pwm_get_duty(1));
	_color[2]=toByte(pwm_get_duty(2));
	return COLOR_A(_color);
}

u32 ICACHE_FLASH_ATTR RGBLED::color(u8 r,u8 g,u8 b){
	return color(COLOR(r,g,b));
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
	ret[1]=hexify((_color[0]>>1)&0xF);
	ret[2]=hexify(_color[0]&0xF);
	ret[3]=hexify((_color[1]>>1)&0xF);
	ret[4]=hexify(_color[1]&0xF);
	ret[5]=hexify((_color[2]>>1)&0xF);
	ret[6]=hexify(_color[2]&0xF);
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

u32 ICACHE_FLASH_ATTR RGBLED::fromString(char* str){
	u32 ret=0;
	u8 c;
	str=skipspaces(str);
	if(*str=='#'){
		c=dehexify(*(++str));if(c==255) return 0;		 ret+=c;//R
		c=dehexify(*(++str));if(c==255) return 0;ret<<=1;ret+=c;//R
		c=dehexify(*(++str));if(c==255) return 0;ret<<=1;ret+=c;//G
		c=dehexify(*(++str));if(c==255) return 0;ret<<=1;ret+=c;//G
		c=dehexify(*(++str));if(c==255) return 0;ret<<=1;ret+=c;//B
		c=dehexify(*(++str));if(c==255) return 0;ret<<=1;ret+=c;//B
		return ret;
	}else if(tolower(*str)=='r' && *(++str) && tolower(*str)=='g' && *(++str) && tolower(*str)=='b'){
		if(*(++str) && tolower(*str)=='a') ++str;
		str=skipspaces(str);
		if(*str!='(') return 0;
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
					return ret;
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
			return 0;
		}
		return 0;

	}
	return ret;
}


