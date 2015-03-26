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

#define LOG_TAG "OMX_MPEG4_VIDDEC"
#include <omx_mpeg4vd.h>

OMX_ERRORTYPE OMXMPEG4VD_Init(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE               eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE          *pHandle = NULL;
    OMXVidDecComp   *pVidDecComp = NULL;
    OMXMPEG4VidDecComp        *pMPEG4VidDecComp = NULL;

    eError = OMXMPEG4_H263VD_Init(hComponent);

    pHandle     = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp = (OMXVidDecComp *)pHandle->pComponentPrivate;

    pMPEG4VidDecComp =(OMXMPEG4VidDecComp *) pVidDecComp->pCodecSpecific;
    pVidDecComp->tVideoParams[OMX_VIDDEC_INPUT_PORT].eCompressionFormat = OMX_VIDEO_CodingMPEG4;
    pVidDecComp->tVideoParams[OMX_VIDDEC_OUTPUT_PORT].eCompressionFormat = OMX_VIDEO_CodingUnused;
    pMPEG4VidDecComp->bIsSorensonSpark = OMX_FALSE;

    return (eError);
}

OMX_ERRORTYPE OMXH263VD_Init(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE               eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE          *pHandle = NULL;
    OMXVidDecComp   *pVidDecComp = NULL;
    OMXMPEG4VidDecComp        *pMPEG4VidDecComp = NULL;

    eError = OMXMPEG4_H263VD_Init(hComponent);

    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp = (OMXVidDecComp *)pHandle->pComponentPrivate;
    pMPEG4VidDecComp =(OMXMPEG4VidDecComp *) pVidDecComp->pCodecSpecific;

    pVidDecComp->tVideoParams[OMX_VIDDEC_INPUT_PORT].eCompressionFormat = OMX_VIDEO_CodingH263;
    pVidDecComp->tVideoParams[OMX_VIDDEC_OUTPUT_PORT].eCompressionFormat = OMX_VIDEO_CodingUnused;

    pMPEG4VidDecComp->bIsSorensonSpark = OMX_FALSE;

    return (eError);
}

OMX_ERRORTYPE OMXMPEG4_H263VD_Init(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE   *pHandle = NULL;
    OMXVidDecComp       *pVidDecComp = NULL;
    OMXMPEG4VidDecComp  *pMPEG4VidDecComp = NULL;

    pHandle     = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp = (OMXVidDecComp *)pHandle->pComponentPrivate;

    /*! Initialize the function pointers */
    pVidDecComp->fpSet_StaticParams     = OMXMPEG4VD_SetStaticParams;
    pVidDecComp->fpSet_DynamicParams    = OMXMPEG4VD_SetDynamicParams;
    pVidDecComp->fpSet_Status           = OMXMPEG4VD_SetStatus;
    pVidDecComp->fpCalc_OubuffDetails   = CalculateMPEG4VD_outbuff_details;
    pVidDecComp->fpDeinit_Codec         = OMXMPEG4VD_DeInit;
    pVidDecComp->cDecoderName           = "ivahd_mpeg4dec";
    pHandle->SetParameter               = OMXMPEG4VD_SetParameter;
    pHandle->GetParameter               = OMXMPEG4VD_GetParameter;
    pVidDecComp->fpHandle_ExtendedError = OMXMPEG4VD_HandleError;

    /*! Initialize the framerate divisor to 10000 as WA because MPEG4 codec is providing framerate by multiplying with 10000 instead of 1000*/
    pVidDecComp->nFrameRateDivisor   = 10000;

    pVidDecComp->pCodecSpecific = (OMXMPEG4VidDecComp *) OSAL_Malloc(sizeof (OMXMPEG4VidDecComp));
    OMX_CHECK((pVidDecComp->pCodecSpecific) != NULL, OMX_ErrorInsufficientResources);

    pMPEG4VidDecComp =(OMXMPEG4VidDecComp *) pVidDecComp->pCodecSpecific;

    OMX_BASE_INIT_STRUCT_PTR(&(pMPEG4VidDecComp->tDeblockingParam), OMX_PARAM_DEBLOCKINGTYPE);
    pMPEG4VidDecComp->tDeblockingParam.nPortIndex = OMX_VIDDEC_OUTPUT_PORT;
    pMPEG4VidDecComp->tDeblockingParam.bDeblocking = OMX_TRUE;

    OMX_BASE_INIT_STRUCT_PTR(&(pMPEG4VidDecComp->tMPEG4VideoParam), OMX_VIDEO_PARAM_MPEG4TYPE);
    pMPEG4VidDecComp->tMPEG4VideoParam.nPortIndex = OMX_VIDDEC_INPUT_PORT;
    pMPEG4VidDecComp->tMPEG4VideoParam.eProfile = OMX_VIDEO_MPEG4ProfileAdvancedSimple;
    pMPEG4VidDecComp->tMPEG4VideoParam.eLevel = OMX_VIDEO_MPEG4Level5;

    /*! Allocate Memory for Static Parameter */
    pVidDecComp->pDecStaticParams   = (IVIDDEC3_Params *) memplugin_alloc(sizeof(IMPEG4VDEC_Params), 1, MEM_CARVEOUT, 0, 0);
    OMX_CHECK(pVidDecComp->pDecStaticParams != NULL, OMX_ErrorInsufficientResources);
    OSAL_Memset(pVidDecComp->pDecStaticParams, 0x0, sizeof(IMPEG4VDEC_Params));

    /*! Allocate Memory for Dynamic Parameter */
    pVidDecComp->pDecDynParams = (IVIDDEC3_DynamicParams *)memplugin_alloc(sizeof(IMPEG4VDEC_DynamicParams), 1, MEM_CARVEOUT, 0, 0);
    OMX_CHECK(pVidDecComp->pDecDynParams != NULL, OMX_ErrorInsufficientResources);
    OSAL_Memset(pVidDecComp->pDecDynParams, 0x0, sizeof(IMPEG4VDEC_DynamicParams));

    /*! Allocate Memory for Status Structure */
    pVidDecComp->pDecStatus = (IVIDDEC3_Status *) memplugin_alloc(sizeof(IMPEG4VDEC_Status), 1, MEM_CARVEOUT, 0, 0);
    OMX_CHECK(pVidDecComp->pDecStatus != NULL, OMX_ErrorInsufficientResources);
    OSAL_Memset(pVidDecComp->pDecStatus, 0x0, sizeof(IMPEG4VDEC_Status));

    /*! Allocate Memory for Input Arguments */
    pVidDecComp->pDecInArgs = (IVIDDEC3_InArgs *) memplugin_alloc(sizeof(IMPEG4VDEC_InArgs), 1, MEM_CARVEOUT, 0, 0);
    OMX_CHECK(pVidDecComp->pDecInArgs != NULL, OMX_ErrorInsufficientResources);
    OSAL_Memset(pVidDecComp->pDecInArgs, 0x0, sizeof(IMPEG4VDEC_InArgs));

    /*! Allocate Memory for Output Arguments */
    pVidDecComp->pDecOutArgs    = (IVIDDEC3_OutArgs *) memplugin_alloc(sizeof(IMPEG4VDEC_OutArgs), 1, MEM_CARVEOUT, 0, 0);
    OMX_CHECK(pVidDecComp->pDecOutArgs != NULL, OMX_ErrorInsufficientResources);
    OSAL_Memset(pVidDecComp->pDecOutArgs, 0x0, sizeof(IMPEG4VDEC_OutArgs));

    pVidDecComp->pDecInArgs->size   = sizeof(IMPEG4VDEC_InArgs);
    pVidDecComp->pDecOutArgs->size  = sizeof(IMPEG4VDEC_OutArgs);

    pVidDecComp->pDecStaticParams->metadataType[0] = IVIDEO_METADATAPLANE_NONE;
    pVidDecComp->pDecStaticParams->metadataType[1] = IVIDEO_METADATAPLANE_NONE;
    pVidDecComp->pDecStaticParams->metadataType[2] = IVIDEO_METADATAPLANE_NONE;

    pVidDecComp->pDecStaticParams->operatingMode = IVIDEO_DECODE_ONLY;

    pVidDecComp->pDecStaticParams->inputDataMode = IVIDEO_ENTIREFRAME;
    pVidDecComp->pDecStaticParams->numInputDataUnits  = 0;

EXIT:
    return (eError);
}

void OMXMPEG4VD_SetStaticParams(OMX_HANDLETYPE hComponent, void *staticparams)
{
    OMX_COMPONENTTYPE          *pHandle = NULL;
    OMXVidDecComp   *pVidDecComp = NULL;
    OMXMPEG4VidDecComp        *pMPEG4VidDecComp = NULL;
    IMPEG4VDEC_Params          *params;

    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp
        = (OMXVidDecComp *)pHandle->pComponentPrivate;

    pMPEG4VidDecComp =
        (OMXMPEG4VidDecComp *) pVidDecComp->pCodecSpecific;

    params = (IMPEG4VDEC_Params *) staticparams;

    params->viddec3Params.size = sizeof(IMPEG4VDEC_Params);
    params->viddec3Params.maxFrameRate        = 300000;
    params->viddec3Params.maxBitRate          = 10000000;
    params->viddec3Params.dataEndianness      = XDM_BYTE;
    params->viddec3Params.forceChromaFormat      = XDM_YUV_420SP;
    params->viddec3Params.displayDelay      = IVIDDEC3_DISPLAY_DELAY_1;
    params->viddec3Params.inputDataMode      = IVIDEO_ENTIREFRAME;
    params->viddec3Params.numInputDataUnits  = 0;
    params->viddec3Params.outputDataMode      = IVIDEO_ENTIREFRAME;
    params->viddec3Params.displayBufsMode = IVIDDEC3_DISPLAYBUFS_EMBEDDED;

    params->viddec3Params.errorInfoMode     = IVIDEO_ERRORINFO_OFF;
    params->outloopDeBlocking               = IMPEG4VDEC_DEBLOCK_DISABLE;
    params->enhancedDeBlockingQp            = 31;
    params->sorensonSparkStream             = pMPEG4VidDecComp->bIsSorensonSpark;
    params->errorConcealmentEnable = pVidDecComp->sBase.pPorts[OMX_VIDDEC_INPUT_PORT]->sPortDef.format.video.bFlagErrorConcealment;
    params->debugTraceLevel                 = 0;
    params->lastNFramesToLog                = 0;
    params->paddingMode                     = IMPEG4VDEC_MPEG4_MODE_PADDING;

    return;
}


void OMXMPEG4VD_SetDynamicParams(OMX_HANDLETYPE hComponent, void *dynParams)
{
    IMPEG4VDEC_DynamicParams   *dynamicParams;
    dynamicParams = (IMPEG4VDEC_DynamicParams *)dynParams;

    /*! Update the Individual fields in the Dyanmic Params of MPEG4 decoder */
    dynamicParams->viddec3DynamicParams.size            = sizeof(IMPEG4VDEC_DynamicParams);
    dynamicParams->viddec3DynamicParams.decodeHeader    = XDM_DECODE_AU;
    dynamicParams->viddec3DynamicParams.displayWidth    = 0;
    dynamicParams->viddec3DynamicParams.frameSkipMode   = IVIDEO_NO_SKIP;
    dynamicParams->viddec3DynamicParams.newFrameFlag    = XDAS_TRUE;
    dynamicParams->viddec3DynamicParams.putDataFxn      = NULL;
    dynamicParams->viddec3DynamicParams.putDataHandle   = NULL;
    dynamicParams->viddec3DynamicParams.getDataFxn      = NULL;
    dynamicParams->viddec3DynamicParams.getDataHandle   = NULL;
    dynamicParams->viddec3DynamicParams.putBufferFxn    = NULL;
    dynamicParams->viddec3DynamicParams.putBufferHandle = NULL;
    return;
}


void OMXMPEG4VD_SetStatus(OMX_HANDLETYPE hComponent, void *decstatus)
{
    IMPEG4VDEC_Status   *status;
    status = (IMPEG4VDEC_Status *)decstatus;

    status->viddec3Status.size     = sizeof(IMPEG4VDEC_Status);
    return;
}


PaddedBuffParams CalculateMPEG4VD_outbuff_details(OMX_HANDLETYPE hComponent,
                                                 OMX_U32 width, OMX_U32 height)
{
    PaddedBuffParams    OutBuffDetails;

    OutBuffDetails.nBufferSize = ((((width + PADX + 127) & ~127) * (height + PADY)));
    /* Multiply buffer size by 1.5 to account for both luma and chroma */
    OutBuffDetails.nBufferSize = (OutBuffDetails.nBufferSize * 3) >> 1;

    OutBuffDetails.nBufferCountMin      = 4;
    OutBuffDetails.nBufferCountActual   = 8;
    OutBuffDetails.n1DBufferAlignment   = 16;
    OutBuffDetails.nPaddedWidth         = (width + PADX + 127) & ~127;
    OutBuffDetails.nPaddedHeight        = height + PADY;
    OutBuffDetails.n2DBufferYAlignment  = 1;
    OutBuffDetails.n2DBufferXAlignment  = 16;
    return (OutBuffDetails);
}


void OMXMPEG4VD_DeInit(OMX_HANDLETYPE hComponent)
{
    OMX_COMPONENTTYPE          *pHandle = NULL;
    OMXVidDecComp   *pVidDecComp = NULL;

    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp = (OMXVidDecComp *)pHandle->pComponentPrivate;
    /*! Delete all the memory which was allocated during init of decoder */
    if( pVidDecComp->pDecStaticParams ) {
        memplugin_free(pVidDecComp->pDecStaticParams);
        pVidDecComp->pDecStaticParams = NULL;
    }
    if( pVidDecComp->pDecDynParams ) {
        memplugin_free(pVidDecComp->pDecDynParams);
        pVidDecComp->pDecDynParams = NULL;
    }
    if( pVidDecComp->pDecStatus ) {
        memplugin_free(pVidDecComp->pDecStatus);
        pVidDecComp->pDecStatus = NULL;
    }
    if( pVidDecComp->pDecInArgs ) {
        memplugin_free(pVidDecComp->pDecInArgs);
        pVidDecComp->pDecInArgs = NULL;
    }
    if( pVidDecComp->pDecOutArgs ) {
        memplugin_free(pVidDecComp->pDecOutArgs);
        pVidDecComp->pDecOutArgs = NULL;
    }
    if( pVidDecComp->pCodecSpecific ) {
        OSAL_Free(pVidDecComp->pCodecSpecific);
        pVidDecComp->pCodecSpecific = NULL;
    }
    pHandle->SetParameter = OMXVidDec_SetParameter;
    pHandle->GetParameter = OMXVidDec_GetParameter;
    pVidDecComp->fpHandle_ExtendedError = NULL;
    pVidDecComp->nFrameRateDivisor      = 1000;
}

OMX_ERRORTYPE OMXMPEG4VD_SetParameter(OMX_HANDLETYPE hComponent,
                                          OMX_INDEXTYPE nIndex, OMX_PTR pParamStruct)
{
    OMX_ERRORTYPE           eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE       *pHandle = NULL;
    OMXVidDecComp           *pVidDecComp  = NULL;
    OMXMPEG4VidDecComp      *pMPEG4VidDecComp = NULL;
    OMX_PARAM_DEBLOCKINGTYPE    *pDeblockingParam = NULL;
    OMX_VIDEO_PARAM_MPEG4TYPE   *pMPEG4VideoParam = NULL;

    OMX_CHECK((hComponent != NULL) && (pParamStruct != NULL),
    OMX_ErrorBadParameter);

    /*! Initialize the pointers */
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp = (OMXVidDecComp *)pHandle->pComponentPrivate;
    pMPEG4VidDecComp =(OMXMPEG4VidDecComp *) pVidDecComp->pCodecSpecific;

    switch( nIndex ) {
        case OMX_IndexParamVideoMpeg4 :
            pMPEG4VideoParam = (OMX_VIDEO_PARAM_MPEG4TYPE *) pParamStruct;
            /* SetParameter can be invoked in Loaded State  or on Disabled ports only*/
            OMX_CHECK((pVidDecComp->sBase.tCurState == OMX_StateLoaded) ||
                    (pVidDecComp->sBase.pPorts[pMPEG4VideoParam->nPortIndex]->sPortDef.bEnabled == OMX_FALSE),
                    OMX_ErrorIncorrectStateOperation);

            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_MPEG4TYPE, eError);
            OMX_CHECK(pMPEG4VideoParam->eProfile == OMX_VIDEO_MPEG4ProfileSimple
                    || pMPEG4VideoParam->eProfile == OMX_VIDEO_MPEG4ProfileMain
                    || pMPEG4VideoParam->eProfile == OMX_VIDEO_MPEG4ProfileAdvancedSimple,
                    OMX_ErrorUnsupportedSetting);
            if( pMPEG4VideoParam->nPortIndex == OMX_VIDDEC_INPUT_PORT ) {
                pMPEG4VidDecComp->tMPEG4VideoParam = *pMPEG4VideoParam;
            } else if( pMPEG4VideoParam->nPortIndex == OMX_VIDDEC_OUTPUT_PORT ) {
                OSAL_ErrorTrace("OMX_IndexParamVideoMpeg4 supported only on i/p port");
                eError = OMX_ErrorUnsupportedIndex;
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
    break;

    default :
        eError = OMXVidDec_SetParameter(hComponent, nIndex, pParamStruct);
    }

EXIT:
    return (eError);
}

OMX_ERRORTYPE OMXMPEG4VD_GetParameter(OMX_HANDLETYPE hComponent,
                                          OMX_INDEXTYPE nIndex, OMX_PTR pParamStruct)
{
    OMX_ERRORTYPE               eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE           *pHandle = NULL;
    OMXVidDecComp               *pVidDecComp  = NULL;
    OMXMPEG4VidDecComp          *pMPEG4VidDecComp = NULL;
    OMX_PARAM_DEBLOCKINGTYPE    *pDeblockingParam = NULL;
    OMX_VIDEO_PARAM_MPEG4TYPE   *pMPEG4VideoParam = NULL;
    OMX_VIDEO_PARAM_PROFILELEVELTYPE    *pMPEG4ProfileLevelParam = NULL;

    OMX_CHECK((hComponent != NULL) && (pParamStruct != NULL),
                OMX_ErrorBadParameter);

    /*! Initialize the pointers */
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp = (OMXVidDecComp *)pHandle->pComponentPrivate;
    /* GetParameter can't be invoked incase the comp is in Invalid State  */
    OMX_CHECK(pVidDecComp->sBase.tCurState != OMX_StateInvalid,
                OMX_ErrorIncorrectStateOperation);

    pMPEG4VidDecComp =(OMXMPEG4VidDecComp *) pVidDecComp->pCodecSpecific;

    switch( nIndex ) {
        case OMX_IndexParamVideoMpeg4 :
            pMPEG4VideoParam = (OMX_VIDEO_PARAM_MPEG4TYPE *) pParamStruct;
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_MPEG4TYPE, eError);
            if( pMPEG4VideoParam->nPortIndex == OMX_VIDDEC_INPUT_PORT ) {
                *pMPEG4VideoParam = pMPEG4VidDecComp->tMPEG4VideoParam;
            } else if( pMPEG4VideoParam->nPortIndex == OMX_VIDDEC_OUTPUT_PORT ) {
                OSAL_ErrorTrace("OMX_IndexParamVideoMpeg4 supported only on i/p port");
                eError = OMX_ErrorUnsupportedIndex;
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
        break;
        case OMX_IndexParamVideoProfileLevelQuerySupported :
        {
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_PROFILELEVELTYPE, eError);
            pMPEG4ProfileLevelParam = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pParamStruct;
            if( pMPEG4ProfileLevelParam->nPortIndex == OMX_VIDDEC_INPUT_PORT ) {
                if( pMPEG4ProfileLevelParam->nProfileIndex == 0 ) {
                    if( pVidDecComp->sBase.pPorts[pMPEG4ProfileLevelParam->nPortIndex]->sPortDef.format.video.eCompressionFormat == OMX_VIDEO_CodingMPEG4 ) {
                        pMPEG4ProfileLevelParam->eProfile = (OMX_U32) OMX_VIDEO_MPEG4ProfileSimple;
                        pMPEG4ProfileLevelParam->eLevel = (OMX_U32) OMX_VIDEO_MPEG4LevelMax;
                    } else if( pVidDecComp->sBase.pPorts[pMPEG4ProfileLevelParam->nPortIndex]->sPortDef.format.video.eCompressionFormat == OMX_VIDEO_CodingH263 ) {
                        pMPEG4ProfileLevelParam->eProfile = (OMX_U32) OMX_VIDEO_H263ProfileBaseline;
                        pMPEG4ProfileLevelParam->eLevel = (OMX_U32) OMX_VIDEO_H263Level70;
                    }
                } else if( pMPEG4ProfileLevelParam->nProfileIndex == 1 ) {
                    if(pVidDecComp->sBase.pPorts[pMPEG4ProfileLevelParam->nPortIndex]->sPortDef.format.video.eCompressionFormat == OMX_VIDEO_CodingMPEG4 ) {
                        pMPEG4ProfileLevelParam->eProfile = (OMX_U32) OMX_VIDEO_MPEG4ProfileAdvancedSimple;
                        pMPEG4ProfileLevelParam->eLevel = (OMX_U32) OMX_VIDEO_MPEG4LevelMax;
                    } else if( pVidDecComp->sBase.pPorts[pMPEG4ProfileLevelParam->nPortIndex]->sPortDef.format.video.eCompressionFormat == OMX_VIDEO_CodingH263 ) {
                        pMPEG4ProfileLevelParam->eProfile = (OMX_U32) OMX_VIDEO_H263ProfileISWV2;
                        pMPEG4ProfileLevelParam->eLevel = (OMX_U32) OMX_VIDEO_H263Level70;
                    }
                } else {
                    eError = OMX_ErrorNoMore;
                }
            } else if( pMPEG4ProfileLevelParam->nPortIndex == OMX_VIDDEC_OUTPUT_PORT ) {
                OSAL_ErrorTrace("OMX_IndexParamVideoProfileLevelQuerySupported supported only on i/p port");
                eError = OMX_ErrorUnsupportedIndex;
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
        }
        break;
        default :
            eError = OMXVidDec_GetParameter(hComponent, nIndex, pParamStruct);
        }

EXIT:
    return (eError);
}


OMX_ERRORTYPE OMXMPEG4VD_HandleError(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE   *pHandle = NULL;
    OMXVidDecComp       *pVidDecComp = NULL;

    /* Initialize pointers */
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp = (OMXVidDecComp *)pHandle->pComponentPrivate;
    /* To send the output buffer to display in case of no valid VOL/VOP header */

    if((!(pVidDecComp->pDecOutArgs->extendedError & 0x8000)
            && ((pVidDecComp->pDecOutArgs->extendedError & 0x100000)
            || (pVidDecComp->pDecOutArgs->extendedError & 0x80000)))
            && (pVidDecComp->pDecInArgs->inputID == pVidDecComp->pDecOutArgs->freeBufID[0]
            && pVidDecComp->pDecOutArgs->outputID[0] == 0)) {
        pVidDecComp->pDecOutArgs->outputID[0] = pVidDecComp->pDecInArgs->inputID;
        pVidDecComp->bSyncFrameReady = OMX_TRUE;
    }

    return (eError);
}

