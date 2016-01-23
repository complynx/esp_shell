/*
 * Config.cpp
 *
 *  Created on: 14 џэт. 2016 у.
 *      Author: complynx
 */

#include "Config.h"
#include <ets_sys.h>
#include "osapi.h"
#include "crc32.h"
#include <os_type.h>
extern "C"{
#include "user_interface.h"
}
#include "user_config.h"

static Config *c=(Config*)0;


#if USED_MEM>SPI_FLASH_SEC_SIZE
#error "Memory for params is too big to be saved"
#endif

#define MAGIC 0xFEEDF00D

Config& ICACHE_FLASH_ATTR Config::I(){
	if(!c)
		c=new Config();
	return *c;
}

ICACHE_FLASH_ATTR Config::Config() {
	U=new Config_inner_mem;
	cnf=&(U->config);
	_disable_saving=false;
	load();
}

ICACHE_FLASH_ATTR Config::~Config() {
	delete U;
}

void ICACHE_FLASH_ATTR Config::reset(){
#undef CONFIG
#define CONFIG(type,name,def) cnf->name=def;
#undef CONFIG_S
#define CONFIG_S(name,maxlen,def) ets_strncpy(cnf->name,def,maxlen-1);

#undef CONFIG_F
#define CONFIG_F(name,ind,def) if(def) MASK_SETN(cnf->flags,ind);\
		else MASK_UNSETN(cnf->flags,ind);

	DPRINT("Resetting settings to default");
	cnf->flags=0;

	CONFIGS

	os_sprintf(cnf->name,"ESP-%lu",system_get_chip_id());

	_is_default=true;
	save();
}

void ICACHE_FLASH_ATTR Config::load(){
	DPRINT("Loading config...");
	_is_default=false;

	spi_flash_read(ESP_PARAM_SEC_1*SPI_FLASH_SEC_SIZE, (unsigned int*)U->mem,USED_MEM);
	u32 crc=crc32(MAGIC,U->mem+sizeof(u32),USED_MEM-sizeof(u32));
	if(U->config.CRC==crc) return;
	DPRINT("Sector 1 corrupt, trying sector 2...");

	spi_flash_read(ESP_PARAM_SEC_2*SPI_FLASH_SEC_SIZE, (unsigned int*)U->mem,USED_MEM);
	crc=crc32(MAGIC,U->mem+sizeof(u32),USED_MEM-sizeof(u32));
	if(U->config.CRC==crc) return;

	DPRINT("Sector 2 corrupt.");
	reset();
}

void ICACHE_FLASH_ATTR Config::save(){
	if(!_disable_saving){
		DPRINT("Saving config...");
		u32 crc=crc32(MAGIC,U->mem+sizeof(u32),USED_MEM-sizeof(u32));
		U->config.CRC=crc;
		spi_flash_erase_sector(ESP_PARAM_SEC_1);
		spi_flash_write(ESP_PARAM_SEC_1*SPI_FLASH_SEC_SIZE, (unsigned int*)U->mem,USED_MEM);
		spi_flash_erase_sector(ESP_PARAM_SEC_2);
		spi_flash_write(ESP_PARAM_SEC_2*SPI_FLASH_SEC_SIZE, (unsigned int*)U->mem,USED_MEM);
	}
}

void ICACHE_FLASH_ATTR Config::disable_saving(){
	_disable_saving=true;
}
void ICACHE_FLASH_ATTR Config::enable_saving(){
	_disable_saving=false;
	save();
}



#undef CONFIG
#define CONFIG(type,name,def) ICACHE_FLASH_ATTR type Config::name(){\
		return cnf->name;\
	}\
	ICACHE_FLASH_ATTR type Config::name(type temp){\
		cnf->name=temp;\
		save();\
		return cnf->name;\
	}
#undef CONFIG_S
#define CONFIG_S(name,maxlen,def) ICACHE_FLASH_ATTR char* Config::name(){\
		return cnf->name;\
	}\
	ICACHE_FLASH_ATTR char* Config::name(char* temp){\
		ets_strncpy(cnf->name,temp,maxlen-1);\
		cnf->name[maxlen-1]=0;\
		save();\
		return cnf->name;\
	}

#undef CONFIG_F
#define CONFIG_F(name,ind,def) ICACHE_FLASH_ATTR bool Config::name(){\
		return (MASK_HASN(cnf->flags,ind));\
	}\
	ICACHE_FLASH_ATTR bool Config::name(bool set){\
		if(set) MASK_SETN(cnf->flags,ind);\
		else MASK_UNSETN(cnf->flags,ind);\
		save();\
		return (MASK_HASN(cnf->flags,ind));\
	}

CONFIGS
