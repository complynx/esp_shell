/*
 * IoTTasks.h
 *
 *  Created on: 26 џэт. 2016 у.
 *      Author: complynx
 */

#ifndef INCLUDE_IOTTASKS_H_
#define INCLUDE_IOTTASKS_H_

#include "IoTServer.h"

#define IOT_TASKS \
	IOT_TASK(whoAmI)\
	IOT_TASK(setcolor)\
	IOT_TASK(getcolor)\
	IOT_TASK(toggle)\
	IOT_TASK(is_enabled)\
	IOT_TASK(getprog)\
	IOT_TASK(setprog)


//#undef IOT_TASK
//#define IOT_TASK(name) IOT_TASK_FUNCTION(#name,task_ ## name)
#define IOT_TASK_FUNCTIONS \


#undef IOT_TASK_FUNCTION
#define IOT_TASK_FUNCTION(name,cb) void cb(RequestProcessor* p,cJSON*params);
#undef IOT_TASK
#define IOT_TASK(name) IOT_TASK_FUNCTION(#name,task_ ## name)
IOT_TASKS
IOT_TASK_FUNCTIONS
#undef IOT_TASK
#undef IOT_TASK_FUNCTION

#endif /* INCLUDE_IOTTASKS_H_ */
