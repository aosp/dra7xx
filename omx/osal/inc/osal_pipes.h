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

#ifndef _OSAL_PIPES_H_
#define _OSAL_PIPES_H_

#ifdef __cplusplus
extern "C"
{
#endif

OSAL_ERROR OSAL_CreatePipe(void **pPipe, uint32_t pipeSize, uint32_t messageSize, uint8_t isFixedMessage);

OSAL_ERROR OSAL_DeletePipe(void *pPipe);

OSAL_ERROR OSAL_WriteToPipe(void *pPipe, void *pMessage, uint32_t size, int32_t timeout);

OSAL_ERROR OSAL_WriteToFrontOfPipe(void *pPipe, void *pMessage, uint32_t, int32_t timeout);

OSAL_ERROR OSAL_ReadFromPipe(void *pPipe, void *pMessage, uint32_t size, uint32_t *actualSize, int32_t timeout);

OSAL_ERROR OSAL_ClearPipe(void *pPipe);

OSAL_ERROR OSAL_IsPipeReady(void *pPipe);

OSAL_ERROR OSAL_GetPipeReadyMessageCount(void *pPipe, uint32_t *count);


#ifdef __cplusplus
}
#endif

#endif  /* _OSAL_PIPES_H_ */
