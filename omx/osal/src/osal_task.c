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
#include <pthread.h>        /*for POSIX calls */
#include <sched.h>          /*for sched structure */
#include <unistd.h>

#include "osal_trace.h"
#include "osal_error.h"
#include "osal_memory.h"
#include "osal_task.h"

/**
* osal_task structure definition
*/
typedef struct osal_task
{
    pthread_t threadID;	   /*SHM check */
    pthread_attr_t ThreadAttr; /* To set the priority and stack size */
    /*parameters to the task */
    uint32_t uArgc;
    void *pArgv;
    uint8_t isCreated;  /** flag to check if task got created */
} OSAL_Task;

/*
* Task creation
*/
OSAL_ERROR OSAL_CreateTask(void **pTask, OSAL_TaskProc pFunc, uint32_t uArgc,
                            void *pArgv, uint32_t uStackSize, uint32_t uPriority, int8_t *pName)
{
    OSAL_ERROR bRet = OSAL_ErrThreadCreate;
    OSAL_Task *pHandle = NULL;
    struct sched_param sched;
    size_t stackSize;
    *pTask = NULL;

    /*Task structure allocation */
    pHandle = (OSAL_Task*)OSAL_Malloc(sizeof(OSAL_Task));
    if (NULL == pHandle) {
        bRet = OSAL_ErrAlloc;
        goto EXIT;
    }

    /* Initial cleaning of the task structure */
    OSAL_Memset((void*)pHandle, 0, sizeof(OSAL_Task));

    /*Arguments for task */
    pHandle->uArgc = uArgc;
    pHandle->pArgv = pArgv;
    pHandle->isCreated = OSAL_FALSE;

    if (SUCCESS != pthread_attr_init(&pHandle->ThreadAttr)) {
        OSAL_ErrorTrace("Task Init Attr Init failed!");
        goto EXIT;
    }

    /* Updation of the priority and the stack size */
    if (SUCCESS != pthread_attr_getschedparam(&pHandle->ThreadAttr, &sched)) {
        OSAL_ErrorTrace("Task Init Get Sched Params failed!");
        goto EXIT;
    }

    sched.sched_priority = uPriority;	/* relative to the default priority */
    if (SUCCESS != pthread_attr_setschedparam(&pHandle->ThreadAttr, &sched)) {
        OSAL_ErrorTrace("Task Init Set Sched Paramsfailed!");
        goto EXIT;
    }

    /*First get the default stack size */
    if (SUCCESS != pthread_attr_getstacksize(&pHandle->ThreadAttr, &stackSize)) {
        OSAL_ErrorTrace("Task Init Set Stack Size failed!");
        goto EXIT;
    }

    /*Check if requested stack size is larger than the current default stack size */
    if (uStackSize > stackSize) {
        stackSize = uStackSize;
        if (SUCCESS != pthread_attr_setstacksize(&pHandle->ThreadAttr, stackSize)) {
            OSAL_ErrorTrace("Task Init Set Stack Size failed!");
            goto EXIT;
        }
    }

    if (SUCCESS != pthread_create(&pHandle->threadID, &pHandle->ThreadAttr, pFunc, pArgv)) {
        OSAL_ErrorTrace("Create_Task failed !");
        goto EXIT;
    }

    /* Task was successfully created */
    pHandle->isCreated = OSAL_TRUE;
    *pTask = (void*) pHandle;
    bRet = OSAL_ErrNone;

EXIT:
    if (OSAL_ErrNone != bRet) {
        OSAL_Free(pHandle);
    }
    return bRet;
}


OSAL_ERROR OSAL_DeleteTask(void *pTask)
{
    OSAL_Task *pHandle = (OSAL_Task*)pTask;
    void *retVal;

    if (NULL == pHandle || OSAL_TRUE != pHandle->isCreated) {
        /* this task was never created */
        return OSAL_ErrParameter;
    }
    if (pthread_attr_destroy(&pHandle->ThreadAttr)) {
        OSAL_ErrorTrace("Delete_Task failed !");
        return OSAL_ErrThreadDestroy;
    }
    if (pthread_join(pHandle->threadID, &retVal)) {
        OSAL_ErrorTrace("Delete_Task failed !");
        return OSAL_ErrThreadDestroy;
    }

    OSAL_Free(pHandle);
    return OSAL_ErrNone;
}


OSAL_ERROR OSAL_SleepTask(uint32_t mSec)
{
    usleep(1000 * mSec);
    return OSAL_ErrNone;
}
