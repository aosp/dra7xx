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

#define LOG_TAG "OMX_BASE_CALLBACKS"

#include <OMX_Core.h>
#include <omx_base.h>

/*
* OMX Base callback ReturnEventNotify
*/
OMX_ERRORTYPE OMXBase_CB_ReturnEventNotify(OMX_HANDLETYPE hComponent,
                                            OMX_EVENTTYPE eEvent,
                                            OMX_U32 nData1, OMX_U32 nData2,
                                            OMX_PTR pEventData)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE   *pComp = NULL;
    OMXBaseComp         *pBaseComp = NULL;
    OMXBaseComp_Pvt     *pBaseCompPvt = NULL;
    OSAL_ERROR          tStatus = OSAL_ErrNone;
    uint32_t            nActualSize = 0;
    OMX_PTR             pCmdData = NULL;

    OMX_CHECK(hComponent != NULL, OMX_ErrorBadParameter);

    pComp = (OMX_COMPONENTTYPE *)hComponent;
    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;

    switch( eEvent ) {
        case OMX_EventCmdComplete :
            if((OMX_COMMANDTYPE)nData1 == OMX_CommandStateSet ) {
                OSAL_SetEvent(pBaseCompPvt->pCmdCompleteEvent,
                            OMXBase_CmdStateSet, OSAL_EVENT_OR);
            } else if((OMX_COMMANDTYPE)nData1 == OMX_CommandPortEnable ) {
                OSAL_SetEvent(pBaseCompPvt->pCmdCompleteEvent,
                            OMXBase_CmdPortEnable, OSAL_EVENT_OR);
            } else if((OMX_COMMANDTYPE)nData1 == OMX_CommandPortDisable ) {
                OSAL_SetEvent(pBaseCompPvt->pCmdCompleteEvent,
                            OMXBase_CmdPortDisable, OSAL_EVENT_OR);
            } else if((OMX_COMMANDTYPE)nData1 == OMX_CommandFlush ) {
                OSAL_SetEvent(pBaseCompPvt->pCmdCompleteEvent,
                            OMXBase_CmdFlush, OSAL_EVENT_OR);
            } else if((OMX_COMMANDTYPE)nData1 == OMX_CommandMarkBuffer ) {
                /*The derived component has completed the mark buffer command so
                the memory allocated for pCmdData is no longer needed - it can
                be freed up*/
                tStatus = OSAL_ReadFromPipe(pBaseCompPvt->pCmdDataPipe,
                                        &pCmdData, sizeof(OMX_PTR), &nActualSize,
                                        OSAL_NO_SUSPEND);
                /*Read from pipe should return immediately with valid value - if
                it does not, return error callback to the client*/
                if( OSAL_ErrNone != tStatus ) {
                    OSAL_ErrorTrace("pCmdData not available to freed up at mark \
                                    buffer command completion. Returning error event.");
                    pBaseCompPvt->sAppCallbacks.EventHandler(hComponent,
                                        pComp->pApplicationPrivate, OMX_EventError,
                                        (OMX_U32)OMX_ErrorInsufficientResources, 0, NULL);
                    goto EXIT;
                }
                OSAL_Free(pCmdData);
                /*For mark buffer cmd complete, directly send callback to the
                client. This callback is not being handled in
                OMXBase_EventNotifyToClient since this can happen much later
                than the time when the mark comand was received*/
                pBaseCompPvt->sAppCallbacks.EventHandler(hComponent,
                                            pComp->pApplicationPrivate, eEvent, nData1, nData2,
                                            pEventData);
            }
        break;

        case OMX_EventError :
            pBaseCompPvt->sAppCallbacks.EventHandler(hComponent,
                                        pComp->pApplicationPrivate, eEvent, nData1, nData2, pEventData);
        break;

        case OMX_EventMark :
            pBaseCompPvt->sAppCallbacks.EventHandler(hComponent,
                                        pComp->pApplicationPrivate, eEvent, nData1, nData2, pEventData);
        break;

        case OMX_EventPortSettingsChanged :
            pBaseCompPvt->sAppCallbacks.EventHandler(hComponent,
                                        pComp->pApplicationPrivate, eEvent, nData1, nData2, pEventData);
        break;

        case OMX_EventBufferFlag :
            pBaseCompPvt->sAppCallbacks.EventHandler(hComponent,
                                        pComp->pApplicationPrivate, eEvent, nData1, nData2, pEventData);
        break;

        case OMX_EventResourcesAcquired :
            pBaseCompPvt->sAppCallbacks.EventHandler(hComponent,
                                        pComp->pApplicationPrivate, eEvent, nData1, nData2, pEventData);
        break;

        case OMX_EventComponentResumed :
            pBaseCompPvt->sAppCallbacks.EventHandler(hComponent,
                                        pComp->pApplicationPrivate, eEvent, nData1, nData2, pEventData);
        break;

        case OMX_EventDynamicResourcesAvailable :
            pBaseCompPvt->sAppCallbacks.EventHandler(hComponent,
                                        pComp->pApplicationPrivate, eEvent, nData1, nData2, pEventData);
        break;

        case OMX_EventPortFormatDetected :
            pBaseCompPvt->sAppCallbacks.EventHandler(hComponent,
                                        pComp->pApplicationPrivate, eEvent, nData1, nData2, pEventData);
        break;

        default :
            OSAL_ErrorTrace("Unknown event received - still making callback");
            pBaseCompPvt->sAppCallbacks.EventHandler(hComponent,
                                        pComp->pApplicationPrivate, eEvent, nData1, nData2, pEventData);
        break;
    }

EXIT:
    return (eError);
}

