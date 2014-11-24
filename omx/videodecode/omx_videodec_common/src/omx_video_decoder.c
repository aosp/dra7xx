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

#define LOG_TAG "OMX_VIDDEC_COMMON"

#include <omx_video_decoder_internal.h>
#include <OMX_TI_Custom.h>

#include <osal_trace.h>
#include <memplugin.h>


#define HAL_NV12_PADDED_PIXEL_FORMAT (OMX_TI_COLOR_FormatYUV420PackedSemiPlanar - OMX_COLOR_FormatVendorStartUnused)

#define MAX_REF_FRAMES 16

/*
* Component Init
*/
OMX_ERRORTYPE OMX_ComponentInit(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE   *pHandle = NULL;
    OMXVidDecComp       *pVidDecComp = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE   *pOutputPortDef = NULL;

    pHandle = (OMX_COMPONENTTYPE *)hComponent;

    /*! Call the Common Decoder Component Init */
    eError = OMXVidDec_ComponentInit(hComponent);
    OMX_CHECK(eError == OMX_ErrorNone, eError);

    /*! Call the Decoder Specific Init as default */
    eError = DecoderList[DEFAULT_COMPOENENT].fpDecoderComponentInit(hComponent);
    OMX_CHECK(eError == OMX_ErrorNone, eError);
    pVidDecComp = (OMXVidDecComp *)pHandle->pComponentPrivate;
    OMXVidDec_InitPortDefs(hComponent, pVidDecComp);
    OMXVidDec_InitPortParams(pVidDecComp);
    OMXVidDec_Set2DBuffParams(hComponent, pVidDecComp);

    /* Call Decoder Specific function to set Static Params */
    pVidDecComp->fpSet_StaticParams(hComponent, pVidDecComp->pDecStaticParams);

    strcpy((char *)pVidDecComp->tComponentRole.cRole,
            (char *)DecoderList[DEFAULT_COMPOENENT].cRole);
    /* Set the Parse Header flag to XDM_PARSE_HEADER */
    pVidDecComp->nDecoderMode = XDM_PARSE_HEADER;

EXIT:
    return (eError);
}

/*
* Video Decoder Component Init
*/
OMX_ERRORTYPE OMXVidDec_ComponentInit(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE               eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE          *pHandle = NULL;
    OMXVidDecComp              *pVidDecComp = NULL;
    Engine_Error                errorCode;

    OMX_CHECK(hComponent != NULL, OMX_ErrorBadParameter);

    /* allocate the decoder component */
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pHandle->pComponentPrivate = (OMXVidDecComp*)OSAL_Malloc(sizeof(OMXVidDecComp));
    OMX_CHECK(pHandle->pComponentPrivate != NULL, OMX_ErrorInsufficientResources);
    OSAL_Memset(pHandle->pComponentPrivate, 0x0, sizeof(OMXVidDecComp));

    pVidDecComp = (OMXVidDecComp*)pHandle->pComponentPrivate;

    /*! Initialize the fields */
    eError = OMXVidDec_InitFields(pHandle->pComponentPrivate);
    OMX_CHECK(eError == OMX_ErrorNone, OMX_ErrorUndefined);

    strcpy(pVidDecComp->sBase.cComponentName, OMX_VIDEODECODER_COMPONENT_NAME);

    /* Initialize the base component */
    eError = OMXBase_ComponentInit(hComponent);
    OMX_CHECK(eError == OMX_ErrorNone, eError);

    /*Setting default properties.
    * PreCondition: NumOfPorts is filled and all pointers allocated*/
    eError = OMXBase_SetDefaultProperties(hComponent);
    OMX_CHECK(eError == OMX_ErrorNone, OMX_ErrorUnsupportedSetting);

    /*! Initialize function pointers for Command and Data Notify */
    pVidDecComp->sBase.fpCommandNotify = OMXVidDec_CommandNotify;
    pVidDecComp->sBase.fpDataNotify = OMXVidDec_DataNotify;
    pVidDecComp->sBase.fpXlateBuffHandle = OMXVidDec_XlateBuffHandle;

    /*! Initialize the function parameters for OMX functions */
    pHandle->SetParameter = OMXVidDec_SetParameter;
    pHandle->GetParameter = OMXVidDec_GetParameter;
    pHandle->GetConfig = OMXVidDec_GetConfig;
    pHandle->SetConfig = OMXVidDec_SetConfig;
    pHandle->ComponentDeInit = OMXVidDec_ComponentDeinit;
    pHandle->GetExtensionIndex = OMXVidDec_GetExtensionIndex;

    /*! Open instance of Codec engine */
    pVidDecComp->ce = Engine_open(engineName, NULL, &errorCode);
    pVidDecComp->pDecHandle = NULL;
    pVidDecComp->nFrameCounter = 0;
    pVidDecComp->bSyncFrameReady = OMX_FALSE;
    pVidDecComp->nOutbufInUseFlag = 0;
    pVidDecComp->nCodecRecreationRequired = 0;
    pVidDecComp->nOutPortReconfigRequired = 0;
    pVidDecComp->nFatalErrorGiven = 0;
    pVidDecComp->bInputBufferCancelled = 0;
    /*! Initialize the Framerate divisor to 1000 as codec is supposed provide framerate after by multiplying with 1000 */
    pVidDecComp->nFrameRateDivisor = 1000;
    pVidDecComp->tCropDimension.nTop = 0;
    pVidDecComp->tCropDimension.nLeft = 0;
    pVidDecComp->tScaleParams.xWidth = 0x10000;
    pVidDecComp->tScaleParams.xHeight = 0x10000;
    pVidDecComp->bUsePortReconfigForCrop = OMX_TRUE;
    pVidDecComp->bUsePortReconfigForPadding = OMX_TRUE;
    pVidDecComp->bSupportDecodeOrderTimeStamp = OMX_FALSE;
    pVidDecComp->bSupportSkipGreyOutputFrames = OMX_TRUE;

    /*Optimize this pipe to be created only if decode timestamps is requested. */
    OSAL_CreatePipe(&(pVidDecComp->pTimeStampStoragePipe), MAX_REF_FRAMES * sizeof(OMX_TICKS),
                    sizeof(OMX_TICKS), 1);

    pVidDecComp->tInBufDesc = (XDM2_BufDesc*)memplugin_alloc(sizeof(XDM2_BufDesc), 1, MEM_CARVEOUT, 0, 0);
    OMX_CHECK(pVidDecComp->tInBufDesc != NULL, OMX_ErrorInsufficientResources);
    OSAL_Memset(pVidDecComp->tInBufDesc, 0x0, sizeof(XDM2_BufDesc));

    pVidDecComp->tOutBufDesc = (XDM2_BufDesc*)memplugin_alloc(sizeof(XDM2_BufDesc), 1, MEM_CARVEOUT, 0, 0);
    OMX_CHECK(pVidDecComp->tOutBufDesc != NULL, OMX_ErrorInsufficientResources);
    OSAL_Memset(pVidDecComp->tOutBufDesc, 0x0, sizeof(XDM2_BufDesc));

EXIT:
    if( eError != OMX_ErrorNone ) {
        if( pHandle != NULL ) {
            OMXVidDec_ComponentDeinit(hComponent);
        }
    }
    return (eError);
}


/*
* Video decoder setParameter
*/
OMX_ERRORTYPE OMXVidDec_SetParameter(OMX_HANDLETYPE hComponent,
                                               OMX_INDEXTYPE nIndex, OMX_PTR pParamStruct)
{
    OMX_ERRORTYPE                        eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE                   *pHandle = NULL;
    OMXVidDecComp                       *pVidDecComp = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE        *pPortDefs = NULL;
    OMX_VIDEO_PARAM_PORTFORMATTYPE      *pVideoParams = NULL;
    OMX_CONFIG_RECTTYPE                 *p2DBufferAllocParams = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE        *pInputPortDef = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE        *pOutputPortDef = NULL;
    OMX_VIDEO_PARAM_PORTFORMATTYPE      *pInPortFmtType = NULL;
    OMX_VIDEO_PARAM_PORTFORMATTYPE      *pOutPortFmtType = NULL;
    OMX_CONFIG_RECTTYPE                 *pOut2DBufAllocParam = NULL;
    OMX_CONFIG_RECTTYPE                 *pIn2DBufAllocParam = NULL;
    OMX_U32                              i=0;
    OMX_U32                              bFound = 0;
    OMX_U32                              nFrameHeight = 0;
    OMX_U32                              nFrameWidth  = 0;

    OMX_CHECK((hComponent != NULL) && (pParamStruct != NULL), OMX_ErrorBadParameter);

    /*! Initialize the pointers */
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp = (OMXVidDecComp*)pHandle->pComponentPrivate;

    switch( nIndex ) {
        case OMX_IndexParamPortDefinition :
        {
            /*! To set the Port Definition */
            OMX_BASE_CHK_VERSION(pParamStruct,
            OMX_PARAM_PORTDEFINITIONTYPE, eError);

            pPortDefs = (OMX_PARAM_PORTDEFINITIONTYPE *) pParamStruct;
            pInputPortDef = &(pVidDecComp->sBase.pPorts[OMX_VIDDEC_INPUT_PORT]->sPortDef);
            pOutputPortDef = &(pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef);

            //Check if an invalid port index is not sent
            OMX_CHECK(pPortDefs->nPortIndex >= pVidDecComp->sBase.nMinStartPortIndex
                       && pPortDefs->nPortIndex < (pVidDecComp->sBase.nNumPorts + pVidDecComp->sBase.nMinStartPortIndex),
                        OMX_ErrorBadPortIndex);

            /* SetParameter can be invoked in Loaded State  or on Disabled ports only*/
           // OMX_CHECK((pVidDecComp->sBase.tCurState == OMX_StateLoaded) ||
             //       (pVidDecComp->sBase.pPorts[pPortDefs->nPortIndex]->sPortDef.bEnabled == OMX_FALSE),
               //     OMX_ErrorIncorrectStateOperation);
            // If Input Port Parameters have to be changed
            if( pPortDefs->nPortIndex == pInputPortDef->nPortIndex ) {
                /*! Check for correct resolution of the stream */
                nFrameWidth = pPortDefs->format.video.nFrameWidth;
                nFrameHeight = pPortDefs->format.video.nFrameHeight;

                OMX_CHECK((nFrameWidth <= OMX_VIDDEC_MAX_WIDTH),
                        OMX_ErrorUnsupportedSetting);
                OMX_CHECK((nFrameHeight <= OMX_VIDDEC_MAX_HEIGHT),
                        OMX_ErrorUnsupportedSetting);
                OMX_CHECK((((nFrameWidth * nFrameHeight) >> 16) <= OMX_VIDDEC_MAX_MACROBLOCK),
                            OMX_ErrorUnsupportedSetting);

                /*! Call function to set input port definition */
                eError = OMXVidDec_SetInPortDef(hComponent, pPortDefs);
                if( eError != OMX_ErrorNone ) {
                    goto EXIT;
                }
                if( pVidDecComp->sBase.tCurState != OMX_StateLoaded ) {
                    pVidDecComp->nCodecRecreationRequired = 1;
                }
            }
            // If Output Port Parameters have to be changed
            else if( pPortDefs->nPortIndex == pOutputPortDef->nPortIndex ) {
                // Check the buffer cnt is greater than min required
                // buffer count
                OMX_CHECK((pPortDefs->nBufferCountActual
                        >= pOutputPortDef->nBufferCountMin),
                        OMX_ErrorUnsupportedSetting);
                // Check if Resolution being set at output port is same
                // as the input port
                OMX_CHECK((pOutputPortDef->format.video.nFrameHeight
                        == pPortDefs->format.video.nFrameHeight)
                        && (pOutputPortDef->format.video.nFrameWidth
                        == pPortDefs->format.video.nFrameWidth),
                        OMX_ErrorUnsupportedSetting);

                OMX_CHECK(pPortDefs->format.video.nStride >=
                        pOutputPortDef->format.video.nFrameWidth,
                        OMX_ErrorUnsupportedSetting);
                if( pPortDefs->format.video.nStride >
                        pVidDecComp->t2DBufferAllocParams[OMX_VIDDEC_OUTPUT_PORT].nWidth ) {
                    //Supported values of stride are only multiples of 128
                    OMX_CHECK((pPortDefs->format.video.nStride & 0x7F) == 0,
                                OMX_ErrorUnsupportedSetting);
                }
                //Check for the supported Color-fromat
                //and compression format.

               if (pVidDecComp->sBase.pPorts[pOutputPortDef->nPortIndex]->sProps.eBufMemoryType == MEM_GRALLOC) {
                    ALOGE("pOutputPortDef->format.video.eColorFormat[%x]\n", pOutputPortDef->format.video.eColorFormat);

                   //OMX_CHECK((pOutputPortDef->format.video.eColorFormat == HAL_NV12_PADDED_PIXEL_FORMAT),
                     //       OMX_ErrorUnsupportedSetting);
                } else {
                    OMX_CHECK((pOutputPortDef->format.video.eColorFormat == OMX_TI_COLOR_FormatYUV420PackedSemiPlanar),
                            OMX_ErrorUnsupportedSetting);
                }

                OMX_CHECK((pOutputPortDef->format.video.eCompressionFormat ==
                            OMX_VIDEO_CodingUnused),
                            OMX_ErrorUnsupportedSetting);

                /*! Call function to set output port definition */
                eError = OMXVidDec_SetOutPortDef(pVidDecComp, pPortDefs);
                if( eError != OMX_ErrorNone ) {
                    goto EXIT;
                }
            }
        }
        break;

        case OMX_IndexParamVideoPortFormat :
        {
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_PORTFORMATTYPE, eError);
            pVideoParams = (OMX_VIDEO_PARAM_PORTFORMATTYPE *) pParamStruct;
            pOutputPortDef = &(pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef);
            OMX_CHECK((pVidDecComp->sBase.tCurState == OMX_StateLoaded) ||
                    (pVidDecComp->sBase.pPorts[pVideoParams->nPortIndex]->sPortDef.bEnabled == OMX_FALSE),
                    OMX_ErrorIncorrectStateOperation);

            pInPortFmtType = &(pVidDecComp->tVideoParams[OMX_VIDDEC_INPUT_PORT]);
            pOutPortFmtType = &(pVidDecComp->tVideoParams[OMX_VIDDEC_OUTPUT_PORT]);
            // If Input Port Format type has to be changed
            if( pVideoParams->nPortIndex == pInPortFmtType->nPortIndex ) {
                // Change Compression type and frame-rate
                pInPortFmtType->eCompressionFormat
                                = pVideoParams->eCompressionFormat;
                pInPortFmtType->xFramerate = pVideoParams->xFramerate;
            }
            // In case Output Port Format type has to be changed
            else if( pVideoParams->nPortIndex == pOutPortFmtType->nPortIndex ) {
                //Check for the supported Color-fromat
                //and compression format.
               if (pVidDecComp->sBase.pPorts[pVideoParams->nPortIndex]->sProps.eBufMemoryType == MEM_GRALLOC) {
                    ALOGE("pOutputPortDef->format.video.eColorFormatttt[%x]\n", pOutputPortDef->format.video.eColorFormat);
                   // OMX_CHECK((pVideoParams->eColorFormat == HAL_NV12_PADDED_PIXEL_FORMAT),
                           // OMX_ErrorUnsupportedSetting);
                } else {
                    OMX_CHECK((pVideoParams->eColorFormat == OMX_TI_COLOR_FormatYUV420PackedSemiPlanar),
                            OMX_ErrorUnsupportedSetting);
                }
                OMX_CHECK((pVideoParams->eCompressionFormat ==
                        OMX_VIDEO_CodingUnused), OMX_ErrorUnsupportedSetting);

                pOutputPortDef->format.video.eColorFormat
                            = pVideoParams->eColorFormat;

                // Change Compression type, color format and frame-rate
                pOutPortFmtType->eCompressionFormat
                            = pVideoParams->eCompressionFormat;
                pOutPortFmtType->eColorFormat = pVideoParams->eColorFormat;
                pOutPortFmtType->xFramerate = pVideoParams->xFramerate;
            }
        }
        break;

        case OMX_IndexParamStandardComponentRole :
        {
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_PARAM_COMPONENTROLETYPE, eError);

            /* SetParameter can be invoked in Loaded State  or on Disabled ports only*/
            OMX_CHECK(pVidDecComp->sBase.tCurState == OMX_StateLoaded,
                    OMX_ErrorIncorrectStateOperation);
            /*! In case there is change in Role */
            if( strcmp((char *)((OMX_PARAM_COMPONENTROLETYPE *)pParamStruct)->cRole,
                    (char *)pVidDecComp->tComponentRole.cRole)) {
                /* De-initialize the current codec */
                pVidDecComp->fpDeinit_Codec(hComponent);

                /* Call specific component init depending upon the
                cRole set. */
                i=0;
                while( NULL != DecoderList[i].cRole[0] ) {
                    if( strcmp((char *)((OMX_PARAM_COMPONENTROLETYPE *)pParamStruct)->cRole,
                            (char *)DecoderList[i].cRole) == 0 ) {
                        /* Component found */
                        bFound = 1;
                        break;
                    }
                    i++;
                }

                if( bFound == 0 ) {
                    OSAL_ErrorTrace("Unsupported Role set");
                    eError = OMX_ErrorUnsupportedSetting;
                    goto EXIT;
                }
                /* Call the Specific Decoder Init function and Initialize Params */
                eError = DecoderList[i].fpDecoderComponentInit(hComponent);
                OMX_CHECK(eError == OMX_ErrorNone, eError);
                OMXVidDec_InitDecoderParams(hComponent, pHandle->pComponentPrivate);

                strcpy((char *)pVidDecComp->tComponentRole.cRole,
                        (char *)((OMX_PARAM_COMPONENTROLETYPE *)pParamStruct)->cRole);
            }
        }
        break;

        case (OMX_INDEXTYPE) OMX_TI_IndexParamTimeStampInDecodeOrder :
        {
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_TI_PARAM_TIMESTAMP_IN_DECODE_ORDER, eError);

            /* SetParameter can be invoked in Loaded State  or on Disabled ports only*/
            OMX_CHECK(pVidDecComp->sBase.tCurState == OMX_StateLoaded,
            OMX_ErrorIncorrectStateOperation);
            if(((OMX_TI_PARAM_TIMESTAMP_IN_DECODE_ORDER *) pParamStruct)->bEnabled == OMX_TRUE ) {
                pVidDecComp->bSupportDecodeOrderTimeStamp = OMX_TRUE;
            } else {
                pVidDecComp->bSupportDecodeOrderTimeStamp = OMX_FALSE;
            }
        }
        break;

        default :
            ALOGE("calling BAse call setparameter\n");
            eError = OMXBase_SetParameter(hComponent, nIndex, pParamStruct);
    }

EXIT:
    return (eError);
}

/*
* Video Decoder Get Parameter
*/
OMX_ERRORTYPE OMXVidDec_GetParameter(OMX_HANDLETYPE hComponent,
                                               OMX_INDEXTYPE nIndex, OMX_PTR pParamStruct)
{
    OMX_ERRORTYPE                        eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE                   *pHandle = NULL;
    OMXVidDecComp                       *pVidDecComp = NULL;
    OMX_VIDEO_PARAM_PORTFORMATTYPE      *pVideoParams = NULL;
    OMX_CONFIG_RECTTYPE                 *p2DBufferAllocParams = NULL;
    OMX_VIDEO_PARAM_PORTFORMATTYPE       inPortParam;
    OMX_VIDEO_PARAM_PORTFORMATTYPE       outPortParam;
    OMX_TI_PARAMNATIVEBUFFERUSAGE        *pUsage = NULL;

    OMX_CHECK((hComponent != NULL) &&
                (pParamStruct != NULL), OMX_ErrorBadParameter);

    // Initialize the local variables
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp = (OMXVidDecComp*)pHandle->pComponentPrivate;

    /* GetParameter can't be invoked incase the comp is in Invalid State  */
    OMX_CHECK(pVidDecComp->sBase.tCurState != OMX_StateInvalid,
            OMX_ErrorIncorrectStateOperation);

ALOGE("OMXVidDec_GetPArameter\n");

    switch( nIndex ) {
        case OMX_IndexParamVideoPortFormat :
ALOGE("OMX_IndexParamVideoPortFormat\n");
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_PORTFORMATTYPE, eError);
            // Initialize Port Params
            pVideoParams = (OMX_VIDEO_PARAM_PORTFORMATTYPE *) pParamStruct;
            inPortParam = pVidDecComp->tVideoParams[OMX_VIDDEC_INPUT_PORT];
            outPortParam = pVidDecComp->tVideoParams[OMX_VIDDEC_OUTPUT_PORT];
            if( pVideoParams->nPortIndex == inPortParam.nPortIndex ) {
                if( pVideoParams->nIndex > inPortParam.nIndex ) {
                    return (OMX_ErrorNoMore);
                }
ALOGE("OMX_IndexParamVideoPortFormat1\n");
                pVideoParams->eCompressionFormat = inPortParam.eCompressionFormat;
                pVideoParams->eColorFormat = inPortParam.eColorFormat;
                pVideoParams->xFramerate = inPortParam.xFramerate;
                if(pVideoParams->eColorFormat == OMX_COLOR_FormatYUV420PackedSemiPlanar)
                    pVideoParams->eColorFormat = OMX_TI_COLOR_FormatYUV420PackedSemiPlanar;
                } else if( pVideoParams->nPortIndex == outPortParam.nPortIndex ) {
ALOGE("OMX_IndexParamVideoPortFormat2\n");
                    if( pVideoParams->nIndex == 0 ) {
ALOGE("OMX_IndexParamVideoPortFormat3\n");
                            if (pVidDecComp->sBase.pPorts[pVideoParams->nPortIndex]->sProps.eBufMemoryType == MEM_GRALLOC) {
                            ALOGE("Buffers are of GRALLOC pointerts, so use HAL format\n");
                            pVideoParams->eColorFormat = HAL_NV12_PADDED_PIXEL_FORMAT;
                        } else {
                            pVideoParams->eColorFormat = OMX_TI_COLOR_FormatYUV420PackedSemiPlanar;
                        }
                    } else if( pVideoParams->nIndex > outPortParam.nIndex ) {
                        return (OMX_ErrorNoMore);
                    }
                    pVideoParams->eCompressionFormat = outPortParam.eCompressionFormat;
                    pVideoParams->xFramerate = outPortParam.xFramerate;
                }
        break;

        case OMX_IndexParamStandardComponentRole :
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_PARAM_COMPONENTROLETYPE, eError);
            strcpy((char *)((OMX_PARAM_COMPONENTROLETYPE *)pParamStruct)->cRole,
                (char *)pVidDecComp->tComponentRole.cRole);
        break;

        case (OMX_INDEXTYPE) OMX_TI_IndexAndroidNativeBufferUsage:
            pUsage = (OMX_TI_PARAMNATIVEBUFFERUSAGE*)pParamStruct;
            pUsage->nUsage = GRALLOC_USAGE_HW_RENDER;
        break;

        case (OMX_INDEXTYPE) OMX_TI_IndexParamTimeStampInDecodeOrder :
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_TI_PARAM_TIMESTAMP_IN_DECODE_ORDER, eError);
            if( pVidDecComp->bSupportDecodeOrderTimeStamp == OMX_TRUE ) {
                ((OMX_TI_PARAM_TIMESTAMP_IN_DECODE_ORDER *) pParamStruct)->bEnabled = OMX_TRUE;
            } else {
                ((OMX_TI_PARAM_TIMESTAMP_IN_DECODE_ORDER *) pParamStruct)->bEnabled = OMX_FALSE;
            }
        break;

        default :
            eError = OMXBase_GetParameter(hComponent, nIndex, pParamStruct);
    }

EXIT:
    return (eError);
}

/*
* Video decoder GetConfig
*/
OMX_ERRORTYPE OMXVidDec_GetConfig(OMX_HANDLETYPE hComponent,
                              OMX_INDEXTYPE nIndex, OMX_PTR pComponentConfigStructure) {
    OMX_ERRORTYPE                   eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE              *pHandle = NULL;
    OMXVidDecComp                  *pVidDecComp = NULL;
    OMX_CONFIG_RECTTYPE            *pCropParams = NULL;
    OMX_CONFIG_SCALEFACTORTYPE     *pScaleParams = NULL;

    OMX_CHECK((hComponent != NULL) &&
            (pComponentConfigStructure != NULL), OMX_ErrorBadParameter);

    // Initialize the local variables
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp = (OMXVidDecComp *)pHandle->pComponentPrivate;

    /* GetConfig can't be invoked incase the comp is in Invalid State  */
    OMX_CHECK(pVidDecComp->sBase.tCurState != OMX_StateInvalid,
                OMX_ErrorIncorrectStateOperation);

    switch( nIndex ) {
        case OMX_IndexConfigCommonOutputCrop :
        {
            OMX_BASE_CHK_VERSION(pComponentConfigStructure, OMX_CONFIG_RECTTYPE, eError);
            pCropParams = (OMX_CONFIG_RECTTYPE *) pComponentConfigStructure;
            if( pCropParams->nPortIndex == OMX_VIDDEC_OUTPUT_PORT ) {
                pCropParams->nWidth = pVidDecComp->tCropDimension.nWidth;
                pCropParams->nHeight = pVidDecComp->tCropDimension.nHeight;
                pCropParams->nTop = pVidDecComp->tCropDimension.nTop;
                pCropParams->nLeft = pVidDecComp->tCropDimension.nLeft;
            } else {
                eError = OMX_ErrorBadParameter;
                OSAL_ErrorTrace("OMX_IndexConfigCommonOutputCrop called on i/p port. Not Supported.");
            }
        }
        break;

        case OMX_IndexConfigCommonScale :
        {
            OMX_BASE_CHK_VERSION(pComponentConfigStructure, OMX_CONFIG_SCALEFACTORTYPE, eError);
            pScaleParams = (OMX_CONFIG_SCALEFACTORTYPE *) pComponentConfigStructure;
            if( pScaleParams->nPortIndex == OMX_VIDDEC_OUTPUT_PORT ) {
                pScaleParams->xWidth = pVidDecComp->tScaleParams.xWidth;
                pScaleParams->xHeight = pVidDecComp->tScaleParams.xHeight;
                OSAL_ErrorTrace("OMX_IndexConfigCommonScale called on o/p port.");
            } else {
                eError = OMX_ErrorBadParameter;
                OSAL_ErrorTrace("OMX_IndexConfigCommonOutputCrop called on i/p port. Not Supported.");
            }
        }
        break;

        default :
            eError = OMXBase_GetConfig(hComponent, nIndex, pComponentConfigStructure);
    }

EXIT:
    return (eError);
}

/*
* Video Decoder SetConfig
*/
OMX_ERRORTYPE OMXVidDec_SetConfig(OMX_HANDLETYPE hComponent,
                              OMX_INDEXTYPE nIndex, OMX_PTR pComponentConfigStructure) {
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE   *pHandle = NULL;
    OMXVidDecComp       *pVidDecComp = NULL;

    OMX_CHECK((hComponent != NULL) &&
            (pComponentConfigStructure != NULL), OMX_ErrorBadParameter);

    // Initialize the local variables
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp = (OMXVidDecComp *)pHandle->pComponentPrivate;

    /* SetConfig can't be invoked incase the comp is in Invalid State  */
    OMX_CHECK(pVidDecComp->sBase.tCurState != OMX_StateInvalid,
            OMX_ErrorIncorrectStateOperation);

    switch( nIndex ) {
        default :
            eError = OMXBase_SetConfig(hComponent, nIndex, pComponentConfigStructure);
    }

EXIT:
    return (eError);
}

/*
* Video Decoder xlateBuffer Handles
*/
OMX_ERRORTYPE OMXVidDec_XlateBuffHandle(OMX_HANDLETYPE hComponent,
                                       OMX_PTR pBufferHdr)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE   *pComp = NULL;
    OMXVidDecComp       *pVidDecComp = NULL;
    OMX_BUFFERHEADERTYPE    *pOMXBufHeader;
    OMXBase_Port        *pPort;
    OMX_U32             nPortIndex;
    int32_t             tRetVal;
    int                 i;

    pComp = (OMX_COMPONENTTYPE *)(hComponent);
    pVidDecComp = (OMXVidDecComp *)pComp->pComponentPrivate;

    pOMXBufHeader = (OMX_BUFFERHEADERTYPE *) (pBufferHdr);
    nPortIndex = (pOMXBufHeader->nInputPortIndex == OMX_NOPORT) ?
                pOMXBufHeader->nOutputPortIndex :
                pOMXBufHeader->nInputPortIndex;

    pPort = pVidDecComp->sBase.pPorts[nPortIndex];

    /* Non a buffer allocator, then check if any xlation is needed */
    if (pPort->sProps.eBufMemoryType == MEM_GRALLOC) {
        //populate the DMA BUFF_FDs from the gralloc pointers
        for ( i = 0; i < MAX_PLANES_PER_BUFFER; i++ ) {
            ((OMXBase_BufHdrPvtData *)(pOMXBufHeader->pPlatformPrivate))->sMemHdr[i].dma_buf_fd = (OMX_U32)(((IMG_native_handle_t*)(pOMXBufHeader->pBuffer))->fd[i]);
        }
    }

EXIT:
    return (eError);
}

/*
* video decoder Command Notify
*/
OMX_ERRORTYPE OMXVidDec_CommandNotify(OMX_HANDLETYPE hComponent,
                                                OMX_COMMANDTYPE Cmd, OMX_U32 nParam, OMX_PTR pCmdData)
{
    OMX_ERRORTYPE                   eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE               *pHandle = NULL;
    OMXVidDecComp                   *pVidDecComp = NULL;
    XDAS_Int32                      status;
    OMX_U32                         ReturnCmdCompletionCallBack = 1, i;
    OMX_PTR                         pDecStaticParams;
    OMX_PTR                         pDecDynamicParams;
    OMX_PTR                         pDecStatus;
    OMX_PARAM_PORTDEFINITIONTYPE    *pOutputPortDef;
    OMX_U32                         nFrameWidth, nFrameHeight;

    OMX_CHECK(hComponent != NULL, OMX_ErrorBadParameter);
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp = pHandle->pComponentPrivate;

    switch( Cmd ) {
        case OMX_CommandStateSet :
            if( pVidDecComp->sBase.tCurState == OMX_StateLoaded &&
                    pVidDecComp->sBase.tNewState == OMX_StateIdle ) {
                pOutputPortDef = &(pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef);
                if( pOutputPortDef->format.video.nStride <
                        pVidDecComp->t2DBufferAllocParams[OMX_VIDDEC_OUTPUT_PORT].nWidth ) {
                    OSAL_ErrorTrace("Unsupported Stride set on o/p port defs");
                    eError = OMX_ErrorUnsupportedSetting;
                    goto EXIT;
                }
                // Check the buffer cnt is greater than min required
                // buffer count
                if( pOutputPortDef->nBufferCountActual < pOutputPortDef->nBufferCountMin ) {
                    OSAL_ErrorTrace("Number of actual buffers on o/p port less than the minmum required by componenet as set on o/p port defs");
                    eError = OMX_ErrorUnsupportedSetting;
                    goto EXIT;
                }

                // Transitioning from Loaded to Idle state
                if(pVidDecComp->pDecHandle == NULL ) {
                    pDecStaticParams = pVidDecComp->pDecStaticParams;
                    // Call the Video Decoder Create Call
                    pVidDecComp->pDecHandle
                            = VIDDEC3_create(pVidDecComp->ce,
                                        pVidDecComp->cDecoderName,
                                        (VIDDEC3_Params *) pDecStaticParams);

                    if( pVidDecComp->pDecHandle == NULL ) {
                        OSAL_ErrorTrace("VIDDEC3_create failed ....! \n");
                        eError = OMX_ErrorInsufficientResources;
                        goto EXIT;
                    }
                    pVidDecComp->nCodecRecreationRequired = 0;
                    pVidDecComp->nOutPortReconfigRequired = 0;

                    // Populate Dynamic Params and the Status structures
                    pVidDecComp->fpSet_DynamicParams(hComponent, pVidDecComp->pDecDynParams);
                    pVidDecComp->fpSet_Status(hComponent, pVidDecComp->pDecStatus);
                    //This is to handle Arbitrary stride requirement given by IL client.
                    if((pOutputPortDef->format.video.nStride != OMX_VIDDEC_TILER_STRIDE) &&
                            (pOutputPortDef->format.video.nStride >
                            pVidDecComp->t2DBufferAllocParams[OMX_VIDDEC_OUTPUT_PORT].nWidth)) {
                        pVidDecComp->pDecDynParams->displayWidth =
                        pOutputPortDef->format.video.nStride;
                    }
                    pDecDynamicParams = pVidDecComp->pDecDynParams;
                    pDecStatus = pVidDecComp->pDecStatus;

                    OMX_CHECK(((pVidDecComp->pDecDynParams != NULL) && (pVidDecComp->pDecStatus != NULL)), OMX_ErrorBadParameter);
                    status    = VIDDEC3_control(pVidDecComp->pDecHandle, XDM_SETPARAMS, pVidDecComp->pDecDynParams, pVidDecComp->pDecStatus);
                    if( status != VIDDEC3_EOK ) {
                        OSAL_ErrorTrace("VIDDEC3_control XDM_SETPARAMS failed! \n");
                        eError = OMX_ErrorInsufficientResources;
                        goto EXIT;
                    }
                    OMX_CHECK(eError == OMX_ErrorNone, eError);

                    // Call the Decoder Control function
                    status    = VIDDEC3_control(pVidDecComp->pDecHandle, XDM_GETBUFINFO, pVidDecComp->pDecDynParams, pVidDecComp->pDecStatus);
                    if( status != VIDDEC3_EOK ) {
                        OSAL_ErrorTrace("VIDDEC3_control XDM_GETBUFINFO failed! \n");
                        eError = OMX_ErrorInsufficientResources;
                        goto EXIT;
                    }
                    OMX_CHECK(eError == OMX_ErrorNone, eError);
                }
            } else if( pVidDecComp->sBase.tCurState == OMX_StateIdle &&
                    pVidDecComp->sBase.tNewState == OMX_StateLoaded ) {
                // Transitioning from Idle to Loaded
                if( pVidDecComp->pDecHandle != NULL ) {
                    // Delete the Decoder Component Private Handle
                    pVidDecComp->nFrameCounter = 0;
                    if( pVidDecComp->bSupportSkipGreyOutputFrames ) {
                        pVidDecComp->bSyncFrameReady = OMX_FALSE;
                    }

                    pVidDecComp->nOutbufInUseFlag = 0;
                    VIDDEC3_delete(pVidDecComp->pDecHandle);
                    pVidDecComp->pDecHandle = NULL;
                }

            } else if(((pVidDecComp->sBase.tCurState == OMX_StateExecuting) &&
                    (pVidDecComp->sBase.tNewState == OMX_StateIdle)) ||
                    ((pVidDecComp->sBase.tCurState == OMX_StatePause) &&
                    (pVidDecComp->sBase.tNewState == OMX_StateIdle))) {
                OMXVidDec_HandleFLUSH_EOS(hComponent, NULL, NULL);
            }

        break;

        case OMX_CommandPortDisable :
            // In case of Port Disable Command
            // Loaded state implies codec is deleted. so no need to delete again
            if( pVidDecComp->sBase.tCurState != OMX_StateLoaded
                    && pVidDecComp->pDecHandle != NULL ) {
                // Call Decoder Flush function
                OMXVidDec_HandleFLUSH_EOS(hComponent, NULL, NULL);
            }
        break;

        case OMX_CommandPortEnable :
            if( nParam == OMX_VIDDEC_OUTPUT_PORT ) {
                // Check the buffer cnt is greater than min required
                // buffer count
                pOutputPortDef =
                        &(pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef);
                if( pOutputPortDef->nBufferCountActual < pOutputPortDef->nBufferCountMin ) {
                    OSAL_ErrorTrace("Number of actual buffers on o/p port less than the minmum required by componenet as set on o/p port defs");
                    eError = OMX_ErrorUnsupportedSetting;
                    goto EXIT;
                }
                if( pOutputPortDef->format.video.nStride <
                    pVidDecComp->t2DBufferAllocParams[OMX_VIDDEC_OUTPUT_PORT].nWidth ) {
                    OSAL_ErrorTrace("Unsupported Stride set on o/p port defs");
                    eError = OMX_ErrorUnsupportedSetting;
                    goto EXIT;
                }
            }
            if( pVidDecComp->sBase.tCurState != OMX_StateLoaded ) {
                if( pVidDecComp->nCodecRecreationRequired == 1
                        && pVidDecComp->pDecHandle != NULL ) {
                    pVidDecComp->nCodecRecreationRequired =  0;
                    pVidDecComp->nFrameCounter = 0;
                    pVidDecComp->tScaleParams.xWidth = 0x10000;
                    pVidDecComp->tScaleParams.xHeight = 0x10000;
                    if( pVidDecComp->bSupportSkipGreyOutputFrames ) {
                        pVidDecComp->bSyncFrameReady = OMX_FALSE;
                    }
                    pVidDecComp->nOutbufInUseFlag = 0;
                    VIDDEC3_delete(pVidDecComp->pDecHandle);
                    pVidDecComp->pDecHandle = NULL;

                    pOutputPortDef =
                            &(pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef);
                    // Set the Static Params
                    pVidDecComp->fpSet_StaticParams(hComponent, pVidDecComp->pDecStaticParams);
                    if( pVidDecComp->bUsePortReconfigForPadding == OMX_TRUE
                            && pVidDecComp->pDecStatus->outputWidth != 0
                            && pVidDecComp->pDecStatus->outputHeight != 0 ) {
                        nFrameWidth =  pVidDecComp->pDecStatus->outputWidth;
                        nFrameHeight =  pVidDecComp->pDecStatus->outputHeight;
                    } else {
                        nFrameWidth = pOutputPortDef->format.video.nFrameWidth;
                        nFrameHeight = pOutputPortDef->format.video.nFrameHeight;
                    }
                    if( nFrameWidth & 0x0F ) {
                        nFrameWidth = nFrameWidth + 16 - (nFrameWidth & 0x0F);
                    }
                    if( nFrameHeight & 0x1F ) {
                        nFrameHeight = nFrameHeight + 32 - (nFrameHeight & 0x1F);
                    }
                    pVidDecComp->pDecStaticParams->maxHeight = nFrameHeight;
                    pVidDecComp->pDecStaticParams->maxWidth = nFrameWidth;

                    // And Call the Codec Create
                    pVidDecComp->pDecHandle = VIDDEC3_create(pVidDecComp->ce,
                                                    pVidDecComp->cDecoderName,
                                                    (VIDDEC3_Params *)
                                                    pVidDecComp->pDecStaticParams);
                    if( pVidDecComp->pDecHandle == NULL ) {
                        OSAL_ErrorTrace("VIDDEC3_create failed ....! \n");
                        eError = OMX_ErrorInsufficientResources;
                        goto EXIT;
                    }

                    // Populate Dynamic Params and the Status structures
                    pVidDecComp->fpSet_DynamicParams(hComponent, pVidDecComp->pDecDynParams);
                    pVidDecComp->fpSet_Status(hComponent, pVidDecComp->pDecStatus);
                    //This is to handle Arbitrary stride requirement given by IL client.
                    if((pOutputPortDef->format.video.nStride != OMX_VIDDEC_TILER_STRIDE) &&
                            (pOutputPortDef->format.video.nStride !=
                            pVidDecComp->t2DBufferAllocParams[OMX_VIDDEC_OUTPUT_PORT].nWidth)) {
                        pVidDecComp->pDecDynParams->displayWidth =
                                            pOutputPortDef->format.video.nStride;
                    }
                    pDecDynamicParams = pVidDecComp->pDecDynParams;
                    pDecStatus = pVidDecComp->pDecStatus;

                    // Call the Decoder Control function
                    OMX_CHECK(((pVidDecComp->pDecDynParams != NULL) && (pVidDecComp->pDecStatus != NULL)), OMX_ErrorBadParameter);
                    status = VIDDEC3_control(pVidDecComp->pDecHandle, XDM_SETPARAMS, pVidDecComp->pDecDynParams, pVidDecComp->pDecStatus);
                    if( status != VIDDEC3_EOK ) {
                        OSAL_ErrorTrace("VIDDEC3_control XDM_SETPARAMS failed! \n");
                        eError = OMX_ErrorInsufficientResources;
                        goto EXIT;
                    }
                    OMX_CHECK(eError == OMX_ErrorNone, eError);
                }
                pVidDecComp->nOutPortReconfigRequired = 0;
            }
            if( pVidDecComp->bUsePortReconfigForCrop == OMX_TRUE ) {
                eError = pVidDecComp->sBase.fpReturnEventNotify(hComponent,
                                OMX_EventPortSettingsChanged, OMX_VIDDEC_OUTPUT_PORT, OMX_IndexConfigCommonOutputCrop, NULL);
                if( eError != OMX_ErrorNone ) {
                    OSAL_ErrorTrace("Port reconfig callback returned error, trying to continue");
                }
            }

        break;

        case OMX_CommandFlush :
        {
            // In case of Flush Command
            // In case all ports have to be flushed or the output port
            // has to be flushed
            if( nParam == OMX_ALL || nParam == OMX_VIDDEC_OUTPUT_PORT ) {
                // Only then flush the port
                OMXVidDec_HandleFLUSH_EOS(hComponent, NULL, NULL);
            }
        }
        break;

        case OMX_CommandMarkBuffer :
            ReturnCmdCompletionCallBack = 0;
        break;

        default:
            OSAL_ErrorTrace("Invalid command");
    }

    if( ReturnCmdCompletionCallBack == 1 ) {
        eError = pVidDecComp->sBase.fpReturnEventNotify(hComponent,
        OMX_EventCmdComplete, Cmd, nParam, NULL);
    }

EXIT:
    return (eError);

}

/*
* Video Decoder DataNotify
*/
OMX_ERRORTYPE OMXVidDec_DataNotify(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE           eError = OMX_ErrorNone, eRMError = OMX_ErrorNone;
    OMX_COMPONENTTYPE       *pHandle = NULL;
    OMX_BUFFERHEADERTYPE    sInBufHeader;
    OMXVidDecComp           *pVidDecComp = NULL;
    OMX_BUFFERHEADERTYPE    *pInBufHeader = NULL;
    OMX_BUFFERHEADERTYPE    *pOutBufHeader = NULL;
    OMX_BUFFERHEADERTYPE    *pDupBufHeader = NULL;
    OMX_BUFFERHEADERTYPE    *pFreeBufHeader = NULL;
    OMX_U32                 nInMsgCount = 0;
    OMX_U32                 nOutMsgCount = 0;
    OMX_U32                 Buffer_locked = 1;
    OMX_U32                 ii = 0, i;
    OMX_U32                 nStride = OMX_VIDDEC_TILER_STRIDE;
    OMX_U32                 nLumaFilledLen, nChromaFilledLen;
    OMX_PARAM_PORTDEFINITIONTYPE    *pOutputPortDef = NULL;
    OMX_BOOL                        nIsDioReady = OMX_FALSE;
    XDAS_Int32              status;
    XDM2_BufDesc            *pInBufDescPtr = NULL;
    XDM2_BufDesc            *pOutBufDescPtr = NULL;
    OMX_CONFIG_RECTTYPE     out2DAllocParam;
    IVIDDEC3_OutArgs        *pDecOutArgs = NULL;
    IVIDDEC3_Status         *pDecStatus = NULL;
    XDM_Rect                activeFrameRegion[2];
    OMX_U32                 nNewInBufferRequired = 0;
    OMX_BOOL                bSendPortReconfigForScale = OMX_FALSE;
    OMX_U32                 nScale, nScaleRem, nScaleQ16Low, nScaleWidth, nScaleHeight;
    OMX_U64                 nScaleQ16 = 0;
    uint32_t                nActualSize;
    OMX_BOOL                went_thru_loop = OMX_FALSE;
    OMX_BOOL                duped_IPbuffer = OMX_TRUE;
    IMG_native_handle_t*    grallocHandle;

    OMX_CHECK(hComponent != NULL, OMX_ErrorBadParameter);

    //! Initialize pointers
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp = (OMXVidDecComp *)pHandle->pComponentPrivate;
    OMX_CHECK(pVidDecComp != NULL, OMX_ErrorBadParameter);

    pOutputPortDef = &(pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef);
    out2DAllocParam = pVidDecComp->t2DBufferAllocParams[OMX_VIDDEC_OUTPUT_PORT];

    /*! Ensure that the stride on output portdef structure is more than
    the padded width. This is needed in the case where application
    sets the Stride less than padded width */
    if( pOutputPortDef->format.video.nStride >=
            pVidDecComp->t2DBufferAllocParams[OMX_VIDDEC_OUTPUT_PORT].nWidth ) {
        nStride = pOutputPortDef->format.video.nStride;
    } else {
        nStride = pVidDecComp->t2DBufferAllocParams[OMX_VIDDEC_OUTPUT_PORT].nWidth;
    }
    if((pVidDecComp->sBase.tCurState != OMX_StateExecuting) ||
            (pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef.bEnabled == OMX_FALSE) ||
            (pVidDecComp->sBase.pPorts[OMX_VIDDEC_INPUT_PORT]->sPortDef.bEnabled == OMX_FALSE)) {
        goto EXIT;
    }
    nIsDioReady = OMXBase_IsDioReady(hComponent, OMX_VIDDEC_OUTPUT_PORT);
    if( nIsDioReady == OMX_FALSE ) {
        goto EXIT;
    }
    //! Get the number of input and output buffers
    pVidDecComp->sBase.pPvtData->fpDioGetCount(hComponent,
                                        OMX_VIDDEC_INPUT_PORT,
                                        (OMX_PTR)&nInMsgCount);
    pVidDecComp->sBase.pPvtData->fpDioGetCount(hComponent,
                                        OMX_VIDDEC_OUTPUT_PORT,
                                        (OMX_PTR)&nOutMsgCount);

    // Loop until input or output buffers are exhausted
    while((nInMsgCount > 0) && (nOutMsgCount > 0)) {
        // Only if Cur-State is Execute proceed.
        if((pVidDecComp->sBase.tCurState != OMX_StateExecuting) ||
                (pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef.bEnabled == OMX_FALSE) ||
                (pVidDecComp->sBase.pPorts[OMX_VIDDEC_INPUT_PORT]->sPortDef.bEnabled == OMX_FALSE)) {
            break;
        }
        if( OMXBase_IsCmdPending(hComponent) && pVidDecComp->nOutbufInUseFlag == 0 ) {
            OSAL_ErrorTrace("Exiting dataNotify because command is pending");
            goto EXIT;
        }
        if( pVidDecComp->nOutPortReconfigRequired == 1 ) {
            OSAL_ErrorTrace("Port disable/reconfiguration needed");
            goto EXIT;
        }
        if( pVidDecComp->nFatalErrorGiven == 1 ) {
            OSAL_ErrorTrace("Fatal error given to IL client");
            goto EXIT;
        }
        // Buffer is locked by the codec by default.
        // It can be freed only if mentioned in freeBufId[] field.
        Buffer_locked = 1;
        ii=0;
        went_thru_loop = OMX_TRUE;
        if( nNewInBufferRequired == 0 && pVidDecComp->sCodecConfig.sBuffer == NULL ) {
            duped_IPbuffer = OMX_FALSE;
            //! Get Input and Output Buffer header from Queue
            eError = pVidDecComp->sBase.pPvtData->fpDioDequeue(hComponent, OMX_VIDDEC_INPUT_PORT,
                                                                (OMX_PTR*)(&pInBufHeader));
            if( eError == OMX_TI_WarningAttributePending ) {
                pVidDecComp->sCodecConfig.sBuffer = P2H(memplugin_alloc(2, 1, MEM_CARVEOUT, 0, 0));
                OMX_CHECK(pVidDecComp->sCodecConfig.sBuffer != NULL, OMX_ErrorInsufficientResources);
                eError = pVidDecComp->sBase.pPvtData->fpDioControl(hComponent, OMX_VIDDEC_INPUT_PORT,
                                                OMX_DIO_CtrlCmd_GetCtrlAttribute, (OMX_PTR)&(pVidDecComp->sCodecConfig));
                if( eError == OMX_TI_WarningInsufficientAttributeSize ) {
                    /*Allocate the data pointer again with correct size*/
                    uint32_t new_size = pVidDecComp->sCodecConfig.sBuffer->size;
                    memplugin_free((void*)H2P(pVidDecComp->sCodecConfig.sBuffer));
                    pVidDecComp->sCodecConfig.sBuffer = P2H(memplugin_alloc(new_size, 1, MEM_CARVEOUT, 0, 0));
                    OMX_CHECK(pVidDecComp->sCodecConfig.sBuffer != NULL, OMX_ErrorInsufficientResources);
                    eError = pVidDecComp->sBase.pPvtData->fpDioControl(hComponent, OMX_VIDDEC_INPUT_PORT,
                                                OMX_DIO_CtrlCmd_GetCtrlAttribute, (OMX_PTR)&(pVidDecComp->sCodecConfig));
                    if( eError != OMX_ErrorNone ) {
                        OSAL_ErrorTrace("Codec config test failed because DIO Control returned buffer");
                        goto EXIT;
                    }
                }
                pVidDecComp->pDecDynParams->decodeHeader  = pVidDecComp->nDecoderMode;
                pInBufHeader = NULL;
                // Call the Decoder Control function
                status = VIDDEC3_control(pVidDecComp->pDecHandle, XDM_SETPARAMS, pVidDecComp->pDecDynParams, pVidDecComp->pDecStatus);
                if( status != VIDDEC3_EOK ) {
                    OSAL_ErrorTrace("VIDDEC3_control XDM_SETPARAMS failed ....! \n");
                    eError = OMX_ErrorInsufficientResources;
                    goto EXIT;
                }
                OMX_CHECK(eError == OMX_ErrorNone, eError);
            } else if( pVidDecComp->nOutbufInUseFlag == 0 ) {
                if( pVidDecComp->pDecDynParams->decodeHeader != XDM_DECODE_AU ) {
                    pVidDecComp->pDecDynParams->decodeHeader = XDM_DECODE_AU;
                    status = VIDDEC3_control(pVidDecComp->pDecHandle, XDM_SETPARAMS, pVidDecComp->pDecDynParams, pVidDecComp->pDecStatus);
                    if( status != VIDDEC3_EOK ) {
                        OSAL_ErrorTrace("VIDDEC3_control XDM_SETPARAMS failed ....! \n");
                        eError = OMX_ErrorInsufficientResources;
                        goto EXIT;
                    }
                    OMX_CHECK(eError == OMX_ErrorNone, eError);
                }
                if( pInBufHeader ) {
                    if(!(pInBufHeader->nFlags & OMX_BUFFERFLAG_EOS) && (pInBufHeader->nFilledLen - pInBufHeader->nOffset == 0)) {
                        // Send Input buffer back to base
                        pVidDecComp->sBase.pPvtData->fpDioSend(hComponent,
                                                    OMX_VIDDEC_INPUT_PORT,
                                                    pInBufHeader);

                        //! Get the number of input and output buffers
                        pVidDecComp->sBase.pPvtData->fpDioGetCount(hComponent,
                                                    OMX_VIDDEC_INPUT_PORT,
                                                    (OMX_PTR)&nInMsgCount);
                        if( nInMsgCount ) {
                            continue;
                        } else {
                            break;
                        }
                    }
                }
                eError = pVidDecComp->sBase.pPvtData->fpDioDequeue(hComponent, OMX_VIDDEC_OUTPUT_PORT,
                                                                (OMX_PTR*)(&pOutBufHeader));
            }
        }
        pOutBufDescPtr = pVidDecComp->tOutBufDesc;
        pInBufDescPtr = pVidDecComp->tInBufDesc;
        if( pVidDecComp->sCodecConfig.sBuffer != NULL &&
                pVidDecComp->pDecDynParams->decodeHeader != pVidDecComp->nDecoderMode ) {
                pVidDecComp->pDecDynParams->decodeHeader = pVidDecComp->nDecoderMode;
            // Call the Decoder Control function
            status = VIDDEC3_control(pVidDecComp->pDecHandle, XDM_SETPARAMS,
                                    pVidDecComp->pDecDynParams, pVidDecComp->pDecStatus);
            if( status != VIDDEC3_EOK ) {
                OSAL_ErrorTrace("VIDDEC3_control XDM_SETPARAMS failed ....! \n");
                eError = OMX_ErrorInsufficientResources;
                goto EXIT;
            }
            OMX_CHECK(eError == OMX_ErrorNone, eError);
        }
        pInBufDescPtr->numBufs = 1;
        if( pInBufHeader != NULL && pVidDecComp->nOutbufInUseFlag == 1 ) {
            pVidDecComp->pDecInArgs->numBytes = pInBufHeader->nFilledLen - pInBufHeader->nOffset;
            //Sending the same input ID for second field
            pVidDecComp->pDecInArgs->inputID = (OMX_S32) pOutBufHeader;
            if(((IVIDDEC3_Params *)(pVidDecComp->pDecStaticParams))->inputDataMode == IVIDEO_ENTIREFRAME ) {
                P2H(pInBufHeader->pBuffer)->offset = pInBufHeader->nOffset;
                pInBufDescPtr->descs[0].buf = (XDAS_Int8*)pInBufHeader->pBuffer;
                pInBufDescPtr->descs[0].memType = XDM_MEMTYPE_RAW;
                pInBufDescPtr->descs[0].bufSize.bytes = pInBufHeader->nFilledLen;
            }
            pOutBufDescPtr->numBufs = 0;
        } else if( pVidDecComp->pDecDynParams->decodeHeader == pVidDecComp->nDecoderMode ) {
            pVidDecComp->pDecInArgs->numBytes = pVidDecComp->sCodecConfig.sBuffer->size;
            pVidDecComp->pDecInArgs->inputID = 1;
            pVidDecComp->sCodecConfig.sBuffer->offset = sizeof(MemHeader);
            pInBufDescPtr->descs[0].buf = (XDAS_Int8*)(pVidDecComp->sCodecConfig.sBuffer);
            pInBufDescPtr->descs[0].memType = XDM_MEMTYPE_RAW;
            pInBufDescPtr->descs[0].bufSize.bytes = pVidDecComp->sCodecConfig.sBuffer->size;
            pOutBufDescPtr->numBufs = 0;
        } else if( pInBufHeader != NULL && pVidDecComp->pDecDynParams->decodeHeader == XDM_DECODE_AU ) {
            // In case EOS and Number of Input bytes=0. Flush Decoder and exit
            if( pInBufHeader->nFlags & OMX_BUFFERFLAG_EOS ) {
                if( pInBufHeader->nFilledLen == 0 ) {
                    pOutBufHeader->nFilledLen = 0;
                    eError = OMXVidDec_HandleFLUSH_EOS(hComponent, pOutBufHeader, pInBufHeader);
                    goto EXIT;
                } else if( pVidDecComp->pDecStaticParams->displayDelay == IVIDDEC3_DECODE_ORDER ) {
                    pOutBufHeader->nFlags |= OMX_BUFFERFLAG_EOS;
                }
            }
            if( pVidDecComp->bUsePortReconfigForPadding == OMX_TRUE ) {
                if( pOutputPortDef->format.video.nFrameWidth < out2DAllocParam.nWidth
                        || pOutputPortDef->format.video.nFrameHeight < out2DAllocParam.nHeight ) {
                    pOutputPortDef->format.video.nFrameWidth = out2DAllocParam.nWidth;
                    pOutputPortDef->format.video.nFrameHeight = out2DAllocParam.nHeight;
                    pVidDecComp->sBase.pPvtData->fpDioCancel(hComponent,
                                                        OMX_VIDDEC_INPUT_PORT, pInBufHeader);
                    pVidDecComp->sBase.pPvtData->fpDioCancel(hComponent,
                                                        OMX_VIDDEC_OUTPUT_PORT, pOutBufHeader);

                    /*! Notify to Client change in output port settings */
                    eError = pVidDecComp->sBase.fpReturnEventNotify(hComponent,
                                    OMX_EventPortSettingsChanged, OMX_VIDDEC_OUTPUT_PORT, 0, NULL);
                    pVidDecComp->nOutPortReconfigRequired = 1;
                    goto EXIT;
                }
            }
            /*! Populate xDM structure */
            pVidDecComp->pDecInArgs->numBytes = pInBufHeader->nFilledLen - pInBufHeader->nOffset;
            pVidDecComp->pDecInArgs->inputID = (OMX_S32) pOutBufHeader;
            if(((IVIDDEC3_Params *)(pVidDecComp->pDecStaticParams))->inputDataMode == IVIDEO_ENTIREFRAME ) {
                /* Fill Input Buffer Descriptor */
                (((OMXBase_BufHdrPvtData*)pInBufHeader->pPlatformPrivate)->sMemHdr[0]).offset = pInBufHeader->nOffset;
                pInBufDescPtr->descs[0].buf = (XDAS_Int8 *)&(((OMXBase_BufHdrPvtData*)pInBufHeader->pPlatformPrivate)->sMemHdr[0]);
                pInBufDescPtr->descs[0].memType = XDM_MEMTYPE_RAW;
                pInBufDescPtr->descs[0].bufSize.bytes = pInBufHeader->nFilledLen - pInBufHeader->nOffset;
            }
            /* Initialize Number of Buffers for input and output */
            pInBufDescPtr->numBufs = 1;
            pOutBufDescPtr->numBufs = 2;

            /* Fill Output Buffer Descriptor */
           /* if (pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sProps.eBufMemoryType == MEM_GRALLOC) {
                pOutBufDescPtr->descs[0].bufSize.tileMem.width  = pVidDecComp->t2DBufferAllocParams[OMX_VIDDEC_OUTPUT_PORT].nWidth;
                pOutBufDescPtr->descs[0].bufSize.tileMem.height = pVidDecComp->t2DBufferAllocParams[OMX_VIDDEC_OUTPUT_PORT].nHeight;
                (((OMXBase_BufHdrPvtData*)pOutBufHeader->pPlatformPrivate)->sMemHdr[0]).offset = 0;
                pOutBufDescPtr->descs[0].buf = (XDAS_Int8 *)&(((OMXBase_BufHdrPvtData*)pOutBufHeader->pPlatformPrivate)->sMemHdr[0]);
                pOutBufDescPtr->descs[0].memType = XDM_MEMTYPE_TILED8;
                (((OMXBase_BufHdrPvtData*)pOutBufHeader->pPlatformPrivate)->sMemHdr[1]).offset = 0;
                pOutBufDescPtr->descs[1].bufSize.tileMem.width =pVidDecComp->t2DBufferAllocParams[OMX_VIDDEC_OUTPUT_PORT].nWidth;
                pOutBufDescPtr->descs[1].bufSize.tileMem.height = pVidDecComp->t2DBufferAllocParams[OMX_VIDDEC_OUTPUT_PORT].nHeight / 2;
                pOutBufDescPtr->descs[1].buf = (XDAS_Int8 *)&(((OMXBase_BufHdrPvtData*)pOutBufHeader->pPlatformPrivate)->sMemHdr[1]);
                pOutBufDescPtr->descs[1].memType = XDM_MEMTYPE_TILED16;
            } else*/ {
                pOutBufDescPtr->descs[0].bufSize.bytes  = pVidDecComp->t2DBufferAllocParams[OMX_VIDDEC_OUTPUT_PORT].nWidth *
                                                          pVidDecComp->t2DBufferAllocParams[OMX_VIDDEC_OUTPUT_PORT].nHeight;
                pOutBufDescPtr->descs[0].buf = (XDAS_Int8 *)&(((OMXBase_BufHdrPvtData*)pOutBufHeader->pPlatformPrivate)->sMemHdr[0]);
                pOutBufDescPtr->descs[0].memType = XDM_MEMTYPE_RAW;

                memcpy(&(((OMXBase_BufHdrPvtData*)pOutBufHeader->pPlatformPrivate)->sMemHdr[1]), &(((OMXBase_BufHdrPvtData*)pOutBufHeader->pPlatformPrivate)->sMemHdr[0]), sizeof(MemHeader));

                (((OMXBase_BufHdrPvtData*)pOutBufHeader->pPlatformPrivate)->sMemHdr[1]).offset = pOutBufDescPtr->descs[0].bufSize.bytes;

                pOutBufDescPtr->descs[1].bufSize.bytes = pVidDecComp->t2DBufferAllocParams[OMX_VIDDEC_OUTPUT_PORT].nWidth *
                                                         pVidDecComp->t2DBufferAllocParams[OMX_VIDDEC_OUTPUT_PORT].nHeight/2;
                pOutBufDescPtr->descs[1].buf = (XDAS_Int8 *)&(((OMXBase_BufHdrPvtData*)pOutBufHeader->pPlatformPrivate)->sMemHdr[1]);
                pOutBufDescPtr->descs[1].memType = XDM_MEMTYPE_RAW;
                nStride = pVidDecComp->t2DBufferAllocParams[OMX_VIDDEC_OUTPUT_PORT].nWidth;
            }
            pOutBufHeader->nTimeStamp = pInBufHeader->nTimeStamp;
            if( pVidDecComp->bSupportDecodeOrderTimeStamp == OMX_TRUE && pVidDecComp->bInputBufferCancelled == 0 ) {
                OSAL_WriteToPipe(pVidDecComp->pTimeStampStoragePipe, &(pInBufHeader->nTimeStamp),
                                sizeof(OMX_TICKS), OSAL_NO_SUSPEND);
            }
        }
        /*Copy OMX_BUFFERFLAG_DECODEONLY from input buffer header to output buffer header*/
        if (pOutBufHeader && pInBufHeader) {
            pOutBufHeader->nFlags |= (pInBufHeader->nFlags & OMX_BUFFERFLAG_DECODEONLY);
            ((OMXBase_BufHdrPvtData *)(pInBufHeader->pPlatformPrivate))->bufSt = OWNED_BY_CODEC;
            ((OMXBase_BufHdrPvtData *)(pOutBufHeader->pPlatformPrivate))->bufSt = OWNED_BY_CODEC;
        }

        if (pOutBufHeader) {
            grallocHandle = (IMG_native_handle_t*)(pOutBufHeader->pBuffer);
            pVidDecComp->grallocModule->lock((gralloc_module_t const *) pVidDecComp->grallocModule,
                                    (buffer_handle_t)grallocHandle, GRALLOC_USAGE_HW_RENDER,
                                    0,0,pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef.format.video.nFrameWidth,
                                    pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef.format.video.nFrameHeight,NULL);
        }
ALOGE("Process++\n");
        status =  VIDDEC3_process(pVidDecComp->pDecHandle, pInBufDescPtr, pOutBufDescPtr,
                                (VIDDEC3_InArgs *)pVidDecComp->pDecInArgs, (VIDDEC3_OutArgs *)pVidDecComp->pDecOutArgs);

ALOGE("Process--\n");

        pDecOutArgs = pVidDecComp->pDecOutArgs;

        /*! In case Process returns error */
        if(status == XDM_EFAIL ){
            OSAL_ErrorTrace("\n Process function returned an Error...  \n");
            OSAL_ErrorTrace("Codec Extended - 0x%x", pVidDecComp->pDecOutArgs->extendedError);
            OSAL_ErrorTrace("Input Buffer Size provided to codec is : %d", pInBufDescPtr->descs[0].bufSize.bytes);
            OSAL_ErrorTrace("Frame count is : %d", pVidDecComp->nFrameCounter + 1);
            OSAL_ErrorTrace("Bytes consumed - %d", pVidDecComp->pDecOutArgs->bytesConsumed);

            if (pInBufHeader && pOutBufHeader) {
                /*! Call function to handle Codec Error */
                eError = OMXVidDec_HandleCodecProcError(hComponent, &(pInBufHeader), &(pOutBufHeader));
            }
            if( eError != OMX_ErrorNone ) {
                goto EXIT;
            }
            if( pVidDecComp->nOutPortReconfigRequired == 1 ) {
                goto EXIT;
            }
        }
        if( pVidDecComp->pDecDynParams->decodeHeader == pVidDecComp->nDecoderMode ) {
            eError = OMXVidDec_HandleFirstFrame(hComponent, NULL);
            //we have to loop once again if we duplicated any input buffer
            if(duped_IPbuffer && nInMsgCount) {
                if( pVidDecComp && pVidDecComp->sCodecConfig.sBuffer && pVidDecComp->nOutPortReconfigRequired == 0 && went_thru_loop) {
                    memplugin_free((void*)H2P(pVidDecComp->sCodecConfig.sBuffer));
                    pVidDecComp->sCodecConfig.sBuffer = NULL;
            }
            continue;
        }
        goto EXIT;
    }
    // Increment the FrameCounter value
    pVidDecComp->nFrameCounter++;
    if( pDecOutArgs->outBufsInUseFlag && pVidDecComp->nOutbufInUseFlag == 1 ) {
        pVidDecComp->nFrameCounter--;
    }
    if( pDecOutArgs->outBufsInUseFlag ) {
        pVidDecComp->nOutbufInUseFlag = 1;
        /* Check for any output buffer which is freed by codec*/
        ii = 0;
        while( pDecOutArgs->freeBufID[ii] ) {
            if( pDecOutArgs->outputID[0] == 0 && pDecOutArgs->freeBufID[ii] == pVidDecComp->pDecInArgs->inputID ) {
                pVidDecComp->nOutbufInUseFlag = 0;
                pVidDecComp->nFrameCounter--;
            }
            pFreeBufHeader = (OMX_BUFFERHEADERTYPE *) pDecOutArgs->freeBufID[ii];
            grallocHandle = (IMG_native_handle_t*)(pFreeBufHeader->pBuffer);
            pVidDecComp->grallocModule->unlock((gralloc_module_t const *) pVidDecComp->grallocModule, (buffer_handle_t)grallocHandle);

            /* Send the Freed buffer back to base component */
            pVidDecComp->sBase.pPvtData->fpDioCancel(hComponent,
                                                OMX_VIDDEC_OUTPUT_PORT, pFreeBufHeader);
            ii++;
        }

        if(((IVIDDEC3_Params *)(pVidDecComp->pDecStaticParams))->inputDataMode == IVIDEO_ENTIREFRAME ) {
            ii = 0;
            if( pInBufHeader != NULL ) {
                if((pInBufHeader->nFilledLen - pInBufHeader->nOffset) != pDecOutArgs->bytesConsumed ) {
                    nNewInBufferRequired = 1;
                    pInBufHeader->nOffset = pInBufHeader->nOffset + pDecOutArgs->bytesConsumed;
                    continue;
                }
                nNewInBufferRequired = 0;
                pInBufHeader->nOffset = pInBufHeader->nOffset + pDecOutArgs->bytesConsumed;
                if( !(pInBufHeader->nFlags & OMX_BUFFERFLAG_EOS)) {
                    // Send Input buffer back to base
                    pVidDecComp->sBase.pPvtData->fpDioSend(hComponent,
                                                    OMX_VIDDEC_INPUT_PORT, pInBufHeader);
                } else if( pInBufHeader->nFlags & OMX_BUFFERFLAG_EOS ) {
                    OMXVidDec_HandleFLUSH_EOS(hComponent, NULL, pInBufHeader);
                    if( eError != OMX_ErrorNone ) {
                        goto EXIT;
                    }
                }
            }
            pVidDecComp->sBase.pPvtData->fpDioGetCount(hComponent,
                                        OMX_VIDDEC_INPUT_PORT, (OMX_PTR)&nInMsgCount);
            if( nInMsgCount > 0 ) {
                continue;
            } else {
                break;
            }
        }
    }

    // Check for the width/height after first frame
    if((pVidDecComp->nFrameCounter == 1) || (pVidDecComp->nOutbufInUseFlag == 1 && pVidDecComp->nFrameCounter == 2)) {
        eError = OMXVidDec_HandleFirstFrame(hComponent, &(pInBufHeader));
        if( eError != OMX_ErrorNone ) {
            goto EXIT;
        }
        if( pVidDecComp->nOutPortReconfigRequired == 1
                && pVidDecComp->pDecStaticParams->displayDelay
                != IVIDDEC3_DECODE_ORDER ) {
            /*! In case Port reconfiguration is required
            *  output buffer */
            goto EXIT;
        }
    }     // End of if condition from nFrameCounter = 1

    if( pVidDecComp->nOutbufInUseFlag == 1 ) {
        pVidDecComp->nOutbufInUseFlag = 0;
        nNewInBufferRequired = 0;
    }
    ii = 0;
    while( pDecOutArgs->outputID[ii] ) {
        pOutBufHeader = (OMX_BUFFERHEADERTYPE *)pDecOutArgs->outputID[ii];
        if( pVidDecComp->bSupportDecodeOrderTimeStamp == OMX_TRUE ) {
            OSAL_ReadFromPipe(pVidDecComp->pTimeStampStoragePipe, &(pOutBufHeader->nTimeStamp),
                                sizeof(OMX_TICKS), &(nActualSize), OSAL_NO_SUSPEND);
        }

        activeFrameRegion[0] = pDecOutArgs->displayBufs.bufDesc[0].activeFrameRegion;
        activeFrameRegion[1].bottomRight.y = (activeFrameRegion[0].bottomRight.y) / 2;
        activeFrameRegion[1].bottomRight.x = activeFrameRegion[0].bottomRight.x;

        pOutBufHeader->nOffset = (activeFrameRegion[0].topLeft.y * nStride)
                                + activeFrameRegion[0].topLeft.x;

        nLumaFilledLen = (nStride * (pVidDecComp->t2DBufferAllocParams[OMX_VIDDEC_OUTPUT_PORT].nHeight))
                        - (nStride * (activeFrameRegion[0].topLeft.y))
                        - activeFrameRegion[0].topLeft.x;

        nChromaFilledLen = (nStride * (activeFrameRegion[1].bottomRight.y))
                        - nStride + activeFrameRegion[1].bottomRight.x;

        pOutBufHeader->nFilledLen = nLumaFilledLen + nChromaFilledLen;

        if((pVidDecComp->tCropDimension.nTop != activeFrameRegion[0].topLeft.y
                || pVidDecComp->tCropDimension.nLeft != activeFrameRegion[0].topLeft.x)
                || (pVidDecComp->tCropDimension.nWidth != activeFrameRegion[0].bottomRight.x - activeFrameRegion[0].topLeft.x
                || pVidDecComp->tCropDimension.nHeight != activeFrameRegion[0].bottomRight.y - activeFrameRegion[0].topLeft.y)) {
            pVidDecComp->tCropDimension.nTop = activeFrameRegion[0].topLeft.y;
            pVidDecComp->tCropDimension.nLeft = activeFrameRegion[0].topLeft.x;
            pVidDecComp->tCropDimension.nWidth = activeFrameRegion[0].bottomRight.x - activeFrameRegion[0].topLeft.x;
            pVidDecComp->tCropDimension.nHeight = activeFrameRegion[0].bottomRight.y - activeFrameRegion[0].topLeft.y;
            if( pVidDecComp->bUsePortReconfigForCrop == OMX_TRUE ) {
                eError = pVidDecComp->sBase.fpReturnEventNotify(hComponent, OMX_EventPortSettingsChanged,
                                                            OMX_VIDDEC_OUTPUT_PORT, OMX_IndexConfigCommonOutputCrop, NULL);
                if( eError != OMX_ErrorNone ) {
                    OSAL_ErrorTrace("Port reconfig callback returned error, trying to continue");
                }
            }
        }

        if( pVidDecComp->bSupportSkipGreyOutputFrames ) {
            if( pDecOutArgs->displayBufs.bufDesc[0].frameType == IVIDEO_I_FRAME ||
                    pDecOutArgs->displayBufs.bufDesc[0].frameType == IVIDEO_IDR_FRAME ) {
                pVidDecComp->bSyncFrameReady = OMX_TRUE;
            }
        }
        grallocHandle = (IMG_native_handle_t*)(pOutBufHeader->pBuffer);
        pVidDecComp->grallocModule->unlock((gralloc_module_t const *) pVidDecComp->grallocModule, (buffer_handle_t)grallocHandle);

        if( pVidDecComp->bSyncFrameReady == OMX_TRUE ) {
            if( !(pInBufHeader->nFlags & OMX_BUFFERFLAG_EOS) || !(pVidDecComp->bIsFlushRequired)) {
                // Send the Output buffer to Base component
                pVidDecComp->sBase.pPvtData->fpDioSend(hComponent,
                                                OMX_VIDDEC_OUTPUT_PORT, pOutBufHeader);
            }
        } else {
            pVidDecComp->sBase.pPvtData->fpDioCancel(hComponent,
                                                OMX_VIDDEC_OUTPUT_PORT, pOutBufHeader);
        }
        pDecOutArgs->outputID[ii] = 0;
        ii++;
    }
    if(((IVIDDEC3_Params *)(pVidDecComp->pDecStaticParams))->inputDataMode == IVIDEO_ENTIREFRAME ) {
        if( pInBufHeader && pDecOutArgs &&
                (pVidDecComp->pDecDynParams->decodeHeader == XDM_DECODE_AU && pVidDecComp->bSupportDecodeOrderTimeStamp == OMX_TRUE)) {
            if((pVidDecComp->bInputBufferCancelled == 1) && (((pDecOutArgs->bytesConsumed == 0) || (((OMX_S32)pInBufHeader->nFilledLen)
                    <= ((OMX_S32)pInBufHeader->nOffset + pDecOutArgs->bytesConsumed + 3))) ||
                    (pVidDecComp->pDecOutArgs->extendedError & 0x8000))) {
                pVidDecComp->bInputBufferCancelled = 0;
            } else if((((OMX_S32)pInBufHeader->nFilledLen) > ((OMX_S32)pInBufHeader->nOffset + pDecOutArgs->bytesConsumed + 3)) &&
                        ((pDecOutArgs->bytesConsumed != 0) && !(pInBufHeader->nFlags & OMX_BUFFERFLAG_EOS))) {
                pInBufHeader->nOffset = pInBufHeader->nOffset + pDecOutArgs->bytesConsumed;
                pVidDecComp->bInputBufferCancelled = 1;
                pVidDecComp->sBase.pPvtData->fpDioCancel(hComponent, OMX_VIDDEC_INPUT_PORT, pInBufHeader);
            }
        }
        /* Currently the assumption is that the entire input buffer is consumed
        * going forward we might have to handle cases where partial buffer is
        * consumed */
        if( pVidDecComp->bInputBufferCancelled == 0 ) {
            if( pInBufHeader != NULL &&
                    !(pInBufHeader->nFlags & OMX_BUFFERFLAG_EOS) &&
                    pVidDecComp->nOutPortReconfigRequired != 1 ) {
                pInBufHeader->nFilledLen
                            = pInBufHeader->nFilledLen - pDecOutArgs->bytesConsumed - pInBufHeader->nOffset;
                pInBufHeader->nOffset = 0;
                // Send Input buffer back to base
                pVidDecComp->sBase.pPvtData->fpDioSend(hComponent,
                                                OMX_VIDDEC_INPUT_PORT, pInBufHeader);
                }
                else if( pInBufHeader != NULL && (pInBufHeader->nFlags & OMX_BUFFERFLAG_EOS) &&
                        pVidDecComp->nOutPortReconfigRequired != 1 ) {
                    pInBufHeader->nFilledLen
                                = pInBufHeader->nFilledLen - pDecOutArgs->bytesConsumed - pInBufHeader->nOffset;
                    pInBufHeader->nOffset = 0;
                    if( Buffer_locked == 1 ) {
                        OMXVidDec_HandleFLUSH_EOS(hComponent, pDupBufHeader, pInBufHeader);
                    } else {
                        OMXVidDec_HandleFLUSH_EOS(hComponent, pOutBufHeader, pInBufHeader);
                    }
                    if( eError != OMX_ErrorNone ) {
                        goto EXIT;
                    }
                }
            }
        }
        nIsDioReady = OMXBase_IsDioReady(hComponent, OMX_VIDDEC_OUTPUT_PORT);
        if( nIsDioReady == OMX_FALSE ) {
            goto EXIT;
        }
        // Get the number of buffers in Input and Output port
        pVidDecComp->sBase.pPvtData->fpDioGetCount(hComponent,
                                    OMX_VIDDEC_INPUT_PORT, (OMX_PTR)&nInMsgCount);
        pVidDecComp->sBase.pPvtData->fpDioGetCount(hComponent,
                                    OMX_VIDDEC_OUTPUT_PORT, (OMX_PTR)&nOutMsgCount);

    } // End of while loop for input and output buffers

EXIT:
    if( pVidDecComp && pVidDecComp->sCodecConfig.sBuffer && pVidDecComp->nOutPortReconfigRequired == 0 && went_thru_loop) {
        memplugin_free((void*)H2P(pVidDecComp->sCodecConfig.sBuffer));
        pVidDecComp->sCodecConfig.sBuffer = NULL;
        }
    return (eError);
}

/*
* Video Decoder DeInit
*/
OMX_ERRORTYPE OMXVidDec_ComponentDeinit(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE   *pComp;
    OMXVidDecComp       *pVidDecComp = NULL;
    OMX_U32             i;
    OMXBase_PortProps   *pOutPortProperties = NULL;

    OMX_CHECK(hComponent != NULL, OMX_ErrorBadParameter);
    pComp = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp = (OMXVidDecComp *)pComp->pComponentPrivate;

    OSAL_DeletePipe(pVidDecComp->pTimeStampStoragePipe);

    OMXBase_UtilCleanupIfError(hComponent);

    // Call to Specific Decoder De-init routine
    pVidDecComp->fpDeinit_Codec(hComponent);
    // Close Codec-Engine
    if( pVidDecComp->ce ) {
        Engine_close(pVidDecComp->ce);
    }

    if( pVidDecComp->sBase.cComponentName ) {
        OSAL_Free(pVidDecComp->sBase.cComponentName);
        pVidDecComp->sBase.cComponentName = NULL;
    }
    if( pVidDecComp->sBase.pVideoPortParams ) {
        OSAL_Free(pVidDecComp->sBase.pVideoPortParams);
        pVidDecComp->sBase.pVideoPortParams = NULL;
    }

    if (pVidDecComp->tInBufDesc) {
        memplugin_free(pVidDecComp->tInBufDesc);
        pVidDecComp->tInBufDesc = NULL;
    }

    if (pVidDecComp->tOutBufDesc) {
        memplugin_free(pVidDecComp->tOutBufDesc);
        pVidDecComp->tOutBufDesc = NULL;
    }

    // Call to base Component De-init routine
    eError = OMXBase_ComponentDeinit(hComponent);
    OSAL_Free(pVidDecComp);
    pVidDecComp = NULL;

EXIT:
    return (eError);
}

/*
* GetExtension Index
*/
OMX_ERRORTYPE OMXVidDec_GetExtensionIndex(OMX_HANDLETYPE hComponent,
                                                    OMX_STRING cParameterName, OMX_INDEXTYPE *pIndexType)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE   *pHandle = NULL;
    OMXVidDecComp       *pVidDecComp = NULL;

    OMX_CHECK(hComponent != NULL, OMX_ErrorBadParameter);
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp = (OMXVidDecComp *)pHandle->pComponentPrivate;
    // Check for NULL Parameters
    if((cParameterName == NULL) || (pIndexType == NULL)) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    // Check for Valid State
    if( pVidDecComp->sBase.tCurState == OMX_StateInvalid ) {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
    // Ensure that String length is not greater than Max allowed length
    if( strlen(cParameterName) > 127 ) {
        //strlen does not include \0 size, hence 127
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if(strcmp(cParameterName, "OMX.google.android.index.enableAndroidNativeBuffers") == 0) {
        ALOGE("OMX_TI_IndexUseNativeBuffers\n");
        // If Index type is 2D Buffer Allocated Dimension
        *pIndexType = (OMX_INDEXTYPE) OMX_TI_IndexUseNativeBuffers;
        goto EXIT;
    } else if (strcmp(cParameterName, "OMX.google.android.index.useAndroidNativeBuffer2") == 0) {
        //This is call just a dummy for android to support backward compatibility
        *pIndexType = (OMX_INDEXTYPE) NULL;
        goto EXIT;
    } else if (strcmp(cParameterName, "OMX.google.android.index.getAndroidNativeBufferUsage") == 0) {
        ALOGE("chekcing OMX.google.android.index.getAndroidNativeBufferUsage\n");
        *pIndexType = (OMX_INDEXTYPE) OMX_TI_IndexAndroidNativeBufferUsage;
    } else if( strcmp(cParameterName, "OMX_TI_IndexParamTimeStampInDecodeOrder") == 0 ) {
        // If Index type is Time Stamp In Decode Order
        *pIndexType = (OMX_INDEXTYPE) OMX_TI_IndexParamTimeStampInDecodeOrder;
    } else {
        //does not match any custom index
        eError = OMX_ErrorUnsupportedIndex;
    }

EXIT:
    return (eError);
}


