/*
 * wifi_errors.h
 *
 *  Created on: 15 сент. 2016 г.
 *      Author: complynx
 */

#ifndef INCLUDE_ERRNO_H_
#define INCLUDE_ERRNO_H_

#include <user_interface.h>

#define ERRNO_OK 0
#define ERRNO_UNDEFINED 1

#define ERRNO_STATION_ERROR 0x08
#define STATION_ERROR_TO_ERRNO(x) ((ERRNO_STATION_ERROR)|(x))
#define STATION_ERROR_FROM_ERRNO(x) (((ERRNO_STATION_ERROR)-1)&(x))


#endif /* INCLUDE_ERRNO_H_ */
