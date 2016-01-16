/*
 * Config.h
 *
 *  Created on: 14 џэт. 2016 у.
 *      Author: complynx
 */

#ifndef USER_CONFIG_H_
#define USER_CONFIG_H_

#include "bitmasks.h"
#include "espmissingincludes.h"
#include "c_types.h"

#define MAX_SAVED_STR 64

#define CONFIGS \
	CONFIG(u16,port)\
	CONFIG_S(name,MAX_SAVED_STR)\
	CONFIG_F(wifi_configured,0)

#undef CONFIG
#define CONFIG(type,name) type name;
#undef CONFIG_S
#define CONFIG_S(name,maxlen) char name[];
#undef CONFIG_F
#define CONFIG_F(name,ind)

struct Config_inner{
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
#define CONFIG(type,name) inline type name(){\
		return cnf->name;\
	}\
	inline type name(type temp){\
		cnf->name=temp;\
		save();\
		return cnf->name;\
	}
#undef CONFIG_S
#define CONFIG_S(name,maxlen) inline char* name(){\
		return cnf->name;\
	}\
	inline char* name(char* temp){\
		ets_strncpy(cnf->name,temp,maxlen-1);\
		cnf->name[maxlen-1]=0;\
		save();\
		return cnf->name;\
	}

#undef CONFIG_F
#define CONFIG_F(name,ind) inline bool name(){\
		return (MASK_HASN(cnf->flags,ind));\
	}\
	inline bool name(bool set){\
		if(set) MASK_SETN(cnf->flags,ind);\
		else MASK_UNSETN(cnf->flags,ind);\
		save();\
		return (MASK_HASN(cnf->flags,ind));\
	}

class Config {
public:
	static Config& instance();
	virtual ~Config();
	void zero();
	CONFIGS
private:
	Config_inner_mem *U;
	Config_inner *cnf;
	Config();
	Config(Config const&);
	void operator=(Config const&);
	void load();
	void save();

};

#endif /* USER_CONFIG_H_ */
