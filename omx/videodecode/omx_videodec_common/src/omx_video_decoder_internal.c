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

#define LOG_TAG "OMX_VIDDEC_INTERNAL"

#include <omx_video_decoder_internal.h>

OMX_ERRORTYPE OMXVidDec_InitFields(OMXVidDecComp *pVidDecComp)
{
    OMX_ERRORTYPE    eError = OMX_ErrorNone;
    hw_module_t const* module;
    OMX_U32          i = 0;

    pVidDecComp->sBase.cComponentName = (OMX_STRING )OSAL_Malloc(sizeof(OMX_U8) * OMX_MAX_STRINGNAME_SIZE);
    OMX_CHECK(pVidDecComp->sBase.cComponentName != NULL, OMX_ErrorInsufficientResources);

    /* Initialize Video Port parameters */
    pVidDecComp->sBase.pVideoPortParams = (OMX_PORT_PARAM_TYPE*)OSAL_Malloc(sizeof(OMX_PORT_PARAM_TYPE));

    OMX_BASE_INIT_STRUCT_PTR(pVidDecComp->sBase.pVideoPortParams, OMX_PORT_PARAM_TYPE);
    pVidDecComp->sBase.pVideoPortParams->nPorts             = OMX_VIDDEC_NUM_OF_PORTS;
    pVidDecComp->sBase.pVideoPortParams->nStartPortNumber   = OMX_VIDDEC_DEFAULT_START_PORT_NUM;
    pVidDecComp->sBase.nNumPorts                            = OMX_VIDDEC_NUM_OF_PORTS;
    pVidDecComp->sBase.nMinStartPortIndex = OMX_VIDDEC_DEFAULT_START_PORT_NUM;

    pVidDecComp->sBase.nComponentVersion.s.nVersionMajor    = OMX_VIDDEC_COMP_VERSION_MAJOR;
    pVidDecComp->sBase.nComponentVersion.s.nVersionMinor    = OMX_VIDDEC_COMP_VERSION_MINOR;
    pVidDecComp->sBase.nComponentVersion.s.nRevision        = OMX_VIDDEC_COMP_VERSION_REVISION;
    pVidDecComp->sBase.nComponentVersion.s.nStep            = OMX_VIDDEC_COMP_VERSION_STEP;

    eError = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
    if (eError == 0) {
        pVidDecComp->grallocModule = (gralloc_module_t const *)module;
    } else {
        eError = OMX_ErrorInsufficientResources;
    }

EXIT:
    return (eError);
}

/*
*/
void OMXVidDec_InitPortDefs(OMX_HANDLETYPE hComponent, OMXVidDecComp *pVidDecComp)
{
    OMXBaseComp *pBaseComp = &(pVidDecComp->sBase);

    OMX_PARAM_PORTDEFINITIONTYPE   *inPortDefs = &(pBaseComp->pPorts[OMX_VIDDEC_INPUT_PORT]->sPortDef);
    OMX_PARAM_PORTDEFINITIONTYPE   *outPortDefs= &(pBaseComp->pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef);

    /* set the default/Actual values of an Input port */
    inPortDefs->nSize              = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    inPortDefs->nVersion           = pBaseComp->nComponentVersion;
    inPortDefs->bEnabled           = OMX_TRUE;
    inPortDefs->bPopulated         = OMX_FALSE;
    inPortDefs->eDir               = OMX_DirInput;
    inPortDefs->nPortIndex         = OMX_VIDDEC_INPUT_PORT;
    inPortDefs->nBufferCountMin    = OMX_VIDDEC_MIN_IN_BUF_COUNT;
    inPortDefs->nBufferCountActual = OMX_VIDDEC_DEFAULT_IN_BUF_COUNT;
    inPortDefs->nBufferSize
            = Calc_InbufSize(OMX_VIDDEC_DEFAULT_FRAME_WIDTH,
                             OMX_VIDDEC_DEFAULT_FRAME_HEIGHT);

    inPortDefs->eDomain              = OMX_PortDomainVideo;
    inPortDefs->bBuffersContiguous   = OMX_TRUE;
    inPortDefs->nBufferAlignment     = OMX_VIDDEC_DEFAULT_1D_INPUT_BUFFER_ALIGNMENT;
    inPortDefs->format.video.cMIMEType= NULL;
    inPortDefs->format.video.pNativeRender = NULL;
    inPortDefs->format.video.nFrameWidth = OMX_VIDDEC_DEFAULT_FRAME_WIDTH;
    inPortDefs->format.video.nFrameHeight = OMX_VIDDEC_DEFAULT_FRAME_HEIGHT;
    inPortDefs->format.video.nStride = 0;
    inPortDefs->format.video.nSliceHeight = 0;
    inPortDefs->format.video.nBitrate = 0;
    inPortDefs->format.video.xFramerate = OMX_VIDEODECODER_DEFAULT_FRAMERATE << 16;
    inPortDefs->format.video.bFlagErrorConcealment = OMX_TRUE;
    inPortDefs->format.video.eCompressionFormat = pVidDecComp->tVideoParams[OMX_VIDDEC_INPUT_PORT].eCompressionFormat;
    inPortDefs->format.video.eColorFormat = OMX_COLOR_FormatUnused;

    /* set the default/Actual values of an output port */
    outPortDefs->nSize              = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    outPortDefs->nVersion           = pVidDecComp->sBase.nComponentVersion;
    outPortDefs->bEnabled           = OMX_TRUE;
    outPortDefs->bPopulated         = OMX_FALSE;
    outPortDefs->eDir               = OMX_DirOutput;
    outPortDefs->nPortIndex         = OMX_VIDDEC_OUTPUT_PORT;
    outPortDefs->nBufferCountMin    = pVidDecComp->fpCalc_OubuffDetails(hComponent,
                                                    OMX_VIDDEC_DEFAULT_FRAME_WIDTH,
                                                    OMX_VIDDEC_DEFAULT_FRAME_HEIGHT).nBufferCountMin;
    outPortDefs->nBufferCountActual = pVidDecComp->fpCalc_OubuffDetails(hComponent,
                                                    OMX_VIDDEC_DEFAULT_FRAME_WIDTH,
                                                    OMX_VIDDEC_DEFAULT_FRAME_HEIGHT).nBufferCountActual;

    outPortDefs->nBufferSize        = pVidDecComp->fpCalc_OubuffDetails(hComponent,
                                                    OMX_VIDDEC_DEFAULT_FRAME_WIDTH,
                                                    OMX_VIDDEC_DEFAULT_FRAME_HEIGHT).nBufferSize;
    outPortDefs->eDomain            = OMX_PortDomainVideo;
    outPortDefs->bBuffersContiguous = OMX_TRUE;
    outPortDefs->nBufferAlignment   = pVidDecComp->fpCalc_OubuffDetails(hComponent,
                                                    OMX_VIDDEC_DEFAULT_FRAME_WIDTH,
                                                    OMX_VIDDEC_DEFAULT_FRAME_HEIGHT).n1DBufferAlignment;
    outPortDefs->format.video.cMIMEType     = NULL;
    outPortDefs->format.video.pNativeRender = NULL;
    outPortDefs->format.video.nFrameWidth   = OMX_VIDDEC_DEFAULT_FRAME_WIDTH;
    outPortDefs->format.video.nFrameHeight  = OMX_VIDDEC_DEFAULT_FRAME_HEIGHT;
    outPortDefs->format.video.nStride       =  OMX_VIDDEC_TILER_STRIDE;
    outPortDefs->format.video.nSliceHeight  = OMX_VIDDEC_DEFAULT_FRAME_HEIGHT;
    outPortDefs->format.video.nBitrate      = 0;
    outPortDefs->format.video.xFramerate    = OMX_VIDEODECODER_DEFAULT_FRAMERATE << 16;
    outPortDefs->format.video.bFlagErrorConcealment = OMX_TRUE;
    outPortDefs->format.video.eCompressionFormat    = pVidDecComp->tVideoParams[OMX_VIDDEC_OUTPUT_PORT].eCompressionFormat;
    outPortDefs->format.video.eColorFormat          = OMX_COLOR_FormatYUV420PackedSemiPlanar;
}


void OMXVidDec_InitPortParams(OMXVidDecComp *pVidDecComp)
{
    OMX_VIDEO_PARAM_PORTFORMATTYPE   *tInPortVideoParam
                                = &(pVidDecComp->tVideoParams[OMX_VIDDEC_INPUT_PORT]);
    OMX_VIDEO_PARAM_PORTFORMATTYPE   *tOutPortVideoParam
                                = &(pVidDecComp->tVideoParams[OMX_VIDDEC_OUTPUT_PORT]);

    // Initialize Input Video Port Param
    tInPortVideoParam->nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
    tInPortVideoParam->nVersion = pVidDecComp->sBase.nComponentVersion;
    tInPortVideoParam->nPortIndex = OMX_VIDDEC_INPUT_PORT;
    tInPortVideoParam->nIndex = 0;
    tInPortVideoParam->eCompressionFormat = pVidDecComp->tVideoParams[OMX_VIDDEC_INPUT_PORT].eCompressionFormat;
    tInPortVideoParam->eColorFormat = OMX_COLOR_FormatUnused;
    tInPortVideoParam->xFramerate = OMX_VIDEODECODER_DEFAULT_FRAMERATE;

    // Initialize Output Video Port Param
    tOutPortVideoParam->nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
    tOutPortVideoParam->nVersion = pVidDecComp->sBase.nComponentVersion;
    tOutPortVideoParam->nPortIndex = OMX_VIDDEC_OUTPUT_PORT;
    tOutPortVideoParam->nIndex = 1;
    tOutPortVideoParam->eCompressionFormat = pVidDecComp->tVideoParams[OMX_VIDDEC_OUTPUT_PORT].eCompressionFormat;
    tOutPortVideoParam->eColorFormat = OMX_COLOR_FormatYUV420PackedSemiPlanar;
    tOutPortVideoParam->xFramerate = OMX_VIDEODECODER_DEFAULT_FRAMERATE;
}

void OMXVidDec_InitDecoderParams(OMX_HANDLETYPE hComponent,
                                      OMXVidDecComp *pVidDecComp)
{
    OMXVidDec_InitPortDefs(hComponent, pVidDecComp);
    OMXVidDec_InitPortParams(pVidDecComp);
    OMXVidDec_Set2DBuffParams(hComponent, pVidDecComp);

    /* Call Decoder Specific function to set Static Params */
    pVidDecComp->fpSet_StaticParams(hComponent, pVidDecComp->pDecStaticParams);

    pVidDecComp->pDecStaticParams->maxHeight = OMX_VIDDEC_DEFAULT_FRAME_HEIGHT;
    pVidDecComp->pDecStaticParams->maxWidth = OMX_VIDDEC_DEFAULT_FRAME_WIDTH;
    return;
}


void OMXVidDec_Set2DBuffParams(OMX_HANDLETYPE hComponent, OMXVidDecComp *pVidDecComp)
{
    OMX_U32                outPort = (OMX_U32)OMX_VIDDEC_OUTPUT_PORT;
    OMX_U32                inPort = (OMX_U32)OMX_VIDDEC_INPUT_PORT;
    OMX_CONFIG_RECTTYPE   *p2DOutBufAllocParam = &(pVidDecComp->t2DBufferAllocParams[outPort]);
    OMX_CONFIG_RECTTYPE   *p2DInBufAllocParam  = &(pVidDecComp->t2DBufferAllocParams[inPort]);

    PaddedBuffParams     outBuffParams;
    OMX_U32              nFrameWidth, nFrameHeight;

    p2DOutBufAllocParam->nSize = sizeof(OMX_CONFIG_RECTTYPE);
    p2DOutBufAllocParam->nVersion = pVidDecComp->sBase.nComponentVersion;
    p2DOutBufAllocParam->nPortIndex = outPort;

    nFrameWidth = pVidDecComp->sBase.pPorts[inPort]->sPortDef.format.video.nFrameWidth;
    nFrameHeight = pVidDecComp->sBase.pPorts[inPort]->sPortDef.format.video.nFrameHeight;
    if( nFrameWidth & 0x0F ) {
        nFrameWidth = nFrameWidth + 16 - (nFrameWidth & 0x0F);
    }
    if( nFrameHeight & 0x1F ) {
        nFrameHeight = nFrameHeight + 32 - (nFrameHeight & 0x1F);
    }
    outBuffParams = pVidDecComp->fpCalc_OubuffDetails(hComponent, nFrameWidth, nFrameHeight);

    p2DOutBufAllocParam->nWidth = outBuffParams.nPaddedWidth;
    p2DOutBufAllocParam->nHeight = outBuffParams.nPaddedHeight;
    p2DOutBufAllocParam->nLeft = outBuffParams.n2DBufferXAlignment;
    p2DOutBufAllocParam->nTop = outBuffParams.n2DBufferYAlignment;

    p2DInBufAllocParam->nSize = sizeof(OMX_CONFIG_RECTTYPE);
    p2DInBufAllocParam->nVersion = pVidDecComp->sBase.nComponentVersion;
    p2DInBufAllocParam->nPortIndex = OMX_VIDDEC_INPUT_PORT;
    p2DInBufAllocParam->nWidth = pVidDecComp->sBase.pPorts[inPort]->sPortDef.nBufferSize;
    p2DInBufAllocParam->nHeight = 1;     //On input port only 1D buffers supported.
    p2DInBufAllocParam->nLeft   = pVidDecComp->sBase.pPorts[inPort]->sPortDef.nBufferAlignment;
    p2DInBufAllocParam->nTop = 1;
}


OMX_ERRORTYPE OMXVidDec_HandleFLUSH_EOS(OMX_HANDLETYPE hComponent,
                                        OMX_BUFFERHEADERTYPE *pLastOutBufHeader,
                                        OMX_BUFFERHEADERTYPE *pInBufHeader)
{
    OMX_ERRORTYPE           eError = OMX_ErrorNone, eRMError = OMX_ErrorNone;
    OMX_COMPONENTTYPE       *pHandle = NULL;
    OMX_BUFFERHEADERTYPE    *pDupBufHeader = NULL;
    OMXVidDecComp           *pVidDecComp = NULL;
    OMX_U32                 i = 0;
    OMX_U32                 nStride;
    IVIDDEC3_OutArgs        *pDecOutArgs = NULL;
    OMX_U32                 outPort = (OMX_U32)OMX_VIDDEC_OUTPUT_PORT;
    OMX_U32                 nLumaFilledLen=0, nChromaFilledLen=0;
    OMX_CONFIG_RECTTYPE     *p2DOutBufAllocParam = NULL;
    XDM_Rect                activeFrameRegion[2];
    XDAS_Int32              status;
    uint32_t                nActualSize;
    IMG_native_handle_t*    grallocHandle;
    OMX_PARAM_PORTDEFINITIONTYPE    *pOutputPortDef = NULL;

    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp = (OMXVidDecComp *)pHandle->pComponentPrivate;
    pDecOutArgs = pVidDecComp->pDecOutArgs;
    p2DOutBufAllocParam = &(pVidDecComp->t2DBufferAllocParams[outPort]);

    pOutputPortDef = &(pVidDecComp->sBase.pPorts[outPort]->sPortDef);
    /*! Ensure that the stride on output portdef structure is more than
    the padded width. This is needed in the case where application
    sets the Stride less than padded width */
    if( pOutputPortDef->format.video.nStride >=
           p2DOutBufAllocParam->nWidth ) {
        nStride = pOutputPortDef->format.video.nStride;
    } else {
        nStride = p2DOutBufAllocParam->nWidth;
    }

    if( pVidDecComp->nFrameCounter > 0 ) {
        /* Call codec flush and call process call until error */
        OMX_CHECK(((pVidDecComp->pDecDynParams != NULL) && (pVidDecComp->pDecStatus != NULL)), OMX_ErrorBadParameter);
        status = VIDDEC3_control(pVidDecComp->pDecHandle, XDM_FLUSH, pVidDecComp->pDecDynParams, pVidDecComp->pDecStatus);
        if( status != VIDDEC3_EOK ) {
            OSAL_ErrorTrace("VIDDEC3_control XDM_FLUSH failed ....! \n");
            eError = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
        OMX_CHECK(eError == OMX_ErrorNone, eError);
        do {
            pVidDecComp->tOutBufDesc->numBufs = 0;
            pVidDecComp->tInBufDesc->numBufs = 0;
            status =  VIDDEC3_process(pVidDecComp->pDecHandle, (XDM2_BufDesc *)pVidDecComp->tInBufDesc,
                                (XDM2_BufDesc *)pVidDecComp->tOutBufDesc,
                                (VIDDEC3_InArgs *)pVidDecComp->pDecInArgs,
                                (VIDDEC3_OutArgs *)pVidDecComp->pDecOutArgs);

            if( status != XDM_EFAIL ) {
                /* Send the buffers out */
                i = 0;
                while( pDecOutArgs->outputID[i] ) {
                    pDupBufHeader = (OMX_BUFFERHEADERTYPE *)pDecOutArgs->outputID[i];
                    if( pVidDecComp->bSupportDecodeOrderTimeStamp == OMX_TRUE ) {
                        OSAL_ReadFromPipe(pVidDecComp->pTimeStampStoragePipe, &(pDupBufHeader->nTimeStamp),
                                        sizeof(OMX_TICKS), &(nActualSize), OSAL_NO_SUSPEND);
                    }

                    activeFrameRegion[0] = pDecOutArgs->displayBufs.bufDesc[0].activeFrameRegion;
                    activeFrameRegion[1].bottomRight.y = (activeFrameRegion[0].bottomRight.y) / 2;
                    activeFrameRegion[1].bottomRight.x = activeFrameRegion[0].bottomRight.x;

                    pDupBufHeader->nOffset = (activeFrameRegion[0].topLeft.y * nStride)
                                            + activeFrameRegion[0].topLeft.x;

                    nLumaFilledLen = (nStride * (pVidDecComp->t2DBufferAllocParams[OMX_VIDDEC_OUTPUT_PORT].nHeight))
                                    - (nStride * (activeFrameRegion[0].topLeft.y))
                                    - activeFrameRegion[0].topLeft.x;

                    nChromaFilledLen = (nStride * (activeFrameRegion[1].bottomRight.y))
                                        - nStride + activeFrameRegion[1].bottomRight.x;

                    pDupBufHeader->nFilledLen = nLumaFilledLen + nChromaFilledLen;

                    if( pVidDecComp->bSupportSkipGreyOutputFrames ) {
                        if( pDecOutArgs->displayBufs.bufDesc[0].frameType == IVIDEO_I_FRAME ||
                            pDecOutArgs->displayBufs.bufDesc[0].frameType == IVIDEO_IDR_FRAME ) {
                            pVidDecComp->bSyncFrameReady = OMX_TRUE;
                        }
                    }
                    grallocHandle = (IMG_native_handle_t*)(pDupBufHeader->pBuffer);
                    pVidDecComp->grallocModule->unlock((gralloc_module_t const *) pVidDecComp->grallocModule, (buffer_handle_t)grallocHandle);

                    if( pVidDecComp->bSyncFrameReady == OMX_TRUE ) {
                        // Send the Output buffer to Base component
                        pVidDecComp->sBase.pPvtData->fpDioSend(hComponent,
                                                            OMX_VIDDEC_OUTPUT_PORT, pDupBufHeader);
                    }
                    pDecOutArgs->outputID[i] = 0;
                    i++;
                }
                i = 0;
                while( pDecOutArgs->freeBufID[i] != 0 ) {
                    pDupBufHeader = (OMX_BUFFERHEADERTYPE *)pDecOutArgs->freeBufID[i];
                    if( pDupBufHeader ) {
                        pDupBufHeader->nOffset = 0;
                        pDupBufHeader->nFilledLen = 0;
                        grallocHandle = (IMG_native_handle_t*)(pDupBufHeader->pBuffer);
                        pVidDecComp->grallocModule->unlock((gralloc_module_t const *) pVidDecComp->grallocModule,
                                                                (buffer_handle_t)grallocHandle);
                        pVidDecComp->sBase.pPvtData->fpDioCancel(hComponent,
                                                        OMX_VIDDEC_OUTPUT_PORT, pDupBufHeader);
                    }
                i++;
                }
            }
        } while( status != XDM_EFAIL );
    }
    if( pVidDecComp->bSupportDecodeOrderTimeStamp == OMX_TRUE ) {
        OSAL_ClearPipe(pVidDecComp->pTimeStampStoragePipe);
    }
    if( pLastOutBufHeader != NULL ) {
        pLastOutBufHeader->nFlags |= OMX_BUFFERFLAG_EOS;
        grallocHandle = (IMG_native_handle_t*)(pLastOutBufHeader->pBuffer);
        pVidDecComp->grallocModule->unlock((gralloc_module_t const *) pVidDecComp->grallocModule,
                                            (buffer_handle_t)grallocHandle);
        pVidDecComp->sBase.pPvtData->fpDioSend(hComponent, OMX_VIDDEC_OUTPUT_PORT, pLastOutBufHeader);
    }
    if( pInBufHeader != NULL ) {
        pVidDecComp->sBase.pPvtData->fpDioSend(hComponent, OMX_VIDDEC_INPUT_PORT, pInBufHeader);
        /* Send the EOS event to client */
        pVidDecComp->sBase.fpReturnEventNotify(hComponent, OMX_EventBufferFlag,
                                        OMX_VIDDEC_OUTPUT_PORT, OMX_BUFFERFLAG_EOS, NULL);
    }
    pVidDecComp->nFrameCounter = 0;
    if( pVidDecComp->bSupportSkipGreyOutputFrames ) {
        pVidDecComp->bSyncFrameReady = OMX_FALSE;
    }
    pVidDecComp->nOutbufInUseFlag = 0;
    pVidDecComp->nFatalErrorGiven = 0;

EXIT:
    if( pVidDecComp->bSupportDecodeOrderTimeStamp == OMX_TRUE ) {
        //Clear the pipe, to discard the stale messages
        OSAL_ERROR    err = OSAL_ClearPipe(pVidDecComp->pTimeStampStoragePipe);
        if( err != OSAL_ErrNone ) {
            /* if pipe clear fails, nothing can be done, just put error trace */
            OSAL_ErrorTrace("\npipe clear failed");
        }
    }
    return (eError);
}

/* ==========================================================================*/
/**
 * @fn Calc_InbufSize()
 *                     This method Calcullates Buffer size given width and
 *                     height of buffer
 *
 *  @param [in] width  : Width of the buffer
 *  @param [in] height : Height of the buffer
 *
 */
/* ==========================================================================*/
OMX_U32 Calc_InbufSize(OMX_U32 width, OMX_U32 height)
{
    return (width * height);      //To be changed.
}

OMX_ERRORTYPE OMXVidDec_SetInPortDef(OMX_HANDLETYPE hComponent,
                                         OMX_PARAM_PORTDEFINITIONTYPE *pPortDefs)
{
    OMX_ERRORTYPE                   eError = OMX_ErrorNone;
    OMX_VIDEO_CODINGTYPE            currentCompressionType, desiredCompressionType;
    OMX_U32                         nFrameWidth, nFrameHeight;
    OMX_PARAM_PORTDEFINITIONTYPE   *pInputPortDef = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE   *pOutputPortDef = NULL;
    PaddedBuffParams                tOutBufParams;
    OMX_U32                         i=0;
    OMX_U32                         bFound = 0;
    OMX_COMPONENTTYPE              *pHandle = NULL;
    OMXVidDecComp                  *pVidDecComp = NULL;

    /*! Initialize pointers and variables */
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp = (OMXVidDecComp *)pHandle->pComponentPrivate;
    pInputPortDef = &(pVidDecComp->sBase.pPorts[OMX_VIDDEC_INPUT_PORT]->sPortDef);
    pOutputPortDef = &(pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef);
    nFrameWidth = pPortDefs->format.video.nFrameWidth;
    nFrameHeight = pPortDefs->format.video.nFrameHeight;
    if( nFrameWidth & 0x0F ) {
        nFrameWidth = nFrameWidth + 16 - (nFrameWidth & 0x0F);
    }
    if( nFrameHeight & 0x1F ) {
        nFrameHeight = nFrameHeight + 32 - (nFrameHeight & 0x1F);
    }
    currentCompressionType = pInputPortDef->format.video.eCompressionFormat;
    desiredCompressionType = pPortDefs->format.video.eCompressionFormat;
    /*! In case there is change in Compression type */
    if( currentCompressionType != desiredCompressionType ) {
        /* De-initialize the current codec */
        pVidDecComp->fpDeinit_Codec(hComponent);
        /* Call specific component init depending upon the
        eCompressionFormat set. */
        i=0;
        while( NULL != DecoderList[i].eCompressionFormat ) {
            if( DecoderList[i].eCompressionFormat
                    == desiredCompressionType ) {
                /* Component found */
                bFound = 1;
                break;
            }
            i++;
        }
        if( bFound == 0 ) {
            OSAL_ErrorTrace("Unsupported Compression format given in port definition");
            eError = OMX_ErrorUnsupportedSetting;
            goto EXIT;
        }
        /* Call the Specific Decoder Init function and Initialize Params */
        eError = DecoderList[i].fpDecoderComponentInit(hComponent);
        OMX_CHECK(eError == OMX_ErrorNone, eError);
        OMXVidDec_InitDecoderParams(hComponent, pHandle->pComponentPrivate);
        strcpy((char *)pVidDecComp->tComponentRole.cRole,
                    (char *)DecoderList[i].cRole);
        }  /* End of if condition for change in codec type */

        /*! set the Actual values of an Input port */
        pInputPortDef->nBufferCountActual = pPortDefs->nBufferCountActual;
        pInputPortDef->format = pPortDefs->format;
        pInputPortDef->nBufferSize = Calc_InbufSize(nFrameWidth, nFrameHeight);
        pVidDecComp->tCropDimension.nTop = 0;
        pVidDecComp->tCropDimension.nLeft = 0;
        pVidDecComp->tCropDimension.nWidth = pInputPortDef->format.video.nFrameWidth;
        pVidDecComp->tCropDimension.nHeight = pInputPortDef->format.video.nFrameHeight;

        /*! Set o/p port details according to width/height set at i/p Port. */
        pOutputPortDef->format.video.nFrameWidth = pInputPortDef->format.video.nFrameWidth;
        pOutputPortDef->format.video.nFrameHeight = pInputPortDef->format.video.nFrameHeight;
        OMXVidDec_Set2DBuffParams(hComponent, pHandle->pComponentPrivate);
        pOutputPortDef->nBufferSize = pOutputPortDef->format.video.nStride *
                                ((pVidDecComp->t2DBufferAllocParams[OMX_VIDDEC_OUTPUT_PORT].nHeight * 3) >> 1);

        tOutBufParams = pVidDecComp->fpCalc_OubuffDetails(hComponent, nFrameWidth, nFrameHeight);
        pOutputPortDef->nBufferCountMin = tOutBufParams.nBufferCountMin;
        pOutputPortDef->nBufferCountActual = tOutBufParams.nBufferCountActual;

        /*! Set the Static Params (Decoder Specific) */
        pVidDecComp->fpSet_StaticParams(hComponent, pVidDecComp->pDecStaticParams);
        pVidDecComp->pDecStaticParams->maxHeight = nFrameHeight;
        pVidDecComp->pDecStaticParams->maxWidth = nFrameWidth;
        if( pOutputPortDef->nBufferCountActual < pOutputPortDef->nBufferCountMin ) {
            pOutputPortDef->nBufferCountActual = pOutputPortDef->nBufferCountMin;
        }

EXIT:
    return (eError);

}

OMX_ERRORTYPE OMXVidDec_SetOutPortDef(OMXVidDecComp *pVidDecComp,
                                          OMX_PARAM_PORTDEFINITIONTYPE *pPortDefs)
{
    OMX_ERRORTYPE                   eError = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE   *pOutputPortDef = NULL;
    OMX_PTR                        *pBufParams = NULL;
    OMX_U32                         nOutPort = OMX_VIDDEC_OUTPUT_PORT;
    OMX_U32                         nNumBuffers = 0;

    pOutputPortDef = &(pVidDecComp->sBase.pPorts[nOutPort]->sPortDef);

    /*! Set Values to output port based on input parameter */
    pOutputPortDef->nBufferCountActual = pPortDefs->nBufferCountActual;
    pOutputPortDef->format             = pPortDefs->format;
    pOutputPortDef->format.video.nSliceHeight
                            = pOutputPortDef->format.video.nFrameHeight;
    pOutputPortDef->nBufferSize = pOutputPortDef->format.video.nStride *
                            ((pVidDecComp->t2DBufferAllocParams[nOutPort].nHeight * 3) >> 1);

EXIT:
    return (eError);
}


OMX_ERRORTYPE OMXVidDec_HandleFirstFrame(OMX_HANDLETYPE hComponent,
                                             OMX_BUFFERHEADERTYPE * *ppInBufHeader)
{
    OMX_ERRORTYPE                   eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE               *pHandle = NULL;
    OMX_U32                         nFrameWidth, nFrameHeight, nFrameWidthNew, nFrameHeightNew, nFrameRate, nFrameRateNew;
    OMXVidDecComp                   *pVidDecComp = NULL;
    XDAS_Int32                      status = 0;
    IVIDDEC3_Status                 *pDecStatus = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE    *pOutputPortDef = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE    *pInputPortDef = NULL;
    PaddedBuffParams                tOutBufParams;
    OMX_BUFFERHEADERTYPE            *pInBufHeader;
    OMX_BOOL                        bPortReconfigRequiredForPadding = OMX_FALSE;
    OMX_CONFIG_RECTTYPE             *p2DOutBufAllocParam = NULL;

    OMX_BOOL    bSendPortReconfigForScale = OMX_FALSE;
    OMX_U32     nScale, nScaleRem, nScaleQ16Low, nScaleWidth, nScaleHeight;
    OMX_U64     nScaleQ16 = 0;

    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp = (OMXVidDecComp *)pHandle->pComponentPrivate;
    pOutputPortDef = &(pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef);
    pInputPortDef = &(pVidDecComp->sBase.pPorts[OMX_VIDDEC_INPUT_PORT]->sPortDef);
    pVidDecComp->nOutPortReconfigRequired = 0;
    p2DOutBufAllocParam = &(pVidDecComp->t2DBufferAllocParams[OMX_VIDDEC_OUTPUT_PORT]);

    /*! Call the Codec Control call to get Status from Codec */
    OMX_CHECK(((pVidDecComp->pDecDynParams != NULL) && (pVidDecComp->pDecStatus != NULL)), OMX_ErrorBadParameter);
    status = VIDDEC3_control(pVidDecComp->pDecHandle, XDM_GETSTATUS, pVidDecComp->pDecDynParams, pVidDecComp->pDecStatus);
    if( status != VIDDEC3_EOK ) {
        OSAL_ErrorTrace("Error in Codec Control Call for GETSTATUS");
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    OMX_CHECK(eError == OMX_ErrorNone, eError);
    if( pVidDecComp->fpHandle_CodecGetStatus != NULL ) {
        eError = pVidDecComp->fpHandle_CodecGetStatus(hComponent);
    }
    pDecStatus = (IVIDDEC3_Status *)(pVidDecComp->pDecStatus);
    nFrameWidth = pOutputPortDef->format.video.nFrameWidth;
    nFrameHeight = pOutputPortDef->format.video.nFrameHeight;
    nFrameWidthNew = (OMX_U32)pDecStatus->outputWidth;
    nFrameHeightNew = (OMX_U32)pDecStatus->outputHeight;
    nFrameRate = pOutputPortDef->format.video.xFramerate >> 16;

    OMX_CHECK(pVidDecComp->nFrameRateDivisor != 0, OMX_ErrorBadParameter);
    nFrameRateNew = (OMX_U32)(pDecStatus->frameRate / pVidDecComp->nFrameRateDivisor);
    /*Set 200 as max cap, as if clip with incorrect setting is present in sdcard
    it breaks havoc on thumbnail generation */
    if((nFrameRateNew == 0) || (nFrameRateNew > 200)) {
        OSAL_ErrorTrace("Codec Returned spurious FrameRate Value - %d Setting Back to - %d",
                    nFrameRateNew, nFrameRate);
                    nFrameRateNew = nFrameRate;
    }

    if( nFrameWidth & 0x0F ) {
        nFrameWidth = nFrameWidth + 16 - (nFrameWidth & 0x0F);
    }
    if( nFrameHeight & 0x1F ) {
        nFrameHeight = nFrameHeight + 32 - (nFrameHeight & 0x1F);
    }
    if( nFrameWidthNew & 0x0F ) {
        nFrameWidthNew = nFrameWidthNew + 16 - (nFrameWidthNew & 0x0F);
    }
    if( nFrameHeightNew & 0x1F ) {
        nFrameHeightNew = nFrameHeightNew + 32 - (nFrameHeightNew & 0x1F);
    }
    if( pVidDecComp->bUsePortReconfigForPadding == OMX_TRUE ) {
        if( pOutputPortDef->format.video.nFrameWidth != p2DOutBufAllocParam->nWidth
                || pOutputPortDef->format.video.nFrameHeight != p2DOutBufAllocParam->nHeight ) {
            bPortReconfigRequiredForPadding = OMX_TRUE;
        }
        nFrameWidth = pVidDecComp->pDecStaticParams->maxWidth;
        nFrameHeight = pVidDecComp->pDecStaticParams->maxHeight;
    }

    tOutBufParams = pVidDecComp->fpCalc_OubuffDetails(hComponent,
                (OMX_U32)pDecStatus->outputWidth, (OMX_U32)pDecStatus->outputHeight);

    /*! Check whether the height and width reported by codec matches
    *  that of output port */
    if( nFrameHeightNew != nFrameHeight || nFrameWidthNew != nFrameWidth
            || bPortReconfigRequiredForPadding == OMX_TRUE ||
            pOutputPortDef->nBufferCountMin < tOutBufParams.nBufferCountMin ||
            nFrameRate < nFrameRateNew ) {     /* Compare the min againt the older min buffer count
        since parameters like display delay also gets set according to ref frame. */
        /*! Since the dimensions does not match trigger port reconfig */
        pVidDecComp->nOutPortReconfigRequired = 1;
        pVidDecComp->nCodecRecreationRequired = 1;
        /* Return back the Input buffer headers Note that the output header
        * will be cancelled later so no need to cancel it here */
        if( ppInBufHeader != NULL ) {
            pInBufHeader = *(ppInBufHeader);
            pVidDecComp->sBase.pPvtData->fpDioCancel(hComponent, OMX_VIDDEC_INPUT_PORT,
                                                pInBufHeader);
            pInBufHeader = NULL;
        }
        /*! Change port definition to match with what codec reports */
        pInputPortDef->format.video.nFrameHeight = (OMX_U32)pDecStatus->outputHeight;
        pInputPortDef->format.video.nFrameWidth = (OMX_U32)pDecStatus->outputWidth;
        pOutputPortDef->format.video.nFrameHeight = (OMX_U32)pDecStatus->outputHeight;
        pOutputPortDef->format.video.nFrameWidth = (OMX_U32)pDecStatus->outputWidth;
        pOutputPortDef->format.video.nSliceHeight = (OMX_U32)pDecStatus->outputHeight;
        if( nFrameRate < nFrameRateNew ) {
            pOutputPortDef->format.video.xFramerate = nFrameRateNew << 16;
            pVidDecComp->tVideoParams[OMX_VIDDEC_OUTPUT_PORT].xFramerate = nFrameRateNew;
        }
        pVidDecComp->tCropDimension.nWidth = (OMX_U32)pDecStatus->outputWidth;
        pVidDecComp->tCropDimension.nHeight = (OMX_U32)pDecStatus->outputHeight;
        tOutBufParams = pVidDecComp->fpCalc_OubuffDetails(hComponent,
                                                (OMX_U32)pDecStatus->outputWidth,
                                                (OMX_U32)pDecStatus->outputHeight);
        pOutputPortDef->nBufferCountMin = tOutBufParams.nBufferCountMin;
        pOutputPortDef->nBufferCountActual = tOutBufParams.nBufferCountActual;
        OMXVidDec_Set2DBuffParams(hComponent, pHandle->pComponentPrivate);

        if( pOutputPortDef->format.video.nStride != OMX_VIDDEC_TILER_STRIDE ) {
            pOutputPortDef->format.video.nStride = p2DOutBufAllocParam->nWidth;
        }
        pOutputPortDef->nBufferSize  = pOutputPortDef->format.video.nStride *
                                    ((pVidDecComp->t2DBufferAllocParams[OMX_VIDDEC_OUTPUT_PORT].nHeight * 3) >> 1);
        if( pVidDecComp->bUsePortReconfigForPadding == OMX_TRUE ) {
            pInputPortDef->format.video.nFrameHeight = p2DOutBufAllocParam->nHeight;
            pInputPortDef->format.video.nFrameWidth = p2DOutBufAllocParam->nWidth;
            pOutputPortDef->format.video.nFrameHeight = p2DOutBufAllocParam->nHeight;
            pOutputPortDef->format.video.nFrameWidth = p2DOutBufAllocParam->nWidth;
        }
    }

    if( pVidDecComp->nOutPortReconfigRequired == 1 ) {
        /*! Notify to Client change in output port settings */
        eError = pVidDecComp->sBase.fpReturnEventNotify(hComponent,
        OMX_EventPortSettingsChanged,
        OMX_VIDDEC_OUTPUT_PORT, 0, NULL);
    } else if( pDecStatus->sampleAspectRatioHeight != 0 && pDecStatus->sampleAspectRatioWidth != 0 ) {
        nScaleWidth = (OMX_U32)pDecStatus->sampleAspectRatioWidth;
        nScaleHeight = (OMX_U32)pDecStatus->sampleAspectRatioHeight;
        nScale = nScaleWidth / nScaleHeight;
        if( nScale >= 1 ) {
            nScaleRem = nScaleWidth % nScaleHeight;
            nScaleQ16Low = 0xFFFF * nScaleRem / nScaleHeight;
            nScaleQ16 = nScale << 16;
            nScaleQ16 |= nScaleQ16Low;
            if( pVidDecComp->tScaleParams.xWidth != nScaleQ16
                    || pVidDecComp->tScaleParams.xHeight != 0x10000 ) {
                pVidDecComp->tScaleParams.xWidth = nScaleQ16;
                pVidDecComp->tScaleParams.xHeight = 0x10000;
                bSendPortReconfigForScale = OMX_TRUE;
            }
        } else {
            nScale = nScaleHeight / nScaleWidth;
            nScaleRem =  nScaleHeight % nScaleWidth;
            nScaleQ16Low = 0xFFFF * nScaleRem / nScaleWidth;
            nScaleQ16 = nScale << 16;
            nScaleQ16 |= nScaleQ16Low;
            if( pVidDecComp->tScaleParams.xWidth != 0x10000
                    || pVidDecComp->tScaleParams.xHeight != nScaleQ16 ) {
                pVidDecComp->tScaleParams.xWidth = 0x10000;
                pVidDecComp->tScaleParams.xHeight = nScaleQ16;
                bSendPortReconfigForScale = OMX_TRUE;
            }
        }
        if( bSendPortReconfigForScale == OMX_TRUE ) {
            /*! Notify to Client change in output port settings */
            eError = pVidDecComp->sBase.fpReturnEventNotify(hComponent,
                                    OMX_EventPortSettingsChanged, OMX_VIDDEC_OUTPUT_PORT, OMX_IndexConfigCommonScale, NULL);
                                    bSendPortReconfigForScale = OMX_FALSE;
        }
    }

EXIT:
    return (eError);

}

OMX_ERRORTYPE OMXVidDec_HandleCodecProcError(OMX_HANDLETYPE hComponent,
                                                 OMX_BUFFERHEADERTYPE * *ppInBufHeader,
                                                 OMX_BUFFERHEADERTYPE * *ppOutBufHeader)
{
    OMX_ERRORTYPE                   eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE               *pHandle = NULL;
    OMX_U32                         nFrameWidth, nFrameHeight, nFrameWidthNew, nFrameHeightNew;
    OMXVidDecComp                   *pVidDecComp = NULL;
    XDAS_Int32                      status = 0;
    IVIDDEC3_Status                 *pDecStatus = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE    *pOutputPortDef = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE    *pInputPortDef = NULL;
    PaddedBuffParams                tOutBufParams;
    OMX_BUFFERHEADERTYPE            *pInBufHeader = *(ppInBufHeader);
    OMX_BUFFERHEADERTYPE            *pDupBufHeader;
    OMX_BUFFERHEADERTYPE            *pNewOutBufHeader = NULL;
    OMX_BUFFERHEADERTYPE            *pOutBufHeader = *(ppOutBufHeader);
    OMX_U32                         ii=0;
    XDM_Rect                        activeFrameRegion[2];
    OMX_U32                         nStride = OMX_VIDDEC_TILER_STRIDE;
    OMX_U32                         nLumaFilledLen, nChromaFilledLen;
    OMX_BOOL                        bPortReconfigRequiredForPadding = OMX_FALSE;
    OMX_CONFIG_RECTTYPE             *p2DOutBufAllocParam = NULL;
    IMG_native_handle_t*            grallocHandle;

    /* Initialize pointers */
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp = (OMXVidDecComp *)pHandle->pComponentPrivate;
    pOutputPortDef = &(pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef);
    pInputPortDef = &(pVidDecComp->sBase.pPorts[OMX_VIDDEC_INPUT_PORT]->sPortDef);
    p2DOutBufAllocParam = &(pVidDecComp->t2DBufferAllocParams[OMX_VIDDEC_OUTPUT_PORT]);

    /*! Call the Codec Status function to know cause of error */
    OMX_CHECK(((pVidDecComp->pDecDynParams != NULL) && (pVidDecComp->pDecStatus != NULL)), OMX_ErrorBadParameter);
    status = VIDDEC3_control(pVidDecComp->pDecHandle, XDM_GETSTATUS, pVidDecComp->pDecDynParams, pVidDecComp->pDecStatus);

    /* Check whether the Codec Status call was succesful */
    if( status != VIDDEC3_EOK ) {
        OSAL_ErrorTrace("VIDDEC3_control XDM_GETSTATUS failed");
        if( pVidDecComp->pDecDynParams->decodeHeader
                != pVidDecComp->nDecoderMode ) {
            // Return the Input and Output buffer header
            grallocHandle = (IMG_native_handle_t*)(pOutBufHeader->pBuffer);
            pVidDecComp->grallocModule->unlock((gralloc_module_t const *) pVidDecComp->grallocModule,
                                                    (buffer_handle_t)grallocHandle);
            pVidDecComp->sBase.pPvtData->fpDioCancel(hComponent,
                                        OMX_VIDDEC_OUTPUT_PORT,
                                        pOutBufHeader);

            pVidDecComp->sBase.pPvtData->fpDioCancel(hComponent,
                                        OMX_VIDDEC_INPUT_PORT,
                                        pInBufHeader);
            /*! Make Input buffer header pointer NULL */
            pInBufHeader = NULL;
        }
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    OMX_CHECK(eError == OMX_ErrorNone, eError);

    pDecStatus = (IVIDDEC3_Status *)(pVidDecComp->pDecStatus);
    nFrameWidth = pOutputPortDef->format.video.nFrameWidth;
    nFrameHeight = pOutputPortDef->format.video.nFrameHeight;
    nFrameWidthNew = pDecStatus->outputWidth;
    nFrameHeightNew = pDecStatus->outputHeight;

    if( nFrameWidth & 0x0F ) {
        nFrameWidth = nFrameWidth + 16 - (nFrameWidth & 0x0F);
    }
    if( nFrameHeight & 0x1F ) {
        nFrameHeight = nFrameHeight + 32 - (nFrameHeight & 0x1F);
    }
    if( nFrameWidthNew & 0x0F ) {
        nFrameWidthNew = nFrameWidthNew + 16 - (nFrameWidthNew & 0x0F);
    }
    if( nFrameHeightNew & 0x1F ) {
        nFrameHeightNew = nFrameHeightNew + 32 - (nFrameHeightNew & 0x1F);
    }

    if( pVidDecComp->bUsePortReconfigForPadding == OMX_TRUE ) {
        if( pOutputPortDef->format.video.nFrameWidth != p2DOutBufAllocParam->nWidth
                || pOutputPortDef->format.video.nFrameHeight != p2DOutBufAllocParam->nHeight ) {
            bPortReconfigRequiredForPadding = OMX_TRUE;
        }
        nFrameWidth = pVidDecComp->pDecStaticParams->maxWidth;
        nFrameHeight = pVidDecComp->pDecStaticParams->maxHeight;
    }
    /*! Check whether the height and width reported by codec matches
    *  that of output port */
    if( nFrameHeightNew != nFrameHeight || nFrameWidthNew != nFrameWidth
            || bPortReconfigRequiredForPadding == OMX_TRUE ) {
        pVidDecComp->nOutPortReconfigRequired = 1;
        pVidDecComp->nCodecRecreationRequired = 1;
    }

    if( pVidDecComp->fpHandle_ExtendedError != NULL ) {
        eError = pVidDecComp->fpHandle_ExtendedError(hComponent);
    }

    if( pVidDecComp->nOutPortReconfigRequired == 1 ) {
        pNewOutBufHeader = (OMX_BUFFERHEADERTYPE *) pVidDecComp->pDecOutArgs->outputID[0];
        if( pNewOutBufHeader != NULL ) {
            pVidDecComp->tCropDimension.nWidth = pDecStatus->outputWidth;
            pVidDecComp->tCropDimension.nHeight = pDecStatus->outputHeight;
            nStride = pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef.format.video.nStride;
            activeFrameRegion[0] = pVidDecComp->pDecOutArgs->displayBufs.bufDesc[0].activeFrameRegion;
            activeFrameRegion[1].bottomRight.y = (activeFrameRegion[0].bottomRight.y) / 2;
            activeFrameRegion[1].bottomRight.x = activeFrameRegion[0].bottomRight.x;

            // Offset = (rows*stride) + x-offset
            pNewOutBufHeader->nOffset = (activeFrameRegion[0].topLeft.y * nStride)
                                        + activeFrameRegion[0].topLeft.x;
            // FilledLen
            nLumaFilledLen
                    = (nStride * (pVidDecComp->t2DBufferAllocParams[OMX_VIDDEC_OUTPUT_PORT].nHeight))
                        - (nStride * (activeFrameRegion[0].topLeft.y))
                        - activeFrameRegion[0].topLeft.x;

            nChromaFilledLen
                        = (nStride * (activeFrameRegion[1].bottomRight.y))
                        - nStride + activeFrameRegion[1].bottomRight.x;
            pNewOutBufHeader->nFilledLen = nLumaFilledLen + nChromaFilledLen;
            grallocHandle = (IMG_native_handle_t*)(pNewOutBufHeader->pBuffer);
            pVidDecComp->grallocModule->unlock((gralloc_module_t const *) pVidDecComp->grallocModule,
                                                (buffer_handle_t)grallocHandle);
            pVidDecComp->sBase.pPvtData->fpDioSend(hComponent, OMX_VIDDEC_OUTPUT_PORT,
                                                pNewOutBufHeader);
        }

        if( pVidDecComp->nOutbufInUseFlag == 0
                && pVidDecComp->pDecDynParams->decodeHeader != pVidDecComp->nDecoderMode ) {
            grallocHandle = (IMG_native_handle_t*)(pOutBufHeader->pBuffer);
            pVidDecComp->grallocModule->unlock((gralloc_module_t const *) pVidDecComp->grallocModule,
                                                (buffer_handle_t)grallocHandle);
            pVidDecComp->sBase.pPvtData->fpDioCancel(hComponent,
                                        OMX_VIDDEC_OUTPUT_PORT, pOutBufHeader); // This buffer header is freed afterwards.
        } else {
            pOutBufHeader = NULL;
        }
        if( pVidDecComp->pDecDynParams->decodeHeader != pVidDecComp->nDecoderMode ) {
            pVidDecComp->sBase.pPvtData->fpDioCancel(hComponent,
                                    OMX_VIDDEC_INPUT_PORT, pInBufHeader);

            while( pVidDecComp->pDecOutArgs->freeBufID[ii] != 0 ) {
                pDupBufHeader = (OMX_BUFFERHEADERTYPE *)pVidDecComp->pDecOutArgs->freeBufID[ii++];
                if( pDupBufHeader != pOutBufHeader && pDupBufHeader != pNewOutBufHeader ) {
                    grallocHandle = (IMG_native_handle_t*)(pDupBufHeader->pBuffer);
                    pVidDecComp->grallocModule->unlock((gralloc_module_t const *) pVidDecComp->grallocModule,
                                                        (buffer_handle_t)grallocHandle);
                    pVidDecComp->sBase.pPvtData->fpDioCancel(hComponent,
                                                OMX_VIDDEC_OUTPUT_PORT, pDupBufHeader);
                }
            }
        }

        /*! Notify to Client change in output port settings */
        eError = pVidDecComp->sBase.fpReturnEventNotify(hComponent,
                                    OMX_EventPortSettingsChanged,
                                    OMX_VIDDEC_OUTPUT_PORT, 0, NULL);
    }
    if( pVidDecComp->nOutPortReconfigRequired == 0 ) {
        if( pVidDecComp->pDecOutArgs->extendedError & 0x8000 ) {
            eError = OMX_ErrorFormatNotDetected;
            if( pVidDecComp->pDecDynParams->decodeHeader != pVidDecComp->nDecoderMode ) {
                if( pVidDecComp->nOutbufInUseFlag == 0 ) {
                    grallocHandle = (IMG_native_handle_t*)(pOutBufHeader->pBuffer);
                    pVidDecComp->grallocModule->unlock((gralloc_module_t const *) pVidDecComp->grallocModule,
                                                        (buffer_handle_t)grallocHandle);
                    pVidDecComp->sBase.pPvtData->fpDioCancel(hComponent,
                                                OMX_VIDDEC_OUTPUT_PORT, pOutBufHeader); // This buffer header is freed afterwards.
                }
                pVidDecComp->sBase.pPvtData->fpDioCancel(hComponent,
                                                OMX_VIDDEC_INPUT_PORT, pInBufHeader);
                while( pVidDecComp->pDecOutArgs->freeBufID[ii] != 0 ) {
                    pDupBufHeader = (OMX_BUFFERHEADERTYPE *)pVidDecComp->pDecOutArgs->freeBufID[ii++];
                    if( pOutBufHeader != pDupBufHeader ) {
                        grallocHandle = (IMG_native_handle_t*)(pDupBufHeader->pBuffer);
                        pVidDecComp->grallocModule->unlock((gralloc_module_t const *) pVidDecComp->grallocModule,
                                                            (buffer_handle_t)grallocHandle);
                        pVidDecComp->sBase.pPvtData->fpDioCancel(hComponent,
                                                OMX_VIDDEC_OUTPUT_PORT, pDupBufHeader);
                    }
                }
            }
            pVidDecComp->nFatalErrorGiven = 1;
        }
    }

    if( nFrameHeightNew != nFrameHeight || nFrameWidthNew != nFrameWidth
            || bPortReconfigRequiredForPadding == OMX_TRUE ) {
        /*Return back the locked buffers before changing the port definition */
        OMXVidDec_HandleFLUSH_EOS(hComponent, NULL, NULL);
        /*! Change Port Definition */
        pInputPortDef->format.video.nFrameHeight = pDecStatus->outputHeight;
        pInputPortDef->format.video.nFrameWidth =  pDecStatus->outputWidth;

        pOutputPortDef->format.video.nFrameHeight = pDecStatus->outputHeight;
        pOutputPortDef->format.video.nFrameWidth =  pDecStatus->outputWidth;
        pOutputPortDef->format.video.nSliceHeight = pDecStatus->outputHeight;
        pVidDecComp->tCropDimension.nWidth = pDecStatus->outputWidth;
        pVidDecComp->tCropDimension.nHeight = pDecStatus->outputHeight;
        tOutBufParams = pVidDecComp->fpCalc_OubuffDetails(hComponent,
                                        pDecStatus->outputWidth,
                                        pDecStatus->outputHeight);
        pOutputPortDef->nBufferCountMin = tOutBufParams.nBufferCountMin;
        pOutputPortDef->nBufferCountActual = tOutBufParams.nBufferCountActual;
        OMXVidDec_Set2DBuffParams(hComponent, pHandle->pComponentPrivate);

        if( pOutputPortDef->format.video.nStride != OMX_VIDDEC_TILER_STRIDE ) {
            pOutputPortDef->format.video.nStride = p2DOutBufAllocParam->nWidth;
        }
        pOutputPortDef->nBufferSize  = pOutputPortDef->format.video.nStride *
                                ((pVidDecComp->t2DBufferAllocParams[OMX_VIDDEC_OUTPUT_PORT].nHeight * 3) >> 1);
        if( pVidDecComp->bUsePortReconfigForPadding == OMX_TRUE ) {
            pInputPortDef->format.video.nFrameHeight = p2DOutBufAllocParam->nHeight;
            pInputPortDef->format.video.nFrameWidth = p2DOutBufAllocParam->nWidth;
            pOutputPortDef->format.video.nFrameHeight = p2DOutBufAllocParam->nHeight;
            pOutputPortDef->format.video.nFrameWidth = p2DOutBufAllocParam->nWidth;
        }
    }

EXIT:
    return (eError);
}

void OMXVidDec_CalcFilledLen(OMX_HANDLETYPE hComponent,
                                 IVIDDEC3_OutArgs *pDecOutArgs,
                                 OMX_U32 nStride)
{
    XDM_Rect                tActiveFrameRegion[2];
    OMX_BUFFERHEADERTYPE    *pOutBufHeader;
    OMX_U32                 nLumaFilledLen = 0;
    OMX_U32                 nChromaFilledLen = 0;
    OMX_COMPONENTTYPE       *pHandle = NULL;
    OMXVidDecComp           *pVidDecComp = NULL;

    /* Initialize the pointers */
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp = (OMXVidDecComp *)pHandle->pComponentPrivate;

    pOutBufHeader = (OMX_BUFFERHEADERTYPE *)pDecOutArgs->outputID[0];
    tActiveFrameRegion[0]
        = pDecOutArgs->displayBufs.bufDesc[0].activeFrameRegion;
    tActiveFrameRegion[1]
        = pDecOutArgs->displayBufs.bufDesc[1].activeFrameRegion;

    /*! Calcullate the offset to start of frame */
    pOutBufHeader->nOffset = (tActiveFrameRegion[0].topLeft.y * nStride)
                             + tActiveFrameRegion[0].topLeft.x;

    /*! Calcullate the Luma Filled Length */
    nLumaFilledLen
        = (nStride * (pVidDecComp->t2DBufferAllocParams[OMX_VIDDEC_OUTPUT_PORT].nHeight))
          - (nStride * (tActiveFrameRegion[0].topLeft.y))
          - tActiveFrameRegion[0].topLeft.x;

    /*! Calcullate the Chroma Filled Length */
    nChromaFilledLen = (nStride * (tActiveFrameRegion[1].bottomRight.y))
                       - nStride + tActiveFrameRegion[1].bottomRight.x;

    /*! Calcullate the Total Filled length */
    pOutBufHeader->nFilledLen = nLumaFilledLen + nChromaFilledLen;

    return;
}


