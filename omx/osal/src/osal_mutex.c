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

#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

#include "osal_trace.h"
#include "osal_error.h"
#include "osal_memory.h"
#include "osal_semaphores.h"

OSAL_ERROR OSAL_CreateMutex(void **pMutex)
{
    pthread_mutex_t *plMutex = (pthread_mutex_t *)OSAL_Malloc(sizeof(pthread_mutex_t));
    if (NULL == plMutex) {
        return OSAL_ErrAlloc;
    }

    if (SUCCESS != pthread_mutex_init(plMutex, NULL)) {
        OSAL_Free(plMutex);
        return OSAL_ErrMutexCreate;
    }

    *pMutex = (void*) plMutex;
    return OSAL_ErrNone;
}

OSAL_ERROR OSAL_DeleteMutex(void *pMutex)
{
    pthread_mutex_t *plMutex = (pthread_mutex_t *) pMutex;
    if (NULL == plMutex) {
        return OSAL_ErrParameter;
    }

    /*can we do away with if or with switch case */
    if (SUCCESS != pthread_mutex_destroy(plMutex)) {
        OSAL_ErrorTrace("Delete Mutex failed !");
        return OSAL_ErrMutexDestroy;
    }

    OSAL_Free(plMutex);
    return OSAL_ErrNone;
}


OSAL_ERROR OSAL_ObtainMutex(void *pMutex, uint32_t uTimeOut)
{
    struct timespec abs_timeout;
    struct timeval ltime_now;
    uint32_t ltimenow_us;
    pthread_mutex_t *plMutex = (pthread_mutex_t *) pMutex;
        if (plMutex == NULL) {
        return OSAL_ErrParameter;
    }

    if (OSAL_SUSPEND == uTimeOut) {
        if (SUCCESS != pthread_mutex_lock(plMutex)) {
            OSAL_ErrorTrace("Lock Mutex failed !");
            return OSAL_ErrMutexLock;
        }
    } else if (OSAL_NO_SUSPEND == uTimeOut) {
        if (SUCCESS != pthread_mutex_trylock(plMutex)) {
            OSAL_ErrorTrace("Lock Mutex failed !");
            return OSAL_ErrMutexLock;
        }
    } else {
        gettimeofday(&ltime_now, NULL);
        /*uTimeOut is assumed to be in milliseconds */
        ltimenow_us = ltime_now.tv_usec + 1000 * uTimeOut;
        abs_timeout.tv_sec = ltime_now.tv_sec + uTimeOut / 1000;
        abs_timeout.tv_nsec = (ltimenow_us % 1000000) * 1000;
        if (SUCCESS != pthread_mutex_lock(plMutex)) {
            OSAL_ErrorTrace("Lock Mutex failed !");
            return OSAL_ErrMutexLock;
        }
    }
    return OSAL_ErrNone;
}


OSAL_ERROR OSAL_ReleaseMutex(void *pMutex)
{
    pthread_mutex_t *plMutex = (pthread_mutex_t *) pMutex;
    if (NULL == plMutex) {
        return OSAL_ErrParameter;
    }

    if (SUCCESS != pthread_mutex_unlock(plMutex)) {
        OSAL_ErrorTrace("Unlock Mutex failed !");
        return OSAL_ErrMutexUnlock;
    }
    return OSAL_ErrNone;
}
