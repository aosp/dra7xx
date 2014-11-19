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

#define LOG_TAG "OMX_BASE_INTERNAL"

#include <string.h>

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <omx_base.h>
#include <omx_base_dio_plugin.h>
#include <osal_semaphores.h>

#define OMXBase_TASKDEL_TRIES 1000
#define OMXBase_TASKDEL_SLEEP 2

#define OMX_BASE_HANDLE_IDLE_TO_LOADED_TRANSITION() do { \
    for( i=0; i < pBaseComp->nNumPorts; i++ ) \
    {   \
        pPort = pBaseComp->pPorts[i]; \
        if( pPort->pBufAllocFreeEvent ) \
        { \
            tStatus = OSAL_SetEvent(pPort->pBufAllocFreeEvent, \
                                    BUF_FAIL_EVENT, OSAL_EVENT_OR); \
            if( tStatus != OSAL_ErrNone ) { \
                eError = OMX_ErrorUndefined; } \
            } \
        } \
    retEvents = 0; \
    tStatus = OSAL_RetrieveEvent( \
                pBaseCompPvt->pErrorCmdcompleteEvent, \
                ERROR_EVENT, OSAL_EVENT_OR_CONSUME, &retEvents, \
                STATE_TRANSITION_LONG_TIMEOUT); \
    OMX_CHECK(tStatus == OSAL_ErrNone, \
                OMX_ErrorUndefined); \
} while( 0 )


/*
* OMX Base Private Init
*/
OMX_ERRORTYPE OMXBase_PrivateInit(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone, eTmpError = OMX_ErrorNone;
    OSAL_ERROR          tStatus = OSAL_ErrNone;
    OMX_COMPONENTTYPE   *pComp = NULL;
    OMXBaseComp         *pBaseComp = NULL;

    OMX_CHECK(hComponent != NULL, OMX_ErrorBadParameter);

    pComp = (OMX_COMPONENTTYPE *)hComponent;
    pBaseComp = (OMXBaseComp*)pComp->pComponentPrivate;
    OMX_CHECK(pBaseComp != NULL, OMX_ErrorBadParameter);
    OMXBaseComp_Pvt *pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;
    /*Create the new state mutex*/
    tStatus = OSAL_CreateMutex(&(pBaseCompPvt->pNewStateMutex));
    OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorInsufficientResources);

    /* create a fixed size command pipe to queueup commands */
    tStatus = OSAL_CreatePipe(&(pBaseCompPvt->pCmdPipe),
                        OMXBase_MAXCMDS * sizeof(OMXBase_CmdParams),
                        sizeof(OMXBase_CmdParams), 1);
    OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorInsufficientResources);


    /* create a fixed size pipe to queueup command data pointers */
    tStatus = OSAL_CreatePipe(&(pBaseCompPvt->pCmdDataPipe),
                        OMXBase_MAXCMDS * sizeof(OMX_PTR),
                        sizeof(OMX_PTR), 1);
    OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorInsufficientResources);

    /*create an Event for Command completion to be set by Dervived Component  */
    tStatus = OSAL_CreateEvent(&(pBaseCompPvt->pCmdCompleteEvent));
    OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorInsufficientResources);

    /*create an Event for state transition notifications for  complete tear down of compoenent */
    tStatus = OSAL_CreateEvent(&(pBaseCompPvt->pErrorCmdcompleteEvent));
    OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorInsufficientResources);

    /*Create mutex for cmd pipe*/
    tStatus = OSAL_CreateMutex(&(pBaseCompPvt->pCmdPipeMutex));
    OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorInsufficientResources);

    pBaseCompPvt->fpInvokeProcessFunction = OMXBase_ProcessEvents;

    tStatus = OSAL_CreateEvent(&(pBaseCompPvt->pTriggerEvent));
    OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorInsufficientResources);

    OSAL_Memcpy(pBaseCompPvt->cTaskName, pBaseComp->cComponentName, strlen(pBaseComp->cComponentName));
    pBaseCompPvt->nStackSize = OMX_BASE_THREAD_STACKSIZE;
    pBaseCompPvt->nPrioirty = OMX_BASE_THREAD_PRIORITY;

    tStatus = OSAL_CreateTask(&pBaseCompPvt->pThreadId,
                            (OSAL_TaskProc)OMXBase_CompThreadEntry, 0,
                            (void *)hComponent,
                            pBaseCompPvt->nStackSize,
                            pBaseCompPvt->nPrioirty,
                            (OMX_S8 *)pBaseCompPvt->cTaskName);
    OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorInsufficientResources);

    pBaseCompPvt->fpInvokeProcessFunction = OMXBase_ProcessTriggerEvent;

    /* Set hooks from Derived to Base communicattion */
    pBaseCompPvt->fpDioGetCount          =  OMXBase_DIO_GetCount;
    pBaseCompPvt->fpDioDequeue           =  OMXBase_DIO_Dequeue;
    pBaseCompPvt->fpDioSend              =  OMXBase_DIO_Send;
    pBaseCompPvt->fpDioCancel            =  OMXBase_DIO_Cancel;
    pBaseCompPvt->fpDioControl           =  OMXBase_DIO_Control;

EXIT:
    return eError;
}

/*
* OMX Base Private DeInit
*/

OMX_ERRORTYPE OMXBase_PrivateDeInit(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE       eError  = OMX_ErrorNone, eTmpError = OMX_ErrorNone;
    OSAL_ERROR          tStatus = OSAL_ErrNone;
    OMX_COMPONENTTYPE   *pComp = NULL;
    OMXBaseComp         *pBaseComp = NULL;
    OMXBaseComp_Pvt     *pBaseCompPvt = NULL;
    OMX_U32             i = 0, nTries = 0;
    uint32_t            nCount = 0;
    OMX_PTR             pData = NULL;
    uint32_t            nActualSize = 0;

    OMX_CHECK(hComponent != NULL, OMX_ErrorBadParameter);
    pComp = (OMX_COMPONENTTYPE *)hComponent;
    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    OMX_CHECK(pBaseComp != NULL, OMX_ErrorBadParameter);
    pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;

    OSAL_ObtainMutex(pBaseCompPvt->pNewStateMutex, OSAL_SUSPEND);

    /* set an ENDEVENT before destroying thread  */
    pBaseCompPvt->fpInvokeProcessFunction(hComponent, ENDEVENT);
    tStatus = OSAL_DeleteTask(pBaseCompPvt->pThreadId);

    while( tStatus == OSAL_ErrNotReady &&
            nTries < OMXBase_TASKDEL_TRIES ) {
        //Wait for some time and try again
        OSAL_SleepTask(OMXBase_TASKDEL_SLEEP);
        nTries++;
        tStatus = OSAL_DeleteTask(pBaseCompPvt->pThreadId);
    }

    if( tStatus != OSAL_ErrNone ) {
        eError = OMX_ErrorTimeout;
        OSAL_ErrorTrace("Error while deleting task");
    }
    if( pBaseCompPvt->pTriggerEvent ) {
        tStatus = OSAL_DeleteEvent(pBaseCompPvt->pTriggerEvent);
        if( tStatus != OSAL_ErrNone ) {
            eError = OMX_ErrorUndefined;
        }
        pBaseCompPvt->pTriggerEvent = NULL;
    }

    if( pBaseCompPvt->pCmdCompleteEvent ) {
        tStatus = OSAL_DeleteEvent(pBaseCompPvt->pCmdCompleteEvent);
        if( tStatus != OSAL_ErrNone ) {
            eError = OMX_ErrorUndefined;
        }
        pBaseCompPvt->pCmdCompleteEvent = NULL;
    }
    if( pBaseCompPvt->pErrorCmdcompleteEvent ) {
        tStatus = OSAL_DeleteEvent(pBaseCompPvt->pErrorCmdcompleteEvent);
        if( tStatus != OSAL_ErrNone ) {
            eError = OMX_ErrorUndefined;
        }
        pBaseCompPvt->pErrorCmdcompleteEvent = NULL;
    }
    if( pBaseCompPvt->pCmdPipe ) {
        tStatus = OSAL_DeletePipe(pBaseCompPvt->pCmdPipe);
        if( tStatus != OSAL_ErrNone ) {
            eError = OMX_ErrorUndefined;
        }
        pBaseCompPvt->pCmdPipe = NULL;
    }
    if( pBaseCompPvt->pCmdDataPipe ) {
        /*If pipe still has some data then empty the data and free the memory*/
        tStatus = OSAL_GetPipeReadyMessageCount(pBaseCompPvt->pCmdDataPipe,
                                                        &nCount);
        if( tStatus != OSAL_ErrNone ) {
            eError = OMX_ErrorUndefined;
        } else {
            while( nCount > 0 ) {
                tStatus = OSAL_ReadFromPipe(pBaseCompPvt->pCmdDataPipe,
                                        pData, sizeof(OMX_PTR), &nActualSize, OSAL_NO_SUSPEND);
                if( tStatus != OSAL_ErrNone ) {
                    eError = OMX_ErrorUndefined;
                    break;
                }
                OSAL_Free(pData);
                nCount--;
            }
        }
        tStatus = OSAL_DeletePipe(pBaseCompPvt->pCmdDataPipe);
        if( tStatus != OSAL_ErrNone ) {
            eError = OMX_ErrorUndefined;
        }
        pBaseCompPvt->pCmdDataPipe = NULL;
    }
    if( pBaseCompPvt->pCmdPipeMutex ) {
        tStatus = OSAL_DeleteMutex(pBaseCompPvt->pCmdPipeMutex);
        if( tStatus != OSAL_ErrNone ) {
            eError = OMX_ErrorUndefined;
        }
        pBaseCompPvt->pCmdPipeMutex = NULL;
    }

    OSAL_ReleaseMutex(pBaseCompPvt->pNewStateMutex);

    if( pBaseCompPvt->pNewStateMutex ) {
        tStatus = OSAL_DeleteMutex(pBaseCompPvt->pNewStateMutex);
        if( tStatus != OSAL_ErrNone ) {
            eError = OMX_ErrorUndefined;
        }
        pBaseCompPvt->pNewStateMutex = NULL;
    }

EXIT:
    return (eError);
}

/*
* OMX Base InitializePorts
*/
OMX_ERRORTYPE OMXBase_InitializePorts(OMX_HANDLETYPE hComponent)
{
    OMX_COMPONENTTYPE   *pComp = NULL;
    OMXBaseComp         *pBaseComp = NULL;
    OMX_ERRORTYPE       eError = OMX_ErrorNone, eTmpError = OMX_ErrorNone;
    OSAL_ERROR          tStatus = OSAL_ErrNone;
    OMX_U32             i = 0;

    OMX_CHECK(hComponent != NULL, OMX_ErrorBadParameter);

    pComp = (OMX_COMPONENTTYPE *)hComponent;
    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;

    pBaseComp->pPorts = (OMXBase_Port **)OSAL_Malloc(sizeof(OMXBase_Port *) *
                                                    pBaseComp->nNumPorts);
    OMX_CHECK(pBaseComp->pPorts != NULL, OMX_ErrorInsufficientResources);
    OSAL_Memset(pBaseComp->pPorts, 0, sizeof(OMXBase_Port *) *
                                    pBaseComp->nNumPorts);

    for( i = 0; i < pBaseComp->nNumPorts; i++ ) {
        pBaseComp->pPorts[i] = (OMXBase_Port *)OSAL_Malloc(sizeof(OMXBase_Port));
        OMX_CHECK(pBaseComp->pPorts[i] != NULL, OMX_ErrorInsufficientResources);
        OSAL_Memset(pBaseComp->pPorts[i], 0x0, sizeof(OMXBase_Port));

        OMX_BASE_INIT_STRUCT_PTR(&(pBaseComp->pPorts[i]->sPortDef),
                                OMX_PARAM_PORTDEFINITIONTYPE);
        pBaseComp->pPorts[i]->sPortDef.nPortIndex = i;

        tStatus = OSAL_CreateEvent(&(pBaseComp->pPorts[i]->pBufAllocFreeEvent));
        OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorInsufficientResources);
        tStatus = OSAL_CreateSemaphore(&(pBaseComp->pPorts[i]->pDioOpenCloseSem), 0);
        OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorInsufficientResources);
    }

EXIT:
    if( OMX_ErrorNone != eError ) {
        eTmpError = eError;
        eError = OMXBase_DeinitializePorts(hComponent);
        eError = eTmpError;
    }
    return (eError);
}

/*
* OMX Base DeInitialize Ports
*/
OMX_ERRORTYPE OMXBase_DeinitializePorts(OMX_HANDLETYPE hComponent)
{
    OMX_COMPONENTTYPE   *pComp = NULL;
    OMXBaseComp         *pBaseComp = NULL;
    OMXBase_Port        *pPort = NULL;
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OSAL_ERROR          tStatus = OSAL_ErrNone;
    OMX_U32             i = 0;

    OMX_CHECK(hComponent != NULL, OMX_ErrorBadParameter);

    pComp = (OMX_COMPONENTTYPE *)hComponent;
    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    OMX_CHECK(pBaseComp != NULL, eError);

    for( i=0; i < pBaseComp->nNumPorts; i++ ) {
        if( !(pBaseComp->pPorts)) {
            break;
        }
        pPort = pBaseComp->pPorts[i];
        if( pPort == NULL ) {
            continue;
        }
        if( pPort->pDioOpenCloseSem ) {
            tStatus = OSAL_DeleteSemaphore(pPort->pDioOpenCloseSem);
            if( tStatus != OSAL_ErrNone ) {
                eError = OMX_ErrorUndefined;
            }
        }
        /*If any tasks are waiting on this event then send fail event to
        indicate that component is being unloaded*/
        if( pPort->pBufAllocFreeEvent ) {
            tStatus = OSAL_SetEvent(pPort->pBufAllocFreeEvent,
                                    BUF_FAIL_EVENT, OSAL_EVENT_OR);
            if( tStatus != OSAL_ErrNone ) {
                eError = OMX_ErrorUndefined;
            }
            tStatus = OSAL_DeleteEvent(pPort->pBufAllocFreeEvent);
            if( tStatus != OSAL_ErrNone ) {
                eError = OMX_ErrorUndefined;
            }
        }
        OSAL_Free(pPort);
        pPort = NULL;
    }

    if( pBaseComp->pPorts ) {
        OSAL_Free(pBaseComp->pPorts);
        pBaseComp->pPorts = NULL;
    }

EXIT:
    return (eError);
}


/*
* OMX Base SetDefault Properties
*/
OMX_ERRORTYPE OMXBase_SetDefaultProperties(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE   *pComp = (OMX_COMPONENTTYPE *)hComponent;
    OMXBaseComp         *pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    OMX_U32             nIndx = 0;

    for( nIndx = 0; nIndx < (pBaseComp->nNumPorts); nIndx++ ) {
        pBaseComp->pPorts[nIndx]->sProps.nWatermark = 1;
        /*Frame mode is the default mode*/
        pBaseComp->pPorts[nIndx]->sProps.eDataAccessMode = MemAccess_8Bit;

        /*Buffer allocation type is set to default*/
        pBaseComp->pPorts[nIndx]->sProps.eBufMemoryType = MEM_CARVEOUT;
        /*Number of component buffers set to 1*/
        pBaseComp->pPorts[nIndx]->sProps.nNumComponentBuffers = 1;
        /*No bufefr params by default. To be used in case of 2D buffers*/
        // pBaseComp->pPortProperties[nIndx]->pBufParams = NULL;
        pBaseComp->pPorts[nIndx]->sProps.nTimeoutForDequeue = OSAL_SUSPEND;
    }

    pBaseComp->bNotifyForAnyPort = OMX_TRUE;

    return (eError);
}


/*
* OMX Base ThreadEntry
*/
void OMXBase_CompThreadEntry(void *arg)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OSAL_ERROR          tStatus = OSAL_ErrNone;
    OMXBaseComp         *pBaseComp = NULL;
    OMXBaseComp_Pvt     *pBaseCompPvt = NULL;
    OMX_COMPONENTTYPE   *pComp = (OMX_COMPONENTTYPE *)arg;
    uint32_t            retrievedEvents = 0;

    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;

    while( 1 ) {
        /*  wait for Any of the event/s to process  */
        tStatus = OSAL_RetrieveEvent(pBaseCompPvt->pTriggerEvent,
                            (CMDEVENT | DATAEVENT | ENDEVENT),
                            OSAL_EVENT_OR_CONSUME,
                            &retrievedEvents, OSAL_SUSPEND);
        OMX_CHECK(tStatus == OSAL_ErrNone, OMX_ErrorInsufficientResources);
        /* terminate the process when it acquires an ENDEVENT */
        if( retrievedEvents & ENDEVENT ) {
            break;
        }
        /* Process Event that has retrieved */
        if( retrievedEvents & CMDEVENT ) {
            while( OSAL_IsPipeReady(pBaseCompPvt->pCmdPipe) ==
                    OSAL_ErrNone ) {
                eError = OMXBase_ProcessEvents(pComp, retrievedEvents);
                /*Callback for error will be sent in the above function*/
                eError = OMX_ErrorNone;
            }
            retrievedEvents &= ~CMDEVENT;
        }
        if( retrievedEvents & DATAEVENT ) {
            eError = OMXBase_ProcessEvents(pComp, retrievedEvents);
            /*Callback for error will be sent in the above function*/
            eError = OMX_ErrorNone;
        }
    }

EXIT:
    if( OMX_ErrorNone != eError ) {
        pBaseCompPvt->sAppCallbacks.EventHandler((OMX_HANDLETYPE)pComp,
                                    pComp->pApplicationPrivate,
                                    OMX_EventError, eError,
                                    0, NULL);
    }
}


/*
* OMX Base Disable Port
*/
OMX_ERRORTYPE OMXBase_DisablePort(OMX_HANDLETYPE hComponent,
                                    OMX_U32 nParam)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE   *pComp = (OMX_COMPONENTTYPE *)hComponent;
    OMXBaseComp         *pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    OMXBase_Port        *pPort = NULL;
    OMX_U32             nStartPortNum;

    nStartPortNum = pBaseComp->nMinStartPortIndex;

    pPort = pBaseComp->pPorts[nParam - nStartPortNum];
    /* If comp is in loaded state, then there wont be any buffers to free up */
    if((pBaseComp->tCurState == OMX_StateLoaded)
            || (pBaseComp->tCurState == OMX_StateWaitForResources)) {
        goto EXIT;
    }
    eError = OMXBase_DIO_Control(hComponent, nParam,
                                OMX_DIO_CtrlCmd_Stop, NULL);
    OMX_CHECK(OMX_ErrorNone == eError, eError);

EXIT:
    return (eError);

}

/*
* OMX Base EnablePort
*/
OMX_ERRORTYPE OMXBase_EnablePort(OMX_HANDLETYPE hComponent,
                                   OMX_U32 nParam)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE   *pComp = (OMX_COMPONENTTYPE *)hComponent;
    OMXBaseComp         *pBaseComp = (OMXBaseComp *) pComp->pComponentPrivate;
    OMXBase_Port        *pPort = NULL;
    OMX_U32             nStartPortNum = 0;
    OMX_DIO_OpenParams  sDIOParams;

    nStartPortNum = pBaseComp->nMinStartPortIndex;

    pPort = pBaseComp->pPorts[nParam - nStartPortNum];
    if( pBaseComp->tCurState != OMX_StateLoaded &&
                pBaseComp->tCurState != OMX_StateWaitForResources ) {
        if( pPort->sPortDef.eDir == OMX_DirOutput ) {
            sDIOParams.nMode = OMX_DIO_WRITER;
        } else {
            sDIOParams.nMode = OMX_DIO_READER;
        }
    }

EXIT:
    return (eError);
}

/*
* OMX Base Flush Buffers
*/
OMX_ERRORTYPE OMXBase_FlushBuffers(OMX_HANDLETYPE hComponent,
                                     OMX_U32 nParam)
{
    return(OMXBase_DIO_Control(hComponent, nParam, OMX_DIO_CtrlCmd_Flush,
                                NULL));
}


/*
* OMX Base Handle Transition
*/
OMX_ERRORTYPE OMXBase_HandleStateTransition(OMX_HANDLETYPE hComponent,
                                              OMX_U32 nParam)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OSAL_ERROR          tStatus = OSAL_ErrNone;
    OMX_COMPONENTTYPE   *pComp = (OMX_COMPONENTTYPE *)hComponent;
    OMXBaseComp         *pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    OMXBaseComp_Pvt     *pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;
    OMXBase_Port        *pPort = NULL;
    OMX_U32             i = 0, nStartPortNum = 0, nPorts = 0;
    uint32_t            retEvents = 0;
    OMX_DIO_OpenParams  sDIOParams;

    nPorts =  pBaseComp->nNumPorts;
    nStartPortNum = pBaseComp->nMinStartPortIndex;

    /* currnet and new state should not be same */
    OMX_CHECK(pBaseComp->tCurState != pBaseComp->tNewState, OMX_ErrorSameState);
    /* Transition to invaild state by IL client is disallowed */
    if( pBaseComp->tNewState == OMX_StateInvalid ) {
        /* Notify to Derived Component */
        pBaseComp->fpCommandNotify(hComponent, OMX_CommandStateSet, nParam, NULL);
        /*Derived component has returned*/
        tStatus = OSAL_RetrieveEvent(pBaseCompPvt->pCmdCompleteEvent,
                                    OMXBase_CmdStateSet, OSAL_EVENT_OR_CONSUME,
                                    &retEvents, OSAL_SUSPEND);
        OMX_CHECK(tStatus == OSAL_ErrNone, OMX_ErrorInsufficientResources);

        /*
        For this case we wont go through OMXBase_EventNotifyToClient function
        The state will be set here and error callback will be made by
        OMXBase_ProcessEvents function
        */
        pBaseComp->tCurState = pBaseComp->tNewState;
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch( pBaseComp->tCurState ) {
        case OMX_StateLoaded :
            if( pBaseComp->tNewState == OMX_StateIdle ) {
                /* Notify to Derived Component */
                eError = pBaseComp->fpCommandNotify(hComponent,
                OMX_CommandStateSet, nParam, NULL);
                OMX_CHECK(OMX_ErrorNone == eError, eError);
                tStatus = OSAL_RetrieveEvent(
                pBaseCompPvt->pCmdCompleteEvent,
                OMXBase_CmdStateSet, OSAL_EVENT_OR_CONSUME,
                &retEvents, OSAL_SUSPEND);
                OMX_CHECK(tStatus == OSAL_ErrNone,
                OMX_ErrorInsufficientResources);

                for( i = nStartPortNum; i < (nStartPortNum + nPorts); i++ ) {
                    pPort = (OMXBase_Port *)pBaseComp->pPorts[i - nStartPortNum];
                    /*If port is disabled then nothing needs to be done*/
                    if( pPort->sPortDef.bEnabled == OMX_FALSE ) {
                        continue;
                    }
                    if( pPort->sPortDef.eDir == OMX_DirOutput ) {
                        sDIOParams.nMode = OMX_DIO_WRITER;
                    } else {
                        sDIOParams.nMode = OMX_DIO_READER;
                    }
                }
            } else if( pBaseComp->tNewState == OMX_StateWaitForResources ) {
                /* Notify to Derived Component */
                eError = pBaseComp->fpCommandNotify(hComponent,
                                                    OMX_CommandStateSet, nParam, NULL);
                OMX_CHECK(OMX_ErrorNone == eError, eError);
                tStatus = OSAL_RetrieveEvent(pBaseCompPvt->pCmdCompleteEvent, OMXBase_CmdStateSet,
                                            OSAL_EVENT_OR_CONSUME, &retEvents, OSAL_SUSPEND);
                OMX_CHECK(tStatus == OSAL_ErrNone, OMX_ErrorUndefined);
            } else {
                eError = OMX_ErrorIncorrectStateTransition;
                goto EXIT;
            }
        break;

        case OMX_StateIdle :
            if( pBaseComp->tNewState == OMX_StateLoaded ) {
                /* Notify to Derived Component */
                eError = pBaseComp->fpCommandNotify(hComponent,
                                            OMX_CommandStateSet, nParam, NULL);
                OMX_CHECK(OMX_ErrorNone == eError, eError);
                tStatus = OSAL_RetrieveEvent(pBaseCompPvt->pCmdCompleteEvent, OMXBase_CmdStateSet,
                                            OSAL_EVENT_OR_CONSUME, &retEvents, OSAL_SUSPEND);
                OMX_CHECK(tStatus == OSAL_ErrNone, OMX_ErrorUndefined);
            } else if( pBaseComp->tNewState == OMX_StateExecuting ) {
                /* Notify to Derived Component */
                eError = pBaseComp->fpCommandNotify(hComponent,
                                            OMX_CommandStateSet, nParam, NULL);
                OMX_CHECK(OMX_ErrorNone == eError, eError);
                tStatus = OSAL_RetrieveEvent(pBaseCompPvt->pCmdCompleteEvent, OMXBase_CmdStateSet,
                                            OSAL_EVENT_OR_CONSUME, &retEvents, OSAL_SUSPEND);
                OMX_CHECK(tStatus == OSAL_ErrNone, OMX_ErrorUndefined);
            } else if( pBaseComp->tNewState == OMX_StatePause ) {
                /* Notify to Derived Component */
                eError = pBaseComp->fpCommandNotify(hComponent,
                                                OMX_CommandStateSet, nParam, NULL);
                OMX_CHECK(OMX_ErrorNone == eError, eError);
                tStatus = OSAL_RetrieveEvent(pBaseCompPvt->pCmdCompleteEvent, OMXBase_CmdStateSet,
                                            OSAL_EVENT_OR_CONSUME, &retEvents, OSAL_SUSPEND);
                OMX_CHECK(tStatus == OSAL_ErrNone, OMX_ErrorUndefined);
            } else {
                eError = OMX_ErrorIncorrectStateTransition;
                goto EXIT;
            }
        break;

        case OMX_StateExecuting :
            if( pBaseComp->tNewState == OMX_StateIdle ) {
                /* Notify to Derived Component */
                eError = pBaseComp->fpCommandNotify(hComponent,
                                            OMX_CommandStateSet, nParam, NULL);
                OMX_CHECK(OMX_ErrorNone == eError, eError);
                tStatus = OSAL_RetrieveEvent(pBaseCompPvt->pCmdCompleteEvent, OMXBase_CmdStateSet,
                                            OSAL_EVENT_OR_CONSUME, &retEvents, OSAL_SUSPEND);
                OMX_CHECK(tStatus == OSAL_ErrNone, OMX_ErrorUndefined);

                for( i = nStartPortNum; i < (nStartPortNum + nPorts); i++ ) {
                    pPort = (OMXBase_Port *)pBaseComp->pPorts[i - nStartPortNum];
                    if( pPort->hDIO != NULL ) {
                        eError = OMXBase_DIO_Control(hComponent, i,
                                                    OMX_DIO_CtrlCmd_Stop, NULL);
                        OMX_CHECK(OMX_ErrorNone == eError, eError);
                    }
                }
            } else if( pBaseComp->tNewState == OMX_StatePause ) {
                /* Notify to Derived Component */
                eError = pBaseComp->fpCommandNotify(hComponent,
                                            OMX_CommandStateSet, nParam, NULL);
                OMX_CHECK(OMX_ErrorNone == eError, eError);
                tStatus = OSAL_RetrieveEvent(pBaseCompPvt->pCmdCompleteEvent, OMXBase_CmdStateSet,
                                            OSAL_EVENT_OR_CONSUME, &retEvents, OSAL_SUSPEND);
                OMX_CHECK(tStatus == OSAL_ErrNone, OMX_ErrorUndefined);
            } else {
                eError = OMX_ErrorIncorrectStateTransition;
                goto EXIT;
            }
        break;

        case OMX_StatePause :
            if( pBaseComp->tNewState == OMX_StateExecuting ) {
                /* Notify to Derived Component */
                eError = pBaseComp->fpCommandNotify(hComponent,
                                                OMX_CommandStateSet, nParam, NULL);
                OMX_CHECK(OMX_ErrorNone == eError, eError);
                tStatus = OSAL_RetrieveEvent(pBaseCompPvt->pCmdCompleteEvent, OMXBase_CmdStateSet,
                                                OSAL_EVENT_OR_CONSUME, &retEvents, OSAL_SUSPEND);
                OMX_CHECK(tStatus == OSAL_ErrNone, OMX_ErrorUndefined);

                /*Pause to Executing so start processing buffers*/
                pBaseCompPvt->fpInvokeProcessFunction(pComp, DATAEVENT);
            } else if( pBaseComp->tNewState == OMX_StateIdle ) {
                /* Notify to Derived Component */
                eError = pBaseComp->fpCommandNotify(hComponent,
                                        OMX_CommandStateSet, nParam, NULL);
                OMX_CHECK(OMX_ErrorNone == eError, eError);
                tStatus = OSAL_RetrieveEvent(pBaseCompPvt->pCmdCompleteEvent, OMXBase_CmdStateSet,
                                        OSAL_EVENT_OR_CONSUME, &retEvents, OSAL_SUSPEND);
                OMX_CHECK(tStatus == OSAL_ErrNone, OMX_ErrorUndefined);

                for( i = nStartPortNum; i < (nStartPortNum + nPorts); i++ ) {
                    pPort = (OMXBase_Port *)pBaseComp->pPorts[i - nStartPortNum];
                    if( pPort->hDIO != NULL ) {
                        eError = OMXBase_DIO_Control(hComponent, i,
                                            OMX_DIO_CtrlCmd_Stop, NULL);
                        OMX_CHECK(OMX_ErrorNone == eError, eError);
                    }
                }
            } else {
                eError = OMX_ErrorIncorrectStateTransition;
                goto EXIT;
            }
        break;

        case OMX_StateWaitForResources :
            if( pBaseComp->tNewState == OMX_StateLoaded ) {
                /* Notify to Derived Component */
                eError = pBaseComp->fpCommandNotify(hComponent,
                                                    OMX_CommandStateSet, nParam, NULL);
                OMX_CHECK(OMX_ErrorNone == eError, eError);
                tStatus = OSAL_RetrieveEvent(pBaseCompPvt->pCmdCompleteEvent, OMXBase_CmdStateSet,
                                                    OSAL_EVENT_OR_CONSUME, &retEvents, OSAL_SUSPEND);
                OMX_CHECK(tStatus == OSAL_ErrNone, OMX_ErrorUndefined);
            } else if( pBaseComp->tNewState == OMX_StateIdle ) {
                /* Notify to Derived Component */
                eError = pBaseComp->fpCommandNotify(hComponent,
                                                    OMX_CommandStateSet, nParam, NULL);
                OMX_CHECK(OMX_ErrorNone == eError, eError);
                tStatus = OSAL_RetrieveEvent(pBaseCompPvt->pCmdCompleteEvent, OMXBase_CmdStateSet,
                                                    OSAL_EVENT_OR_CONSUME, &retEvents, OSAL_SUSPEND);
                OMX_CHECK(tStatus == OSAL_ErrNone, OMX_ErrorUndefined);

                for( i = nStartPortNum; i < (nStartPortNum + nPorts); i++ ) {
                    pPort = (OMXBase_Port *)pBaseComp->pPorts[i - nStartPortNum];
                    /*If port is disabled then nothing needs to be done*/
                    if( pPort->sPortDef.bEnabled == OMX_FALSE ) {
                        continue;
                    }
                    if( pPort->sPortDef.eDir == OMX_DirOutput ) {
                        sDIOParams.nMode = OMX_DIO_WRITER;
                    } else {
                        sDIOParams.nMode = OMX_DIO_READER;
                    }
                }
            } else {
                eError = OMX_ErrorIncorrectStateTransition;
                goto EXIT;
            }
        break;

        default :
            OSAL_ErrorTrace(" unknown command ");
        break;
    }

EXIT:
    if( eError != OMX_ErrorNone ) {
        /* Since no state transition is in progress put the new state again to OMX_StateMax */
        pBaseComp->tNewState = OMX_StateMax;
    }
    return (eError);
}

/*
* OMX Base Event Notufy to Client
*/
OMX_ERRORTYPE OMXBase_EventNotifyToClient(OMX_HANDLETYPE hComponent,
                                            OMX_COMMANDTYPE Cmd,
                                            OMX_U32 nParam,
                                            OMX_PTR pCmdData)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OSAL_ERROR          tStatus = OSAL_ErrNone;
    OMX_COMPONENTTYPE   *pComp = (OMX_COMPONENTTYPE *)hComponent;
    OMXBaseComp         *pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    OMXBaseComp_Pvt     *pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;
    OMXBase_Port        *pPort = NULL;
    uint32_t            retEvents = 0;
    OMX_U32             i, nStartPortNum, nPorts;
    OMX_BOOL            bStartFlag = OMX_FALSE;

    nPorts =  pBaseComp->nNumPorts;
    nStartPortNum = pBaseComp->nMinStartPortIndex;

    switch( Cmd ) {
        case OMX_CommandStateSet :
            if(((pBaseComp->tCurState == OMX_StateLoaded ||
                    pBaseComp->tCurState == OMX_StateWaitForResources) &&
                    pBaseComp->tNewState == OMX_StateIdle) ||
                    (pBaseComp->tCurState == OMX_StateIdle &&
                    pBaseComp->tNewState == OMX_StateLoaded)) {
                /* Incase of loaded to idle and idle to loaded state transition, comp
                * should wait for buffers to be allocated/freed for enabled ports */
                for( i = nStartPortNum; i < (nStartPortNum + nPorts); i++ ) {
                    pPort = pBaseComp->pPorts[i - nStartPortNum];
                    if( pPort->sPortDef.bEnabled == OMX_TRUE ) {
                        retEvents = 0;
                        tStatus = OSAL_RetrieveEvent(pPort->pBufAllocFreeEvent,
                                            (BUF_ALLOC_EVENT | BUF_FREE_EVENT |
                                            BUF_FAIL_EVENT),
                                            OSAL_EVENT_OR_CONSUME, &retEvents,
                                            OSAL_SUSPEND);
                        OMX_CHECK(tStatus == OSAL_ErrNone, OMX_ErrorUndefined);
                        if( retEvents & BUF_FAIL_EVENT ) {
                            /*Fail event so free up all DIO resources and move
                            back to loaded state*/
                            OMXBase_HandleFailEvent(hComponent, Cmd, nParam);
                            if( pBaseComp->tCurState == OMX_StateIdle &&
                                    pBaseComp->tNewState == OMX_StateLoaded ) {
                                eError = OMX_ErrorPortUnresponsiveDuringDeallocation;
                            } else {
                                eError = OMX_ErrorPortUnresponsiveDuringAllocation;
                            }
                            goto EXIT;
                        }
                        /* free up the pool incase if idle to loaded */
                        if( pBaseComp->tCurState == OMX_StateIdle &&
                                pBaseComp->tNewState == OMX_StateLoaded ) {
                            eError = OMXBase_DIO_Close(hComponent, i);
                            OMX_CHECK(OMX_ErrorNone == eError, eError);
                            eError = OMXBase_DIO_Deinit(hComponent, i);
                            OMX_CHECK(OMX_ErrorNone == eError, eError);
                        }
                    }
                }
            } else if( pBaseComp->tCurState == OMX_StateIdle &&
                    (pBaseComp->tNewState == OMX_StatePause ||
                    pBaseComp->tNewState == OMX_StateExecuting)) {
                bStartFlag = OMX_TRUE;
            }
            OSAL_ObtainMutex(pBaseCompPvt->pNewStateMutex, OSAL_SUSPEND);

            pBaseComp->tCurState = pBaseComp->tNewState;
            pBaseComp->tNewState = OMX_StateMax;
            /* Notify Completion to the Client */
            pBaseCompPvt->sAppCallbacks.EventHandler(hComponent,
                                                pComp->pApplicationPrivate,
                                                OMX_EventCmdComplete, OMX_CommandStateSet,
                                                nParam, NULL);
            if( bStartFlag ) {
                for( i = nStartPortNum; i < (nStartPortNum + nPorts); i++ ) {
                    pPort = (OMXBase_Port *)pBaseComp->pPorts[i - nStartPortNum];
                    if( pPort->hDIO != NULL ) {
                        eError = OMXBase_DIO_Control(hComponent, i,
                                                    OMX_DIO_CtrlCmd_Start, NULL);
                        if( OMX_ErrorNone != eError ) {
                            OSAL_ReleaseMutex(pBaseCompPvt->pNewStateMutex);
                            goto EXIT;
                        }
                    }
                }
            }
            OSAL_ReleaseMutex(pBaseCompPvt->pNewStateMutex);
        break;

        case OMX_CommandPortEnable :
            if( OMX_ALL == nParam ) {
                for( i = nStartPortNum; i < (nStartPortNum + nPorts); i++ ) {
                    pPort = pBaseComp->pPorts[i - nStartPortNum];
                    if( pBaseComp->tCurState != OMX_StateLoaded ) {
                        retEvents = 0;
                        tStatus = OSAL_RetrieveEvent(pPort->pBufAllocFreeEvent,
                                            (BUF_ALLOC_EVENT | BUF_FAIL_EVENT),
                                            OSAL_EVENT_OR_CONSUME, &retEvents,
                                            OSAL_SUSPEND);
                        OMX_CHECK(tStatus == OSAL_ErrNone, OMX_ErrorUndefined);
                        if( retEvents & BUF_FAIL_EVENT ) {
                            /*Fail event so free up all DIO resources and move
                            back to port in disabled state*/
                            OMXBase_HandleFailEvent(hComponent, Cmd, nParam);
                            eError = OMX_ErrorPortUnresponsiveDuringAllocation;

                            for( i = 0; i < pBaseComp->nNumPorts; i++ ) {
                                pPort = pBaseComp->pPorts[i];
                                pPort->bIsInTransition = OMX_FALSE;
                                pPort->sPortDef.bEnabled = OMX_FALSE;
                            }
                            goto EXIT;
                        }
                    }
                    pPort->bIsInTransition = OMX_FALSE;
                    /* Notify Completion to the Client */
                    pBaseCompPvt->sAppCallbacks.EventHandler(hComponent,
                                                        pComp->pApplicationPrivate,
                                                        OMX_EventCmdComplete, OMX_CommandPortEnable,
                                                        i, NULL);
                    /*If current state is executing, start buffer transfer*/
                    if( pBaseComp->tCurState == OMX_StateExecuting ) {
                        eError = OMXBase_DIO_Control(hComponent, i,
                                                    OMX_DIO_CtrlCmd_Start, NULL);
                        OMX_CHECK(OMX_ErrorNone == eError, eError);
                    }
                }
            } else {
                pPort = pBaseComp->pPorts[nParam - nStartPortNum];
                if( pBaseComp->tCurState != OMX_StateLoaded ) {
                    retEvents = 0;
                    tStatus = OSAL_RetrieveEvent(pPort->pBufAllocFreeEvent,
                                                    (BUF_ALLOC_EVENT | BUF_FAIL_EVENT),
                                                    OSAL_EVENT_OR_CONSUME, &retEvents,
                                                    3 * 1000); /* wait for 3sec */

                    if( tStatus == OSAL_ErrTimeOut ) {
                        pPort->bIsInTransition = OMX_FALSE;
                        pPort->sPortDef.bEnabled = OMX_FALSE;
                    }

                    OMX_CHECK(tStatus == OSAL_ErrNone,
                                tStatus != OSAL_ErrTimeOut ?
                                OMX_ErrorUndefined : OMX_ErrorPortUnresponsiveDuringAllocation);

                    if( retEvents & BUF_FAIL_EVENT ) {
                        /*Fail event so free up all DIO resources and move
                        back to port in disabled state*/
                        OMXBase_HandleFailEvent(hComponent, Cmd, nParam);
                        eError = OMX_ErrorPortUnresponsiveDuringAllocation;
                        pPort->bIsInTransition = OMX_FALSE;
                        pPort->sPortDef.bEnabled = OMX_FALSE;
                        goto EXIT;
                    }
                }
                pPort->bIsInTransition = OMX_FALSE;

                /* Notify Completion to the Client */
                pBaseCompPvt->sAppCallbacks.EventHandler(hComponent,
                                                    pComp->pApplicationPrivate,
                                                    OMX_EventCmdComplete, OMX_CommandPortEnable,
                                                    nParam, NULL);
                /*If current state is executing, start buffer transfer*/
                if( pBaseComp->tCurState == OMX_StateExecuting ) {
                    eError = OMXBase_DIO_Control(hComponent, nParam,
                                                OMX_DIO_CtrlCmd_Start, NULL);
                    OMX_CHECK(OMX_ErrorNone == eError, eError);
                }
            }
        break;

        case OMX_CommandPortDisable :
            if( OMX_ALL == nParam ) {
                for( i = nStartPortNum; i < (nStartPortNum + nPorts); i++ ) {
                    pPort = pBaseComp->pPorts[i - nStartPortNum];
                    if( pBaseComp->tCurState != OMX_StateLoaded ) {
                        retEvents = 0;
                        tStatus = OSAL_RetrieveEvent(pPort->pBufAllocFreeEvent,
                                        (BUF_FREE_EVENT | BUF_FAIL_EVENT),
                                        OSAL_EVENT_OR_CONSUME, &retEvents,
                                        OSAL_SUSPEND);
                        OMX_CHECK(tStatus == OSAL_ErrNone, OMX_ErrorUndefined);
                        if( retEvents & BUF_FAIL_EVENT ) {
                            /*Fail event so free up all DIO resources and move
                            port to disabled state*/
                            OMXBase_HandleFailEvent(hComponent, Cmd, nParam);
                            eError = OMX_ErrorPortUnresponsiveDuringDeallocation;

                            for( i = 0; i < pBaseComp->nNumPorts; i++ ) {
                                pPort = pBaseComp->pPorts[i];
                                pPort->bIsInTransition = OMX_FALSE;
                            }
                            goto EXIT;
                        }
                        eError = OMXBase_DIO_Close(hComponent, i);
                        OMX_CHECK(OMX_ErrorNone == eError, eError);
                        eError = OMXBase_DIO_Deinit(hComponent, i);
                        OMX_CHECK(OMX_ErrorNone == eError, eError);
                    }
                    pPort->bIsInTransition = OMX_FALSE;
                    /* Notify Completion to the Client */
                    pBaseCompPvt->sAppCallbacks.EventHandler(hComponent,
                                                pComp->pApplicationPrivate,
                                                OMX_EventCmdComplete, OMX_CommandPortDisable,
                                                i, NULL);
                }
            } else {
                pPort = pBaseComp->pPorts[nParam - nStartPortNum];
                if( pBaseComp->tCurState != OMX_StateLoaded ) {
                    retEvents = 0;
                    tStatus = OSAL_RetrieveEvent(pPort->pBufAllocFreeEvent,
                                            (BUF_FREE_EVENT | BUF_FAIL_EVENT),
                                            OSAL_EVENT_OR_CONSUME, &retEvents,
                                            OSAL_SUSPEND);
                    OMX_CHECK(tStatus == OSAL_ErrNone, OMX_ErrorUndefined);
                    if( retEvents & BUF_FAIL_EVENT ) {
                        /*Fail event so free up all DIO resources and move
                        back to port in disabled state*/
                        OMXBase_HandleFailEvent(hComponent, Cmd, nParam);
                        eError = OMX_ErrorPortUnresponsiveDuringDeallocation;
                        pPort->bIsInTransition = OMX_FALSE;
                        goto EXIT;
                    }
                    eError = OMXBase_DIO_Close(hComponent, nParam);
                    OMX_CHECK(OMX_ErrorNone == eError, eError);
                    eError = OMXBase_DIO_Deinit(hComponent, nParam);
                    OMX_CHECK(OMX_ErrorNone == eError, eError);
                }
                pPort->bIsInTransition = OMX_FALSE;
                /* Notify Completion to the Client */
                pBaseCompPvt->sAppCallbacks.EventHandler(hComponent,
                                            pComp->pApplicationPrivate,
                                            OMX_EventCmdComplete, OMX_CommandPortDisable,
                                            nParam, NULL);
            }
        break;

        case OMX_CommandFlush :
            if( nParam == OMX_ALL ) {
                for( i = nStartPortNum; i < (nStartPortNum + nPorts); i++ ) {
                    pPort = pBaseComp->pPorts[i - nStartPortNum];
                    /* Notify Completion to the Client */
                    pBaseCompPvt->sAppCallbacks.EventHandler(hComponent,
                                            pComp->pApplicationPrivate,
                                            OMX_EventCmdComplete, OMX_CommandFlush,
                                            i, NULL);
                }
            } else {
                /* Notify Completion to the Client */
                pBaseCompPvt->sAppCallbacks.EventHandler(hComponent,
                                        pComp->pApplicationPrivate,
                                        OMX_EventCmdComplete, OMX_CommandFlush,
                                        nParam, NULL);
            }
        break;

    case OMX_CommandMarkBuffer :

    break;

    default :
        OSAL_ErrorTrace("InValid command");
    }

EXIT:
    return (eError);
}

/*
* OMX Base IsCmdPending
*/
OMX_BOOL OMXBase_IsCmdPending (OMX_HANDLETYPE hComponent)
{
    OMX_COMPONENTTYPE   *pComp = (OMX_COMPONENTTYPE *)hComponent;
    OMXBaseComp         *pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    OMXBaseComp_Pvt     *pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;
    OMX_BOOL            bRetVal = OMX_FALSE;

    if( OSAL_IsPipeReady(pBaseCompPvt->pCmdPipe) == OSAL_ErrNone ) {
        /*Set data event so that derived component can get data notification
        after it processes the pending command*/
        pBaseCompPvt->bForceNotifyOnce = OMX_TRUE;
        pBaseCompPvt->fpInvokeProcessFunction(hComponent, DATAEVENT);
        bRetVal = OMX_TRUE;
    }

    return (bRetVal);
}


/*
* OMX Base Is DIO Ready
*/
OMX_BOOL OMXBase_IsDioReady(OMX_HANDLETYPE hComponent, OMX_U32 nPortIndex)
{
    OMX_BOOL            bRet = OMX_TRUE;
    OMX_COMPONENTTYPE   *pComp = (OMX_COMPONENTTYPE *)hComponent;
    OMXBaseComp         *pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    OMXBaseComp_Pvt     *pBaseCompPvt = NULL;
    OMXBase_Port        *pPort = NULL;
    OMX_U32             nStartPortNumber = 0;

    if( pBaseComp == NULL ) {
        OSAL_ErrorTrace("Pvt structure is NULL - DIO cannot be used");
        bRet = OMX_FALSE;
        goto EXIT;
    }
    pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;
    if( pBaseCompPvt == NULL ) {
        OSAL_ErrorTrace("Base internal structure is NULL - DIO cannot be used");
        bRet = OMX_FALSE;
        goto EXIT;
    }
    if( pBaseComp->pPorts == NULL ) {
        OSAL_ErrorTrace("No port has been initialized - DIO cannot be used");
        bRet = OMX_FALSE;
        goto EXIT;
    }
    nStartPortNumber = pBaseComp->nMinStartPortIndex;
    pPort = (OMXBase_Port *)
    pBaseComp->pPorts[nPortIndex - nStartPortNumber];
    if( pPort == NULL ) {
        OSAL_ErrorTrace("This port is not initialized - DIO cannot be used");
        bRet = OMX_FALSE;
        goto EXIT;
    }
    if( pPort->hDIO == NULL ) {
        OSAL_ErrorTrace("DIO handle is NULL - DIO cannot be used");
        bRet = OMX_FALSE;
        goto EXIT;
    }
    if( !(((OMX_DIO_Object *)pPort->hDIO)->bOpened)) {
        OSAL_ErrorTrace("DIO has not yet been opened");
        bRet = OMX_FALSE;
        goto EXIT;
    }

EXIT:
    return (bRet);
}

/*
* OMX Base Get UV Buffer shared fd
*/
OMX_ERRORTYPE OMXBase_GetUVBuffer(OMX_HANDLETYPE hComponent,
                                   OMX_U32 nPortIndex,
                                   OMX_PTR pBufHdr, OMX_PTR *pUVBuffer)
{
    OMX_ERRORTYPE   eError = OMX_ErrorNone;
    OMX_CHECK(pBufHdr != NULL, OMX_ErrorBadParameter);
    OMX_CHECK(((OMX_BUFFERHEADERTYPE *)pBufHdr)->pPlatformPrivate !=
                            NULL, OMX_ErrorBadParameter);

    *pUVBuffer = (void*)((OMXBase_BufHdrPvtData *)((OMX_BUFFERHEADERTYPE *)pBufHdr)->
                    pPlatformPrivate)->sMemHdr[1].dma_buf_fd;

EXIT:
    return (eError);
}


/*
* OMXBase Error Handling for WaitForResource State
*/
OMX_ERRORTYPE OMXBase_Error_HandleWFRState(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE       eError  = OMX_ErrorNone;
    OSAL_ERROR          tStatus = OSAL_ErrNone;
    OMX_COMPONENTTYPE   *pComp = (OMX_COMPONENTTYPE *)hComponent;
    OMXBaseComp         *pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    OMXBaseComp_Pvt     *pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;
    uint32_t            retEvents = 0;

    eError = OMXBase_SendCommand(hComponent, OMX_CommandStateSet, OMX_StateLoaded, NULL);

    //Wait for WaitForResources state transition to complete.
    retEvents = 0;
    tStatus = OSAL_RetrieveEvent(pBaseCompPvt->pErrorCmdcompleteEvent,
                            (STATE_LOADED_EVENT),
                            OSAL_EVENT_OR_CONSUME, &retEvents,
                            STATE_TRANSITION_LONG_TIMEOUT);
    OMX_CHECK(tStatus == OSAL_ErrNone, OMX_ErrorUndefined);

EXIT:
    return (eError);
}

/*
* OMX Base Error handling for Idle State
*/
OMX_ERRORTYPE OMXBase_Error_HandleIdleState(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE       eError  = OMX_ErrorNone;
    OSAL_ERROR          tStatus = OSAL_ErrNone;
    OMX_COMPONENTTYPE   *pComp = (OMX_COMPONENTTYPE *)hComponent;
    OMXBaseComp         *pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    OMXBaseComp_Pvt     *pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;
    OMXBase_Port        *pPort = NULL;
    OMX_U32             i = 0, j = 0;
    uint32_t            retEvents = 0;

    eError = OMXBase_SendCommand(hComponent, OMX_CommandStateSet, OMX_StateLoaded, NULL);

    //Send free buffers to complete the Idle transition.
    for( i=0; i < pBaseComp->nNumPorts; i++ ) {
        pPort = pBaseComp->pPorts[i];
        if( pPort->sPortDef.bEnabled == OMX_TRUE ) {
            for( j=0; j < pPort->sPortDef.nBufferCountActual; j++ ) {
                OMXBase_FreeBuffer(hComponent,
                            i + pBaseComp->nMinStartPortIndex,
                            pPort->pBufferlist[j]);
            }
        }
    }

    //Wait for Idle state transition to complete.
    retEvents = 0;
    tStatus = OSAL_RetrieveEvent(pBaseCompPvt->pErrorCmdcompleteEvent,
                            (STATE_LOADED_EVENT),
                            OSAL_EVENT_OR_CONSUME, &retEvents,
                            STATE_TRANSITION_LONG_TIMEOUT);
    OMX_CHECK(tStatus == OSAL_ErrNone, OMX_ErrorUndefined);

EXIT:
    return (eError);
}

/*
* OMX Base UtilCleanup On Error
*/
OMX_ERRORTYPE OMXBase_UtilCleanupIfError(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE       eError  = OMX_ErrorNone;
    OSAL_ERROR          tStatus = OSAL_ErrNone;
    OMX_COMPONENTTYPE   *pComp = (OMX_COMPONENTTYPE *)hComponent;
    OMXBaseComp         *pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    OMXBaseComp_Pvt     *pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;
    OMXBase_Port        *pPort = NULL;
    OMX_U32             i = 0;
    uint32_t            retEvents = 0;
    OMX_BOOL            bPortTransitioning = OMX_FALSE;

    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;

    if((pBaseComp->tCurState == OMX_StateLoaded) && (pBaseComp->tNewState == OMX_StateMax)) {
        for( i=0; i < pBaseComp->nNumPorts; i++ ) {
            pPort = pBaseComp->pPorts[i];
            if( pPort->bIsInTransition) {
                bPortTransitioning = OMX_TRUE;
                break;
            }
        }

        if( bPortTransitioning == OMX_FALSE ) {
            goto EXIT;
        }
    }
    pBaseCompPvt->sAppCallbacks.EventHandler = OMXBase_Error_EventHandler;
    pBaseCompPvt->sAppCallbacks.EmptyBufferDone = OMXBase_Error_EmptyBufferDone;
    pBaseCompPvt->sAppCallbacks.FillBufferDone = OMXBase_Error_FillBufferDone;

    if((pBaseComp->tCurState == OMX_StateWaitForResources) && (pBaseComp->tNewState == OMX_StateMax)) {
        for( i=0; i < pBaseComp->nNumPorts; i++ ) {
            pPort = pBaseComp->pPorts[i];
            if( pPort->bIsInTransition) {
                bPortTransitioning = OMX_TRUE;
                break;
            }
        }

        if( bPortTransitioning == OMX_FALSE ) {
            OSAL_ErrorTrace("Free Handle called in WaitForResources state, attempting a transition to LOADED state");
            eError = OMXBase_Error_HandleWFRState(pComp);
            if( eError != OMX_ErrorNone ) {
                OSAL_ErrorTrace("Error occured. Cleanup might not be complete");
            } else {
                OSAL_ErrorTrace("Cleanup from WaitForResources state is successful");
            }
            goto EXIT;
        }
    }
    //Wait for timeout to let any pending send commands complete.
    retEvents = 0;
    tStatus = OSAL_RetrieveEvent(pBaseCompPvt->pErrorCmdcompleteEvent,
                        (STATE_LOADED_EVENT | STATE_IDLE_EVENT
                        | STATE_EXEC_EVENT | STATE_PAUSE_EVENT
                        | ERROR_EVENT),
                        OSAL_EVENT_OR_CONSUME, &retEvents,
                        STATE_TRANSITION_TIMEOUT);
    if( tStatus != OSAL_ErrNone && tStatus != OSAL_ErrTimeOut ) {
        eError = OMX_ErrorUndefined;
        OSAL_ErrorTrace("Error occured. Cleanup might not be complete");
    }
    //Clear any port disable/enable events which are pending.
    retEvents = 0;
    tStatus = OSAL_RetrieveEvent(pBaseCompPvt->pErrorCmdcompleteEvent,
                            (PORT_ENABLE_EVENT | PORT_DISABLE_EVENT),
                            OSAL_EVENT_OR_CONSUME, &retEvents,
                            OSAL_NO_SUSPEND);
    if( tStatus != OSAL_ErrNone && tStatus != OSAL_ErrTimeOut ) {
        eError = OMX_ErrorUndefined;
        OSAL_ErrorTrace("Error occured. Cleanup might not be complete");
    }

    //This is for the condition if freehandle is being sent while port enable/disable is ongoing.
    for( i=0; i < pBaseComp->nNumPorts; i++ ) {
        pPort = pBaseComp->pPorts[i];
        if( pPort->bIsInTransition) {
            tStatus = OSAL_SetEvent(pPort->pBufAllocFreeEvent,
                             BUF_FAIL_EVENT, OSAL_EVENT_OR);
            if( tStatus != OSAL_ErrNone ) {
                eError = OMX_ErrorUndefined;
            }
            //Wait for port to transition to disable.
            retEvents = 0;
            tStatus = OSAL_RetrieveEvent(pBaseCompPvt->pErrorCmdcompleteEvent,
                                    ERROR_EVENT, OSAL_EVENT_OR_CONSUME, &retEvents,
                                    STATE_TRANSITION_TIMEOUT);
            if( tStatus != OSAL_ErrNone ) {
                eError = OMX_ErrorUndefined;
                OSAL_ErrorTrace("Error occured. Cleanup might not be complete");
            }
            if( pPort->bIsInTransition) {
                pPort->bIsInTransition = OMX_FALSE;
            }
        }
    }

    if( pBaseComp->tCurState == OMX_StateLoaded && pBaseComp->tNewState == OMX_StateMax ) {
        goto EXIT;
    }

    switch( pBaseComp->tCurState ) {
        case OMX_StateLoaded :
            OMX_BASE_HANDLE_IDLE_TO_LOADED_TRANSITION();
        break;

        case OMX_StateWaitForResources :
                // Since BUFFER FAIL done in cleanup utility happened successfully, all that is required is switch to LOADED state.
                if( pBaseComp->tCurState == OMX_StateWaitForResources && pBaseComp->tNewState == OMX_StateMax ) {
                    eError = OMXBase_Error_HandleWFRState(hComponent);
                } else {
                    OSAL_ErrorTrace("Error occured. Cleanup might not be complete");
                }
        break;

        case OMX_StateIdle :
            if( pBaseComp->tCurState == OMX_StateIdle && pBaseComp->tNewState == OMX_StateMax ) {
                eError = OMXBase_Error_HandleIdleState(hComponent);
            } else { // This is to handle scenerio if there is a crash in Idle to Loaded transition
                OMX_BASE_HANDLE_IDLE_TO_LOADED_TRANSITION();
            }
        break;

        case OMX_StateExecuting :
            // Move the component to Idle State.
            OMXBase_SendCommand(hComponent, OMX_CommandStateSet, OMX_StateIdle, NULL);
            retEvents = 0;
            tStatus = OSAL_RetrieveEvent(pBaseCompPvt->pErrorCmdcompleteEvent,
                                        STATE_IDLE_EVENT,
                                        OSAL_EVENT_OR_CONSUME, &retEvents,
                                        STATE_TRANSITION_LONG_TIMEOUT);
            if( tStatus != OSAL_ErrNone ) {
                eError = OMX_ErrorUndefined;
                OSAL_ErrorTrace("I am here b'cs of TIMEOUT in ER");
            }
            eError = OMXBase_Error_HandleIdleState(hComponent);
        break;

        case OMX_StatePause :
            // Move the component to Idle State.
            OMXBase_SendCommand(hComponent, OMX_CommandStateSet, OMX_StateIdle, NULL);
            retEvents = 0;
            tStatus = OSAL_RetrieveEvent(pBaseCompPvt->pErrorCmdcompleteEvent,
                                        (STATE_IDLE_EVENT),
                                        OSAL_EVENT_OR_CONSUME, &retEvents,
                                        STATE_TRANSITION_LONG_TIMEOUT);
            if( tStatus != OSAL_ErrNone ) {
                eError = OMX_ErrorUndefined;
                OSAL_ErrorTrace("I am here b'cs of TIMEOUT in ER");
            }
            eError = OMXBase_Error_HandleIdleState(hComponent);
        break;

        default :
            OSAL_ErrorTrace("Invalid state requested");
    }

EXIT:
    return (eError);
}

/*
* OMX Base Error handling for Empty Buffer Done
*/
OMX_ERRORTYPE OMXBase_Error_EmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR pAppData,
                                             OMX_BUFFERHEADERTYPE *pBuffer)
{
    OMX_ERRORTYPE    eError  = OMX_ErrorNone;
    return (eError);
}

/*
* OMX Base Error Handling for FillBufferDone
*/
OMX_ERRORTYPE OMXBase_Error_FillBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR pAppData,
                                            OMX_BUFFERHEADERTYPE *pBuffer)
{
    OMX_ERRORTYPE    eError  = OMX_ErrorNone;
    return (eError);
}

/*
* OMX Base Error Handling for EventHandler
*/
OMX_ERRORTYPE OMXBase_Error_EventHandler(OMX_HANDLETYPE hComponent, OMX_PTR pAppData,
                                          OMX_EVENTTYPE eEvent, OMX_U32 nData1,
                                          OMX_U32 nData2, OMX_PTR pEventData)
{
    OMX_ERRORTYPE       eError  = OMX_ErrorNone;
    OSAL_ERROR          tStatus = OSAL_ErrNone;
    OMX_COMPONENTTYPE   *pComp = (OMX_COMPONENTTYPE *)hComponent;
    OMXBaseComp         *pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    OMXBaseComp_Pvt     *pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;

    if( eEvent == OMX_EventCmdComplete ) {
        if((OMX_COMMANDTYPE)nData1 == OMX_CommandPortEnable ) {
            tStatus = OSAL_SetEvent(pBaseCompPvt->pErrorCmdcompleteEvent,
                                    PORT_ENABLE_EVENT, OSAL_EVENT_OR);
            if( tStatus != OSAL_ErrNone ) {
                eError = OMX_ErrorUndefined;
            }
        } else if((OMX_COMMANDTYPE)nData1 == OMX_CommandPortEnable ) {
            tStatus = OSAL_SetEvent(pBaseCompPvt->pErrorCmdcompleteEvent,
                                PORT_DISABLE_EVENT, OSAL_EVENT_OR);
            if( tStatus != OSAL_ErrNone ) {
                eError = OMX_ErrorUndefined;
            }
        } else if((OMX_COMMANDTYPE)nData1 == OMX_CommandStateSet ) {
            switch((OMX_STATETYPE) nData2 ) {
                case OMX_StateLoaded :
                    tStatus = OSAL_SetEvent(pBaseCompPvt->pErrorCmdcompleteEvent,
                                            STATE_LOADED_EVENT, OSAL_EVENT_OR);
                    if( tStatus != OSAL_ErrNone ) {
                        eError = OMX_ErrorUndefined;
                    }
                break;

                case OMX_StateIdle :
                    tStatus = OSAL_SetEvent(pBaseCompPvt->pErrorCmdcompleteEvent,
                                            STATE_IDLE_EVENT, OSAL_EVENT_OR);
                    if( tStatus != OSAL_ErrNone ) {
                        eError = OMX_ErrorUndefined;
                    }
                break;

                case OMX_StateExecuting :
                    tStatus = OSAL_SetEvent(pBaseCompPvt->pErrorCmdcompleteEvent,
                                            STATE_EXEC_EVENT, OSAL_EVENT_OR);
                    if( tStatus != OSAL_ErrNone ) {
                        eError = OMX_ErrorUndefined;
                    }
                break;

                case OMX_StatePause :
                    tStatus = OSAL_SetEvent(pBaseCompPvt->pErrorCmdcompleteEvent,
                                            STATE_PAUSE_EVENT, OSAL_EVENT_OR);
                    if( tStatus != OSAL_ErrNone ) {
                        eError = OMX_ErrorUndefined;
                    }
                break;

                default:
                    OSAL_ErrorTrace("Invalid command");
            }
        }
    } else if( eEvent == OMX_EventError ) {
        if(((OMX_ERRORTYPE)nData1 == OMX_ErrorPortUnresponsiveDuringDeallocation) ||
                ((OMX_ERRORTYPE)nData1 == OMX_ErrorPortUnresponsiveDuringAllocation)) {
            tStatus = OSAL_SetEvent(pBaseCompPvt->pErrorCmdcompleteEvent,
                                    ERROR_EVENT, OSAL_EVENT_OR);
        }
    }
    return (eError);
}

/*
* OMX Base Handle Fail Event
*/
void OMXBase_HandleFailEvent(OMX_HANDLETYPE hComponent, OMX_COMMANDTYPE eCmd,
                               OMX_U32 nPortIndex)
{
    OMX_COMPONENTTYPE   *pComp = (OMX_COMPONENTTYPE *)hComponent;
    OMXBaseComp         *pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    OMXBaseComp_Pvt     *pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;
    OMXBase_Port        *pPort = NULL;
    OSAL_ERROR          retval = OSAL_ErrNone;
    OMX_U32             i = 0;
    uint32_t            retEvents = 0;
    OMX_ERRORTYPE       eError  = OMX_ErrorNone;

    switch( eCmd ) {
        case OMX_CommandStateSet :
            OSAL_ObtainMutex(pBaseCompPvt->pNewStateMutex, OSAL_SUSPEND);
            if(((pBaseComp->tCurState == OMX_StateLoaded ||
                    pBaseComp->tCurState == OMX_StateWaitForResources) &&
                    pBaseComp->tNewState == OMX_StateIdle) ||
                    (pBaseComp->tCurState == OMX_StateIdle &&
                    pBaseComp->tNewState == OMX_StateLoaded)) {
                /*Failure in L --> I transition. First step is to tell the derived component
                to revert back to loaded state*/
                if(((pBaseComp->tCurState == OMX_StateLoaded) ||
                        (pBaseComp->tCurState == OMX_StateWaitForResources)) &&
                        (pBaseComp->tNewState == OMX_StateIdle)) {
                    /*Force setting states*/
                    pBaseComp->tCurState = OMX_StateIdle;
                    pBaseComp->tNewState = OMX_StateLoaded;
                    /*Return error values dont matter here since this is cleanup*/
                    eError = pBaseComp->fpCommandNotify(hComponent,
                                                        OMX_CommandStateSet, OMX_StateLoaded, NULL);
                    if( eError == OMX_ErrorNone ) {
                        OSAL_RetrieveEvent(pBaseCompPvt->pCmdCompleteEvent,
                                        OMXBase_CmdStateSet, OSAL_EVENT_OR_CONSUME, &retEvents,
                                        OSAL_SUSPEND);
                    }
                }

                /*If DIO for any port is open close those*/
                for( i = 0; i < pBaseComp->nNumPorts; i++ ) {
                    pPort = pBaseComp->pPorts[i];
                    retval = OSAL_RetrieveEvent(pPort->pBufAllocFreeEvent,
                                            BUF_FREE_EVENT, OSAL_EVENT_OR_CONSUME, &retEvents,
                                            500);
                    if( retval == OSAL_ErrTimeOut ) {
                        OSAL_ErrorTrace("Event retrieve timed out on BUF_FREE_EVENT within function:%s", __FUNCTION__);
                    }
                    if( pPort != NULL ) {
                        if( pPort->hDIO != NULL ) {
                            OMXBase_DIO_Close(hComponent,
                                            (i + pBaseComp->nMinStartPortIndex));

                            OMXBase_DIO_Deinit(hComponent,
                                        (i + pBaseComp->nMinStartPortIndex));
                        }
                    }
                }
                /*Force setting states*/
                pBaseComp->tCurState = OMX_StateLoaded;
                pBaseComp->tNewState = OMX_StateMax;
            }
            OSAL_ReleaseMutex(pBaseCompPvt->pNewStateMutex);
        break;

        case OMX_CommandPortEnable :
            /*Tell derived comp to move back to disabled state*/
            pBaseComp->fpCommandNotify(hComponent,
                                    OMX_CommandPortDisable, nPortIndex, NULL);

            OSAL_RetrieveEvent(pBaseCompPvt->pCmdCompleteEvent,
                                OMXBase_CmdPortDisable, OSAL_EVENT_OR_CONSUME, &retEvents,
                                OSAL_SUSPEND);
            if( nPortIndex == OMX_ALL ) {
                for( i = 0; i < pBaseComp->nNumPorts; i++ ) {
                    pPort = pBaseComp->pPorts[i];
                    pPort->bIsInTransition = OMX_FALSE;
                }
            } else {
                pPort = pBaseComp->pPorts[
                nPortIndex - pBaseComp->nMinStartPortIndex];
                pPort->bIsInTransition = OMX_FALSE;
            }
            /*NO break to OMX_CommandPortEnable case :::Intention is to have a fall through logic to the port disable*/

        case OMX_CommandPortDisable :
            /*Close DIO on the relevant ports for both enable as well as disable
            commands*/
            if( nPortIndex == OMX_ALL ) {
                for( i = 0; i < pBaseComp->nNumPorts; i++ ) {
                    pPort = pBaseComp->pPorts[i];
                    retval = OSAL_RetrieveEvent(pPort->pBufAllocFreeEvent,
                                            BUF_FREE_EVENT, OSAL_EVENT_OR_CONSUME, &retEvents,
                                            500);
                    if( retval == OSAL_ErrTimeOut ) {
                        OSAL_ErrorTrace("Event retrieve timed out on BUF_FREE_EVENT within function:%s", __FUNCTION__);
                    }
                    if( pPort != NULL ) {
                        if( pPort->hDIO != NULL ) {
                            OMXBase_DIO_Close(hComponent, (i + pBaseComp->nMinStartPortIndex));

                            OMXBase_DIO_Deinit(hComponent, (i + pBaseComp->nMinStartPortIndex));
                        }
                    }
                }
            } else {
                pPort = pBaseComp->pPorts[nPortIndex - pBaseComp->nMinStartPortIndex];
                retval = OSAL_RetrieveEvent(pPort->pBufAllocFreeEvent,
                                            BUF_FREE_EVENT, OSAL_EVENT_OR_CONSUME, &retEvents,
                                            500);
                if( retval == OSAL_ErrTimeOut ) {
                    OSAL_ErrorTrace("Event retrieve timed out on BUF_FREE_EVENT within function:%s", __FUNCTION__);
                }
                if( pPort != NULL ) {
                    if( pPort->hDIO != NULL ) {
                        OMXBase_DIO_Close(hComponent, nPortIndex);
                        OMXBase_DIO_Deinit(hComponent, nPortIndex);
                    }
                }
            }
        break;

        default :
            OSAL_ErrorTrace("Invalid Command");
    }

    return;
}


