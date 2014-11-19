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

#ifndef _OMX_BASE_INTERNAL_H_
#define _OMX_BASE_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <omx_base_dio_plugin.h>

#define CMDEVENT  (0x00000100)

#define DATAEVENT (0x00000200)

#define ENDEVENT  (0x00f00000)

#define OMXBase_CmdStateSet (0x00001000)

#define OMXBase_CmdPortEnable (0x00002000)

#define OMXBase_CmdPortDisable (0x00003000)

#define OMXBase_CmdFlush (0x00004000)

#define OMXBase_CmdMarkBuffer  (0x00005000)

#define OMXBase_MAXCMDS 10

#define BUF_ALLOC_EVENT (0x00000001)

#define BUF_FREE_EVENT (0x00000002)

#define BUF_FAIL_EVENT (0x00000004)

#define STATE_LOADED_EVENT  (0x00000001)
#define STATE_IDLE_EVENT  (0x00000002)
#define STATE_EXEC_EVENT  (0x00000004)
#define STATE_PAUSE_EVENT  (0x00000008)
#define ERROR_EVENT  (0x00000010)
#define PORT_ENABLE_EVENT  (0x00000020)
#define PORT_DISABLE_EVENT  (0x00000040)

#define STATE_TRANSITION_TIMEOUT 500
#define STATE_TRANSITION_LONG_TIMEOUT 5000

/** Priority of the Component thread  */
#define OMX_BASE_THREAD_PRIORITY               (10)

/** Stack Size of the Comp thread*/
#define OMX_BASE_THREAD_STACKSIZE              (50 * 1024)


/** OMX Base Command params
 *  This structure contains the params required to execute command
 *  @param cmd       :  command to execute
 *  @param nParam    :  parameter for the command to be executed
 *  @param pCmdData  :  pointer to the command data
 */
typedef struct OMXBase_CmdParams {
    OMX_COMMANDTYPE eCmd;
    OMX_U32         unParam;
    OMX_PTR         pCmdData;
}OMXBase_CmdParams;

/** OMX Base component private structure
 */
typedef struct OMXBaseComp_Private {
    OMX_U8               cTaskName[OMX_BASE_MAXNAMELEN];
    OMX_U32              nStackSize;
    OMX_U32              nPrioirty;
    OMX_PTR              pThreadId;
    OMX_CALLBACKTYPE     sAppCallbacks;
    OMX_BOOL             bForceNotifyOnce;
    OMX_PTR              pCmdPipe;
    OMX_PTR              pCmdDataPipe;
    OMX_PTR              pTriggerEvent;
    OMX_PTR              pCmdCompleteEvent;
    OMX_PTR              pErrorCmdcompleteEvent;
    OMX_PTR              pCmdPipeMutex;
    OMX_PTR              pNewStateMutex;
    OMX_ERRORTYPE (*fpInvokeProcessFunction)(OMX_HANDLETYPE hComponent,
                                            OMX_U32 retEvent);

    OMX_ERRORTYPE (*fpDioGetCount)(OMX_HANDLETYPE hComponent, OMX_U32 nPortIndex, OMX_U32 *pCount);

    OMX_ERRORTYPE (*fpDioDequeue)(OMX_HANDLETYPE hComponent, OMX_U32 nPortIndex, OMX_PTR *pBuffHeader);

    OMX_ERRORTYPE (*fpDioSend)(OMX_HANDLETYPE hComponent, OMX_U32 nPortIndex, OMX_PTR pBuffHeader);

    OMX_ERRORTYPE (*fpDioCancel)(OMX_HANDLETYPE hComponent, OMX_U32 nPortIndex, OMX_PTR pBuffHeader);

    OMX_ERRORTYPE (*fpDioControl)(OMX_HANDLETYPE hComponent, OMX_U32 nPortIndex,
                                OMX_DIO_CtrlCmdType nCmdType, OMX_PTR pParams);
}OMXBaseComp_Pvt;

/* Initialization and control functions */
OMX_ERRORTYPE OMXBase_PrivateInit(OMX_HANDLETYPE hComponent);

OMX_ERRORTYPE OMXBase_PrivateDeInit(OMX_HANDLETYPE hComponent);

OMX_ERRORTYPE OMXBase_SetDefaultProperties(OMX_HANDLETYPE hComponent);

void OMXBase_CompThreadEntry(void *arg);

OMX_ERRORTYPE OMXBase_InitializePorts(OMX_HANDLETYPE hComponent);

OMX_ERRORTYPE OMXBase_DeinitializePorts(OMX_HANDLETYPE hComponent);

OMX_ERRORTYPE OMXBase_DisablePort(OMX_HANDLETYPE hComponent,
                                OMX_U32 nParam);

OMX_ERRORTYPE OMXBase_EnablePort(OMX_HANDLETYPE hComponent,
                                OMX_U32 nParam);

OMX_ERRORTYPE OMXBase_FlushBuffers(OMX_HANDLETYPE hComponent,
                                OMX_U32 nParam);

OMX_ERRORTYPE OMXBase_HandleStateTransition(OMX_HANDLETYPE hComponent,
                                OMX_U32 nParam);


/* Event processing */
OMX_ERRORTYPE OMXBase_EventNotifyToClient(OMX_HANDLETYPE hComponent,
                                        OMX_COMMANDTYPE Cmd,
                                        OMX_U32 nParam,
                                        OMX_PTR pCmdData);
OMX_BOOL OMXBase_IsCmdPending (OMX_HANDLETYPE hComponent);

OMX_ERRORTYPE OMXBase_ProcessEvents(OMX_HANDLETYPE hComponent,
                                      OMX_U32 retEvent);

OMX_ERRORTYPE OMXBase_ProcessTriggerEvent(OMX_HANDLETYPE hComponent,
                                        OMX_U32 EventToSet);

OMX_ERRORTYPE OMXBase_CB_ReturnEventNotify(OMX_HANDLETYPE hComponent,
                                        OMX_EVENTTYPE eEvent,
                                        OMX_U32 nData1, OMX_U32 nData2,
                                        OMX_PTR pEventData);

/* DIO functions */
OMX_BOOL OMXBase_IsDioReady(OMX_HANDLETYPE hComponent, OMX_U32 nPortIndex);

OMX_ERRORTYPE OMXBase_DIO_Init (OMX_HANDLETYPE hComponent,
                                OMX_U32 nPortIndex,
                                OMX_STRING cChannelType,
                                OMX_PTR pCreateParams);

OMX_ERRORTYPE OMXBase_DIO_Open (OMX_HANDLETYPE hComponent,
                                OMX_U32 nPortIndex,
                                OMX_PTR pOpenParams);

OMX_ERRORTYPE OMXBase_DIO_Close (OMX_HANDLETYPE hComponent,
                                OMX_U32 nPortIndex);

OMX_ERRORTYPE OMXBase_DIO_Queue (OMX_HANDLETYPE hComponent,
                                OMX_U32 nPortIndex,
                                OMX_PTR pBuffHeader);

OMX_ERRORTYPE OMXBase_DIO_Dequeue (OMX_HANDLETYPE hComponent,
                                OMX_U32 nPortIndex,
                                OMX_PTR *pBuffHeader);

OMX_ERRORTYPE OMXBase_DIO_Send (OMX_HANDLETYPE hComponent,
                                OMX_U32 nPortIndex,
                                OMX_PTR pBuffHeader);

OMX_ERRORTYPE OMXBase_DIO_Cancel (OMX_HANDLETYPE hComponent,
                                OMX_U32 nPortIndex,
                                OMX_PTR pBuffHeader);

OMX_ERRORTYPE OMXBase_DIO_Control (OMX_HANDLETYPE hComponent,
                                OMX_U32 nPortIndex,
                                OMX_DIO_CtrlCmdType nCmdType,
                                OMX_PTR pParams);

OMX_ERRORTYPE OMXBase_DIO_GetCount (OMX_HANDLETYPE hComponent,
                                OMX_U32 nPortIndex,
                                OMX_U32 *pCount);

OMX_ERRORTYPE OMXBase_DIO_Deinit (OMX_HANDLETYPE hComponent,
                                OMX_U32 nPortIndex);

/* Error Handling */

OMX_ERRORTYPE OMXBase_Error_EventHandler(OMX_HANDLETYPE hComponent, OMX_PTR pAppData,
                                OMX_EVENTTYPE eEvent, OMX_U32 nData1,
                                OMX_U32 nData2, OMX_PTR pEventData);

OMX_ERRORTYPE OMXBase_Error_FillBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR pAppData,
                                OMX_BUFFERHEADERTYPE *pBuffer);

OMX_ERRORTYPE OMXBase_Error_EmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR pAppData,
                                OMX_BUFFERHEADERTYPE *pBuffer);

void OMXBase_HandleFailEvent(OMX_HANDLETYPE hComponent, OMX_COMMANDTYPE eCmd,
                                OMX_U32 nPortIndex);
                                OMX_ERRORTYPE OMXBase_UtilCleanupIfError(OMX_HANDLETYPE hComponent);

/* Miscellaneous */

OMX_ERRORTYPE OMXBase_GetUVBuffer(OMX_HANDLETYPE hComponent,
                                OMX_U32 nPortIndex,
                                OMX_PTR pBufHdr, OMX_PTR *pUVBuffer);



#ifdef __cplusplus
}
#endif

#endif /* _OMX_BASE_INTERNAL_H_ */

