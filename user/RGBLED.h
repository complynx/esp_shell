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

#pragma pack(push,1)
struct ColorAligned{
	u8 b;
	u8 g;
	u8 r;
};

union Color{
	u32 whole:24;
	ColorAligned components;
	inline Color() __attribute__((always_inline)){whole=0;}
	inline Color(const u32&i) __attribute__((always_inline)){whole=i;}
	inline Color(const Color&i) __attribute__((always_inline)){whole=i.whole;}
	inline Color(const u8& r,const u8& g,const u8& b) __attribute__((always_inline)){
		components.r=r;
		components.g=g;
		components.b=b;
	}
	inline u8& R() __attribute__((always_inline)){return components.r;}
	inline u8& G() __attribute__((always_inline)){return components.g;}
	inline u8& B() __attribute__((always_inline)){return components.b;}
	inline u8& operator[](const u8& i) __attribute__((always_inline)){
		if(i>2) return *((u8 *)(&components));
		return ((u8 *)(&components))[2-i];
	}
	inline operator u32() const __attribute__((always_inline)){return whole;}
	inline const Color operator+(const Color& right) __attribute__((always_inline)){
		Color c(*this);
		c+=right;
		return c;
	}
	inline Color& operator+=(const Color& right) __attribute__((always_inline)){
		components.r+=right.components.r;
		components.g+=right.components.g;
		components.b+=right.components.b;
		return *this;
	}
	inline const Color operator-(const Color& right) __attribute__((always_inline)){
		Color c(*this);
		c-=right;
		return c;
	}
	inline Color& operator-=(const Color& right) __attribute__((always_inline)){
		components.r-=right.components.r;
		components.g-=right.components.g;
		components.b-=right.components.b;
	    return *this;
	}
	inline Color& operator*=(const float& right) __attribute__((always_inline)){
		float c;
		c=((float)components.r)*right;
		if(c>255) c=255;
		else if(c<0) c=0;
		components.r=c;
		c=((float)components.g)*right;
		if(c>255) c=255;
		else if(c<0) c=0;
		components.g=c;
		c=((float)components.b)*right;
		if(c>255) c=255;
		else if(c<0) c=0;
		components.b=c;
		return (*this);
	}
	inline Color& operator/=(const float& right) __attribute__((always_inline)){
		return operator *=(1/right);
	}
	inline bool operator==(const Color& right) __attribute__((always_inline)){
		return whole==right.whole;
	}
};

struct ControlByte{
	u8 check_bit:1;
	u8 has_operations:1;
	u8 fire:1;
	u8 reverse:1;
	u8 tail:4;
};
struct OperationSequence{
	u8 check_bit:1;
	u8 absolute:1;
	u8 smooth:1;
	u8 next_is_operation:1;
	u32 time:20;
	Color color;
};
#pragma pack(pop)

class RGBLED {
	Color _color;
	Color _color_start;
	void* program;
	OperationSequence* current_operation;
	ControlByte* current_control;
	u8 timer_relative;
	u32 timer;
	RGBLED();
	RGBLED(RGBLED const&);
	void operator=(RGBLED const&);
	friend void cycle_tick();
public:
	static RGBLED &I();
	inline u8 R() __attribute__((always_inline)){return _color.components.r;}
	inline u8 G() __attribute__((always_inline)){return _color.components.g;}
	inline u8 B() __attribute__((always_inline)){return _color.components.b;}
	inline u8 R(const u8& c) __attribute__((always_inline)){return color_n(c,0);}
	inline u8 G(const u8& c) __attribute__((always_inline)){return color_n(c,1);}
	inline u8 B(const u8& c) __attribute__((always_inline)){return color_n(c,2);}
	char* toString();
	Color fromString(char*);
	u8 toByte(u32);
	u32 toDuty(u8);
	inline Color color() __attribute__((always_inline)){return _color;}
	Color color(Color);
	Color color(u8,u8,u8);
	u8 color_n(u8,u8);
	virtual ~RGBLED();
};

#endif /* USER_RGBLED_H_ */
