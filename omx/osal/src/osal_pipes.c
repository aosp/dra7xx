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

#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "osal_error.h"
#include "osal_memory.h"
#include "osal_trace.h"

/**
* structure definition for the OSAL pipe
*/
typedef struct timm_osal_pipe
{
    int pfd[2];
    uint32_t pipeSize;
    uint32_t messageSize;
    uint8_t isFixedMessage;
    int messageCount;
    int totalBytesInPipe;
    pthread_mutex_t mutex;
} OSAL_Pipe;

typedef enum
{
    PIPE_RESET,
    PIPE_INCREMENT,
    PIPE_DECREMENT,
}PIPE_UPDATE;

static inline int UpdatePipe(void *pPipe, uint32_t msgCnt, uint32_t size, PIPE_UPDATE updateMode)
{
    OSAL_Pipe *pHandle = (OSAL_Pipe*)pPipe;
    if (SUCCESS != pthread_mutex_lock(&(pHandle->mutex))) {
        OSAL_ErrorTrace("update Pipe: Mutex Lock failed !");
        return OSAL_ErrMutexLock;
    }
    switch(updateMode) {
        case PIPE_RESET:
            pHandle->messageCount = msgCnt;
            pHandle->totalBytesInPipe = size;
            break;

        case PIPE_INCREMENT:
            pHandle->messageCount += msgCnt;
            pHandle->totalBytesInPipe += size;
            break;

        case PIPE_DECREMENT:
            pHandle->messageCount -= msgCnt;
            pHandle->totalBytesInPipe -= size;
            break;

        default:
            OSAL_ErrorTrace("InValid Pipe update Mode");
    }

    if (SUCCESS != pthread_mutex_unlock(&(pHandle->mutex))) {
        OSAL_ErrorTrace("update Pipe: Mutex Unlock failed !");
        return OSAL_ErrMutexUnlock;
    }

    return OSAL_ErrNone;
}

OSAL_ERROR OSAL_CreatePipe(void **pPipe, uint32_t pipeSize,
                            uint32_t messageSize, uint8_t isFixedMessage)
{
    OSAL_Pipe *pHandle = (OSAL_Pipe*)OSAL_Malloc(sizeof(OSAL_Pipe));
    if (NULL == pHandle) {
        return OSAL_ErrAlloc;
    }

    OSAL_Memset(pHandle, 0x0, sizeof(OSAL_Pipe));
    pHandle->pfd[0] = -1;
    pHandle->pfd[1] = -1;
    if (SUCCESS != pipe(pHandle->pfd)) {
        OSAL_ErrorTrace("Pipe failed: %s!!!", strerror(errno));
        OSAL_Free(pHandle);
        return OSAL_ErrAlloc;
    }

    if (SUCCESS != pthread_mutex_init(&(pHandle->mutex), NULL)) {
        OSAL_ErrorTrace("Pipe Create:Mutex Init failed !");
        OSAL_Free(pHandle);
        return OSAL_ErrMutexCreate;
    }
    pHandle->pipeSize = pipeSize;
    pHandle->messageSize = messageSize;
    pHandle->isFixedMessage = isFixedMessage;
    pHandle->messageCount = 0;
    pHandle->totalBytesInPipe = 0;

    *pPipe = (void*)pHandle;
    return OSAL_ErrNone;
}

OSAL_ERROR OSAL_DeletePipe(void *pPipe)
{
    OSAL_Pipe *pHandle = (OSAL_Pipe*) pPipe;
    if (NULL == pHandle) {
        return OSAL_ErrParameter;
    }

    if (SUCCESS != close(pHandle->pfd[0])) {
        OSAL_ErrorTrace("Delete_Pipe Read fd failed!!!");
        return OSAL_ErrPipeClose;
    }
    if (SUCCESS != close(pHandle->pfd[1])) {
        OSAL_ErrorTrace("Delete_Pipe Write fd failed!!!");
        return OSAL_ErrPipeClose;
    }

    if (SUCCESS != pthread_mutex_destroy(&(pHandle->mutex))) {
        OSAL_ErrorTrace("Pipe Delete: Mutex Destory failed !");
        return OSAL_ErrMutexDestroy;
    }

    OSAL_Free(pHandle);
    return OSAL_ErrNone;
}


OSAL_ERROR OSAL_WriteToPipe(void *pPipe, void *pMessage, uint32_t size, int32_t timeout)
{
    uint32_t lSizeWritten = -1;
    OSAL_ERROR ret = OSAL_ErrNone;
    OSAL_Pipe *pHandle = (OSAL_Pipe*)pPipe;
    if (NULL == pHandle || size == 0) {
        OSAL_ErrorTrace("0 size!!!");
        return OSAL_ErrParameter;
    }

    /*Update message count and size */
    ret = UpdatePipe(pHandle, 1, size, PIPE_INCREMENT);
    if (ret) {
        return ret;
    }

    lSizeWritten = write(pHandle->pfd[1], pMessage, size);
    if (lSizeWritten != size) {
        OSAL_ErrorTrace("Write of pipe failed!!!");

        /*Update message count and size */
        UpdatePipe(pHandle, 1, size, PIPE_DECREMENT);
        return OSAL_ErrPipeWrite;
    }

    return OSAL_ErrNone;
}


OSAL_ERROR OSAL_WriteToFrontOfPipe(void *pPipe, void *pMessage, uint32_t size, int32_t timeout)
{
    uint32_t lSizeWritten = -1;
    uint32_t lSizeRead = -1;
    OSAL_ERROR ret = OSAL_ErrNone;
    OSAL_Pipe *pHandle = (OSAL_Pipe*) pPipe;
    uint8_t *tempPtr = NULL;

    /*First write to this pipe */
    if (NULL == pHandle || size == 0) {
        return OSAL_ErrParameter;
    }

    /*Update message count and size */
    ret = UpdatePipe(pHandle, 1, size, PIPE_INCREMENT);
    if (ret) {
        return ret;
    }

    lSizeWritten = write(pHandle->pfd[1], pMessage, size);
    if (lSizeWritten != size) {
        /*Update message count and size */
        UpdatePipe(pHandle, 1, size, PIPE_DECREMENT);
        return OSAL_ErrPipeWrite;
    }

    if (pHandle->messageCount > 1) {
        /*First allocate memory */
        tempPtr = (uint8_t*)OSAL_Malloc(pHandle->totalBytesInPipe-size);
        if (NULL == tempPtr) {
            return OSAL_ErrAlloc;
        }

        /*Read out of pipe */
        lSizeRead = read(pHandle->pfd[0], tempPtr, pHandle->totalBytesInPipe-size);

        /*Write back to pipe */
        lSizeWritten = write(pHandle->pfd[1], tempPtr, pHandle->totalBytesInPipe-size);
        if (lSizeWritten != lSizeRead) {
            OSAL_Free(tempPtr);
            /*Update message count and size */
            UpdatePipe(pHandle, 1, size, PIPE_RESET);
            return OSAL_ErrPipeWrite;
        }
        OSAL_Free(tempPtr);
    }

    return OSAL_ErrNone;
}


OSAL_ERROR OSAL_ReadFromPipe(void *pPipe, void *pMessage, uint32_t size,
                            uint32_t *actualSize, int32_t timeout)
{
    uint32_t lSizeRead = -1;
    OSAL_ERROR ret = OSAL_ErrNone;
    OSAL_Pipe *pHandle = (OSAL_Pipe*) pPipe;

    if (NULL == pHandle || size == 0) {
        OSAL_ErrorTrace("nRead size has error!!!");
        return OSAL_ErrParameter;
    }

    if (pHandle->messageCount == 0 && timeout == OSAL_NO_SUSPEND) {
        /*If timeout is 0 and pipe is empty, return error */
        OSAL_ErrorTrace("Pipe is empty!!!");
        return OSAL_ErrPipeEmpty;
    }
    if (timeout != OSAL_NO_SUSPEND && timeout != (int32_t)OSAL_SUSPEND) {
        OSAL_WarningTrace("Only infinite or no timeouts \
        supported. Going to read with infinite timeout now");
    }

    /*read blocks infinitely until message is available */
    *actualSize = lSizeRead = read(pHandle->pfd[0], pMessage, size);
    if (0 == lSizeRead) {
        OSAL_ErrorTrace("EOF reached or no data in pipe!!!");
        return OSAL_ErrPipeRead;
    }

    ret = UpdatePipe(pHandle, 1, lSizeRead, PIPE_DECREMENT);
    return ret;

}


OSAL_ERROR OSAL_ClearPipe(void *pPipe)
{
    OSAL_WarningTrace("This function is currently not implemented");
    return OSAL_ErrNone;
}

OSAL_ERROR OSAL_IsPipeReady(void *pPipe)
{
    OSAL_Pipe *pHandle = (OSAL_Pipe *) pPipe;
    int isReady;

    if (NULL == pHandle) {
        return OSAL_ErrParameter;
    }

    if (SUCCESS != pthread_mutex_lock(&(pHandle->mutex))) {
        OSAL_ErrorTrace("IsPipeReady: Mutex Lock failed !");
        return OSAL_ErrMutexLock;
    }

    isReady = (pHandle->messageCount <= 0) ? OSAL_ErrNotReady : OSAL_ErrNone;

    if (SUCCESS != pthread_mutex_unlock(&(pHandle->mutex))) {
        OSAL_ErrorTrace("IsPipeReady: Mutex Unlock failed !");
        return OSAL_ErrMutexUnlock;
    }
    return isReady;
}

OSAL_ERROR OSAL_GetPipeReadyMessageCount(void *pPipe, uint32_t *count)
{
    OSAL_Pipe *pHandle = (OSAL_Pipe*) pPipe;
    if (NULL == pHandle) {
        *count = 0;
        return OSAL_ErrParameter;
    }

    if (SUCCESS != pthread_mutex_lock(&(pHandle->mutex))) {
        OSAL_ErrorTrace("GetPipeReadyMessageCount: Mutex Lock failed !");
        return OSAL_ErrMutexLock;
    }


    *count = pHandle->messageCount;

    if (SUCCESS != pthread_mutex_unlock(&(pHandle->mutex))) {
        OSAL_ErrorTrace("GetPipeReadyMessageCount: Mutex Unlock failed !");
        return OSAL_ErrMutexUnlock;
    }

    return OSAL_ErrNone;
}
