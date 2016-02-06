/*
 * RGBLED.h
 *
 *  Created on: 2 февр. 2016 г.
 *      Author: complynx
 */

#ifndef USER_RGBLED_H_
#define USER_RGBLED_H_

extern "C"{
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "user_interface.h"
}

#define PWM_R_OUT_IO_MUX 	PERIPHS_IO_MUX_MTDI_U
#define PWM_R_OUT_IO_NUM 	12
#define PWM_R_OUT_IO_FUNC  	FUNC_GPIO12

#define PWM_G_OUT_IO_MUX 	PERIPHS_IO_MUX_MTMS_U
#define PWM_G_OUT_IO_NUM 	14
#define PWM_G_OUT_IO_FUNC  	FUNC_GPIO14

#define PWM_B_OUT_IO_MUX 	PERIPHS_IO_MUX_MTCK_U
#define PWM_B_OUT_IO_NUM 	13
#define PWM_B_OUT_IO_FUNC  	FUNC_GPIO13

//#define PWM_3_OUT_IO_MUX PERIPHS_IO_MUX_MTDO_U
//#define PWM_3_OUT_IO_NUM 15
//#define PWM_3_OUT_IO_FUNC  FUNC_GPIO15
//
//#define PWM_4_OUT_IO_MUX PERIPHS_IO_MUX_GPIO5_U
//#define PWM_4_OUT_IO_NUM 5
//#define PWM_4_OUT_IO_FUNC  FUNC_GPIO5

#define COLOR(R,G,B) ((((u32)R)<<16) | (((u32)G)<<8) | ((u32)B))
#define COLOR_A(A) ((((u32)(A)[0])<<16) | (((u32)(A)[1])<<8) | ((u32)(A)[2]))
#define SPLIT_COLOR(R,G,B,C) do{(B)=(C)&0xFF;(G)=((C)>>8)&0xFF;(R)=((C)>>16)&0xFF;}while(0);
#define SPLIT_COLOR_A(A,C) do{(A)[2]=(C)&0xFF;(A)[1]=((C)>>8)&0xFF;(A)[0]=((C)>>16)&0xFF;}while(0);
#define EXEC_COLOR_A(F,A) do{F((A)[0]);F((A)[1]);F((A)[2]);}while(0)

class RGBLED {
	u8 _color[3];
	RGBLED();
	RGBLED(RGBLED const&);
	void operator=(RGBLED const&);
public:
	static RGBLED &I();
	u8 R();
	u8 R(u8);
	u8 G();
	u8 G(u8);
	u8 B();
	u8 B(u8);
	char* toString();
	u32 fromString(char*);
	u8 toByte(u32);
	u32 toDuty(u8);
	u32 color();
	u32 color(u32);
	u32 color(u8,u8,u8);
	u8 color_n(u8,u8);
	virtual ~RGBLED();
};

#endif /* USER_RGBLED_H_ */
