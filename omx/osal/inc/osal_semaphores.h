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

#ifndef _OSAL_SEMAPHORE_H_
#define _OSAL_SEMAPHORE_H_

#ifdef __cplusplus
extern "C"
{
#endif

OSAL_ERROR OSAL_CreateSemaphore(void **pSemaphore, uint32_t uInitCount);
OSAL_ERROR OSAL_DeleteSemaphore(void *pSemaphore);
OSAL_ERROR OSAL_ObtainSemaphore(void *pSemaphore, uint32_t uTimeOut);
OSAL_ERROR OSAL_ReleaseSemaphore(void *pSemaphore);
OSAL_ERROR OSAL_ResetSemaphore(void *pSemaphore, uint32_t uInitCount);
OSAL_ERROR OSAL_GetSemaphoreCount(void *pSemaphore, uint32_t *count);

#ifdef __cplusplus
}
#endif

#endif  /* _OSAL_SEMAPHORE_H_ */
