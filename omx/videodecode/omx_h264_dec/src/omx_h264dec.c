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

#define LOG_TAG "OMX_H264_VIDDEC"

#include <omx_h264vd.h>
#include <omx_video_decoder_internal.h>

#define OMX_MAX_DEC_OP_BUFFERS 20

OMX_ERRORTYPE OMXH264VD_Init(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE   *pHandle = NULL;
    OMXVidDecComp       *pVidDecComp  = NULL;
    OMXH264VidDecComp   *pH264VidDecComp = NULL;
    IVIDDEC3_Params     *pDecParams;
    IH264VDEC_Params    *params;

    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp  = (OMXVidDecComp *)pHandle->pComponentPrivate;

    /*! Initialize the function parameters for OMX functions */
    pHandle->SetParameter = OMXH264VD_SetParameter;
    pHandle->GetParameter = OMXH264VD_GetParameter;

    /*! Initialize the function pointers */
    pVidDecComp->fpSet_StaticParams = OMXH264VD_Set_StaticParams;
    pVidDecComp->fpSet_DynamicParams = OMXH264VD_Set_DynamicParams;
    pVidDecComp->fpSet_Status = OMXH264VD_Set_Status;
    pVidDecComp->fpCalc_OubuffDetails
                            = CalculateH264VD_outbuff_details;
    pVidDecComp->fpDeinit_Codec = OMXH264VD_DeInit;
    pVidDecComp->cDecoderName = "ivahd_h264dec";
    pVidDecComp->tVideoParams[OMX_VIDDEC_INPUT_PORT].eCompressionFormat = OMX_VIDEO_CodingAVC;
    pVidDecComp->tVideoParams[OMX_VIDDEC_OUTPUT_PORT].eCompressionFormat = OMX_VIDEO_CodingUnused;
    pVidDecComp->fpHandle_ExtendedError = OMXH264VD_HandleError;

    pVidDecComp->pCodecSpecific =
                        (OMXH264VidDecComp *) OSAL_Malloc(sizeof (OMXH264VidDecComp));
    OMX_CHECK((pVidDecComp->pCodecSpecific) != NULL, OMX_ErrorInsufficientResources);

    pH264VidDecComp =(OMXH264VidDecComp *) pVidDecComp->pCodecSpecific;

    OMX_BASE_INIT_STRUCT_PTR(&(pH264VidDecComp->tH264VideoParam), OMX_VIDEO_PARAM_AVCTYPE);
    pH264VidDecComp->tH264VideoParam.nPortIndex = OMX_VIDDEC_INPUT_PORT;
    pH264VidDecComp->tH264VideoParam.eProfile = OMX_VIDEO_AVCProfileHigh;
    pH264VidDecComp->tH264VideoParam.eLevel = OMX_VIDEO_AVCLevel41;
    pH264VidDecComp->tH264VideoParam.nRefFrames = 0xFFFFFFFF;

    /*! Allocate Memory for Static Parameter */
    pVidDecComp->pDecStaticParams
                    = (IVIDDEC3_Params *) memplugin_alloc(sizeof(IH264VDEC_Params), 1, MEM_CARVEOUT, 0, 0);
    OMX_CHECK(pVidDecComp->pDecStaticParams != NULL, OMX_ErrorInsufficientResources);
    OSAL_Memset(pVidDecComp->pDecStaticParams, 0x0, sizeof(IH264VDEC_Params));

    params = (IH264VDEC_Params *) (pVidDecComp->pDecStaticParams);
    pDecParams = &(params->viddec3Params);
    pDecParams->displayDelay  = IVIDDEC3_DISPLAY_DELAY_AUTO;
    params->presetLevelIdc =  IH264VDEC_LEVEL41;
    params->dpbSizeInFrames = IH264VDEC_DPB_NUMFRAMES_AUTO;

    /*! Allocate Memory for Dynamic Parameter */
    pVidDecComp->pDecDynParams
    = (IVIDDEC3_DynamicParams *) memplugin_alloc(sizeof(IH264VDEC_DynamicParams), 1, MEM_CARVEOUT, 0, 0);
    OMX_CHECK(pVidDecComp->pDecDynParams != NULL, OMX_ErrorInsufficientResources);
    OSAL_Memset(pVidDecComp->pDecDynParams, 0x0, sizeof(IH264VDEC_DynamicParams));

    /*! Allocate Memory for Status Structure */
    pVidDecComp->pDecStatus
                = (IVIDDEC3_Status *) memplugin_alloc(sizeof(IH264VDEC_Status), 1, MEM_CARVEOUT, 0, 0);
    OMX_CHECK(pVidDecComp->pDecStatus != NULL, OMX_ErrorInsufficientResources);
    OSAL_Memset(pVidDecComp->pDecStatus, 0x0, sizeof(IH264VDEC_Status));
    ((IH264VDEC_Status *)pVidDecComp->pDecStatus)->spsMaxRefFrames = 1;

    /*! Allocate Memory for Input Arguments */
    pVidDecComp->pDecInArgs
    = (IVIDDEC3_InArgs *) memplugin_alloc(sizeof(IH264VDEC_InArgs), 1, MEM_CARVEOUT, 0, 0);
    OMX_CHECK(pVidDecComp->pDecInArgs != NULL, OMX_ErrorInsufficientResources);
    OSAL_Memset(pVidDecComp->pDecInArgs, 0x0, sizeof(IH264VDEC_InArgs));

    /*! Allocate Memory for Output Arguments */
    pVidDecComp->pDecOutArgs
                    = (IVIDDEC3_OutArgs *) memplugin_alloc(sizeof(IH264VDEC_OutArgs), 1, MEM_CARVEOUT, 0, 0);
    OMX_CHECK(pVidDecComp->pDecOutArgs != NULL, OMX_ErrorInsufficientResources);
    OSAL_Memset(pVidDecComp->pDecOutArgs, 0x0, sizeof(IH264VDEC_OutArgs));

    pVidDecComp->pDecInArgs->size = sizeof(IH264VDEC_InArgs);
    pVidDecComp->pDecOutArgs->size = sizeof(IH264VDEC_OutArgs);

    pDecParams->metadataType[0] = IVIDEO_METADATAPLANE_NONE;
    pDecParams->metadataType[1] = IVIDEO_METADATAPLANE_NONE;
    pDecParams->metadataType[2] = IVIDEO_METADATAPLANE_NONE;

    pDecParams->operatingMode = IVIDEO_DECODE_ONLY;
    pDecParams->inputDataMode = IVIDEO_ENTIREFRAME;
    pDecParams->numInputDataUnits  = 0;

EXIT:
    return (eError);
}


void OMXH264VD_Set_StaticParams(OMX_HANDLETYPE hComponent, void *staticparams)
{
    OMX_COMPONENTTYPE   *pHandle = NULL;
    OMXVidDecComp       *pVidDecComp = NULL;
    IVIDDEC3_Params     *pDecParams;
    IH264VDEC_Params    *params = (IH264VDEC_Params *)staticparams;

    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp = (OMXVidDecComp *)pHandle->pComponentPrivate;

    pDecParams = &(params->viddec3Params);
    pDecParams->size = sizeof(IH264VDEC_Params);
    pDecParams->maxWidth = 1920;
    pDecParams->maxHeight = 1088;
    pDecParams->maxFrameRate = 30000;
    pDecParams->maxBitRate   = 10000000;
    pDecParams->dataEndianness = XDM_BYTE;

    /*init outArgs IVIDDEC3_OutArgs*/
    pDecParams->forceChromaFormat  = XDM_YUV_420SP;
    pDecParams->displayBufsMode = IVIDDEC3_DISPLAYBUFS_EMBEDDED;
    pDecParams->outputDataMode = IVIDEO_ENTIREFRAME;
    pDecParams->numOutputDataUnits = 0;
    pDecParams->errorInfoMode = IVIDEO_ERRORINFO_OFF;
    params->errConcealmentMode =
    pVidDecComp->sBase.pPorts[OMX_VIDDEC_INPUT_PORT]->sPortDef.format.video.bFlagErrorConcealment;

    params->temporalDirModePred = IH264VDEC_ENABLE_TEMPORALDIRECT;
    params->debugTraceLevel= 0;
    params->lastNFramesToLog= 0;

    return;
}


void OMXH264VD_Set_DynamicParams(OMX_HANDLETYPE hComponent, void *dynParams)
{
    OMX_COMPONENTTYPE       *pHandle = NULL;
    OMXVidDecComp           *pVidDecComp = NULL;
    IVIDDEC3_DynamicParams  *pDecDynParams;
    IH264VDEC_DynamicParams *params = (IH264VDEC_DynamicParams *) dynParams;

    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp = (OMXVidDecComp *)pHandle->pComponentPrivate;
    pDecDynParams = &(params->viddec3DynamicParams);
    pDecDynParams->size = sizeof(IVIDDEC3_DynamicParams);

    /* init dynamic params IVIDDEC3_DynamicParams */
    pDecDynParams->decodeHeader  = XDM_DECODE_AU; /* Supported */

    pDecDynParams->displayWidth  = 0; /* Not Supported: Set default */
    /*Not Supported: Set default*/
    pDecDynParams->frameSkipMode = H264VD_DEFAULT_FRAME_SKIP;
    pDecDynParams->newFrameFlag = XDAS_TRUE; //Not Supported: Set default

    if( ((IVIDDEC3_Params *)(pVidDecComp->pDecStaticParams))->inputDataMode == IVIDEO_ENTIREFRAME )
    {
        pDecDynParams->putBufferFxn = NULL;
        pDecDynParams->putBufferHandle = NULL;
        pDecDynParams->putDataFxn    = NULL;
        pDecDynParams->putDataHandle = NULL;
        pDecDynParams->getDataHandle = NULL;
        pDecDynParams->getDataFxn    = NULL;
    }

    return;
}

/**/
void OMXH264VD_Set_Status(OMX_HANDLETYPE hComponent, void *decstatus)
{
    IH264VDEC_Status   *status;
    status = (IH264VDEC_Status *)decstatus;
    status->viddec3Status.size = sizeof(IH264VDEC_Status);
    return;
}

PaddedBuffParams CalculateH264VD_outbuff_details(OMX_HANDLETYPE hComponent, OMX_U32 width, OMX_U32 height)
{
    OMX_COMPONENTTYPE   *pHandle = NULL;
    OMXVidDecComp       *pVidDecComp  = NULL;
    OMXH264VidDecComp   *pH264VidDecComp = NULL;
    OMX_U32             nRefBufferCount=16;
    IH264VDEC_Status    *pDecStatus = NULL;
    IH264VDEC_Params    *staticparams;

    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp  = (OMXVidDecComp *)pHandle->pComponentPrivate;
    pH264VidDecComp =(OMXH264VidDecComp *) pVidDecComp->pCodecSpecific;

    pDecStatus = (IH264VDEC_Status *)(pVidDecComp->pDecStatus);

    PaddedBuffParams    OutBuffDetails;
    OutBuffDetails.nBufferSize
                    = ((((width + (2 * PADX) + 127) & 0xFFFFFF80) * (height + 4 * PADY)));
    /* Multiply buffer size by 1.5 to account for both luma and chroma */
    OutBuffDetails.nBufferSize = (OutBuffDetails.nBufferSize * 3) >> 1;
    OutBuffDetails.nBufferCountMin = OMXH264VD_Calculate_TotalRefFrames(width, height, pH264VidDecComp->tH264VideoParam.eLevel);
    staticparams = (IH264VDEC_Params *)(pVidDecComp->pDecStaticParams);

    /* Assume 0 ref frames initially only if IL client is using port reconfig for allocating padded buffers.
    * In that case use the correct ref frames at the time of port reconfig to calculate nBufferCountMin/Actual.
    */
    if( pDecStatus->spsMaxRefFrames != 0 || pVidDecComp->bUsePortReconfigForPadding == OMX_TRUE ) {
        staticparams->viddec3Params.displayDelay = pDecStatus->spsMaxRefFrames;
        staticparams->dpbSizeInFrames = pDecStatus->spsMaxRefFrames;
        pH264VidDecComp->tH264VideoParam.nRefFrames = pDecStatus->spsMaxRefFrames;
        if( pH264VidDecComp->tH264VideoParam.eProfile == OMX_VIDEO_AVCProfileBaseline ) {
            /*Base profile*/
            OutBuffDetails.nBufferCountMin = pDecStatus->spsMaxRefFrames + 1;
        } else {
            /* High Profile */
            OutBuffDetails.nBufferCountMin = 2 * pDecStatus->spsMaxRefFrames + 1;
        }
    } else if( pH264VidDecComp->tH264VideoParam.nRefFrames == 0xFFFFFFFF ) {
        if( pH264VidDecComp->tH264VideoParam.eProfile == OMX_VIDEO_AVCProfileBaseline ) {
            OutBuffDetails.nBufferCountMin = OMXH264VD_Calculate_TotalRefFrames
                                            (pVidDecComp->sBase.pPorts[OMX_VIDDEC_INPUT_PORT]->sPortDef.format.video.nFrameWidth,
                                            pVidDecComp->sBase.pPorts[OMX_VIDDEC_INPUT_PORT]->sPortDef.format.video.nFrameHeight,
                                            pH264VidDecComp->tH264VideoParam.eLevel);
        } else {
            nRefBufferCount = OMXH264VD_Calculate_TotalRefFrames
                                (pVidDecComp->sBase.pPorts[OMX_VIDDEC_INPUT_PORT]->sPortDef.format.video.nFrameWidth,
                                pVidDecComp->sBase.pPorts[OMX_VIDDEC_INPUT_PORT]->sPortDef.format.video.nFrameHeight,
                                pH264VidDecComp->tH264VideoParam.eLevel) - 1;
            if((2 * nRefBufferCount + 1) < OMX_MAX_DEC_OP_BUFFERS ) {
                OutBuffDetails.nBufferCountMin = 2 * nRefBufferCount + 1;
            } else {
                OutBuffDetails.nBufferCountMin = OMX_MAX_DEC_OP_BUFFERS; //Cap max buffers to 20
            }
        }
    } else if( pH264VidDecComp->tH264VideoParam.nRefFrames <= 16 ) {
        if( pH264VidDecComp->tH264VideoParam.eProfile == OMX_VIDEO_AVCProfileBaseline ) {
            OutBuffDetails.nBufferCountMin = pH264VidDecComp->tH264VideoParam.nRefFrames + 1;
        } else {
            OutBuffDetails.nBufferCountMin = 2 * pH264VidDecComp->tH264VideoParam.nRefFrames + 1;
        }
    }
    OutBuffDetails.nBufferCountActual = OutBuffDetails.nBufferCountMin + 2;
    OutBuffDetails.n1DBufferAlignment = 16;
    OutBuffDetails.nPaddedWidth = (width + (2 * PADX) + 127) & 0xFFFFFF80;
    OutBuffDetails.nPaddedHeight = height + 4 * PADY;
    OutBuffDetails.n2DBufferYAlignment = 1;
    OutBuffDetails.n2DBufferXAlignment = 16;

    return (OutBuffDetails);
}


void OMXH264VD_DeInit(OMX_HANDLETYPE hComponent)
{
    OMX_COMPONENTTYPE   *pHandle = NULL;
    OMXVidDecComp       *pVidDecComp  = NULL;

    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp  = (OMXVidDecComp *)pHandle->pComponentPrivate;
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
}


OMX_U32 OMXH264VD_Calculate_TotalRefFrames(OMX_U32 nWidth, OMX_U32 nHeight, OMX_VIDEO_AVCLEVELTYPE eLevel)
{
    OMX_U32    ref_frames = 0;
    OMX_U32    MaxDpbMbs;
    OMX_U32    PicWidthInMbs;
    OMX_U32    FrameHeightInMbs;

    switch( eLevel ) {
        case OMX_VIDEO_AVCLevel1 :
        case OMX_VIDEO_AVCLevel1b :
        {
            MaxDpbMbs = 396;
        break;
        }

        case OMX_VIDEO_AVCLevel11 :
        {
            MaxDpbMbs = 900;
        break;
        }

        case OMX_VIDEO_AVCLevel12 :
        case OMX_VIDEO_AVCLevel13 :
        case OMX_VIDEO_AVCLevel2 :
        {
            MaxDpbMbs = 2376;
        break;
        }

        case OMX_VIDEO_AVCLevel21 :
        {
            MaxDpbMbs = 4752;
        break;
        }

        case OMX_VIDEO_AVCLevel22 :
        case OMX_VIDEO_AVCLevel3 :
        {
            MaxDpbMbs = 8100;
        break;
        }

        case OMX_VIDEO_AVCLevel31 :
        {
            MaxDpbMbs = 18000;
        break;
        }

        case OMX_VIDEO_AVCLevel32 :
        {
            MaxDpbMbs = 20480;
        break;
        }

        case OMX_VIDEO_AVCLevel5 :
        {
            MaxDpbMbs = 110400; //Maximum value for upto level 5
        break;
        }

        case OMX_VIDEO_AVCLevel51 :
        {
            MaxDpbMbs = 184320; //Maximum value for upto level 5.1
        break;
        }

        default :
        {
            MaxDpbMbs = 32768; //Maximum value for upto level 4.1
        }
    }

    PicWidthInMbs = nWidth / 16;
    FrameHeightInMbs = nHeight / 16;
    ref_frames =  (OMX_U32)(MaxDpbMbs / (PicWidthInMbs * FrameHeightInMbs));

    ref_frames = (ref_frames > 16) ? 16 : ref_frames;

    /* Three is added to total reference frames because of the N+3 buffer issue
    * It was found that theoretically 2N+1 buffers are required but from a practical
    * point of view N+3 was sufficient */
    return (ref_frames + 1);
}


OMX_ERRORTYPE OMXH264VD_SetParameter(OMX_HANDLETYPE hComponent,
                                         OMX_INDEXTYPE nIndex, OMX_PTR pParamStruct)
{
    OMX_ERRORTYPE           eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE       *pHandle = NULL;
    OMXVidDecComp           *pVidDecComp  = NULL;
    OMXH264VidDecComp       *pH264VidDecComp = NULL;
    OMX_VIDEO_PARAM_AVCTYPE *pH264VideoParam = NULL;
    OMX_U32                 nRefBufferCount = 0;
    OMX_U8                  i;
    IH264VDEC_Params        *staticparams;

    OMX_CHECK((hComponent != NULL) && (pParamStruct != NULL),
                OMX_ErrorBadParameter);

    /*! Initialize the pointers */
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp = (OMXVidDecComp *)pHandle->pComponentPrivate;
    pH264VidDecComp = (OMXH264VidDecComp *) pVidDecComp->pCodecSpecific;
    staticparams = (IH264VDEC_Params *)(pVidDecComp->pDecStaticParams);

    switch( nIndex ) {
        case OMX_IndexParamVideoAvc :
        {
            pH264VideoParam = (OMX_VIDEO_PARAM_AVCTYPE *) pParamStruct;
            /* SetParameter can be invoked in Loaded State  or on Disabled ports only*/
            OMX_CHECK((pVidDecComp->sBase.tCurState == OMX_StateLoaded) ||
                    (pVidDecComp->sBase.pPorts[pH264VideoParam->nPortIndex]->sPortDef.bEnabled == OMX_FALSE),
                    OMX_ErrorIncorrectStateOperation);

            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_AVCTYPE, eError);
            OMX_CHECK(pH264VideoParam->eProfile == OMX_VIDEO_AVCProfileBaseline
                    || pH264VideoParam->eProfile == OMX_VIDEO_AVCProfileMain
                    || pH264VideoParam->eProfile == OMX_VIDEO_AVCProfileExtended
                    || pH264VideoParam->eProfile == OMX_VIDEO_AVCProfileHigh,
                    OMX_ErrorUnsupportedSetting);
            OMX_CHECK(pH264VideoParam->eLevel <= OMX_VIDEO_AVCLevel51,
                    OMX_ErrorUnsupportedSetting);
            if( pH264VideoParam->nPortIndex == OMX_VIDDEC_INPUT_PORT ) {
                pH264VidDecComp->tH264VideoParam = *pH264VideoParam;
            } else if( pH264VideoParam->nPortIndex == OMX_VIDDEC_OUTPUT_PORT ) {
                OSAL_ErrorTrace("OMX_IndexParamVideoAvc supported only on i/p port");
                eError = OMX_ErrorUnsupportedIndex;
                goto EXIT;
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            OSAL_ErrorTrace("Profile set = %x, Level Set = %x, num ref frames set = %d",
                    pH264VideoParam->eProfile, pH264VideoParam->eLevel, pH264VideoParam->nRefFrames);
            if( pH264VideoParam->eLevel == OMX_VIDEO_AVCLevel5 ) {
                staticparams->presetLevelIdc =  IH264VDEC_LEVEL5;
            } else if( pH264VideoParam->eLevel == OMX_VIDEO_AVCLevel51 ) {
                staticparams->presetLevelIdc =  IH264VDEC_LEVEL51;
            }
            if( pH264VidDecComp->tH264VideoParam.nRefFrames == 0xFFFFFFFF ) {
                if( pH264VideoParam->eProfile == OMX_VIDEO_AVCProfileBaseline ) {
                    staticparams->viddec3Params.displayDelay = IVIDDEC3_DECODE_ORDER;

                    pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef.nBufferCountMin =
                            OMXH264VD_Calculate_TotalRefFrames(
                                        pVidDecComp->sBase.pPorts[OMX_VIDDEC_INPUT_PORT]->sPortDef.format.video.nFrameWidth,
                                        pVidDecComp->sBase.pPorts[OMX_VIDDEC_INPUT_PORT]->sPortDef.format.video.nFrameHeight,
                                        pH264VideoParam->eLevel);
                } else {
                    nRefBufferCount = OMXH264VD_Calculate_TotalRefFrames
                                    (pVidDecComp->sBase.pPorts[OMX_VIDDEC_INPUT_PORT]->sPortDef.format.video.nFrameWidth,
                                    pVidDecComp->sBase.pPorts[OMX_VIDDEC_INPUT_PORT]->sPortDef.format.video.nFrameHeight,
                                    pH264VideoParam->eLevel) - 1;
                    staticparams->viddec3Params.displayDelay = nRefBufferCount;
                    if((2 * nRefBufferCount + 1) < OMX_MAX_DEC_OP_BUFFERS ) {
                        pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef.nBufferCountMin
                                                                    = 2 * nRefBufferCount + 1;
                    } else {
                        pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef.nBufferCountMin
                                                                    = OMX_MAX_DEC_OP_BUFFERS;     //Cap max buffers to 20
                    }
                }
            } else if( pH264VidDecComp->tH264VideoParam.nRefFrames <= 16 ) {
                staticparams->dpbSizeInFrames = pH264VidDecComp->tH264VideoParam.nRefFrames;
                if( pH264VideoParam->eProfile == OMX_VIDEO_AVCProfileBaseline ) {
                    staticparams->viddec3Params.displayDelay
                                                    = IVIDDEC3_DECODE_ORDER;
                    pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef.nBufferCountMin
                                                    = pH264VidDecComp->tH264VideoParam.nRefFrames + 1;
                } else {
                    staticparams->viddec3Params.displayDelay
                                                    = pH264VidDecComp->tH264VideoParam.nRefFrames;
                    pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef.nBufferCountMin =
                                                    2 * pH264VidDecComp->tH264VideoParam.nRefFrames + 1;
                }
            } else {
                OSAL_ErrorTrace("Invalid value of nRefFrames = %d of the structure OMX_VIDEO_PARAM_AVCTYPE provided ",
                                pH264VidDecComp->tH264VideoParam.nRefFrames);
            }
        }
        break;

        case OMX_IndexParamVideoProfileLevelQuerySupported :
        {
            /* SetParameter can be invoked in Loaded State  or on Disabled ports only*/
            OMX_CHECK(pVidDecComp->sBase.tCurState == OMX_StateLoaded,
                            OMX_ErrorIncorrectStateOperation);
            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_PROFILELEVELTYPE, eError);
            /*As of now do nothing. This index is required for StdVideoDecoderTest. Later on do code review and fill in proper code here.*/
            eError = OMX_ErrorNoMore;
        }
        break;

        case OMX_IndexParamVideoProfileLevelCurrent :
        {
            /* SetParameter can be invoked in Loaded State  or on Disabled ports only*/
            OMX_CHECK(pVidDecComp->sBase.tCurState == OMX_StateLoaded,
                        OMX_ErrorIncorrectStateOperation);
            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_PROFILELEVELTYPE, eError);
            /*As of now do nothing. This index is required for StdVideoDecoderTest. Later on do code review and fill in proper code here.*/
            eError = OMX_ErrorNoMore;
        }
        break;

        default :
            eError = OMXVidDec_SetParameter(hComponent, nIndex, pParamStruct);
    }

EXIT:
    return (eError);
}


OMX_ERRORTYPE OMXH264VD_GetParameter(OMX_HANDLETYPE hComponent,
                                         OMX_INDEXTYPE nIndex, OMX_PTR pParamStruct)
{
    OMX_ERRORTYPE                   eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE               *pHandle = NULL;
    OMXVidDecComp                   *pVidDecComp = NULL;
    OMXH264VidDecComp               *pH264VidDecComp = NULL;
    OMX_VIDEO_PARAM_AVCTYPE         *pH264VideoParam = NULL;
    OMX_VIDEO_PARAM_PROFILELEVELTYPE    *pH264ProfileLevelParam = NULL;


    OMX_CHECK((hComponent != NULL) &&
                (pParamStruct != NULL), OMX_ErrorBadParameter);

    // Initialize the local variables
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp = (OMXVidDecComp *)pHandle->pComponentPrivate;

    pH264VidDecComp = (OMXH264VidDecComp *) pVidDecComp->pCodecSpecific;

    /* GetParameter can't be invoked incase the comp is in Invalid State  */
    OMX_CHECK(pVidDecComp->sBase.tCurState != OMX_StateInvalid,
                OMX_ErrorIncorrectStateOperation);

    switch( nIndex ) {
        case OMX_IndexParamVideoAvc :
        {
            pH264VideoParam = (OMX_VIDEO_PARAM_AVCTYPE *) pParamStruct;
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_AVCTYPE, eError);
            if( pH264VideoParam->nPortIndex == OMX_VIDDEC_INPUT_PORT ) {
                *pH264VideoParam = pH264VidDecComp->tH264VideoParam;
            } else if( pH264VideoParam->nPortIndex == OMX_VIDDEC_OUTPUT_PORT ) {
                OSAL_ErrorTrace("OMX_IndexParamVideoAvc supported only on i/p port");
                eError = OMX_ErrorUnsupportedIndex;
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
        }
        break;

        case OMX_IndexParamVideoProfileLevelQuerySupported :
        {
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_PROFILELEVELTYPE, eError);
            pH264ProfileLevelParam = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pParamStruct;
            if( pH264ProfileLevelParam->nPortIndex == OMX_VIDDEC_INPUT_PORT ) {
                if( pH264ProfileLevelParam->nProfileIndex == 0 ) {
                    pH264ProfileLevelParam->eProfile = (OMX_U32) OMX_VIDEO_AVCProfileBaseline;
                    pH264ProfileLevelParam->eLevel = (OMX_U32) OMX_VIDEO_AVCLevel41;
                } else if( pH264ProfileLevelParam->nProfileIndex == 1 ) {
                    pH264ProfileLevelParam->eProfile = (OMX_U32) OMX_VIDEO_AVCProfileMain;
                    pH264ProfileLevelParam->eLevel = (OMX_U32) OMX_VIDEO_AVCLevel41;
                } else if( pH264ProfileLevelParam->nProfileIndex == 2 ) {
                    pH264ProfileLevelParam->eProfile = (OMX_U32) OMX_VIDEO_AVCProfileHigh;
                    pH264ProfileLevelParam->eLevel = (OMX_U32) OMX_VIDEO_AVCLevel41;
                } else {
                    eError = OMX_ErrorNoMore;
                }
            } else if( pH264ProfileLevelParam->nPortIndex == OMX_VIDDEC_OUTPUT_PORT ) {
                OSAL_ErrorTrace("OMX_IndexParamVideoProfileLevelQuerySupported supported only on i/p port");
                eError = OMX_ErrorUnsupportedIndex;
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
        }
        break;

        case OMX_IndexParamVideoProfileLevelCurrent :
        {
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_PROFILELEVELTYPE, eError);
            eError = OMX_ErrorNoMore;
        }
        break;

        case OMX_IndexParamVideoMacroblocksPerFrame :
        {
            OMX_U32    MBwidth = 0, MBheight = 0;
            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_PARAM_MACROBLOCKSTYPE, eError);
            MBwidth = (pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef.format.video.nFrameWidth) / 16;
            MBwidth = MBwidth + ((pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef.format.video.nFrameWidth) % 16);
            MBheight = pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef.format.video.nFrameHeight / 16;
            MBheight = MBheight + ((pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef.format.video.nFrameHeight) % 16);
            ((OMX_PARAM_MACROBLOCKSTYPE *)(pParamStruct))->nMacroblocks = MBwidth * MBheight;
        }
        break;

        default :
        eError = OMXVidDec_GetParameter(hComponent, nIndex, pParamStruct);
    }

EXIT:
    return (eError);
}

/* */
OMX_ERRORTYPE OMXH264VD_HandleError(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OMX_U32             nRefFramesOld, nRefFrames41, nRefFrames5;
    OMXH264VidDecComp   *pH264VidDecComp = NULL;
    IH264VDEC_Params    *staticparams = NULL;
    IH264VDEC_Status    *pDecStatus = NULL;
    OMX_COMPONENTTYPE   *pHandle = NULL;
    OMXVidDecComp       *pVidDecComp = NULL;
    OMX_U32             nBufferCountMin_old = 0;

    /* Initialize pointers */
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pVidDecComp = (OMXVidDecComp *)pHandle->pComponentPrivate;
    staticparams = (IH264VDEC_Params *)(pVidDecComp->pDecStaticParams);
    pH264VidDecComp =(OMXH264VidDecComp *) pVidDecComp->pCodecSpecific;

    pDecStatus = (IH264VDEC_Status *)(pVidDecComp->pDecStatus);
    if( pH264VidDecComp->tH264VideoParam.nRefFrames == 0xFFFFFFFF ) {
        nRefFramesOld = OMXH264VD_Calculate_TotalRefFrames
                        (pVidDecComp->sBase.pPorts[OMX_VIDDEC_INPUT_PORT]->sPortDef.format.video.nFrameWidth,
                        pVidDecComp->sBase.pPorts[OMX_VIDDEC_INPUT_PORT]->sPortDef.format.video.nFrameHeight,
                        pH264VidDecComp->tH264VideoParam.eLevel) - 1;
    } else {
        nRefFramesOld = pH264VidDecComp->tH264VideoParam.nRefFrames;
    }
    nRefFrames41 = OMXH264VD_Calculate_TotalRefFrames
                    (pVidDecComp->sBase.pPorts[OMX_VIDDEC_INPUT_PORT]->sPortDef.format.video.nFrameWidth,
                    pVidDecComp->sBase.pPorts[OMX_VIDDEC_INPUT_PORT]->sPortDef.format.video.nFrameHeight,
                    OMX_VIDEO_AVCLevel41) - 1;
    nRefFrames5 = OMXH264VD_Calculate_TotalRefFrames
                    (pVidDecComp->sBase.pPorts[OMX_VIDDEC_INPUT_PORT]->sPortDef.format.video.nFrameWidth,
                    pVidDecComp->sBase.pPorts[OMX_VIDDEC_INPUT_PORT]->sPortDef.format.video.nFrameHeight,
                    OMX_VIDEO_AVCLevel5) - 1;
    nBufferCountMin_old = pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef.nBufferCountMin;
    if( pDecStatus->spsMaxRefFrames > nRefFramesOld ) {
        OSAL_ErrorTrace("spsMaxRefFrames = %d, nRefFrames set initially = %d",
        pDecStatus->spsMaxRefFrames, pH264VidDecComp->tH264VideoParam.nRefFrames);
        pH264VidDecComp->tH264VideoParam.nRefFrames
                                = pDecStatus->spsMaxRefFrames;
        staticparams->dpbSizeInFrames
                                = pDecStatus->spsMaxRefFrames;
        staticparams->viddec3Params.displayDelay
                                = pDecStatus->spsMaxRefFrames;
        if( pH264VidDecComp->tH264VideoParam.eProfile == OMX_VIDEO_AVCProfileBaseline ) {
            staticparams->viddec3Params.displayDelay = IVIDDEC3_DECODE_ORDER;
            pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef.nBufferCountMin
                                        = pH264VidDecComp->tH264VideoParam.nRefFrames + 1;
            pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef.nBufferCountActual
                        = pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef.nBufferCountMin + 2;
        } else {
            staticparams->viddec3Params.displayDelay = pDecStatus->spsMaxRefFrames;
            pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef.nBufferCountMin
                                    = 2 * pH264VidDecComp->tH264VideoParam.nRefFrames + 1;
            pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef.nBufferCountActual
                        = pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef.nBufferCountMin + 2;
        }
        if( pDecStatus->spsMaxRefFrames > nRefFrames5 ) {
            pH264VidDecComp->tH264VideoParam.eLevel = OMX_VIDEO_AVCLevel51;
            staticparams->presetLevelIdc =  IH264VDEC_LEVEL51;
            OSAL_ErrorTrace("Resetting level of the stream to Level 5.1");
            pVidDecComp->nCodecRecreationRequired = 1;
            pVidDecComp->nOutPortReconfigRequired = 1;
        } else if( pDecStatus->spsMaxRefFrames > nRefFrames41 ) {
            pH264VidDecComp->tH264VideoParam.eLevel = OMX_VIDEO_AVCLevel5;
            staticparams->presetLevelIdc =  IH264VDEC_LEVEL5;
            OSAL_ErrorTrace("Resetting level of the stream to Level 5");
            pVidDecComp->nCodecRecreationRequired = 1;
            pVidDecComp->nOutPortReconfigRequired = 1;
        } else if( pDecStatus->spsMaxRefFrames > nRefFramesOld ) {
            pH264VidDecComp->tH264VideoParam.eLevel = OMX_VIDEO_AVCLevel41;
            OSAL_ErrorTrace("Resetting level of the stream to Level 4.1");
        }
        if( pVidDecComp->sBase.pPorts[OMX_VIDDEC_OUTPUT_PORT]->sPortDef.nBufferCountMin
                > nBufferCountMin_old ) {
            OSAL_ErrorTrace("nBufferCountMin_old = %d, nBufferCountMin_new = %d",
                        nBufferCountMin_old, pVidDecComp->pPortdefs[OMX_VIDEODECODER_OUTPUT_PORT]->nBufferCountMin);
            pVidDecComp->nOutPortReconfigRequired = 1;
            pVidDecComp->nCodecRecreationRequired = 1;
        }
    }

    return (eError);
}

