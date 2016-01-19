/*
 * crc32.h
 *
 *  Created on: 19 џэт. 2016 у.
 *      Author: complynx
 */

#ifndef USER_CRC32_H_
#define USER_CRC32_H_

#ifdef __cplusplus
extern "C"{
#endif//__cplusplus
#include "os_type.h"

u32
crc32(u32 crc, const void *buf, size_t size);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif /* USER_CRC32_H_ */
