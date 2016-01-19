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


#if USED_MEM>4096
#error "Memory for params is too big to be saved"
#endif


Config& ICACHE_FLASH_ATTR Config::instance(){
	if(!c)
		c=new Config();
	return *c;
}

ICACHE_FLASH_ATTR Config::Config() {
	U=new Config_inner_mem;
	cnf=&(U->config);
	load();
}

ICACHE_FLASH_ATTR Config::~Config() {
	delete U;
}

void Config::zero(){
	DPRINT("Resetting settings to default");
	cnf->port=UDP_PORT;
	os_memset(cnf->name,0,MAX_SAVED_STR);
	os_sprintf(cnf->name,"ESP-%lu",system_get_chip_id());
	cnf->flags=0;
	save();
}

void Config::load(){
	DPRINT("Loading config...");
	system_param_load(ESP_PARAM_START_SEC,0, U->mem,USED_MEM);
	u32 crc=crc32(0,U->mem+sizeof(u32),USED_MEM-sizeof(u32));
	if(U->config.CRC!=crc){
		DPRINT("Checking CRC32 failed.");
		zero();
	}
}

void Config::save(){
	DPRINT("Saving config...");
	u32 crc=crc32(0,U->mem+sizeof(u32),USED_MEM-sizeof(u32));
	U->config.CRC=crc;
	system_param_save_with_protect(ESP_PARAM_START_SEC, U->mem,USED_MEM);
}
