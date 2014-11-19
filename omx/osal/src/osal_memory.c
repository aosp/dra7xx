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

#include <string.h>
#include <malloc.h>
#include <stdint.h>

#include "osal_trace.h"
#include "osal_error.h"
#include "osal_memory.h"

void* OSAL_Malloc(size_t size)
{
    void* pData = NULL;
    pData = malloc((size_t) size);
    return pData;
}

void OSAL_Free(void *pData)
{
    if (NULL == pData) {
        OSAL_WarningTrace("TIMM_OSAL_Free called on NULL pointer");
        return;
    }

    free(pData);
    pData = NULL;
}


OSAL_ERROR OSAL_Memset(void *pBuffer, uint8_t uValue, uint32_t uSize)
{
    if (NULL == pBuffer || uSize == 0) {
        OSAL_WarningTrace("OSAL_Memset is called on NULL pointer");
        return OSAL_ErrParameter;
    }

    memset((void *)pBuffer, (int)uValue, (size_t) uSize);
    return OSAL_ErrNone;
}



int32_t OSAL_Memcmp(void *pBuffer1, void *pBuffer2, uint32_t uSize)
{
    if (NULL == pBuffer1 || NULL == pBuffer2) {
        OSAL_WarningTrace("OSAL_MemCmp called with null buffers");
        return OSAL_ErrParameter;
    }

    int32_t result = memcmp(pBuffer1, pBuffer2, uSize);
    return (result == 0) ?  result : ((result > 0) ? 1 : -1);
}


OSAL_ERROR OSAL_Memcpy(void *pBufDst, void *pBufSrc, uint32_t uSize)
{
    if (NULL == pBufDst || NULL == pBufSrc) {
        OSAL_WarningTrace("OSAL_Memcpy called with null buffers");
        return OSAL_ErrParameter;
    }
    memcpy(pBufDst, pBufSrc, uSize);
    return OSAL_ErrNone;
}


