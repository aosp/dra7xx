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

#ifndef _OSAL_ERROR_H_
#define _OSAL_ERROR_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define OSAL_TRUE 1
#define OSAL_FALSE 0

#define SUCCESS 0
#define NO_SUCCESS -1

#define OSAL_SUSPEND     0xFFFFFFFFUL
#define OSAL_NO_SUSPEND  0x0
#define OSAL_TIMED_OUT   0x7FFFFFFFUL

typedef enum osal_error {
    OSAL_ErrNone = 0,
/* General module error list */
    OSAL_ErrNoPermissions   = -1,
    OSAL_ErrNotSupported    = -2,
    OSAL_ErrAlloc           = -3,
    OSAL_ErrOutOfResource   = -4,
    OSAL_ErrTimeOut         = -5,
    OSAL_ErrParameter       = -6,
/* Pipe Module error list */
    OSAL_ErrNotReady        = -11,
    OSAL_ErrPipeFull        = -12,
    OSAL_ErrPipeEmpty       = -13,
    OSAL_ErrPipeDeleted     = -14,
    OSAL_ErrPipeReset       = -15,
    OSAL_ErrPipeClose       = -16,
    OSAL_ErrPipeWrite       = -17,
    OSAL_ErrPipeRead        = -18,
/* Semaphore module error list */
    OSAL_ErrSemCreate       = -20,
    OSAL_ErrSemDelete       = -21,
    OSAL_ErrSemPost         = -22,
    OSAL_ErrSemGetValue     = -23,
/* message queue module error list */
    OSAL_ErrMsgSzieMismatch = -30,
    OSAL_ErrMsgTypeNotFound = -31,
    OSAL_ErrUnKnown         = -32,
/* Mutex module error list */
    OSAL_ErrMutexCreate     = -40,
    OSAL_ErrMutexDestroy    = -41,
    OSAL_ErrMutexLock       = -42,
    OSAL_ErrMutexUnlock     = -43,
/* Events module error list */
    OSAL_ErrEventCreate     = -50,
    OSAL_ErrEventDestroy    = -51,
    OSAL_ErrSetEvent        = -52,
    OSAL_ErrRetrieve        = -53,
    OSAL_ErrEventSignal     = -54,
/* Thread module error list */
    OSAL_ErrThreadCreate    = -60,
    OSAL_ErrThreadDestroy   = -61,
} OSAL_ERROR;


#ifdef __cplusplus
}
#endif

#endif /*_OSAL_ERROR_H_*/
