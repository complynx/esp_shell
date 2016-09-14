/*
 * wifi_errors.h
 *
 *  Created on: 15 сент. 2016 г.
 *      Author: complynx
 */

#ifndef INCLUDE_ERRNO_H_
#define INCLUDE_ERRNO_H_

#define ERRNO_OK ((u32)0)
#define ERRNO_UNDEFINED ((u32)1)

#define ERRNO_ADD_PERFIX(prefix,x) ((u32)(prefix)|(x))
#define ERRNO_REMOVE_PERFIX(prefix,x) ((x)&((prefix)-1))

#define ERRNO_STATION_ERROR_PREFIX ((u32)0x08)
#define STATION_ERROR_TO_ERRNO(x) ERRNO_ADD_PERFIX(ERRNO_STATION_ERROR_PREFIX,x)
#define STATION_ERROR_FROM_ERRNO(x) ERRNO_REMOVE_PERFIX(ERRNO_STATION_ERROR_PREFIX,x)


#endif /* INCLUDE_ERRNO_H_ */
