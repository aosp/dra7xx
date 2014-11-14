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

#include "omx_H264videoencoder.h"

#define LOG_TAG "OMX_H264_ENCODERUTILS"

OMX_H264_LVL_BITRATE    OMX_H264_BP_MP_BITRATE_SUPPORT[] =
{
    { OMX_VIDEO_AVCLevel1,      64000 },
    { OMX_VIDEO_AVCLevel1b,    128000 },
    { OMX_VIDEO_AVCLevel11,    192000 },
    { OMX_VIDEO_AVCLevel12,    384000 },
    { OMX_VIDEO_AVCLevel13,    768000 },
    { OMX_VIDEO_AVCLevel2,    2000000 },
    { OMX_VIDEO_AVCLevel21,   4000000 },
    { OMX_VIDEO_AVCLevel22,   4000000 },
    { OMX_VIDEO_AVCLevel3,   10000000 },
    { OMX_VIDEO_AVCLevel31,  14000000 },
    { OMX_VIDEO_AVCLevel32,  20000000 },
    { OMX_VIDEO_AVCLevel4,   20000000 },
    { OMX_VIDEO_AVCLevel41,  50000000 },
    { OMX_VIDEO_AVCLevel42,  50000000 },
    { OMX_VIDEO_AVCLevel5,   50000000 }, //according to the spec the bit rate supported is 135000000, here the 50mpbs limit is as per the current codec version
    { OMX_VIDEO_AVCLevel51,  50000000 }  //according to the spec the bit rate supported is 240000000, here the 50mpbs limit is as per the current codec version
};

OMX_H264_LVL_BITRATE    OMX_H264_HP_BITRATE_SUPPORT[] =
{
    { OMX_VIDEO_AVCLevel1,     80000 },
    { OMX_VIDEO_AVCLevel1b,   160000 },
    { OMX_VIDEO_AVCLevel11,   240000 },
    { OMX_VIDEO_AVCLevel12,   480000 },
    { OMX_VIDEO_AVCLevel13,   960000 },
    { OMX_VIDEO_AVCLevel2,   2500000 },
    { OMX_VIDEO_AVCLevel21,  5000000 },
    { OMX_VIDEO_AVCLevel22,  5000000 },
    { OMX_VIDEO_AVCLevel3,  12500000 },
    { OMX_VIDEO_AVCLevel31, 17500000 },
    { OMX_VIDEO_AVCLevel32, 25000000 },
    { OMX_VIDEO_AVCLevel4,  25000000 },
    { OMX_VIDEO_AVCLevel41, 62500000 },
    { OMX_VIDEO_AVCLevel42, 62500000 },
    { OMX_VIDEO_AVCLevel5,  62500000 }, //according to the spec the bit rate supported is 168750000, here the 62.5mpbs limit is as per the current codec version
    { OMX_VIDEO_AVCLevel51, 62500000 }  //according to the spec the bit rate supported is 300000000, here the 62.5mpbs limit is as per the current codec version
};

OMX_ERRORTYPE  OMXH264VE_InitFields(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE eError            = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle      = NULL;
    OMXH264VidEncComp *pH264VEComp  = NULL;

    OMX_CHECK(hComponent != NULL, OMX_ErrorBadParameter);
    pHandle = (OMX_COMPONENTTYPE *)hComponent;

    OMX_BASE_CHK_VERSION(pHandle, OMX_COMPONENTTYPE, eError);

    pH264VEComp = (OMXH264VidEncComp *)pHandle->pComponentPrivate;
    pH264VEComp->sBase.cComponentName = OMX_H264VE_COMP_NAME;

    /* Fill component's version, this may not be same as the OMX Specification version */
    pH264VEComp->sBase.nComponentVersion.s.nVersionMajor    = OMX_H264VE_COMP_VERSION_MAJOR;
    pH264VEComp->sBase.nComponentVersion.s.nVersionMinor    = OMX_H264VE_COMP_VERSION_MINOR;
    pH264VEComp->sBase.nComponentVersion.s.nRevision        = OMX_H264VE_COMP_VERSION_REVISION;
    pH264VEComp->sBase.nComponentVersion.s.nStep            = OMX_H264VE_COMP_VERSION_STEP;

    /* Initialize Audio Port parameters */
    OSAL_Info("Initialize Audio Port Params");
    pH264VEComp->sBase.pAudioPortParams = (OMX_PORT_PARAM_TYPE*)OSAL_Malloc(sizeof(OMX_PORT_PARAM_TYPE));
    OMX_CHECK(pH264VEComp->sBase.pAudioPortParams != NULL, OMX_ErrorInsufficientResources);

    OMX_BASE_INIT_STRUCT_PTR(pH264VEComp->sBase.pAudioPortParams, OMX_PORT_PARAM_TYPE);
    pH264VEComp->sBase.pAudioPortParams->nPorts           = 0;
    pH264VEComp->sBase.pAudioPortParams->nStartPortNumber = 0;

    /* Initialize Video Port parameters */
    OSAL_Info("Initialize Video Port Params");
    pH264VEComp->sBase.pVideoPortParams = (OMX_PORT_PARAM_TYPE*)OSAL_Malloc(sizeof(OMX_PORT_PARAM_TYPE));
    OMX_CHECK(pH264VEComp->sBase.pVideoPortParams != NULL, OMX_ErrorInsufficientResources);

    OMX_BASE_INIT_STRUCT_PTR(pH264VEComp->sBase.pVideoPortParams, OMX_PORT_PARAM_TYPE);
    pH264VEComp->sBase.pVideoPortParams->nPorts           = OMX_H264VE_NUM_PORTS;
    pH264VEComp->sBase.pVideoPortParams->nStartPortNumber = OMX_H264VE_DEFAULT_START_PORT_NUM;

    /* Initialize Image Port parameters */
    OSAL_Info("Initialize Image Port Params");
    pH264VEComp->sBase.pImagePortParams = (OMX_PORT_PARAM_TYPE*)OSAL_Malloc(sizeof(OMX_PORT_PARAM_TYPE));
    OMX_CHECK(pH264VEComp->sBase.pImagePortParams != NULL, OMX_ErrorInsufficientResources);

    OMX_BASE_INIT_STRUCT_PTR(pH264VEComp->sBase.pImagePortParams, OMX_PORT_PARAM_TYPE);
    pH264VEComp->sBase.pImagePortParams->nPorts           = 0;
    pH264VEComp->sBase.pImagePortParams->nStartPortNumber = 0;

    /* Initialize Other Port parameters */
    OSAL_Info("Initialize Other Port Params");
    pH264VEComp->sBase.pOtherPortParams = (OMX_PORT_PARAM_TYPE*)OSAL_Malloc(sizeof(OMX_PORT_PARAM_TYPE));
    OMX_CHECK(pH264VEComp->sBase.pOtherPortParams != NULL, OMX_ErrorInsufficientResources);

    OMX_BASE_INIT_STRUCT_PTR(pH264VEComp->sBase.pOtherPortParams, OMX_PORT_PARAM_TYPE);
    pH264VEComp->sBase.pOtherPortParams->nPorts           = 0;
    pH264VEComp->sBase.pOtherPortParams->nStartPortNumber = 0;

    /* Initialize the Total Number of Ports and Start Port Number*/
    OSAL_Info("Initialize Component Port Params");
    pH264VEComp->sBase.nNumPorts            = OMX_H264VE_NUM_PORTS;
    pH264VEComp->sBase.nMinStartPortIndex   = OMX_H264VE_DEFAULT_START_PORT_NUM;

    /* Overriding this value. Notify derived component only when data is available on all ports */
    pH264VEComp->sBase.bNotifyForAnyPort = OMX_FALSE;

    /* Allocate Memory for Static Parameter */
    pH264VEComp->pVidEncStaticParams = (IH264ENC_Params *) memplugin_alloc(sizeof(IH264ENC_Params), 1, MEM_CARVEOUT, 0, 0);
    OMX_CHECK(pH264VEComp->pVidEncStaticParams != NULL, OMX_ErrorInsufficientResources);
    OSAL_Memset(pH264VEComp->pVidEncStaticParams, 0x0, sizeof(IH264ENC_Params));

    /* Allocate Memory for Dynamic Parameter */
    pH264VEComp->pVidEncDynamicParams = (IH264ENC_DynamicParams *) memplugin_alloc(sizeof(IH264ENC_DynamicParams), 1, MEM_CARVEOUT, 0, 0);
    OMX_CHECK(pH264VEComp->pVidEncDynamicParams != NULL, OMX_ErrorInsufficientResources);
    OSAL_Memset(pH264VEComp->pVidEncDynamicParams, 0x0, sizeof(IH264ENC_DynamicParams));

    /* Allocate Memory for status Parameter */
    pH264VEComp->pVidEncStatus = (IH264ENC_Status *) memplugin_alloc(sizeof(IH264ENC_Status), 1, MEM_CARVEOUT, 0, 0);
    OMX_CHECK(pH264VEComp->pVidEncStatus != NULL, OMX_ErrorInsufficientResources);
    OSAL_Memset(pH264VEComp->pVidEncStatus, 0x0, sizeof(IH264ENC_Status));

    /* Allocate Memory for InArgs Parameter */
    pH264VEComp->pVidEncInArgs = (IH264ENC_InArgs *) memplugin_alloc(sizeof(IH264ENC_InArgs), 1, MEM_CARVEOUT, 0, 0);
    OMX_CHECK(pH264VEComp->pVidEncInArgs != NULL, OMX_ErrorInsufficientResources);
    OSAL_Memset(pH264VEComp->pVidEncInArgs, 0x0, sizeof(IH264ENC_InArgs));

    /* Allocate Memory for OutArgs Parameter */
    pH264VEComp->pVidEncOutArgs = (IH264ENC_OutArgs *) memplugin_alloc(sizeof(IH264ENC_OutArgs), 1, MEM_CARVEOUT, 0, 0);
    OMX_CHECK(pH264VEComp->pVidEncOutArgs != NULL, OMX_ErrorInsufficientResources);
    OSAL_Memset(pH264VEComp->pVidEncOutArgs, 0x0, sizeof(IH264ENC_OutArgs));

    /* Allocate Memory for InDesc Parameter */
    pH264VEComp->pVedEncInBufs = (IVIDEO2_BufDesc *) memplugin_alloc(sizeof(IVIDEO2_BufDesc), 1, MEM_CARVEOUT, 0, 0);
    OMX_CHECK(pH264VEComp->pVedEncInBufs != NULL, OMX_ErrorInsufficientResources);
    OSAL_Memset(pH264VEComp->pVedEncInBufs, 0x0, sizeof(IVIDEO2_BufDesc));

    /* Allocate Memory for OutDesc Parameter */
    pH264VEComp->pVedEncOutBufs = (XDM2_BufDesc *) memplugin_alloc(sizeof(XDM2_BufDesc), 1, MEM_CARVEOUT, 0, 0);
    OMX_CHECK(pH264VEComp->pVedEncOutBufs != NULL, OMX_ErrorInsufficientResources);
    OSAL_Memset(pH264VEComp->pVedEncOutBufs, 0x0, sizeof(XDM2_BufDesc));

EXIT:
    if( eError != OMX_ErrorNone ) {
        OSAL_ErrorTrace("in fn OMXH264VE_SesBaseParameters");
    }

    return (eError);

    }


OMX_ERRORTYPE  OMXH264VE_InitialzeComponentPrivateParams(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE   eError          = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle      = NULL;
    OMXH264VidEncComp *pH264VEComp  = NULL;
    OMX_U32             i           = 0;
    OSAL_ERROR  tStatus           =  OSAL_ErrNone;

    OMX_CHECK(hComponent != NULL, OMX_ErrorBadParameter);
    pHandle = (OMX_COMPONENTTYPE *)hComponent;

    OMX_BASE_CHK_VERSION(pHandle, OMX_COMPONENTTYPE, eError);

    pH264VEComp = (OMXH264VidEncComp *)pHandle->pComponentPrivate;

    OSAL_Info("Update the default Port Params");

    /* Set the Port Definition (OMX_PARAM_PORTDEFINITIONTYPE)  Values : INPUT PORT */
    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.nPortIndex         = OMX_H264VE_INPUT_PORT;
    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.eDir               = OMX_DirInput;
    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.nBufferCountActual = OMX_H264VE_DEFAULT_INPUT_BUFFER_COUNT;
    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.nBufferCountMin    = OMX_H264VE_MIN_INPUT_BUFFER_COUNT;
    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.nBufferSize        = OMX_H264VE_DEFAULT_INPUT_BUFFER_SIZE;
    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.bEnabled           = OMX_TRUE;
    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.bPopulated         = OMX_FALSE;
    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.eDomain            = OMX_PortDomainVideo;
    /*Update the Domain (Video) Specific values*/
    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.cMIMEType = NULL;
    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.pNativeRender = NULL;
    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameWidth = OMX_H264VE_DEFAULT_FRAME_WIDTH; /*should be multiples of 16*/
    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameHeight = OMX_H264VE_DEFAULT_FRAME_HEIGHT;
    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nStride = OMX_H264VE_DEFAULT_FRAME_WIDTH;     /*setting the stride as atleaset equal to width (should be multiples of 16)*/
    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nSliceHeight = OMX_H264VE_DEFAULT_FRAME_HEIGHT; /*setting the sliceheight as equal to frame height*/
    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nBitrate = OMX_H264VE_DEFAULT_BITRATE;
    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.xFramerate = (OMX_H264VE_DEFAULT_FRAME_RATE << 16);
    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.bFlagErrorConcealment = OMX_FALSE;
    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.eColorFormat =OMX_TI_COLOR_FormatYUV420PackedSemiPlanar;
    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.pNativeWindow = NULL;
    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.bBuffersContiguous=OMX_FALSE;
    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.nBufferAlignment=32; /*H264 Encoder Codec has alignment restriction for input buffers */

    /* Set the  Port Definition (OMX_PARAM_PORTDEFINITIONTYPE)Values : OUTPUT PORT */
    pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.nPortIndex         = OMX_H264VE_OUTPUT_PORT;
    pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.eDir               = OMX_DirOutput;
    pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.nBufferCountActual = OMX_H264VE_DEFAULT_OUTPUT_BUFFER_COUNT;
    pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.nBufferCountMin    = OMX_H264VE_MIN_OUTPUT_BUFFER_COUNT;
    pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.nBufferSize        = OMX_H264VE_DEFAULT_OUTPUT_BUFFER_SIZE;
    pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.bEnabled           = OMX_TRUE;
    pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.bPopulated        = OMX_FALSE;
    pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.eDomain            = OMX_PortDomainVideo;
    /*Update the Domain (Video) Specific values*/
    pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.format.video.cMIMEType = NULL;
    pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.format.video.pNativeRender = NULL;
    pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.format.video.nFrameWidth = OMX_H264VE_DEFAULT_FRAME_WIDTH;
    pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.format.video.nFrameHeight = OMX_H264VE_DEFAULT_FRAME_HEIGHT;
    pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.format.video.nStride = 0; //Stride is not used on port having bitstream buffers
    pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.format.video.nSliceHeight = OMX_H264VE_DEFAULT_FRAME_HEIGHT; /*setting the sliceheight as equal frame height*/
    pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.format.video.nBitrate = OMX_H264VE_DEFAULT_BITRATE;
    pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.format.video.xFramerate = 0;
    pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.format.video.bFlagErrorConcealment = OMX_FALSE;
    pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.format.video.pNativeWindow = NULL;
    pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.bBuffersContiguous=OMX_FALSE;
    pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.nBufferAlignment=0; /*No Alignment required for output buffers*/

    OSAL_Info("SetH264AlgDefaultCreationParams ");
    /* Set the Default IVIDENC2_Params:    videnc2Params . Does not Adhere to Codec Defaults. Can be modified */
    SET_H264CODEC_DEFAULT_IVIDENC2_PARAMS(pH264VEComp, i);
    /* Adheres to Codec Defaults. To be modified only when codec default params change */
    eError = OMXH264VE_SetAlgDefaultCreationParams(hComponent);
    OMX_CHECK(eError == OMX_ErrorNone, OMX_ErrorUnsupportedSetting);

    /*Overwrite some of the codec static defaults*/
    OVERWRITE_H264CODEC_DEFAULT_STATIC_PARAMS(pH264VEComp);
    pH264VEComp->pVidEncStaticParams->IDRFrameInterval     = 1;     /*All I frames are IDR frames*/

    OSAL_Info("SetH264AlgDefaultDynamicParams ");
    /* Set the  IVIDENC2_DynamicParams       videnc2DynamicParams */
    SET_H264CODEC_DEFAULT_IVIDENC2_DYNAMICPARAMS(pH264VEComp);
    eError = OMXH264VE_SetAlgDefaultDynamicParams(hComponent);
    OMX_CHECK(eError == OMX_ErrorNone, OMX_ErrorUnsupportedSetting);

    /*Overwrite some of the codec static defaults*/
    OVERWRITE_H264CODEC_DEFAULT_DYNAMIC_PARAMS(pH264VEComp);
    /*Enable 4 MV*/
    ENABLE_4MV(pH264VEComp);
    pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.generateHeader = XDM_GENERATE_HEADER;

    pH264VEComp->bCodecCreate = OMX_FALSE; /*codec creation hasn't happened yet*/
    pH264VEComp->bCodecCreateSettingsChange = OMX_FALSE;  /*set to true when Create time settings are modified*/
    pH264VEComp->bInputPortDisable = OMX_FALSE; /*flag to indicate codec creation is required or not */
    pH264VEComp->bCodecFlush = OMX_FALSE;

    pH264VEComp->bCallxDMSetParams = NO_PARAM_CHANGE;
    pH264VEComp->pCodecInBufferArray = NULL;
    pH264VEComp->bAfterEOSReception = OMX_FALSE;
    pH264VEComp->bNotifyEOSEventToClient = OMX_FALSE;
    pH264VEComp->bPropagateEOSToOutputBuffer = OMX_FALSE;
    pH264VEComp->bSetParamInputIsDone = OMX_FALSE;

    pH264VEComp->bSendCodecConfig = OMX_TRUE;

    pH264VEComp->sCodecConfigData.sBuffer = OSAL_Malloc(sizeof(MemHeader));
    pH264VEComp->sCodecConfigData.sBuffer->ptr
                    = memplugin_alloc_noheader(pH264VEComp->sCodecConfigData.sBuffer, (SPS_PPS_HEADER_DATA_SIZE), 1, MEM_CARVEOUT, 0, 0);
    OMX_CHECK(pH264VEComp->sCodecConfigData.sBuffer->ptr != NULL, OMX_ErrorInsufficientResources);

    tStatus = OSAL_Memset(pH264VEComp->sCodecConfigData.sBuffer->ptr, 0x0, (SPS_PPS_HEADER_DATA_SIZE));
    OMX_CHECK(tStatus == OSAL_ErrNone, OMX_ErrorBadParameter);


    pH264VEComp->pTempBuffer[0] = OSAL_Malloc(sizeof(MemHeader));
    pH264VEComp->pTempBuffer[0]->ptr
                = memplugin_alloc_noheader(pH264VEComp->pTempBuffer[0], (SPS_PPS_HEADER_DATA_SIZE), 1, MEM_CARVEOUT, 0, 0);
    OMX_CHECK(pH264VEComp->pTempBuffer[0]->ptr != NULL, OMX_ErrorInsufficientResources);

    tStatus = OSAL_Memset(pH264VEComp->pTempBuffer[0]->ptr, 0x0, (SPS_PPS_HEADER_DATA_SIZE));
    OMX_CHECK(tStatus == OSAL_ErrNone, OMX_ErrorBadParameter);

    pH264VEComp->pTempBuffer[1] = OSAL_Malloc(sizeof(MemHeader));
    pH264VEComp->pTempBuffer[1]->ptr
                = memplugin_alloc_noheader(pH264VEComp->pTempBuffer[1], (SPS_PPS_HEADER_DATA_SIZE), 1, MEM_CARVEOUT, 0, 0);
    OMX_CHECK(pH264VEComp->pTempBuffer[1]->ptr != NULL, OMX_ErrorInsufficientResources);

    tStatus = OSAL_Memset(pH264VEComp->pTempBuffer[1]->ptr, 0x0, (SPS_PPS_HEADER_DATA_SIZE));
    OMX_CHECK(tStatus == OSAL_ErrNone, OMX_ErrorBadParameter);

    pH264VEComp->nCodecConfigSize = 0;
    pH264VEComp->bAfterGenHeader  = OMX_FALSE;

EXIT:
    if( eError != OMX_ErrorNone ) {
        OSAL_ErrorTrace(" in fn OMXH264VE_InitialzeComponentPrivateParams");
    }

    return (eError);

}

OMX_ERRORTYPE OMXH264VE_SetAlgDefaultCreationParams(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE         eError    = OMX_ErrorNone;
    OMX_COMPONENTTYPE    *pHandle   = NULL;
    OMXH264VidEncComp *pH264VEComp  = NULL;

    OMX_CHECK(hComponent != NULL, OMX_ErrorBadParameter);

    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    OMX_BASE_CHK_VERSION(pHandle, OMX_COMPONENTTYPE, eError);
    pH264VEComp = (OMXH264VidEncComp *)pHandle->pComponentPrivate;

    /* Set the Default IH264ENC_RateControlParams:   rateControlParams */
    SET_H264CODEC_DEFAULT_STATIC_IH264ENC_RATECONTROLPARAMS(pH264VEComp);

    /* Set the  Default IH264ENC_InterCodingParams   interCodingParams */
    SET_H264CODEC_DEFAULT_STATIC_IH264ENC_INTERCODINGPARAMS(pH264VEComp);

    /* Set the Default IH264ENC_IntraCodingParams   intraCodingParams */
    SET_H264CODEC_DEFAULT_STATIC_IH264ENC_INTRACODINGPARAMS(pH264VEComp);

    /* Set the Default IH264ENC_NALUControlParams   nalUnitControlParams */
    SET_H264CODEC_DEFAULT_STATIC_IH264ENC_NALUCONTROLPARAMS(pH264VEComp);

    /* Set the Default IH264ENC_SliceCodingParams   sliceCodingParams */
    SET_H264CODEC_DEFAULT_STATIC_IH264ENC_SLICECODINGPARAMS(pH264VEComp);

    /* Set the Default IH264ENC_LoopFilterParams    loopFilterParams */
    SET_H264CODEC_DEFAULT_STATIC_IH264ENC_LOOPFILTERPARAMS(pH264VEComp);

    /* Set the Default IH264ENC_FMOCodingParams     fmoCodingParams */
    SET_H264CODEC_DEFAULT_STATIC_IH264ENC_FMOCODINGPARAMS(pH264VEComp);

    /* Set the Default IH264ENC_VUICodingParams     vuiCodingParams */
    SET_H264CODEC_DEFAULT_STATIC_IH264ENC_VUICODINGPARAMS(pH264VEComp);

    /* Set the Default IH264ENC_StereoInfoParams     stereoInfoParams */
    SET_H264CODEC_DEFAULT_STATIC_IH264ENC_STEREOINFOPARAMS(pH264VEComp);

    /* Set the Default IH264ENC_FramePackingSEIParams     framePackingSEIParams */
    SET_H264CODEC_DEFAULT_STATIC_IH264ENC_STEREOFRAMEPACKINGPARAMS(pH264VEComp);

    /* Set the Default IH264ENC_SVCCodingParams    svcCodingParams */
    SET_H264CODEC_DEFAULT_STATIC_IH264ENC_SVCCODINGPARAMS(pH264VEComp);

    SET_H264CODEC_DEFAULT_STATIC_IH264ENC_EXTENDEDPARAMS(pH264VEComp);

EXIT:
    if( eError != OMX_ErrorNone ) {
        OSAL_ErrorTrace("in fn OMXH264VE_SetAlgDefaultCreationParams");
    }

    return (eError);
}

OMX_ERRORTYPE OMXH264VE_SetAlgDefaultDynamicParams(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE         eError    = OMX_ErrorNone;
    OMX_COMPONENTTYPE    *pHandle   = NULL;
    OMXH264VidEncComp  *pH264VEComp = NULL;

    OMX_CHECK(hComponent != NULL, OMX_ErrorBadParameter);
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    OMX_BASE_CHK_VERSION(pHandle, OMX_COMPONENTTYPE, eError);
    pH264VEComp = (OMXH264VidEncComp *)pHandle->pComponentPrivate;

    /* Set the  IH264ENC_RateControlParams   rateControlParams */
    SET_H264CODEC_DEFAULT_DYNAMIC_RATECONTROLPARAMS(pH264VEComp);

    /* Set the  IH264ENC_InterCodingParams   interCodingParams */
    SET_H264CODEC_DEFAULT_DYNAMIC_INTERCODINGPARAMS(pH264VEComp);

    /* Set the  IH264ENC_IntraCodingParams   interCodingParams */
    SET_H264CODEC_DEFAULT_DYNAMIC_INTRACODINGPARAMS(pH264VEComp);

    /* Set the IH264ENC_SliceCodingParams     sliceCodingParams */
    SET_H264CODEC_DEFAULT_DYNAMIC_SLICECODINGPARAMS(pH264VEComp);

    SET_H264CODEC_DEFAULT_DYNAMIC_IH264_EXTENDEDPARAMS(pH264VEComp);

EXIT:
    if( eError != OMX_ErrorNone ) {
        OSAL_ErrorTrace(" in fn OMXH264VE_SetAlgDefaultDynamicParams");
    }
    return (eError);
}


OMX_ERRORTYPE OMXH264VE_SetBufferDesc(OMX_HANDLETYPE hComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE         eError    = OMX_ErrorNone;
    OMX_COMPONENTTYPE    *pHandle   = NULL;
    OMXH264VidEncComp  *pH264VEComp = NULL;
    OMX_U8                i;
    OMX_U32               extWidth, extHeight, extStride;

    /* Check the input parameters, this should be TRUE else report an Error */
    OMX_CHECK(hComponent != NULL, OMX_ErrorBadParameter);
    pHandle = (OMX_COMPONENTTYPE *)hComponent;

    OMX_BASE_CHK_VERSION(pHandle, OMX_COMPONENTTYPE, eError);
    pH264VEComp = (OMXH264VidEncComp *)pHandle->pComponentPrivate;

    if( nPortIndex == OMX_H264VE_INPUT_PORT ) {
        pH264VEComp->pVedEncInBufs->numPlanes = pH264VEComp->pVidEncStatus->videnc2Status.bufInfo.minNumInBufs;
        for( i=0; i < pH264VEComp->pVedEncInBufs->numPlanes; i++ ) {
            pH264VEComp->pVedEncInBufs->planeDesc[i].memType = pH264VEComp->pVidEncStatus->videnc2Status.bufInfo.inBufMemoryType[i];
            if( pH264VEComp->pVedEncInBufs->planeDesc[i].memType == XDM_MEMTYPE_RAW ) {
                pH264VEComp->pVedEncInBufs->planeDesc[i].bufSize.bytes =
                    pH264VEComp->pVidEncStatus->videnc2Status.bufInfo.minInBufSize[i].bytes;
            } else {
                /* Since we support non-tiler input buffers for encoding, change the memtype */
                pH264VEComp->pVedEncInBufs->planeDesc[i].memType = XDM_MEMTYPE_RAW;
                pH264VEComp->pVedEncInBufs->planeDesc[i].bufSize.bytes = (pH264VEComp->pVidEncStatus->videnc2Status.bufInfo.minInBufSize[i].tileMem.width) * (pH264VEComp->pVidEncStatus->videnc2Status.bufInfo.minInBufSize[i].tileMem.height);
            }
        }

        OSAL_Info("Update the Image,Active region of Codec params");
        extWidth = pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameWidth; /*stride is already checked during codec creation that it should be multiples of 16*/
        extStride = pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nStride;
        extHeight = pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameHeight;

        pH264VEComp->pVedEncInBufs->imageRegion.topLeft.x = 0;
        pH264VEComp->pVedEncInBufs->imageRegion.topLeft.y = 0;
        pH264VEComp->pVedEncInBufs->imageRegion.bottomRight.x = extWidth;

        pH264VEComp->pVedEncInBufs->activeFrameRegion.topLeft.x = 0;
        pH264VEComp->pVedEncInBufs->activeFrameRegion.topLeft.y = 0;
        pH264VEComp->pVedEncInBufs->activeFrameRegion.bottomRight.x = extWidth;

        pH264VEComp->pVedEncInBufs->imagePitch[0] = extStride;
        pH264VEComp->pVedEncInBufs->imagePitch[1] = extStride;
        pH264VEComp->pVedEncInBufs->topFieldFirstFlag = OMX_TRUE;

        pH264VEComp->pVedEncInBufs->contentType = IVIDEO_PROGRESSIVE;
        pH264VEComp->pVedEncInBufs->activeFrameRegion.bottomRight.y = extHeight;
        pH264VEComp->pVedEncInBufs->imageRegion.bottomRight.y = extHeight;

        pH264VEComp->pVedEncInBufs->secondFieldOffsetWidth[0] = 0;
        pH264VEComp->pVedEncInBufs->secondFieldOffsetWidth[1] = 0;
        pH264VEComp->pVedEncInBufs->secondFieldOffsetHeight[0] = 0;
        pH264VEComp->pVedEncInBufs->secondFieldOffsetHeight[1] = 0;
        pH264VEComp->pVedEncInBufs->chromaFormat = XDM_YUV_420SP;

    } else if( nPortIndex == OMX_H264VE_OUTPUT_PORT ) {
        pH264VEComp->pVedEncOutBufs->numBufs = pH264VEComp->pVidEncStatus->videnc2Status.bufInfo.minNumOutBufs;
        for( i=0; i < pH264VEComp->pVedEncOutBufs->numBufs; i++ ) {
            pH264VEComp->pVedEncOutBufs->descs[i].memType = pH264VEComp->pVidEncStatus->videnc2Status.bufInfo.outBufMemoryType[i];
            if( pH264VEComp->pVedEncOutBufs->descs[i].memType == XDM_MEMTYPE_RAW ) {
                pH264VEComp->pVedEncOutBufs->descs[i].bufSize.bytes =
                    pH264VEComp->pVidEncStatus->videnc2Status.bufInfo.minOutBufSize[i].bytes;
            } else {
               /* since the output buffers are always non-tiled, change the memory type & size to RAW memory type*/
                pH264VEComp->pVedEncOutBufs->descs[i].memType = XDM_MEMTYPE_RAW;
                pH264VEComp->pVedEncOutBufs->descs[i].bufSize.bytes =
                        pH264VEComp->pVidEncStatus->videnc2Status.bufInfo.minOutBufSize[i].bytes;
            }
        }
    } else {
        eError=OMX_ErrorBadPortIndex;
    }

EXIT:
    if( eError != OMX_ErrorNone ) {
        OSAL_ErrorTrace("in fn OMXH264VE_SetBufferDesc");
    }

    return (eError);

}

OMX_ERRORTYPE OMXH264VE_SetEncCodecReady(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE         eError    = OMX_ErrorNone;
    OMXH264VidEncComp  *pH264VEComp = NULL;
    OMX_COMPONENTTYPE    *pHandle   = NULL;
    XDAS_Int32            retval    = 0;

    /* Check the input parameters */
    OMX_CHECK((hComponent != NULL), OMX_ErrorBadParameter);

    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pH264VEComp = (OMXH264VidEncComp *)pHandle->pComponentPrivate;

    /*set the HRD buffer size appropriately*/
    if( pH264VEComp->pVidEncStaticParams->rateControlParams.rcAlgo == IH264_RATECONTROL_PRC_LOW_DELAY ) {
        pH264VEComp->pVidEncStaticParams->rateControlParams.HRDBufferSize  =
                (pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.targetBitRate) / 2;
        pH264VEComp->pVidEncDynamicParams->rateControlParams.HRDBufferSize =
                (pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.targetBitRate) / 2;
    } else {
        pH264VEComp->pVidEncStaticParams->rateControlParams.HRDBufferSize  =
                (pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.targetBitRate);
        pH264VEComp->pVidEncDynamicParams->rateControlParams.HRDBufferSize =
                (pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.targetBitRate);
    }
    pH264VEComp->pVidEncStaticParams->rateControlParams.initialBufferLevel =
                pH264VEComp->pVidEncStaticParams->rateControlParams.HRDBufferSize;
    pH264VEComp->pVidEncDynamicParams->rateControlParams.initialBufferLevel =
                pH264VEComp->pVidEncStaticParams->rateControlParams.HRDBufferSize;
    pH264VEComp->pVidEncStaticParams->rateControlParams.rateControlParamsPreset = IH264_RATECONTROLPARAMS_USERDEFINED;

    if( pH264VEComp->pVidEncStaticParams->videnc2Params.maxHeight & 0x01 ) {
        eError = OMX_ErrorUnsupportedSetting;
        OSAL_ErrorTrace("Incorrect Height Settings");
        OSAL_ErrorTrace("for Progressive Port Def Height need be multiple of 2");
        OSAL_ErrorTrace("for Interlace Port Def Height need be multiple of 4");
        OMX_CHECK(eError == OMX_ErrorNone, eError);
    }

    /* Create H264 Encoder Instance */
    pH264VEComp->pVidEncHandle = VIDENC2_create(pH264VEComp->pCEhandle,
                  OMX_H264V_ENCODER_NAME,
                  (VIDENC2_Params *)(pH264VEComp->pVidEncStaticParams));

    OMX_CHECK(pH264VEComp->pVidEncHandle != NULL, OMX_ErrorInsufficientResources);

    OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
    pH264VEComp->bCodecCreate=OMX_TRUE;
    pH264VEComp->bCodecCreateSettingsChange=OMX_FALSE;
    OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);

    /* Set the Dynamic Parameters to the Codec */
    eError = OMXH264VE_SetDynamicParamsToCodec(hComponent);
    OMX_CHECK(eError == OMX_ErrorNone, eError);

    /* Get the Codec Status Parametrs and Update the Component private Params accordingly */
    eError = OMXH264VE_VISACONTROL(pH264VEComp->pVidEncHandle, XDM_GETSTATUS,
                                        (VIDENC2_DynamicParams *)(pH264VEComp->pVidEncDynamicParams),
                                        (IVIDENC2_Status *)(pH264VEComp->pVidEncStatus), hComponent, &retval);
    if( retval == VIDENC2_EFAIL ) {
        OSAL_Info("Got error from the Codec GetbufInfo call");
        OMX_TI_GET_ERROR(pH264VEComp, pH264VEComp->pVidEncStatus->videnc2Status.extendedError, eError);
        OMX_CHECK(eError == OMX_ErrorNone, eError);
    }
    OMX_CHECK(eError == OMX_ErrorNone, eError);

    eError= OMXH264VE_UpdateParams(pHandle);
    OMX_CHECK(eError == OMX_ErrorNone, eError);

    OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
    pH264VEComp->bCallxDMSetParams = NO_PARAM_CHANGE;
    OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);

    /* get bufffer information:
    * control->XDM_GETBUFINFO call always has to ensure the control->XDM_SETPARAMS call has done before
    * Get the Buf Info from the Codec */
    OSAL_Info("call to xDM GetBufInfo");
    eError = OMXH264VE_VISACONTROL(pH264VEComp->pVidEncHandle, XDM_GETBUFINFO,
                                        (VIDENC2_DynamicParams *)(pH264VEComp->pVidEncDynamicParams),
                                        (IVIDENC2_Status *)(pH264VEComp->pVidEncStatus), hComponent, &retval);
    if( retval == VIDENC2_EFAIL ) {
        OSAL_Info("Got error from the Codec GetbufInfo call");
        OMX_TI_GET_ERROR(pH264VEComp, pH264VEComp->pVidEncStatus->videnc2Status.extendedError, eError);
        OMX_CHECK(eError == OMX_ErrorNone, eError);
    }
    OMX_CHECK(eError == OMX_ErrorNone, eError);
    /* Initialize the Inputbufs and Outputbufs with the values that Codec Expects */
    eError=OMXH264VE_SetBufferDesc(pHandle, OMX_H264VE_INPUT_PORT);
    OMX_CHECK(eError == OMX_ErrorNone, eError);

    eError=OMXH264VE_SetBufferDesc(pHandle, OMX_H264VE_OUTPUT_PORT);
    OMX_CHECK(eError == OMX_ErrorNone, eError);

EXIT:
    if( eError != OMX_ErrorNone ) {
        OSAL_ErrorTrace("in fn OMXH264VE_SetEncCodecReady");
    }

    return (eError);

}

OMX_ERRORTYPE OMXH264VE_UpdateParams(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE         eError    = OMX_ErrorNone;
    OMXH264VidEncComp  *pH264VEComp = NULL;
    OMX_COMPONENTTYPE    *pHandle   = NULL;
    OMX_STATETYPE         tState;

    OMX_CHECK((hComponent != NULL), OMX_ErrorBadParameter);
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pH264VEComp = (OMXH264VidEncComp *)pHandle->pComponentPrivate;

    OSAL_Info("Update the OMX Component structures with Codec Structures");

    pH264VEComp->pVidEncStaticParams->videnc2Params.encodingPreset = pH264VEComp->pVidEncStatus->videnc2Status.encodingPreset;

    pH264VEComp->pVidEncStaticParams->videnc2Params.rateControlPreset = pH264VEComp->pVidEncStatus->videnc2Status.rateControlPreset;

    pH264VEComp->pVidEncStaticParams->videnc2Params.maxInterFrameInterval = pH264VEComp->pVidEncStatus->videnc2Status.maxInterFrameInterval;

    pH264VEComp->pVidEncStaticParams->videnc2Params.inputChromaFormat = pH264VEComp->pVidEncStatus->videnc2Status.inputChromaFormat;

    pH264VEComp->pVidEncStaticParams->videnc2Params.inputContentType = pH264VEComp->pVidEncStatus->videnc2Status.inputContentType;

    pH264VEComp->pVidEncStaticParams->videnc2Params.operatingMode =  pH264VEComp->pVidEncStatus->videnc2Status.operatingMode;

    pH264VEComp->pVidEncStaticParams->videnc2Params.profile = pH264VEComp->pVidEncStatus->videnc2Status.profile;

    pH264VEComp->pVidEncStaticParams->videnc2Params.level =  pH264VEComp->pVidEncStatus->videnc2Status.level;

    pH264VEComp->pVidEncStaticParams->videnc2Params.inputDataMode = pH264VEComp->pVidEncStatus->videnc2Status.inputDataMode;
    pH264VEComp->pVidEncStaticParams->videnc2Params.outputDataMode = pH264VEComp->pVidEncStatus->videnc2Status.outputDataMode;

    OSAL_Memcpy(&(pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams), &(pH264VEComp->pVidEncStatus->videnc2Status.encDynamicParams),
                     sizeof (IVIDENC2_DynamicParams));

    OSAL_Memcpy(&(pH264VEComp->pVidEncDynamicParams->rateControlParams), &(pH264VEComp->pVidEncStatus->rateControlParams),
                     sizeof (IH264ENC_RateControlParams));

    OSAL_Memcpy(&(pH264VEComp->pVidEncDynamicParams->interCodingParams), &(pH264VEComp->pVidEncStatus->interCodingParams),
                     sizeof (IH264ENC_InterCodingParams));

    OSAL_Memcpy(&(pH264VEComp->pVidEncDynamicParams->intraCodingParams), &(pH264VEComp->pVidEncStatus->intraCodingParams),
                     sizeof (IH264ENC_IntraCodingParams));

    OSAL_Memcpy(&(pH264VEComp->pVidEncStaticParams->nalUnitControlParams), &(pH264VEComp->pVidEncStatus->nalUnitControlParams),
                     sizeof (IH264ENC_NALUControlParams));

    OSAL_Memcpy(&(pH264VEComp->pVidEncDynamicParams->sliceCodingParams), &(pH264VEComp->pVidEncStatus->sliceCodingParams),
                     sizeof (IH264ENC_SliceCodingParams));

    OSAL_Memcpy(&(pH264VEComp->pVidEncStaticParams->loopFilterParams), &(pH264VEComp->pVidEncStatus->loopFilterParams),
                     sizeof (IH264ENC_LoopFilterParams));

    OSAL_Memcpy(&(pH264VEComp->pVidEncStaticParams->fmoCodingParams), &(pH264VEComp->pVidEncStatus->fmoCodingParams),
                     sizeof (IH264ENC_FMOCodingParams));

    OSAL_Memcpy(&(pH264VEComp->pVidEncStaticParams->vuiCodingParams), &(pH264VEComp->pVidEncStatus->vuiCodingParams),
                     sizeof (IH264ENC_VUICodingParams));

    OSAL_Memcpy(&(pH264VEComp->pVidEncStaticParams->stereoInfoParams), &(pH264VEComp->pVidEncStatus->stereoInfoParams),
                     sizeof (IH264ENC_StereoInfoParams));

    OSAL_Memcpy(&(pH264VEComp->pVidEncStaticParams->framePackingSEIParams), &(pH264VEComp->pVidEncStatus->framePackingSEIParams),
                     sizeof (IH264ENC_FramePackingSEIParams));

    OSAL_Memcpy(&(pH264VEComp->pVidEncStaticParams->svcCodingParams), &(pH264VEComp->pVidEncStatus->svcCodingParams),
                     sizeof (IH264ENC_SVCCodingParams));

    pH264VEComp->pVidEncStaticParams->interlaceCodingType     = pH264VEComp->pVidEncStatus->interlaceCodingType;

    pH264VEComp->pVidEncStaticParams->bottomFieldIntra     = pH264VEComp->pVidEncStatus->bottomFieldIntra;

    pH264VEComp->pVidEncStaticParams->gopStructure        = pH264VEComp->pVidEncStatus->gopStructure;

    pH264VEComp->pVidEncStaticParams->entropyCodingMode   = pH264VEComp->pVidEncStatus->entropyCodingMode;

    pH264VEComp->pVidEncStaticParams->transformBlockSize  =pH264VEComp->pVidEncStatus->transformBlockSize;

    pH264VEComp->pVidEncStaticParams->log2MaxFNumMinus4   = pH264VEComp->pVidEncStatus->log2MaxFNumMinus4;

    pH264VEComp->pVidEncStaticParams->picOrderCountType   = pH264VEComp->pVidEncStatus->picOrderCountType;

    pH264VEComp->pVidEncStaticParams->enableWatermark   = pH264VEComp->pVidEncStatus->enableWatermark;

    pH264VEComp->pVidEncStaticParams->IDRFrameInterval    =pH264VEComp->pVidEncStatus->IDRFrameInterval;

    pH264VEComp->pVidEncStaticParams->maxIntraFrameInterval    =pH264VEComp->pVidEncStatus->maxIntraFrameInterval;

    pH264VEComp->pVidEncStaticParams->debugTraceLevel = pH264VEComp->pVidEncStatus->debugTraceLevel;

    pH264VEComp->pVidEncStaticParams->lastNFramesToLog = pH264VEComp->pVidEncStatus->lastNFramesToLog;

    pH264VEComp->pVidEncStaticParams->enableAnalyticinfo    =pH264VEComp->pVidEncStatus->enableAnalyticinfo;

    pH264VEComp->pVidEncStaticParams->enableGMVSei    =pH264VEComp->pVidEncStatus->enableGMVSei;

    pH264VEComp->pVidEncStaticParams->constraintSetFlags    =pH264VEComp->pVidEncStatus->constraintSetFlags;

    pH264VEComp->pVidEncStaticParams->enableRCDO    =pH264VEComp->pVidEncStatus->enableRCDO;

    pH264VEComp->pVidEncStaticParams->enableLongTermRefFrame    =pH264VEComp->pVidEncStatus->enableLongTermRefFrame;

    pH264VEComp->pVidEncStaticParams->LTRPPeriod    =pH264VEComp->pVidEncStatus->LTRPPeriod;

    pH264VEComp->pVidEncStaticParams->numTemporalLayer    =pH264VEComp->pVidEncStatus->numTemporalLayer;

    pH264VEComp->pVidEncStaticParams->referencePicMarking    =pH264VEComp->pVidEncStatus->referencePicMarking;

    pH264VEComp->pVidEncDynamicParams->searchCenter.x = pH264VEComp->pVidEncStatus->searchCenter.x;

    pH264VEComp->pVidEncDynamicParams->searchCenter.x = pH264VEComp->pVidEncStatus->searchCenter.y;

    pH264VEComp->pVidEncDynamicParams->enableStaticMBCount    =pH264VEComp->pVidEncStatus->enableStaticMBCount;

    pH264VEComp->pVidEncDynamicParams->enableROI = pH264VEComp->pVidEncStatus->enableROI;

    OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
    tState=pH264VEComp->sBase.tCurState;
    OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);

    if((tState == OMX_StateLoaded) || (tState == OMX_StateIdle)) {
        OSAL_Memcpy(&(pH264VEComp->pVidEncStaticParams->rateControlParams), &(pH264VEComp->pVidEncStatus->rateControlParams),
                         sizeof (IH264ENC_RateControlParams));

        OSAL_Memcpy(&(pH264VEComp->pVidEncStaticParams->interCodingParams), &(pH264VEComp->pVidEncStatus->interCodingParams),
                         sizeof (IH264ENC_InterCodingParams));

        OSAL_Memcpy(&(pH264VEComp->pVidEncStaticParams->sliceCodingParams), &(pH264VEComp->pVidEncStatus->sliceCodingParams),
                         sizeof (IH264ENC_SliceCodingParams));

        OSAL_Memcpy(&(pH264VEComp->pVidEncStaticParams->intraCodingParams), &(pH264VEComp->pVidEncStatus->intraCodingParams),
                         sizeof (IH264ENC_IntraCodingParams));
    }

EXIT:
    if( eError != OMX_ErrorNone ) {
        OSAL_ErrorTrace("in fn OMXH264VE_UpdateParams");
    }

    return (eError);
}

OMX_ERRORTYPE OMXH264VE_FLUSHLockedBuffers(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE         eError        = OMX_ErrorNone;
    OMXH264VidEncComp   *pH264VEComp    = NULL;
    OMX_COMPONENTTYPE    *pHandle       = NULL;
    OMX_U32               i;

    /* Check the input parameters */
    OMX_CHECK((hComponent != NULL), OMX_ErrorBadParameter);
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pH264VEComp = (OMXH264VidEncComp *)pHandle->pComponentPrivate;

    for( i=0; i < pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.nBufferCountActual; i++ ) {
        if((pH264VEComp->pCodecInBufferArray[i]) &&
                ((OMXBase_BufHdrPvtData *)(pH264VEComp->pCodecInBufferArray[i]->pPlatformPrivate))->bufSt == OWNED_BY_CODEC ) {
            pH264VEComp->pCodecInBufferArray[i]->nOffset= 0;
            /*update the status to free*/
            ((OMXBase_BufHdrPvtData *)(pH264VEComp->pCodecInBufferArray[i]->pPlatformPrivate))->bufSt = OWNED_BY_US;
            pH264VEComp->sBase.pPvtData->fpDioSend(hComponent, OMX_H264VE_INPUT_PORT, pH264VEComp->pCodecInBufferArray[i]);
        }
    }

EXIT:
    return (eError);
}

OMX_ERRORTYPE  OMXH264VE_GetNextFreeBufHdr(OMX_HANDLETYPE hComponent, OMX_S32 *nBuffIndex, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE         eError        = OMX_ErrorNone;
    OMXH264VidEncComp   *pH264VEComp    = NULL;
    OMX_COMPONENTTYPE    *pHandle       = NULL;
    OMX_BOOL              bFreeBuffFound = OMX_FALSE;
    OMX_S32               LBufIndex     = -1;
    OMX_U32               i;

    /* Check the input parameters */
    OMX_CHECK((hComponent != NULL), OMX_ErrorBadParameter);
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pH264VEComp = (OMXH264VidEncComp *)pHandle->pComponentPrivate;

    if( nPortIndex == OMX_H264VE_INPUT_PORT ) {
        for( i=0; i < pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.nBufferCountActual; i++ ) {
            if(!pH264VEComp->pCodecInBufferArray[i] ||
                    ((OMXBase_BufHdrPvtData *)(pH264VEComp->pCodecInBufferArray[i]->pPlatformPrivate))->bufSt != OWNED_BY_CODEC ) {
                bFreeBuffFound=OMX_TRUE;
                LBufIndex=i;
                break;
            }
        }
    } else {
        eError = OMX_ErrorBadParameter;
    }

EXIT:

    if( bFreeBuffFound ) {
        *nBuffIndex=LBufIndex;
    } else {
        *nBuffIndex=-1;
    }
    return (eError);
}

OMX_ERRORTYPE  OMXH264VE_SetDynamicParamsToCodec(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE         eError        = OMX_ErrorNone;
    OMXH264VidEncComp   *pH264VEComp    = NULL;
    OMX_COMPONENTTYPE    *pHandle       = NULL;
    XDAS_Int32            retval        = 0;

    /* Check the input parameters */
    OMX_CHECK((hComponent != NULL), OMX_ErrorBadParameter);
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pH264VEComp = (OMXH264VidEncComp *)pHandle->pComponentPrivate;

    pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.size  = sizeof(IH264ENC_DynamicParams);
    pH264VEComp->pVidEncStatus->videnc2Status.size = sizeof(IH264ENC_Status);

    if((pH264VEComp->bSendCodecConfig)) {
        pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.generateHeader = XDM_GENERATE_HEADER;
        pH264VEComp->nCodecConfigSize = 0;
        pH264VEComp->bAfterGenHeader  = OMX_FALSE;
    }

    pH264VEComp->pVidEncDynamicParams->rateControlParams.rateControlParamsPreset = 2; //Existing

    pH264VEComp->pVidEncDynamicParams->interCodingParams.interCodingPreset = 2; //Existing

    pH264VEComp->pVidEncDynamicParams->intraCodingParams.intraCodingPreset = IH264_INTRACODING_EXISTING;

    pH264VEComp->pVidEncDynamicParams->sliceCodingParams.sliceCodingPreset = 2; //Existing

    OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
    pH264VEComp->bCallxDMSetParams=PARAMS_UPDATED_AT_CODEC;
    OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);

    eError = OMXH264VE_VISACONTROL(pH264VEComp->pVidEncHandle, XDM_SETPARAMS,
                                        (VIDENC2_DynamicParams *)(pH264VEComp->pVidEncDynamicParams),
                                        (IVIDENC2_Status *)(pH264VEComp->pVidEncStatus), hComponent, &retval);
    if( retval == VIDENC2_EFAIL ) {
        ALOGE("pH264VEComp->pVidEncStatus->videnc2Status.extendedError = %x", pH264VEComp->pVidEncStatus->videnc2Status.extendedError);
        OMX_TI_GET_ERROR(pH264VEComp, pH264VEComp->pVidEncStatus->videnc2Status.extendedError, eError);
        OMX_CHECK(eError == OMX_ErrorNone, eError);
    }


EXIT:
    return (eError);
}

OMX_ERRORTYPE  OMXH264VE_GetNumCodecLockedBuffers(OMX_HANDLETYPE hComponent, OMX_U32 *nLockedBuffCount)
{
    OMX_ERRORTYPE         eError        = OMX_ErrorNone;
    OMXH264VidEncComp   *pH264VEComp    = NULL;
    OMX_COMPONENTTYPE    *pHandle       = NULL;
    OMX_U32               LBuffLockedCount = 0;
    OMX_U32               i;

    /* Check the input parameters */
    OMX_CHECK((hComponent != NULL), OMX_ErrorBadParameter);
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pH264VEComp = (OMXH264VidEncComp *)pHandle->pComponentPrivate;

    for( i=0; i < pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.nBufferCountActual; i++ ) {
        if((pH264VEComp->pCodecInBufferArray[i]) &&
                ((OMXBase_BufHdrPvtData *)(pH264VEComp->pCodecInBufferArray[i]->pPlatformPrivate))->bufSt == OWNED_BY_CODEC ) {
            LBuffLockedCount++;

        }
    }

    *nLockedBuffCount=LBuffLockedCount;

EXIT:
    return (eError);
}

OMX_ERRORTYPE OMXH264VE_VISACONTROL(VIDENC2_Handle handle, IVIDENC2_Cmd id, IVIDENC2_DynamicParams *dynParams,
                                        IVIDENC2_Status *status, OMX_HANDLETYPE hComponent, XDAS_Int32 *retval)
{
    OMX_ERRORTYPE        eError     = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle      = NULL;
    OMXH264VidEncComp *pH264VEComp  = NULL;

    /*Check for the Params*/
    OMX_CHECK(((hComponent != NULL) && (dynParams != NULL) && (status != NULL) && (retval != NULL)), OMX_ErrorBadParameter);
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pH264VEComp = pHandle->pComponentPrivate;

    (*retval) = VIDENC2_control(pH264VEComp->pVidEncHandle, id, dynParams, status);

EXIT:
    return (eError);
}


OMX_ERRORTYPE OMXH264VE_VISAPROCESS_AND_UPDATEPARAMS(OMX_HANDLETYPE hComponent, XDAS_Int32 *retval)
{
    OMX_ERRORTYPE           eError              = OMX_ErrorNone;
    OMX_COMPONENTTYPE      *pHandle             = NULL;
    OMXH264VidEncComp     *pH264VEComp          = NULL;
    PARAMS_UPDATE_STATUS    bLCallxDMSetParams  = NO_PARAM_CHANGE;
    OMX_U32 sLretval;

    /*Check for the Params*/
    OMX_CHECK(((hComponent != NULL) && (retval != NULL)), OMX_ErrorBadParameter);
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pH264VEComp = pHandle->pComponentPrivate;

    OSAL_Info("Before the Codec Process call");
    sLretval = VIDENC2_process(pH264VEComp->pVidEncHandle,
                            (pH264VEComp->pVedEncInBufs),
                            (pH264VEComp->pVedEncOutBufs),
                            (IVIDENC2_InArgs *)(pH264VEComp->pVidEncInArgs),
                            (IVIDENC2_OutArgs *)(pH264VEComp->pVidEncOutArgs));

    if( sLretval == VIDENC2_EFAIL ) {
        OMX_TI_GET_ERROR(pH264VEComp, pH264VEComp->pVidEncOutArgs->videnc2OutArgs.extendedError, eError);
        if( eError != OMX_ErrorNone ) {
            ALOGE("Got error 0x%x from the Codec Process call", eError);
            goto UPDATE_PARAMS;
        }
    }

    /* Get the Codec Status Parameters if there has been a setconfig which has been translated to the codec */
    OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
    bLCallxDMSetParams=pH264VEComp->bCallxDMSetParams;
    OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);

    if( bLCallxDMSetParams == PARAMS_UPDATED_AT_CODEC ) {
        OSAL_Info("Update the Codec Params after Codec Process call: call xDM control->GetStatus");
        sLretval = VIDENC2_control(pH264VEComp->pVidEncHandle, XDM_GETSTATUS,
                                (VIDENC2_DynamicParams *)(pH264VEComp->pVidEncDynamicParams),
                                (IVIDENC2_Status *)(pH264VEComp->pVidEncStatus));
        if( sLretval == VIDENC2_EFAIL ) {
            OMX_TI_GET_ERROR(pH264VEComp, pH264VEComp->pVidEncOutArgs->videnc2OutArgs.extendedError, eError);
            if( eError != OMX_ErrorNone ) {
                OSAL_ErrorTrace("Got error 0x%x from the Codec Get Status Control call", eError);
                goto UPDATE_PARAMS;
            }
        }
    }

UPDATE_PARAMS:
    if( eError == OMX_ErrorNone ) {
        OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
        bLCallxDMSetParams = pH264VEComp->bCallxDMSetParams;
        if( bLCallxDMSetParams == PARAMS_UPDATED_AT_CODEC ) {
            OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
            eError= OMXH264VE_UpdateParams(hComponent);
            OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
            pH264VEComp->bCallxDMSetParams = NO_PARAM_CHANGE;
        }
        OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
    }

EXIT:
    if( retval ) {
        *retval = sLretval;
    }

    return (eError);
}

/* Function to check the max bit rate supported as per profile & level  settings*/
OMX_ERRORTYPE OMXH264VE_CheckBitRateCap(OMX_U32 targetBitRate, OMX_HANDLETYPE hComponent)
{
    OMX_COMPONENTTYPE   *pHandle        = NULL;
    OMXH264VidEncComp   *pH264VEComp    = NULL;
    OMX_ERRORTYPE       eError          = OMX_ErrorNone;
    OMX_VIDEO_AVCLEVELTYPE  eOMXLevel;
    OMX_U32                 nTableIndex = 0xFFFFFFFF;

    /* Check the input parameters */
    OMX_CHECK((hComponent != NULL), OMX_ErrorBadParameter);
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pH264VEComp = (OMXH264VidEncComp *)pHandle->pComponentPrivate;
    OMX_CHECK((pH264VEComp != NULL), OMX_ErrorBadParameter);

    MAP_CODEC_TO_OMX_AVCLEVEL(pH264VEComp->pVidEncStaticParams->videnc2Params.level, eOMXLevel, nTableIndex, eError);
    OMX_CHECK(eError == OMX_ErrorNone, OMX_ErrorUnsupportedSetting);
    OMX_CHECK(nTableIndex != 0xFFFFFFFF, OMX_ErrorUnsupportedSetting);

    if( pH264VEComp->pVidEncStaticParams->videnc2Params.profile == IH264_HIGH_PROFILE ) {
        OSAL_Info(" HIGH PROFILE, Level %d Max Bit Rate Supported = %d", eOMXLevel, OMX_H264_HP_BITRATE_SUPPORT[nTableIndex].nMaxBitRateSupport);
        OMX_CHECK(targetBitRate <= OMX_H264_HP_BITRATE_SUPPORT[nTableIndex].nMaxBitRateSupport, OMX_ErrorUnsupportedSetting);
    } else {
        OSAL_Info(" BASE/MAIN PROFILE, Level %d Max Bit Rate Supported = %d", eOMXLevel, OMX_H264_BP_MP_BITRATE_SUPPORT[nTableIndex].nMaxBitRateSupport);
        OMX_CHECK(targetBitRate <= OMX_H264_BP_MP_BITRATE_SUPPORT[nTableIndex].nMaxBitRateSupport, OMX_ErrorUnsupportedSetting);
    }

EXIT:
    return (eError);
}


