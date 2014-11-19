/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _OSAL_MEMORY_H_
#define _OSAL_MEMORY_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "osal_error.h"

void *OSAL_Malloc(uint32_t size);

void OSAL_Free(void *pData);

OSAL_ERROR OSAL_Memset(void *pBuffer, uint8_t uValue, uint32_t uSize);

int32_t OSAL_Memcmp(void *pBuffer1, void *pBuffer2, uint32_t uSize);

OSAL_ERROR OSAL_Memcpy(void *pBufDst, void *pBufSrc, uint32_t uSize);


#ifdef __cplusplus
}
#endif

#endif  /* _OSAL_DEFINES_H_ */
