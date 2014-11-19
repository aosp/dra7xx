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

#define LOG_TAG "OMX_BASE_PROCESS"

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <omx_base.h>
#include <OMX_TI_Custom.h>

static OMX_ERRORTYPE OMXBase_ProcessDataEvent(OMX_HANDLETYPE hComponent);

static OMX_ERRORTYPE OMXBase_ProcessCmdEvent(OMX_HANDLETYPE hComponent,
                                            OMX_COMMANDTYPE Cmd,
                                            OMX_U32 nParam,
                                            OMX_PTR pCmdData);


/*
* OMX Base ProcessTrigger Event
*/
OMX_ERRORTYPE OMXBase_ProcessTriggerEvent(OMX_HANDLETYPE hComponent,
                                            OMX_U32 EventToSet)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OSAL_ERROR          tStatus = OSAL_ErrNone;
    OMX_COMPONENTTYPE   *pComp = (OMX_COMPONENTTYPE *)hComponent;
    OMXBaseComp         *pBaseComp = NULL;
    OMXBaseComp_Pvt     *pBaseCompPvt = NULL;
    OMXBase_Port        *pPort = NULL;
    OMX_U32             i, nStartPort, nCount = 0;
    OMX_BOOL            bNotify = OMX_TRUE;

    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;
    nStartPort = pBaseComp->nMinStartPortIndex;
    if( EventToSet == DATAEVENT ) {
        /*This var mey be accessed by multiple threads but it need not be mutex
        protected*/
        if( pBaseCompPvt->bForceNotifyOnce ) {
            pBaseCompPvt->bForceNotifyOnce = OMX_FALSE;
            bNotify = OMX_TRUE;
            goto EXIT;
        }

        for( i = 0; i < (pBaseComp->nNumPorts); i++ ) {
            pPort = pBaseComp->pPorts[i];
            /*If port is disabled then move on*/
            if( !pPort->sPortDef.bEnabled ) {
                continue;
            }
            /*If EOS has been recd. on any one port then always send notification*/
            if( pPort->sPortDef.eDir == OMX_DirInput && pPort->bEosRecd ) {
                bNotify = OMX_TRUE;
                goto EXIT;
            }
        }

        if( pBaseComp->bNotifyForAnyPort == OMX_TRUE ) {
            bNotify = OMX_FALSE;
            for( i = 0; i < (pBaseComp->nNumPorts); i++ ) {
                pPort = pBaseComp->pPorts[i];
                /*If port is disabled then move on*/
                if( !pPort->sPortDef.bEnabled ||
                        pPort->bIsInTransition) {
                    continue;
                }
                eError = pBaseCompPvt->fpDioGetCount(hComponent, nStartPort + i,
                                                    &nCount);
                OMX_CHECK((eError == OMX_ErrorNone || eError ==
                            (OMX_ERRORTYPE)OMX_TI_WarningEosReceived), eError);
                /*Resetting to ErrorNone in case EOS warning is recd.*/
                eError = OMX_ErrorNone;
                if((nCount >= pBaseComp->pPorts[i]->sProps.nWatermark)) {
                    bNotify = OMX_TRUE;
                    break;
                }
            }
        } else {
            for( i = 0; i < (pBaseComp->nNumPorts); i++ ) {
                pPort = pBaseComp->pPorts[i];
                /*If port is disabled then move on*/
                if( !pPort->sPortDef.bEnabled ||
                        pPort->bIsInTransition) {
                    continue;
                }
                eError = pBaseCompPvt->fpDioGetCount(hComponent, nStartPort + i,
                                                                    &nCount);
                OMX_CHECK((eError == OMX_ErrorNone || eError ==
                                                (OMX_ERRORTYPE)OMX_TI_WarningEosReceived), eError);
                /*Resetting to ErrorNone in case EOS warning is recd.*/
                eError = OMX_ErrorNone;
                if((nCount < pBaseComp->pPorts[i]->sProps.nWatermark)) {
                    bNotify = OMX_FALSE;
                    break;
                }
            }
        }
    }

EXIT:
    if( bNotify == OMX_TRUE && eError == OMX_ErrorNone ) {
        tStatus = OSAL_SetEvent(pBaseCompPvt->pTriggerEvent, EventToSet,
                                OSAL_EVENT_OR);
        if( tStatus != OSAL_ErrNone ) {
            eError = OMX_ErrorUndefined;
        }
    }
    return (eError);
}

/*
* OMX Base ProcessDataNotify
*/
OMX_ERRORTYPE OMXBase_ProcessDataNotify(OMX_HANDLETYPE hComponent)
{
    OMX_COMPONENTTYPE   *pComp = (OMX_COMPONENTTYPE *)hComponent;
    OMXBaseComp         *pBaseComp = NULL;
    OMXBaseComp_Pvt     *pBaseCompPvt = NULL;

    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;
    pBaseCompPvt->bForceNotifyOnce = OMX_FALSE;
    return (OMXBase_ProcessTriggerEvent(hComponent, DATAEVENT));
}

/*
* OMXBase Process Events
*/
OMX_ERRORTYPE OMXBase_ProcessEvents(OMX_HANDLETYPE hComponent,
                                      OMX_U32 retEvent)
{
    OMX_ERRORTYPE           eError = OMX_ErrorNone;
    OMX_ERRORTYPE           eErrorAux = OMX_ErrorNone;
    OSAL_ERROR              tStatus = OSAL_ErrNone;
    OMX_COMPONENTTYPE       *pComp = (OMX_COMPONENTTYPE *)hComponent;
    OMXBaseComp             *pBaseComp = NULL;
    OMXBaseComp_Pvt         *pBaseCompPvt = NULL;
    OMXBase_CmdParams       sCmdParams;
    uint32_t                actualSize = 0;
    OMX_BOOL                bHandleFailEvent = OMX_FALSE;


    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;
    if( retEvent & CMDEVENT ) {
        tStatus = OSAL_ObtainMutex(pBaseCompPvt->pCmdPipeMutex,
                                                OSAL_SUSPEND);
        OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorUndefined);
        /* process command event */
        tStatus = OSAL_IsPipeReady(pBaseCompPvt->pCmdPipe);
        if( tStatus != OSAL_ErrNone ) {
            /*No command in pipe - return*/
            tStatus = OSAL_ReleaseMutex(pBaseCompPvt->pCmdPipeMutex);
            OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorUndefined);
            goto EXIT;
        }
        tStatus = OSAL_ReadFromPipe(pBaseCompPvt->pCmdPipe, &sCmdParams,
        sizeof(OMXBase_CmdParams), &actualSize, OSAL_NO_SUSPEND);
        if( OSAL_ErrNone != tStatus ) {
            eError = OMX_ErrorUndefined;
        }
        tStatus = OSAL_ReleaseMutex(pBaseCompPvt->pCmdPipeMutex);
        OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorUndefined);
        OMX_CHECK(eError == OMX_ErrorNone, eError);

        eError = OMXBase_ProcessCmdEvent(hComponent, sCmdParams.eCmd,
        sCmdParams.unParam, sCmdParams.pCmdData);
        if( OMX_ErrorNone != eError ) {
            bHandleFailEvent = OMX_TRUE;
            goto EXIT;
        }
    } else if( retEvent & DATAEVENT ) {
        /* process data Event */
        eError = OMXBase_ProcessDataEvent(hComponent);
        OMX_CHECK(OMX_ErrorNone == eError, eError);
    }
EXIT:
    if( OMX_ErrorNone != eError ) {
        if(eError != OMX_ErrorDynamicResourcesUnavailable) {
            eErrorAux = pBaseCompPvt->sAppCallbacks.EventHandler(hComponent,
                                                pComp->pApplicationPrivate,
                                                OMX_EventError, eError,
                                                OMX_StateInvalid, NULL);
        } else {
            eErrorAux = pBaseCompPvt->sAppCallbacks.EventHandler(hComponent,
                                                pComp->pApplicationPrivate,
                                                OMX_EventError, eError,
                                                OMX_StateLoaded, NULL);
        }
        /*Component can do nothing if callback returns error*/
        /*Just calling OMXBase_HandleFailEvent for SetState since it was the
        * the intention. When Allocation fails while Dynamic Resources Allocation
        * we are experiencing hang waiting forever for PortDisable event completion
        * after attempted an unsuccessful PortEnable.
        */
        if( bHandleFailEvent && (eError != OMX_ErrorDynamicResourcesUnavailable) ) {
            OMXBase_HandleFailEvent(hComponent, sCmdParams.eCmd, sCmdParams.unParam);
        }
    }
    if( (OMX_ErrorNone != eError) && (eError != OMX_ErrorDynamicResourcesUnavailable) ) {
        return (eError); //return actual error if any
    }

    return (eErrorAux);
}

/*
* OMX Base Process Command Event
*/
static OMX_ERRORTYPE OMXBase_ProcessCmdEvent(OMX_HANDLETYPE hComponent,
                                               OMX_COMMANDTYPE Cmd,
                                               OMX_U32 nParam,
                                               OMX_PTR pCmdData)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OSAL_ERROR          tStatus = OSAL_ErrNone;
    OMX_COMPONENTTYPE   *pComp = (OMX_COMPONENTTYPE *)hComponent;
    OMXBaseComp         *pBaseComp = NULL;
    OMXBaseComp_Pvt     *pBaseCompPvt = NULL;
    OMX_U32             i, nPorts, nStartPortNum;
    uint32_t            retEvents = 0;

    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;
    nPorts =  pBaseComp->nNumPorts;
    nStartPortNum = pBaseComp->nMinStartPortIndex;

    switch( Cmd ) {
        case OMX_CommandStateSet :
            eError = OMXBase_HandleStateTransition(hComponent, nParam);
            OMX_CHECK(OMX_ErrorNone == eError, eError);
        break;

        case OMX_CommandPortDisable :
            /* Notify to Derived Component here so that derived component
            receives correct param - ALL or specific port no. */
            eError = pBaseComp->fpCommandNotify(hComponent,
            OMX_CommandPortDisable, nParam, pCmdData);
            OMX_CHECK(OMX_ErrorNone == eError, eError);

            tStatus = OSAL_RetrieveEvent(pBaseCompPvt->pCmdCompleteEvent,
            OMXBase_CmdPortDisable, OSAL_EVENT_OR_CONSUME,
            &retEvents, OSAL_SUSPEND);
            OMX_CHECK(OSAL_ErrNone == tStatus,
            OMX_ErrorInsufficientResources);
            if( nParam == OMX_ALL ) {
                for( i = nStartPortNum; i < nPorts; i++ ) {
                    eError = OMXBase_DisablePort(hComponent, i);
                    OMX_CHECK(OMX_ErrorNone == eError, eError);
                }
            } else {
                eError = OMXBase_DisablePort(hComponent, nParam);
                OMX_CHECK(OMX_ErrorNone == eError, eError);
            }
        break;

        case OMX_CommandPortEnable :
            /* Notify to Derived Component here so that derived component
            receives correct param - ALL or specific port no. */
            eError = pBaseComp->fpCommandNotify(hComponent,
            OMX_CommandPortEnable, nParam, pCmdData);
            OMX_CHECK(OMX_ErrorNone == eError, eError);
            tStatus = OSAL_RetrieveEvent(pBaseCompPvt->pCmdCompleteEvent,
            OMXBase_CmdPortEnable, OSAL_EVENT_OR_CONSUME,
            &retEvents, OSAL_SUSPEND);
            OMX_CHECK(OSAL_ErrNone == tStatus,
            OMX_ErrorInsufficientResources);
            if( nParam == OMX_ALL ) {
                for( i = nStartPortNum; i < nPorts; i++ ) {
                    eError = OMXBase_EnablePort(hComponent, i);
                    OMX_CHECK(OMX_ErrorNone == eError, eError);
                }
            } else {
                eError = OMXBase_EnablePort(hComponent, nParam);
                OMX_CHECK(OMX_ErrorNone == eError, eError);
            }
        break;

        case OMX_CommandFlush :
            if( pBaseComp->tCurState == OMX_StateLoaded ||
                    pBaseComp->tCurState == OMX_StateWaitForResources ) {
            } else if((nParam != OMX_ALL) && (pBaseComp->pPorts
                    [nParam - nStartPortNum]->sPortDef.bEnabled == OMX_FALSE)) {
                /*Nothing to be done for disabled port, just exit*/
            } else {
                /* Notify to Derived Component here so that derived component
                receives correct param - ALL or specific port no. */
                eError = pBaseComp->fpCommandNotify(hComponent,
                OMX_CommandFlush, nParam, pCmdData);
                OMX_CHECK(OMX_ErrorNone == eError, eError);
                tStatus = OSAL_RetrieveEvent(pBaseCompPvt->pCmdCompleteEvent,
                                        OMXBase_CmdFlush, OSAL_EVENT_OR_CONSUME,
                                        &retEvents, OSAL_SUSPEND);
                OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorInsufficientResources);
                if( nParam == OMX_ALL ) {
                    for( i = nStartPortNum; i < nPorts; i++ ) {
                        eError = OMXBase_FlushBuffers(hComponent, i);
                        OMX_CHECK(OMX_ErrorNone == eError, eError);
                    }
                } else {
                    eError = OMXBase_FlushBuffers(hComponent, nParam);
                    OMX_CHECK(OMX_ErrorNone == eError, eError);
                }
            }
        break;

        case OMX_CommandMarkBuffer :
            eError = pBaseComp->fpCommandNotify(hComponent, Cmd,
                                        nParam, pCmdData);
            OMX_CHECK(OMX_ErrorNone == eError, eError);
        break;

        default :
            OSAL_ErrorTrace(" unknown command received ");
        break;
        }

    eError = OMXBase_EventNotifyToClient(hComponent, Cmd, nParam, pCmdData);
    OMX_CHECK(OMX_ErrorNone == eError, eError);

EXIT:
    return (eError);
}

/*
* OMX Base Process Data Event
*/
static OMX_ERRORTYPE OMXBase_ProcessDataEvent(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE           eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE      *pComp = (OMX_COMPONENTTYPE *)hComponent;
    OMXBaseComp   *pBaseComp = NULL;

    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    eError = pBaseComp->fpDataNotify(hComponent);
    OMX_CHECK(OMX_ErrorNone == eError, eError);

EXIT:
    return (eError);
}

