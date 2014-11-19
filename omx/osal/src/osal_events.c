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
#include <pthread.h>    /*for POSIX calls */
#include <sys/time.h>
#include <errno.h>

#include "osal_trace.h"
#include "osal_error.h"
#include "osal_memory.h"
#include "osal_events.h"

/*
* Thread event internal structure
*/
typedef struct
{
    uint8_t bSignaled;
    uint32_t eFlags;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
} OSAL_ThreadEvent;

/* Event Create Method */
OSAL_ERROR OSAL_CreateEvent(void **pEvents)
{
    OSAL_ERROR bRet = OSAL_ErrUnKnown;
    OSAL_ThreadEvent *plEvent = (OSAL_ThreadEvent *) OSAL_Malloc(sizeof(OSAL_ThreadEvent));
    if (NULL == plEvent) {
        bRet = OSAL_ErrAlloc;
        goto EXIT;
    }
    plEvent->bSignaled = OSAL_FALSE;
    plEvent->eFlags = 0;

    if (SUCCESS != pthread_mutex_init(&(plEvent->mutex), NULL)) {
        OSAL_ErrorTrace("Event Create:Mutex Init failed !");
        bRet = OSAL_ErrMutexCreate;
        goto EXIT;
    }

    if (SUCCESS != pthread_cond_init(&(plEvent->condition), NULL)) {
        OSAL_ErrorTrace("Event Create:Conditional Variable  Init failed !");
        pthread_mutex_destroy(&(plEvent->mutex));
        bRet = OSAL_ErrEventCreate;
    } else {
        *pEvents = (void *) plEvent;
        bRet = OSAL_ErrNone;
    }
EXIT:
    if ((OSAL_ErrNone != bRet) && (NULL != plEvent)) {
        OSAL_Free(plEvent);
    }
    return bRet;
}

/*
* Method to delete the event
*/
OSAL_ERROR OSAL_DeleteEvent(void *pEvents)
{
    OSAL_ERROR bRet = OSAL_ErrNone;
    OSAL_ThreadEvent *plEvent = (OSAL_ThreadEvent*) pEvents;
    if (NULL == plEvent) {
        bRet = OSAL_ErrParameter;
        goto EXIT;
    }

    if (SUCCESS != pthread_mutex_lock(&(plEvent->mutex))) {
        OSAL_ErrorTrace("Event Delete: Mutex Lock failed !");
        bRet = OSAL_ErrMutexLock;
    }
    if (SUCCESS != pthread_cond_destroy(&(plEvent->condition))) {
        OSAL_ErrorTrace("Event Delete: Conditional Variable Destroy failed !");
        bRet = OSAL_ErrEventDestroy;
    }
    if (SUCCESS != pthread_mutex_unlock(&(plEvent->mutex))) {
        OSAL_ErrorTrace("Event Delete: Mutex Unlock failed !");
        bRet = OSAL_ErrMutexUnlock;
    }
    if (SUCCESS != pthread_mutex_destroy(&(plEvent->mutex))) {
        OSAL_ErrorTrace("Event Delete: Mutex Destory failed !");
        bRet = OSAL_ErrMutexDestroy;
    }
    OSAL_Free(plEvent);
EXIT:
    return bRet;
}

/*
* Method to set event operation
*/
OSAL_ERROR OSAL_SetEvent(void *pEvents, uint32_t uEventFlags, OSAL_EventOp eOperation)
{
    OSAL_ThreadEvent *plEvent = (OSAL_ThreadEvent*) pEvents;
    if (NULL == plEvent) {
        return OSAL_ErrParameter;
    }
    if (SUCCESS != pthread_mutex_lock(&(plEvent->mutex))) {
        OSAL_ErrorTrace("Event Set: Mutex Lock failed !");
        return OSAL_ErrMutexLock;
    }

    switch (eOperation) {
    case OSAL_EVENT_AND:
        plEvent->eFlags = plEvent->eFlags & uEventFlags;
        break;
    case OSAL_EVENT_OR:
        plEvent->eFlags = plEvent->eFlags | uEventFlags;
        break;
    default:
        OSAL_ErrorTrace("Event Set: Bad eOperation !");
        pthread_mutex_unlock(&plEvent->mutex);
        return OSAL_ErrMutexUnlock;
    };

    plEvent->bSignaled = OSAL_TRUE;
    if (SUCCESS != pthread_cond_signal(&plEvent->condition)) {
        OSAL_ErrorTrace("Event Set: Condition Variable Signal failed !");
        pthread_mutex_unlock(&plEvent->mutex);
        return OSAL_ErrEventSignal;
    }

    if (SUCCESS != pthread_mutex_unlock(&plEvent->mutex)) {
        OSAL_ErrorTrace("Event Set: Mutex Unlock failed !");
        return OSAL_ErrMutexUnlock;
    }
    return OSAL_ErrNone;
}

/*
* @fn OSAL_RetrieveEvent
* Spurious  wakeups  from  the  pthread_cond_timedwait() or pthread_cond_wait() functions  may  occur.
* A representative sequence for using condition variables is shown below
*
* Thread A (Retrieve Events)				                      |Thread B (Set Events)
*------------------------------------------------------------------------------------------------------------
*1) Do work up to the point where a certain condition must occur              |1)Do work
*   (such as "count" value)
2) Lock associated mutex and check value of a global variable                 |2)Lock associated mutex
*3) Call pthread_cond_wait() to perform a blocking wait                       |3)Change the value of the global variable
*  for signal from Thread-B. Note that a call to                                 that Thread-A is waiting upon.
*  pthread_cond_wait() automatically and atomically
*  unlocks the associated mutex variable so that it can
*  be used by Thread-B.
*4) When signalled, wake up. Mutex is automatically and                       |4)Check value of the global Thread-A wait
*  atomically locked.                                                            variable. If it fulfills the desired
*                                                                                condition, signal Thread-A.
*5) Explicitly unlock mutex                                                   |5) Unlock Mutex.
*6) Continue                                                                  |6) Continue
*/

OSAL_ERROR OSAL_RetrieveEvent(void *pEvents, uint32_t uRequestedEvents,
                                OSAL_EventOp eOperation, uint32_t *pRetrievedEvents, uint32_t uTimeOutMsec)
{
    OSAL_ERROR bRet = OSAL_ErrUnKnown;
    struct timespec timeout;
    struct timeval now;
    uint32_t timeout_us;
    uint32_t isolatedFlags;
    int status = -1;
    int and_operation;
    OSAL_ThreadEvent *plEvent = (OSAL_ThreadEvent *) pEvents;
    if (NULL == plEvent) {
        return OSAL_ErrParameter;
    }

    /* Lock the mutex for access to the eFlags global variable */
    if (SUCCESS != pthread_mutex_lock(&(plEvent->mutex))) {
        OSAL_ErrorTrace("Event Retrieve: Mutex Lock failed !");
        bRet = OSAL_ErrMutexLock;
        goto EXIT;
    }

    /*Check the eOperation and put it in a variable */
    and_operation = (OSAL_EVENT_AND == eOperation || OSAL_EVENT_AND_CONSUME == eOperation);
    /* Isolate the flags. The & operation is suffice for an TIMM_OSAL_EVENT_OR eOperation */
    isolatedFlags = plEvent->eFlags & uRequestedEvents;
    /*Check if it is the AND operation. If yes then, all the flags must match */
    if (and_operation) {
        isolatedFlags = (isolatedFlags == uRequestedEvents);
    }
    if (isolatedFlags) {
        /*We have got required combination of the eFlags bits and will return it back */
        *pRetrievedEvents = plEvent->eFlags;
        bRet = OSAL_ErrNone;
    } else {
        /*Required combination of bits is not yet available */
        if (OSAL_NO_SUSPEND == uTimeOutMsec) {
            *pRetrievedEvents = 0;
            bRet = OSAL_ErrNone;
        } else if (OSAL_SUSPEND == uTimeOutMsec) {
            /*Wait till we get the required combination of bits. We we get the required
            *bits then we go out of the while loop
            */
            while (!isolatedFlags) {
                /*Wait on the conditional variable for another thread to set the eFlags and signal */
                pthread_cond_wait(&(plEvent->condition), &(plEvent->mutex));
                /* eFlags set by some thread. Now, isolate the flags.
                * The & operation is suffice for an TIMM_OSAL_EVENT_OR eOperation
                */
                isolatedFlags = plEvent->eFlags & uRequestedEvents;
                /*Check if it is the AND operation. If yes then, all the flags must match */
                if (and_operation) {
                    isolatedFlags = (isolatedFlags == uRequestedEvents);
                }
            }

            /* Obtained the requested combination of bits on eFlags */
            *pRetrievedEvents = plEvent->eFlags;
            bRet = OSAL_ErrNone;
            } else {
            /* Calculate uTimeOutMsec in terms of the absolute time. uTimeOutMsec is in milliseconds */
            gettimeofday(&now, NULL);
            timeout_us = now.tv_usec + 1000 * uTimeOutMsec;
            timeout.tv_sec = now.tv_sec + timeout_us / 1000000;
            timeout.tv_nsec = (timeout_us % 1000000) * 1000;

            while (!isolatedFlags) {
                /* Wait till uTimeOutMsec for a thread to signal on the conditional variable */
                status = pthread_cond_timedwait(&(plEvent->condition), &(plEvent->mutex), &timeout);
                /*Timedout or error and returned without being signalled */
                if (SUCCESS != status) {
                    if (ETIMEDOUT == status)
                    bRet = OSAL_ErrNone;
                    *pRetrievedEvents = 0;
                    break;
                }

                /* eFlags set by some thread. Now, isolate the flags.
                * The & operation is suffice for an TIMM_OSAL_EVENT_OR eOperation
                */
                isolatedFlags = plEvent->eFlags & uRequestedEvents;

                /*Check if it is the AND operation. If yes then, all the flags must match */
                if (and_operation) {
                    isolatedFlags = (isolatedFlags == uRequestedEvents);
                    }
            }
        }
    }

    /*If we have got the required combination of bits, we will have to reset the eFlags if CONSUME is mentioned
    *in the eOperations
    */
    if (isolatedFlags && ((eOperation == OSAL_EVENT_AND_CONSUME) ||
                        (eOperation == OSAL_EVENT_OR_CONSUME))) {
        plEvent->eFlags = 0;
        bRet = OSAL_ErrNone;
    }

    /*unlock the mutex */
    if (SUCCESS != pthread_mutex_unlock(&(plEvent->mutex))) {
        OSAL_ErrorTrace("Event Retrieve: Mutex Unlock failed !");
        bRet = OSAL_ErrNone;
    }

EXIT:
    return bRet;
}

