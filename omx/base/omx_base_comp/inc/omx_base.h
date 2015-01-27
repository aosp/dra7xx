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

#ifndef _D_OMX_BASE_H_
#define _D_OMX_BASE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <osal_error.h>
#include <osal_mutex.h>
#include <osal_memory.h>
#include <osal_pipes.h>
#include <osal_events.h>
#include <osal_task.h>
#include <omx_base_internal.h>
#include <omx_base_utils.h>
#include <memplugin.h>

#define OMX_NOPORT 0xFFFFFFFE
#define OMX_BASE_INPUT_PORT 0
#define OMX_BASE_OUTPUT_PORT 1
#define  OMX_BASE_NUM_OF_PORTS 2
#define  OMX_BASE_DEFAULT_START_PORT_NUM 0
#define DEFAULT_COMPOENENT 0
#define MAX_PLANES_PER_BUFFER 3


/*
* buffer life cycle
*/
typedef enum OMXBase_BufferStatus{
    OWNED_BY_US,
    OWNED_BY_CLIENT,
    OWNED_BY_CODEC
}OMXBase_BufStatus;

/** Platform private buffer header
 */
typedef struct OMXBase_BufHdrPrivateData {
    MemHeader sMemHdr[MAX_PLANES_PER_BUFFER];
    OMXBase_BufStatus bufSt;
}OMXBase_BufHdrPvtData;

typedef struct OMXBase_CodecConfigBuffer {
    MemHeader *sBuffer;
} OMXBase_CodecConfigBuf;

/** Port properties.
 */
typedef struct OMXBase_PortProperties {
    OMX_U32              nWatermark;
    BufAccessMode        eDataAccessMode;
    MemRegion            eBufMemoryType;
    OMX_U32              nNumComponentBuffers;
    OMX_U32              nTimeoutForDequeue;
}OMXBase_PortProps;

/** Base port definition
 */
typedef struct OMXBase_Port{
    OMX_PARAM_PORTDEFINITIONTYPE    sPortDef;
    OMX_BUFFERHEADERTYPE            **pBufferlist;
    OMXBase_PortProps               sProps;
    OMX_BOOL                        bIsBufferAllocator;
    OMX_BOOL                        bIsInTransition;
    OMX_BOOL                        bIsFlushingBuffers;
    OMX_BOOL                        bEosRecd;
    OMX_U32                         nBufferCnt;
    OMX_PTR                         pBufAllocFreeEvent;
    OMX_PTR                         pDioOpenCloseSem;
    OMX_PTR                         hDIO;
    OMX_PTR                         nCachedBufferCnt;
}OMXBase_Port;

/* OMX base component structure
*/
typedef struct OMXBaseComp
{
    OMX_STRING                  cComponentName;
    OMX_VERSIONTYPE             nComponentVersion;
    OMX_PORT_PARAM_TYPE         *pAudioPortParams;
    OMX_PORT_PARAM_TYPE         *pVideoPortParams;
    OMX_PORT_PARAM_TYPE         *pImagePortParams;
    OMX_PORT_PARAM_TYPE         *pOtherPortParams;
    OMX_U32                     nNumPorts;
    OMX_U32                     nMinStartPortIndex;
    OMXBase_Port                **pPorts;
    OMX_BOOL                    bNotifyForAnyPort;
    OMXBaseComp_Pvt             *pPvtData;
    OMX_STATETYPE               tCurState;
    OMX_STATETYPE               tNewState;
    OMX_PTR                     pMutex;

    OMX_ERRORTYPE (*fpCommandNotify)(OMX_HANDLETYPE hComponent, OMX_COMMANDTYPE Cmd,
                                    OMX_U32 nParam, OMX_PTR pCmdData);

    OMX_ERRORTYPE (*fpDataNotify)(OMX_HANDLETYPE hComponent);


    OMX_ERRORTYPE (*fpReturnEventNotify)(OMX_HANDLETYPE hComponent, OMX_EVENTTYPE eEvent,
                                    OMX_U32 nEventData1, OMX_U32 nEventData2, OMX_PTR pEventData);

    OMX_ERRORTYPE (*fpXlateBuffHandle)(OMX_HANDLETYPE hComponent, OMX_PTR pBufferHdr, OMX_BOOL bRegister);

}OMXBaseComp;


OMX_ERRORTYPE OMXBase_ComponentInit(OMX_IN OMX_HANDLETYPE hComponent);

OMX_ERRORTYPE OMXBase_SetCallbacks(OMX_IN OMX_HANDLETYPE hComponent,
                                OMX_IN OMX_CALLBACKTYPE *pCallbacks,
                                OMX_IN OMX_PTR pAppData);

OMX_ERRORTYPE OMXBase_GetComponentVersion(OMX_IN OMX_HANDLETYPE hComponent,
                                        OMX_OUT OMX_STRING pComponentName,
                                        OMX_OUT OMX_VERSIONTYPE *pComponentVersion,
                                        OMX_OUT OMX_VERSIONTYPE *pSpecVersion,
                                        OMX_OUT OMX_UUIDTYPE *pComponentUUID);

OMX_ERRORTYPE OMXBase_SendCommand(OMX_IN OMX_HANDLETYPE hComponent,
                                OMX_IN OMX_COMMANDTYPE Cmd,
                                OMX_IN OMX_U32 nParam1,
                                OMX_IN OMX_PTR pCmdData);

OMX_ERRORTYPE OMXBase_GetParameter(OMX_IN OMX_HANDLETYPE hComponent,
                                OMX_IN OMX_INDEXTYPE nParamIndex,
                                OMX_INOUT OMX_PTR pParamStruct);

OMX_ERRORTYPE OMXBase_SetParameter(OMX_IN OMX_HANDLETYPE hComponent,
                                OMX_IN OMX_INDEXTYPE nIndex,
                                OMX_IN OMX_PTR pParamStruct);


OMX_ERRORTYPE OMXBase_GetConfig(OMX_IN OMX_HANDLETYPE hComponent,
                                OMX_IN OMX_INDEXTYPE nIndex,
                                OMX_INOUT OMX_PTR pComponentConfigStructure);


OMX_ERRORTYPE OMXBase_SetConfig(OMX_IN OMX_HANDLETYPE hComponent,
                                OMX_IN OMX_INDEXTYPE nIndex,
                                OMX_IN OMX_PTR pComponentConfigStructure);

OMX_ERRORTYPE OMXBase_GetExtensionIndex(OMX_IN OMX_HANDLETYPE hComponent,
                                OMX_IN OMX_STRING cParameterName,
                                OMX_OUT OMX_INDEXTYPE *pIndexType);


OMX_ERRORTYPE OMXBase_GetState(OMX_IN OMX_HANDLETYPE hComponent,
                                OMX_OUT OMX_STATETYPE *pState);


OMX_ERRORTYPE OMXBase_UseBuffer(OMX_IN OMX_HANDLETYPE hComponent,
                                OMX_INOUT OMX_BUFFERHEADERTYPE * *ppBufferHdr,
                                OMX_IN OMX_U32 nPortIndex,
                                OMX_IN OMX_PTR pAppPrivate,
                                OMX_IN OMX_U32 nSizeBytes,
                                OMX_IN OMX_U8 *pBuffer);

OMX_ERRORTYPE OMXBase_AllocateBuffer(OMX_IN OMX_HANDLETYPE hComponent,
                                OMX_INOUT OMX_BUFFERHEADERTYPE * *ppBuffer,
                                OMX_IN OMX_U32 nPortIndex,
                                OMX_IN OMX_PTR pAppPrivate,
                                OMX_IN OMX_U32 nSizeBytes);


OMX_ERRORTYPE OMXBase_FreeBuffer(OMX_IN OMX_HANDLETYPE hComponent,
                                OMX_IN OMX_U32 nPortIndex,
                                OMX_IN OMX_BUFFERHEADERTYPE *pBuffer);


OMX_ERRORTYPE OMXBase_EmptyThisBuffer(OMX_IN OMX_HANDLETYPE hComponent,
                                OMX_IN OMX_BUFFERHEADERTYPE *pBuffer);


OMX_ERRORTYPE OMXBase_FillThisBuffer(OMX_IN OMX_HANDLETYPE hComponent,
                                OMX_IN OMX_BUFFERHEADERTYPE *pBuffer);


OMX_ERRORTYPE OMXBase_ComponentDeinit(OMX_IN OMX_HANDLETYPE hComponent);


OMX_ERRORTYPE OMXBase_UseEGLImage(OMX_IN OMX_HANDLETYPE hComponent,
                                OMX_INOUT OMX_BUFFERHEADERTYPE * *ppBufferHdr,
                                OMX_IN OMX_U32 nPortIndex,
                                OMX_IN OMX_PTR pAppPrivate,
                                OMX_IN void *eglImage);

OMX_ERRORTYPE OMXBase_ComponentRoleEnum(OMX_IN OMX_HANDLETYPE hComponent,
                                OMX_OUT OMX_U8 *cRole,
                                OMX_IN OMX_U32 nIndex);

#ifdef __cplusplus
}
#endif

#endif /* _OMX_BASE_H_ */

