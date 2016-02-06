/*
 * Config.h
 *
 *  Created on: 14 џэт. 2016 у.
 *      Author: complynx
 */

#ifndef USER_CONFIG_H_
#define USER_CONFIG_H_

#include "user_config.h"
#include "bitmasks.h"
#include "espmissingincludes.h"
#include "c_types.h"

#define MAX_SAVED_STR 64

#define CONFIGS \
	CONFIG(u32,pwm_period,1000)\
	CONFIG(u32,led_color,0x00FFFFFF)\
	CONFIG_S(name,MAX_SAVED_STR,"")\
	CONFIG_F(wifi_configured,0,false)

#define CNF_DEFAULTS \
	DEFAULT(port,)

#undef CONFIG
#define CONFIG(type,name,def) type name;
#undef CONFIG_S
#define CONFIG_S(name,maxlen,def) char name[];
#undef CONFIG_F
#define CONFIG_F(name,ind,def)

struct Config_inner{
	u32 CRC;
	u8 flags;
	CONFIGS
};
//#define USED_MEM (((sizeof(Config_inner)/4)+1)*4)
#define USED_MEM 4096

union Config_inner_mem{
	char mem[USED_MEM];
	Config_inner config;
};

#undef CONFIG
#define CONFIG(type,name,def) type name();\
		type name(type temp);

#undef CONFIG_S
#define CONFIG_S(name,maxlen,def) char* name();\
	char* name(char* temp);

#undef CONFIG_F
#define CONFIG_F(name,ind,def) bool name();\
	bool name(bool set);

class Config {
	bool _is_default;
	bool _disable_saving;
public:
	static Config& I();
	virtual ~Config();
	void reset();
	void load();
	void save();
	void disable_saving();
	void enable_saving();
	inline bool is_default(){return _is_default;};
	CONFIGS
private:
	Config_inner_mem *U;
	Config_inner *cnf;
	Config();
	Config(Config const&);
	void operator=(Config const&);

};

#endif /* USER_CONFIG_H_ */
