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

#include <stdio.h>

#include <semaphore.h>
#include <sys/time.h>

#include "osal_trace.h"
#include "osal_error.h"
#include "osal_memory.h"

OSAL_ERROR OSAL_CreateSemaphore(void **pSemaphore, uint32_t uInitCount)
{
    OSAL_ERROR bRet = OSAL_ErrUnKnown;
    *pSemaphore = NULL;
    sem_t *psem = (sem_t *)OSAL_Malloc(sizeof(sem_t));
    if (NULL == psem) {
        bRet = OSAL_ErrAlloc;
        goto EXIT;
    }

    /*Unnamed semaphore */
    if (SUCCESS != sem_init(psem, 0, uInitCount)) {
        OSAL_ErrorTrace("Semaphore Create failed !");
        bRet = OSAL_ErrSemCreate;
        goto EXIT;
    }
    *pSemaphore = (void *) psem;
    bRet = OSAL_ErrNone;

EXIT:
    if (OSAL_ErrNone != bRet && NULL != psem) {
        OSAL_Free(psem);
    }
    return bRet;
}

OSAL_ERROR OSAL_DeleteSemaphore(void *pSemaphore)
{
    sem_t *psem = (sem_t *)pSemaphore;
    if (NULL == psem) {
        return OSAL_ErrParameter;
    }
    /* Release the semaphore.  */
    if (SUCCESS != sem_destroy(psem)) {
        OSAL_ErrorTrace("Semaphore Delete failed !");
        return OSAL_ErrSemDelete;
    }

    OSAL_Free(psem);
    return OSAL_ErrNone;
}

OSAL_ERROR OSAL_ObtainSemaphore(void *pSemaphore, uint32_t uTimeOut)
{
    struct timeval ltime_now;
    struct timespec abs_timeout;
    sem_t *psem = (sem_t *) pSemaphore;
    if (psem == NULL) {
        return OSAL_ErrParameter;
    }

    if (OSAL_SUSPEND == uTimeOut) {
        if (SUCCESS != sem_wait(psem)) {
            OSAL_ErrorTrace("Semaphore Wait failed !");
            return OSAL_ErrTimeOut;
        }
    } else if (OSAL_NO_SUSPEND == uTimeOut) {
        if (SUCCESS != sem_trywait(psem)) {
            OSAL_ErrorTrace("Semaphore blocked !");
            return OSAL_ErrTimeOut;
        }
    } else {
        /* Some delay in calling gettimeofday and sem_timedwait - cant
        * be avoided. Possibility of thread switch after gettimeofday
        * in which case time out will be less than expected */
        gettimeofday(&ltime_now, NULL);
        /*uTimeOut is assumed to be in milliseconds */
        abs_timeout.tv_sec = ltime_now.tv_sec + (uTimeOut / 1000);
        abs_timeout.tv_nsec = 1000 * (ltime_now.tv_usec + ((uTimeOut % 1000) * 1000));
        if (SUCCESS != sem_timedwait(psem, &abs_timeout)) {
            OSAL_ErrorTrace("Semaphore Timed Wait failed !");
            return OSAL_ErrTimeOut;
        }
    }
    return OSAL_ErrNone;
}

OSAL_ERROR OSAL_ReleaseSemaphore(void *pSemaphore)
{
    sem_t *psem = (sem_t *) pSemaphore;
    if (NULL == psem) {
        return OSAL_ErrParameter;
    }
    /* Release the semaphore.  */
    if (SUCCESS != sem_post(psem)) {
        OSAL_ErrorTrace("Release failed !");
        return OSAL_ErrSemPost;
    }
    return OSAL_ErrNone;
}

OSAL_ERROR OSAL_ResetSemaphore(void *pSemaphore, uint32_t uInitCount)
{
    return OSAL_ErrNone;
}

OSAL_ERROR OSAL_GetSemaphoreCount(void *pSemaphore, uint32_t *count)
{
    int sval = -2;		/*value that is not possible */
    sem_t *psem = (sem_t *) pSemaphore;
    if (NULL == psem) {
        return OSAL_ErrParameter;
    }

    /* Release the semaphore.  */
    if (SUCCESS != sem_getvalue(psem, &sval)) {
        OSAL_ErrorTrace("Get Semaphore Count failed !");
        return OSAL_ErrSemGetValue;
    }

    *count = sval;
    return OSAL_ErrNone;
}
