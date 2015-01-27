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

#define LOG_TAG "OMX_BASE_DIO_NONTUNNEL"

#include <omx_base.h>
#include <memplugin.h>
#include <OMX_TI_Custom.h>

typedef struct DIO_NonTunnel_Attrs {
    OMX_DIO_CreateParams sCreateParams;
    OMX_U32              nFlags;
    OMX_PTR              pPipeHandle;
    OMX_PTR              pHdrPool;
    OMX_PTR              pPlatformPrivatePool;
}DIO_NonTunnel_Attrs;


static OMX_ERRORTYPE OMX_DIO_NonTunnel_Open (OMX_HANDLETYPE handle,
                                            OMX_DIO_OpenParams *pParams);

static OMX_ERRORTYPE OMX_DIO_NonTunnel_Close (OMX_HANDLETYPE handle);

static OMX_ERRORTYPE OMX_DIO_NonTunnel_Queue (OMX_HANDLETYPE handle,
                                            OMX_PTR pBuffHeader);

static OMX_ERRORTYPE OMX_DIO_NonTunnel_Dequeue (OMX_HANDLETYPE handle,
                                            OMX_PTR *pBuffHeader);

static OMX_ERRORTYPE OMX_DIO_NonTunnel_Send (OMX_HANDLETYPE handle,
                                            OMX_PTR pBuffHeader);

static OMX_ERRORTYPE OMX_DIO_NonTunnel_Cancel (OMX_HANDLETYPE handle,
                                            OMX_PTR pBuffHeader);

static OMX_ERRORTYPE OMX_DIO_NonTunnel_Control (OMX_HANDLETYPE handle,
                                            OMX_DIO_CtrlCmdType nCmdType,
                                            OMX_PTR pParams);

static OMX_ERRORTYPE OMX_DIO_NonTunnel_Getcount (OMX_HANDLETYPE handle,
                                            OMX_U32 *pCount);

static OMX_ERRORTYPE OMX_DIO_NonTunnel_Deinit (OMX_HANDLETYPE handle);

/*
* _DIO_GetPort from Port Index
*/
static OMX_PTR _DIO_GetPort(OMX_HANDLETYPE handle, OMX_U32 nPortIndex)
{
    OMX_COMPONENTTYPE       *pComp;
    OMXBaseComp    *pBaseComp;
    OMXBase_Port       *pPort = NULL;
    OMX_U32                  nStartPortNumber = 0;
    OMX_DIO_Object   *hDIO = (OMX_DIO_Object *)handle;
    DIO_NonTunnel_Attrs       *pContext = NULL;
    pContext = (DIO_NonTunnel_Attrs *)hDIO->pContext;

    pComp = (OMX_COMPONENTTYPE *)(pContext->sCreateParams.hComponent);
    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;

    nStartPortNumber = pBaseComp->nMinStartPortIndex;
    if( pBaseComp->pPorts == NULL ) {
        pPort = NULL;
        goto EXIT;
    }

    pPort = (OMXBase_Port *)pBaseComp->pPorts[nPortIndex - nStartPortNumber];

EXIT:
    return (pPort);
}

/*
* DIO NoneTunnel Init
*/
OMX_ERRORTYPE OMX_DIO_NonTunnel_Init(OMX_HANDLETYPE handle,
                                     OMX_PTR pCreateParams)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OMX_DIO_Object      *hDIO = (OMX_DIO_Object *)handle;
    DIO_NonTunnel_Attrs *pContext = NULL;

    OMX_COMPONENTTYPE   *pComp = (OMX_COMPONENTTYPE *)(((OMX_DIO_CreateParams *)(pCreateParams))->hComponent);
    OMXBaseComp         *pBaseComPvt = (OMXBaseComp *)pComp->pComponentPrivate;

    /* creating memory for DIO object private area */
    hDIO->pContext = OSAL_Malloc(sizeof(DIO_NonTunnel_Attrs));
    OMX_CHECK(NULL != hDIO->pContext, OMX_ErrorInsufficientResources);

    OSAL_Memset(hDIO->pContext, 0x0, sizeof(DIO_NonTunnel_Attrs));

    hDIO->open        =  OMX_DIO_NonTunnel_Open;
    hDIO->close       =  OMX_DIO_NonTunnel_Close;
    hDIO->queue       =  OMX_DIO_NonTunnel_Queue;
    hDIO->dequeue     =  OMX_DIO_NonTunnel_Dequeue;
    hDIO->send        =  OMX_DIO_NonTunnel_Send;
    hDIO->cancel      =  OMX_DIO_NonTunnel_Cancel;
    hDIO->control     =  OMX_DIO_NonTunnel_Control;
    hDIO->getcount    =  OMX_DIO_NonTunnel_Getcount;
    hDIO->deinit      =  OMX_DIO_NonTunnel_Deinit;

    pContext = hDIO->pContext;
    /* Initialize private data */
    pContext->sCreateParams = *(OMX_DIO_CreateParams *)pCreateParams;

EXIT:
    return (eError);
}

/*
* DIO NonTunnel DeInit
*/
static OMX_ERRORTYPE OMX_DIO_NonTunnel_Deinit (OMX_HANDLETYPE handle)
{
    OMX_ERRORTYPE              eError = OMX_ErrorNone;
    OMX_DIO_Object   *hDIO = (OMX_DIO_Object *)handle;
    DIO_NonTunnel_Attrs       *pContext = NULL;

    pContext = (DIO_NonTunnel_Attrs *)hDIO->pContext;
    if( NULL != pContext ) {
        OSAL_Free(pContext);
        pContext = NULL;
    }

    return (eError);
}

/*
* DIO NonTunnel Open
*/
static OMX_ERRORTYPE OMX_DIO_NonTunnel_Open (OMX_HANDLETYPE handle,
                                             OMX_DIO_OpenParams *pParams)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OSAL_ERROR          tStatus = OSAL_ErrNone;
    OMX_DIO_Object      *hDIO = (OMX_DIO_Object *)handle;
    DIO_NonTunnel_Attrs *pContext = NULL;
    OMXBase_Port        *pPort = NULL;
    OMX_U32             i = 0;
    OMX_U32             nPortIndex = 0;
    OMX_U32             nStartPortNumber = 0;
    OMX_U8              *pTmpBuffer = NULL;
    OMX_U32             nLocalComBuffers = 0;
    OMX_PTR             *pBufArr = NULL;

    OMX_COMPONENTTYPE   *pComp = NULL;
    OMXBaseComp         *pBaseComp = NULL;

    pContext = (DIO_NonTunnel_Attrs *)hDIO->pContext;
    pPort = (OMXBase_Port *)_DIO_GetPort(handle, pContext->sCreateParams.nPortIndex);
    pComp = (OMX_COMPONENTTYPE *)(pContext->sCreateParams.hComponent);
    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    nStartPortNumber = pBaseComp->nMinStartPortIndex;
    nPortIndex = pContext->sCreateParams.nPortIndex;

    /* supplier should allocate both the buffer and buffer headers
    non supplier should allocate only the buffer headers  */
    pPort->pBufferlist = (OMX_BUFFERHEADERTYPE * *)OSAL_Malloc(
    (pPort->sPortDef.nBufferCountActual * sizeof(OMX_BUFFERHEADERTYPE *)));

    OMX_CHECK(NULL != pPort->pBufferlist, OMX_ErrorInsufficientResources);
    OSAL_Memset(pPort->pBufferlist, 0, (pPort->sPortDef.nBufferCountActual
                                * sizeof(OMX_BUFFERHEADERTYPE *)));
    /* create a buffer header pool */
    pContext->pHdrPool = (OMX_PTR)OSAL_Malloc(sizeof(OMX_BUFFERHEADERTYPE)
                                            * (pPort->sPortDef.nBufferCountActual));
    OMX_CHECK(NULL != pContext->pHdrPool, OMX_ErrorInsufficientResources);
    OSAL_Memset(pContext->pHdrPool, 0, sizeof(OMX_BUFFERHEADERTYPE) *
                                    (pPort->sPortDef.nBufferCountActual));

    pContext->pPlatformPrivatePool = (OMX_PTR)OSAL_Malloc(pPort->sPortDef.nBufferCountActual
                                                * sizeof(OMXBase_BufHdrPvtData));
    OMX_CHECK(NULL != pContext->pPlatformPrivatePool, OMX_ErrorInsufficientResources);

    /*Setting platform port pvt pool to 0*/
    OSAL_Memset(pContext->pPlatformPrivatePool, 0,
                    pPort->sPortDef.nBufferCountActual *
                    sizeof(OMXBase_BufHdrPvtData));

    if( pPort->bIsBufferAllocator) {
        //Setting up fields for calling configure
        nLocalComBuffers = pBaseComp->pPorts[nPortIndex - nStartPortNumber]->sProps.
        nNumComponentBuffers;
        if( nLocalComBuffers != 1 && pBaseComp->
                pPorts[nPortIndex - nStartPortNumber]->sProps.eBufMemoryType !=
                MEM_TILER8_2D ) {
            //For non 2D buffers multiple component buffers not supported
            OMX_CHECK(OMX_FALSE, OMX_ErrorBadParameter);
        }

        pBufArr = OSAL_Malloc(sizeof(OMX_PTR) * nLocalComBuffers);
        OMX_CHECK(NULL != pBufArr, OMX_ErrorInsufficientResources);
    }

    /* update buffer list with buffer and buffer headers */
    for( i = 0; i < pPort->sPortDef.nBufferCountActual; i++ ) {
        pPort->pBufferlist[i] = (OMX_BUFFERHEADERTYPE *)(pContext->pHdrPool) + i;
        OMX_BASE_INIT_STRUCT_PTR(pPort->pBufferlist[i], OMX_BUFFERHEADERTYPE);
        pPort->pBufferlist[i]->pPlatformPrivate =
                            (OMXBase_BufHdrPvtData *)(pContext->pPlatformPrivatePool) + i;

        ((OMXBase_BufHdrPvtData *)(pPort->pBufferlist[i]->pPlatformPrivate))->bufSt = OWNED_BY_CLIENT;

        if( pPort->bIsBufferAllocator) {
            MemHeader *h = &(((OMXBase_BufHdrPvtData *)(pPort->pBufferlist[i]->pPlatformPrivate))->sMemHdr[0]);
            pPort->pBufferlist[i]->pBuffer = memplugin_alloc_noheader(h, pParams->nBufSize, 1, MEM_CARVEOUT, 0, 0);
            if( nLocalComBuffers == 2 ) {
                OMX_CHECK(OMX_FALSE, OMX_ErrorNotImplemented);
            }
        }
    }

    /* create a fixed size OS pipe */
    tStatus = OSAL_CreatePipe(&pContext->pPipeHandle,
                        (sizeof(OMX_BUFFERHEADERTYPE *) *
                        pPort->sPortDef.nBufferCountActual),
                        sizeof(OMX_BUFFERHEADERTYPE *), 1);
    OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorInsufficientResources);
    pPort->nCachedBufferCnt = pPort->sPortDef.nBufferCountActual;


EXIT:
    if( pBufArr != NULL ) {
        OSAL_Free(pBufArr);
        pBufArr = NULL;
    }
    if( OMX_ErrorNone != eError ) {
        OMX_DIO_NonTunnel_Close(handle);
    }
    return (eError);
}

/*
* DIO NonTunnel Close
*/
static OMX_ERRORTYPE OMX_DIO_NonTunnel_Close(OMX_HANDLETYPE handle)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OSAL_ERROR          tStatus = OSAL_ErrNone;
    OMX_DIO_Object      *hDIO = (OMX_DIO_Object *)handle;
    DIO_NonTunnel_Attrs *pContext = NULL;
    OMXBase_Port        *pPort = NULL;
    OMX_U32             i = 0, j =0, nPortIndex = 0, nStartPortNumber = 0, nCompBufs = 0;
    OMX_PTR             pTmpBuffer = NULL;
    OMX_COMPONENTTYPE   *pComp = NULL;
    OMXBaseComp         *pBaseComp = NULL;

    pContext = (DIO_NonTunnel_Attrs *)hDIO->pContext;
    OMX_CHECK(pContext != NULL, OMX_ErrorNone);

    pPort = (OMXBase_Port *)_DIO_GetPort(handle, pContext->sCreateParams.nPortIndex);
    if( pPort ) {
        pComp = (OMX_COMPONENTTYPE *)(pContext->sCreateParams.hComponent);
        if( pComp ) {
        pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
        }
        if( pBaseComp ) {
        nStartPortNumber = pBaseComp->nMinStartPortIndex;
        nPortIndex = pPort->sPortDef.nPortIndex;
        nCompBufs = pBaseComp->pPorts[nPortIndex - nStartPortNumber]->sProps.nNumComponentBuffers;
        }
        if( pPort->pBufferlist ) {
            for( i = 0; i < pPort->nCachedBufferCnt; i++ ) {
                if( pPort->pBufferlist[i] ) {
                    if( pPort->bIsBufferAllocator) {
                        MemHeader *h = &(((OMXBase_BufHdrPvtData *)(pPort->pBufferlist[i]->pPlatformPrivate))->sMemHdr[0]);
                        memplugin_free_noheader(h);
                        if( nCompBufs == 2 ) {
                            OMX_CHECK(OMX_FALSE, OMX_ErrorNotImplemented);
                        }
                    }
                }
            }
            /* freeup the buffer list */
            OSAL_Free(pPort->pBufferlist);
            pPort->pBufferlist = NULL;
            pPort->nCachedBufferCnt = 0;
        }
    }

    if( pContext->pPlatformPrivatePool ) {
        OSAL_Free(pContext->pPlatformPrivatePool);
    }
    pContext->pPlatformPrivatePool = NULL;
    if( pContext->pHdrPool ) {
        OSAL_Free(pContext->pHdrPool);
    }
    pContext->pHdrPool = NULL;
    /* delete a OS pipe */
    if( pContext->pPipeHandle != NULL ) {
        tStatus = OSAL_DeletePipe(pContext->pPipeHandle);
        if( tStatus != OSAL_ErrNone ) {
            eError = OMX_ErrorUndefined;
        }
        pContext->pPipeHandle = NULL;
    }

EXIT:
    return (eError);
}

/*
* DIO NonTunnel Queue
*/
static OMX_ERRORTYPE OMX_DIO_NonTunnel_Queue (OMX_HANDLETYPE handle,
                                              OMX_PTR pBuffHeader)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OSAL_ERROR          tStatus = OSAL_ErrNone;
    OMX_DIO_Object      *hDIO = (OMX_DIO_Object *)handle;
    DIO_NonTunnel_Attrs *pContext = NULL;
    OMX_BUFFERHEADERTYPE    *pOMXBufHeader;
    OMXBase_Port            *pPort;

    pOMXBufHeader = (OMX_BUFFERHEADERTYPE *) pBuffHeader;
    pContext = (DIO_NonTunnel_Attrs *)hDIO->pContext;

    pPort = (OMXBase_Port *)_DIO_GetPort(handle, pContext->sCreateParams.nPortIndex);
    tStatus = OSAL_WriteToPipe(pContext->pPipeHandle, &pOMXBufHeader,
                            sizeof(pOMXBufHeader), OSAL_SUSPEND);
    OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorUndefined);

EXIT:
    return (eError);
}

/*
* DIO NonTunnel Dequeue
*/
static OMX_ERRORTYPE OMX_DIO_NonTunnel_Dequeue (OMX_HANDLETYPE handle,
                                                OMX_PTR *pBuffHeader)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OSAL_ERROR          tStatus = OSAL_ErrNone;
    OMX_DIO_Object      *hDIO = (OMX_DIO_Object *)handle;
    DIO_NonTunnel_Attrs *pContext = NULL;
    OMXBase_Port        *pPort;
    uint32_t            actualSize = 0;
    OMX_BUFFERHEADERTYPE    *pOrigOMXBufHeader = NULL;
    OMX_COMPONENTTYPE       *pComp = NULL;
    OMXBaseComp             *pBaseComp = NULL;
    OMX_U32                 nPortIndex, nTimeout;

    pContext = (DIO_NonTunnel_Attrs *)hDIO->pContext;
    pPort = (OMXBase_Port *)_DIO_GetPort(handle, pContext->sCreateParams.nPortIndex);
    pComp = (OMX_COMPONENTTYPE *)pContext->sCreateParams.hComponent;
    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    nPortIndex = pPort->sPortDef.nPortIndex;
    nTimeout = pBaseComp->pPorts[nPortIndex]->sProps.nTimeoutForDequeue;
    /* dequeue the buffer from the data pipe */
    tStatus = OSAL_ReadFromPipe(pContext->pPipeHandle, &pOrigOMXBufHeader,
                                sizeof(pOrigOMXBufHeader), &actualSize, nTimeout);
    OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorUndefined);

    /*Cancel the buffer and return warning so that derived component may call
    GetAttribute*/
    if( pOrigOMXBufHeader->nFlags & OMX_BUFFERFLAG_CODECCONFIG ) {
        /*Reset codec config flag on o/p port*/
        if( OMX_DirOutput == pPort->sPortDef.eDir ) {
            pOrigOMXBufHeader->nFlags &= (~OMX_BUFFERFLAG_CODECCONFIG);
        } else {
            tStatus = OSAL_WriteToFrontOfPipe(pContext->pPipeHandle, &pOrigOMXBufHeader,
                                        sizeof(pOrigOMXBufHeader), OSAL_NO_SUSPEND);
            OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorUndefined);
            eError = (OMX_ERRORTYPE)OMX_TI_WarningAttributePending;
            goto EXIT;
        }
    }
    *pBuffHeader = (OMX_PTR)pOrigOMXBufHeader;

EXIT:
    return (eError);
}

/*
* DIO NonTunnel Send
*/
static OMX_ERRORTYPE OMX_DIO_NonTunnel_Send (OMX_HANDLETYPE handle,
                                             OMX_PTR pBuffHeader)
{
    OMX_ERRORTYPE           eError = OMX_ErrorNone;
    OMX_DIO_Object          *hDIO = (OMX_DIO_Object *)handle;
    DIO_NonTunnel_Attrs     *pContext = NULL;
    OMXBase_Port            *pPort = NULL;
    OMX_COMPONENTTYPE       *pComp = NULL;
    OMXBaseComp             *pBaseComp = NULL;
    OMX_CALLBACKTYPE        *pAppCallbacks = NULL;
    OMX_BUFFERHEADERTYPE    *pOMXBufHeader;

    pOMXBufHeader = (OMX_BUFFERHEADERTYPE *) pBuffHeader;

    pContext = (DIO_NonTunnel_Attrs *)hDIO->pContext;
    pPort = (OMXBase_Port *)_DIO_GetPort(handle, pContext->sCreateParams.nPortIndex);
    pComp = (OMX_COMPONENTTYPE *)pContext->sCreateParams.hComponent;
    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    pAppCallbacks = (OMX_CALLBACKTYPE *)pContext->sCreateParams.pAppCallbacks;

    /* return the buffer back to the Application using EBD or FBD
    * depending on the direction of the port (input or output) */
    if (((OMXBase_BufHdrPvtData *)(pOMXBufHeader->pPlatformPrivate))->bufSt != OWNED_BY_CLIENT) {
        ((OMXBase_BufHdrPvtData *)(pOMXBufHeader->pPlatformPrivate))->bufSt = OWNED_BY_CLIENT;
        if( OMX_DirInput == pPort->sPortDef.eDir ) {
            eError = pAppCallbacks->EmptyBufferDone(pComp,
                                pComp->pApplicationPrivate, pOMXBufHeader);
        } else if( OMX_DirOutput == pPort->sPortDef.eDir ) {
            eError = pAppCallbacks->FillBufferDone(pComp,
                                pComp->pApplicationPrivate, pOMXBufHeader);
        }
    }
EXIT:
    return (eError);
}

/*
* DIO NonTunnel Cancel
*/
static OMX_ERRORTYPE OMX_DIO_NonTunnel_Cancel (OMX_HANDLETYPE handle,
                                               OMX_PTR pBuffHeader)
{
    OMX_ERRORTYPE           eError = OMX_ErrorNone;
    OSAL_ERROR              tStatus = OSAL_ErrNone;
    OMX_DIO_Object          *hDIO = (OMX_DIO_Object *)handle;
    DIO_NonTunnel_Attrs     *pContext = NULL;
    OMXBase_Port            *pPort = NULL;
    OMX_BUFFERHEADERTYPE    *pOMXBufHeader;
    OMX_COMPONENTTYPE       *pComp;
    OMXBaseComp             *pBaseComp;
    OMXBaseComp_Pvt         *pBaseCompPvt;
    OMX_BOOL                bCallProcess = OMX_FALSE;

    pOMXBufHeader = (OMX_BUFFERHEADERTYPE *) pBuffHeader;

    pContext = (DIO_NonTunnel_Attrs *)hDIO->pContext;
    pPort = (OMXBase_Port*)_DIO_GetPort(handle, pContext->sCreateParams.nPortIndex);

    pComp = (OMX_COMPONENTTYPE *)pContext->sCreateParams.hComponent;
    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;

    tStatus = OSAL_WriteToFrontOfPipe(pContext->pPipeHandle, &pOMXBufHeader,
                                sizeof(pOMXBufHeader), OSAL_NO_SUSPEND);
    OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorUndefined);

    if( bCallProcess ) {
        /*Calling process fn so that event to process the buffer can be generated*/
        pBaseCompPvt->fpInvokeProcessFunction(pComp, DATAEVENT);
    }

EXIT:
    return (eError);
}


/*
* DIO NonTunnel Control
*/
static OMX_ERRORTYPE OMX_DIO_NonTunnel_Control (OMX_HANDLETYPE handle,
                                                OMX_DIO_CtrlCmdType nCmdType,
                                                OMX_PTR pParams)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OSAL_ERROR          tStatus = OSAL_ErrNone;
    OMX_DIO_Object      *hDIO = (OMX_DIO_Object *)handle;
    DIO_NonTunnel_Attrs *pContext = NULL;
    OMXBase_Port        *pPort = NULL;
    OMX_COMPONENTTYPE   *pComp = NULL;
    OMXBaseComp         *pBaseComp = NULL;
    OMXBaseComp_Pvt     *pBaseCompPvt = NULL;
    OMX_CALLBACKTYPE    *pAppCallbacks = NULL;
    OMX_BUFFERHEADERTYPE    *pBuffHeader = NULL;
    MemHeader           *pAttrDesc = NULL;
    uint32_t            elementsInpipe = 0;
    uint32_t            actualSize = 0;

    pContext = (DIO_NonTunnel_Attrs *)hDIO->pContext;
    pComp = (OMX_COMPONENTTYPE *)pContext->sCreateParams.hComponent;
    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;
    pPort = (OMXBase_Port *)_DIO_GetPort(handle, pContext->sCreateParams.nPortIndex);
    pAppCallbacks = (OMX_CALLBACKTYPE *)pContext->sCreateParams.pAppCallbacks;

    switch( nCmdType ) {
        /* Flush the queues or buffers on a port. Both Flush and Stop perform
        the same operation i.e. send all the buffers back to the client */
        case OMX_DIO_CtrlCmd_Stop :
        case OMX_DIO_CtrlCmd_Flush :
            /* return all buffers to the IL client using EBD/FBD depending
            * on the direction(input or output) of port */

            OSAL_GetPipeReadyMessageCount(pContext->pPipeHandle, &elementsInpipe);

            while( elementsInpipe ) {
                OSAL_ReadFromPipe(pContext->pPipeHandle, &pBuffHeader,
                                sizeof(pBuffHeader), &actualSize, OSAL_NO_SUSPEND);
                elementsInpipe--;
                if (((OMXBase_BufHdrPvtData *)(pBuffHeader->pPlatformPrivate))->bufSt != OWNED_BY_CLIENT) {
                    ((OMXBase_BufHdrPvtData *)(pBuffHeader->pPlatformPrivate))->bufSt = OWNED_BY_CLIENT;
                    if( OMX_DirInput == pPort->sPortDef.eDir ) {
                        eError = pAppCallbacks->EmptyBufferDone(pComp,
                                                    pComp->pApplicationPrivate, pBuffHeader);
                    } else if( OMX_DirOutput == pPort->sPortDef.eDir ) {
                        pBuffHeader->nFilledLen = 0;
                        eError = pAppCallbacks->FillBufferDone(pComp,
                                pComp->pApplicationPrivate, pBuffHeader);
                    }
                }
            }
        break;

        case OMX_DIO_CtrlCmd_Start :
            /*If there are buffers in the pipe in case of pause to executing
            then start processing them*/
            OSAL_GetPipeReadyMessageCount(pContext->pPipeHandle, &elementsInpipe);
            if( elementsInpipe ) {
                pBaseCompPvt->fpInvokeProcessFunction(pComp, DATAEVENT);
            } else {
                OSAL_ErrorTrace(" Nothing to do ");
            }
        break;

        case OMX_DIO_CtrlCmd_GetCtrlAttribute :
            /*Buffer should be available when calling GetAttribute*/
            tStatus = OSAL_IsPipeReady(pContext->pPipeHandle);
            OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorUndefined);

            tStatus = OSAL_ReadFromPipe(pContext->pPipeHandle, &pBuffHeader,
                                    sizeof(pBuffHeader), &actualSize, OSAL_NO_SUSPEND);
            OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorUndefined);
            if( !(pBuffHeader->nFlags & OMX_BUFFERFLAG_CODECCONFIG)) {
                /*This buffer does not contain codec config*/
                eError = OMX_ErrorUndefined;
                /*Write the buffer back to front of pipe*/
                OSAL_WriteToFrontOfPipe(pContext->pPipeHandle, &pBuffHeader,
                                        sizeof(pBuffHeader), OSAL_SUSPEND);
                goto EXIT;
            }
            pAttrDesc = ((OMXBase_CodecConfigBuf*)pParams)->sBuffer;
            if( pAttrDesc->size < pBuffHeader->nFilledLen ) {
                tStatus = OSAL_WriteToFrontOfPipe(pContext->pPipeHandle,
                                            &pBuffHeader, sizeof(pBuffHeader), OSAL_SUSPEND);
                pAttrDesc->size = pBuffHeader->nFilledLen;

                eError = (OMX_ERRORTYPE)OMX_TI_WarningInsufficientAttributeSize;
                goto EXIT;
            }
            pAttrDesc->size = pBuffHeader->nFilledLen;
            OSAL_Memcpy(H2P(pAttrDesc), pBuffHeader->pBuffer + pBuffHeader->nOffset,
                                            pAttrDesc->size);

            /*Send the buffer back*/
            pBuffHeader->nFlags &= (~OMX_BUFFERFLAG_CODECCONFIG);
            if (((OMXBase_BufHdrPvtData *)(pBuffHeader->pPlatformPrivate))->bufSt != OWNED_BY_CLIENT) {
                ((OMXBase_BufHdrPvtData *)(pBuffHeader->pPlatformPrivate))->bufSt = OWNED_BY_CLIENT;
                if( OMX_DirInput == pPort->sPortDef.eDir ) {
                    eError = pAppCallbacks->EmptyBufferDone(pComp,
                                        pComp->pApplicationPrivate, pBuffHeader);
                } else if( OMX_DirOutput == pPort->sPortDef.eDir ) {
                    /*So that the other port does not try to interpret any garbage
                    data that may be present*/
                    pBuffHeader->nFilledLen = 0;
                    eError = pAppCallbacks->FillBufferDone(pComp,
                                    pComp->pApplicationPrivate, pBuffHeader);
                }
            }
        break;

        case OMX_DIO_CtrlCmd_SetCtrlAttribute :
            /*Buffer should be available when calling SetAttribute*/
            tStatus = OSAL_IsPipeReady(pContext->pPipeHandle);
            OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorUndefined);

            tStatus = OSAL_ReadFromPipe(pContext->pPipeHandle, &pBuffHeader,
                                    sizeof(pBuffHeader), &actualSize, OSAL_NO_SUSPEND);
            OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorUndefined);

            pBuffHeader->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;
            pAttrDesc = ((OMXBase_CodecConfigBuf *)pParams)->sBuffer;
            if( pBuffHeader->nAllocLen < pAttrDesc->size ) {
                OSAL_ErrorTrace("Cannot send attribute data, size is too large");
                tStatus = OSAL_WriteToFrontOfPipe(pContext->pPipeHandle,
                                            &pBuffHeader, sizeof(pBuffHeader), OSAL_SUSPEND);
                eError = OMX_ErrorInsufficientResources;
                goto EXIT;
            }
            pBuffHeader->nFilledLen = pAttrDesc->size;
            OSAL_Memcpy(pBuffHeader->pBuffer, pAttrDesc->ptr, pAttrDesc->size);
            if (((OMXBase_BufHdrPvtData *)(pBuffHeader->pPlatformPrivate))->bufSt != OWNED_BY_CLIENT) {
                ((OMXBase_BufHdrPvtData *)(pBuffHeader->pPlatformPrivate))->bufSt = OWNED_BY_CLIENT;
                /*Send the buffer*/
                if( OMX_DirInput == pPort->sPortDef.eDir ) {
                    eError = pAppCallbacks->EmptyBufferDone(pComp,
                                        pComp->pApplicationPrivate, pBuffHeader);
                } else if( OMX_DirOutput == pPort->sPortDef.eDir ) {
                    eError = pAppCallbacks->FillBufferDone(pComp,
                                            pComp->pApplicationPrivate, pBuffHeader);
                }
            }
        break;

        default :
            OSAL_ErrorTrace(" Invalid Command received \n");
            eError = OMX_ErrorUnsupportedIndex;
        break;
    }

EXIT:
    return (eError);
}


/*
* DIO Non Tunnel GEtCount
*/
static OMX_ERRORTYPE OMX_DIO_NonTunnel_Getcount (OMX_HANDLETYPE handle,
                                                 OMX_U32 *pCount)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OMX_DIO_Object      *hDIO = (OMX_DIO_Object *)handle;
    DIO_NonTunnel_Attrs *pContext = NULL;
    OMXBase_Port        *pPort = NULL;

    pContext = (DIO_NonTunnel_Attrs *)hDIO->pContext;
    pPort = (OMXBase_Port *)_DIO_GetPort(handle, pContext->sCreateParams.nPortIndex);
    if( pPort->bEosRecd && pPort->sPortDef.eDir == OMX_DirInput ) {
        eError = (OMX_ERRORTYPE)OMX_TI_WarningEosReceived;
    }

    OSAL_GetPipeReadyMessageCount(pContext->pPipeHandle, (uint32_t*)pCount);

    return (eError);
}


