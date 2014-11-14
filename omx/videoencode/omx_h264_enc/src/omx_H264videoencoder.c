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

#include <MetadataBufferType.h>
#include "omx_H264videoencoder.h"

#define LOG_TAG "OMX_H264_ENCODER"

#define COLORCONVERT_MAX_SUB_BUFFERS (3)

#define COLORCONVERT_BUFTYPE_VIRTUAL (0x0)
#define COLORCONVERT_BUFTYPE_ION     (0x1)
#define COLORCONVERT_BUFTYPE_GRALLOCOPAQUE (0x2)

static OMX_ERRORTYPE OMXH264VE_GetParameter(OMX_HANDLETYPE hComponent,
                                                OMX_INDEXTYPE nParamIndex,
                                                OMX_PTR pParamStruct);

static OMX_ERRORTYPE OMXH264VE_SetParameter(OMX_HANDLETYPE hComponent,
                                                OMX_INDEXTYPE nParamIndex,
                                                OMX_PTR pParamStruct);

static OMX_ERRORTYPE OMXH264VE_GetConfig(OMX_HANDLETYPE hComponent,
                                             OMX_INDEXTYPE nIndex,
                                             OMX_PTR pConfigData);

static OMX_ERRORTYPE OMXH264VE_SetConfig(OMX_HANDLETYPE hComponent,
                                             OMX_INDEXTYPE nIndex,
                                             OMX_PTR pConfigData);

static OMX_ERRORTYPE OMXH264VE_ComponentTunnelRequest(OMX_HANDLETYPE hComponent,
                                                          OMX_U32 nPort,
                                                          OMX_HANDLETYPE hTunneledComp,
                                                          OMX_U32 nTunneledPort,
                                                          OMX_TUNNELSETUPTYPE *pTunnelSetup);

static OMX_ERRORTYPE OMXH264VE_CommandNotify(OMX_HANDLETYPE hComponent,
                                                         OMX_COMMANDTYPE Cmd,
                                                         OMX_U32 nParam,
                                                         OMX_PTR pCmdData);

static OMX_ERRORTYPE OMXH264VE_DataNotify(OMX_HANDLETYPE hComponent);


static OMX_ERRORTYPE OMXH264VE_ComponentDeinit(OMX_IN OMX_HANDLETYPE hComponent);


static OMX_ERRORTYPE OMXH264VE_GetExtensionIndex(OMX_HANDLETYPE hComponent,
                                                     OMX_STRING cParameterName,
                                                     OMX_INDEXTYPE *pIndexType);


int COLORCONVERT_PlatformOpaqueToNV12(void *hCC,
                                    void *pSrc[COLORCONVERT_MAX_SUB_BUFFERS],
                                    void *pDst[COLORCONVERT_MAX_SUB_BUFFERS],
                                    int nWidth, int nHeight, int nStride,
                                    int nSrcBufType,int nDstBufType)
{
    IMG_gralloc_module_public_t const* module = hCC;
    int nErr = -1;

    if((nSrcBufType == COLORCONVERT_BUFTYPE_GRALLOCOPAQUE) && (nDstBufType == COLORCONVERT_BUFTYPE_VIRTUAL)) {
        nErr = module->Blit(module, pSrc[0], pDst, HAL_PIXEL_FORMAT_TI_NV12);
    } else if((nSrcBufType == COLORCONVERT_BUFTYPE_GRALLOCOPAQUE) && (nDstBufType == COLORCONVERT_BUFTYPE_GRALLOCOPAQUE )) {
        nErr = module->Blit2(module, pSrc[0], pDst[0], nWidth, nHeight, 0, 0);
    }

    return nErr;
}

OMX_ERRORTYPE OMX_ComponentInit(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE                       eError = OMX_ErrorNone;
    OSAL_ERROR                 tStatus =  OSAL_ErrNone;
    OMXH264VidEncComp                 *pH264VEComp = NULL;
    OMX_COMPONENTTYPE                  *pHandle = NULL;

    OMX_CHECK(hComponent != NULL, OMX_ErrorBadParameter);
    pHandle = (OMX_COMPONENTTYPE *)hComponent;

    OMX_BASE_CHK_VERSION(pHandle, OMX_COMPONENTTYPE, eError);

    /* Allocate memory for H264VE Component's private data area */
    pHandle->pComponentPrivate = (OMXH264VidEncComp *)OSAL_Malloc(sizeof(OMXH264VidEncComp));
    OMX_CHECK(pHandle->pComponentPrivate != NULL, OMX_ErrorInsufficientResources);
    tStatus = OSAL_Memset(pHandle->pComponentPrivate, 0x0, sizeof(OMXH264VidEncComp));
    OMX_CHECK(tStatus == OSAL_ErrNone, OMX_ErrorBadParameter);

    /*Initialize the Component Private handle*/
    pH264VEComp = (OMXH264VidEncComp *)pHandle->pComponentPrivate;

    eError= OMXH264VE_InitFields(pHandle);
    OMX_CHECK(OMX_ErrorNone == eError, eError);

    /*initialize the  Hooks to Notify Command and Data from Base Comp to Derived Comp */
    pH264VEComp->sBase.fpCommandNotify  = OMXH264VE_CommandNotify;
    pH264VEComp->sBase.fpDataNotify     = OMXH264VE_DataNotify;

    /* initialize the base component */
    eError = OMXBase_ComponentInit(hComponent);
    OMX_CHECK(OMX_ErrorNone == eError, eError);

    strcpy((char *)pH264VEComp->sBase.pPvtData->cTaskName, "H264VideoEncoder");
    pH264VEComp->sBase.pPvtData->nStackSize   = OMX_H264VE_STACKSIZE;
    pH264VEComp->sBase.pPvtData->nPrioirty    = OMX_H264VE_DEFAULT_TASKPRIORITY;

    /*Setting default properties. PreCondition: NumOfPorts is filled and all pointers allocated*/
    eError = OMXBase_SetDefaultProperties(hComponent);
    OMX_CHECK(eError == OMX_ErrorNone, OMX_ErrorUnsupportedSetting);

    /* Override the function pointers of pHandle "OMX_COMPONENTTYPE"
    * which Base cannot handle and needs to be taken care by the Dervied comp*/
    pHandle->GetParameter             = OMXH264VE_GetParameter;
    pHandle->SetParameter             = OMXH264VE_SetParameter;
    pHandle->SetConfig                = OMXH264VE_SetConfig;
    pHandle->GetConfig                = OMXH264VE_GetConfig;
    pHandle->ComponentDeInit          = OMXH264VE_ComponentDeinit;
    pHandle->GetExtensionIndex        = OMXH264VE_GetExtensionIndex;
    pHandle->ComponentTunnelRequest   = OMXH264VE_ComponentTunnelRequest;

    eError= OMXH264VE_InitialzeComponentPrivateParams(pHandle);
    OMX_CHECK(OMX_ErrorNone == eError, eError);

    /* Open the Engine*/
    pH264VEComp->pCEhandle = Engine_open(OMX_ENGINE_NAME, NULL, &pH264VEComp->tCEerror);
    if( pH264VEComp->pCEhandle == NULL ) {
        OSAL_ErrorTrace("Engine creation: Could not open engine \n");
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pH264VEComp->bInputMetaDataBufferMode = OMX_FALSE;
    pH264VEComp->mAllocDev = NULL;
    pH264VEComp->hCC = NULL;

EXIT:
    if( eError != OMX_ErrorNone ) {
        OSAL_ErrorTrace(" H264VE Comp Initialization Failed...! ");
        if( pHandle ) {
            pHandle->ComponentDeInit(hComponent);
        }
    }

    return (eError);
}


static OMX_ERRORTYPE OMXH264VE_GetParameter(OMX_HANDLETYPE hComponent,
                                                OMX_INDEXTYPE nParamIndex,
                                                OMX_PTR pParamStruct)
{
    OMX_ERRORTYPE                              eError = OMX_ErrorNone;
    OMXH264VidEncComp                          *pH264VEComp = NULL;
    OMX_COMPONENTTYPE                          *pHandle = NULL;
    OMX_VIDEO_PARAM_PORTFORMATTYPE             *pVideoPortParams = NULL;
    OMX_VIDEO_PARAM_AVCTYPE                    *LAVCParams=NULL;
    OMX_VIDEO_PARAM_VBSMCTYPE                  *LVBSMC =  NULL;
    OMX_STATETYPE                               nLCurState;
    OMX_BOOL                                    bAllocateLocalAVC=OMX_FALSE;
    OMX_U32                                     nPortIndex;

    /* Check all the input parametrs */
    OMX_CHECK(hComponent != NULL, OMX_ErrorBadParameter);
    OMX_CHECK(pParamStruct != NULL, OMX_ErrorBadParameter);

    /*initialize the component handle and component pvt handle*/
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pH264VEComp = (OMXH264VidEncComp *)pHandle->pComponentPrivate;

    /* Allocate memory for LAVCParams */
    LAVCParams = (OMX_VIDEO_PARAM_AVCTYPE *)OSAL_Malloc(sizeof(OMX_VIDEO_PARAM_AVCTYPE));
    OMX_CHECK(LAVCParams != NULL, OMX_ErrorInsufficientResources);
    bAllocateLocalAVC=OMX_TRUE;

    OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
    nLCurState=pH264VEComp->sBase.tCurState;
    OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
    /* GetParameter can't be invoked incase the comp is in Invalid State  */
    OMX_CHECK(nLCurState != OMX_StateInvalid, OMX_ErrorIncorrectStateOperation);

    switch( nParamIndex ) {
        /* case OMX_IndexParamVideoInit: required for Standard Video Encoder as per Spec & is defined at BASE */
        /* case OMX_IndexParamPortDefinition:  required for Standard Video Encoder as per Spec & is defined at BASE */
        /* required for Standard Video Encoder as per Spec ..
        client uses this to query the format supported by the port */
        case OMX_IndexParamVideoPortFormat :
            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_PORTFORMATTYPE, eError);
            pVideoPortParams = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)pParamStruct;
            /* Retrieving the Input Port params */
            if( pVideoPortParams->nPortIndex == pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.nPortIndex ) {
                /*Get the compression format*/
                pVideoPortParams->eCompressionFormat = OMX_VIDEO_CodingUnused;

                /*Get the Frame rate : from the codec Dynamic Params...Q16 format*/
                pVideoPortParams->xFramerate = (pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.targetFrameRate / 1000) << 16;

                /*Get the supported (only 420SP is supported) color formats  : from the Codec creation time Params*/
                switch( pVideoPortParams->nIndex ) {
                    case 0 :
                        GET_OMX_COLORFORMAT(pH264VEComp, eError);
                        pVideoPortParams->eColorFormat = pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.eColorFormat;
                        break;
                    case 1 :
                            pVideoPortParams->eColorFormat = OMX_COLOR_FormatAndroidOpaque;
                        break;
                    default :
                        eError = OMX_ErrorNoMore;
                        break;
                }
            }
            /* Retrieving the Output Port params */
            else if( pVideoPortParams->nPortIndex == pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.nPortIndex ) {

                /*Get the Color Format*/
                pVideoPortParams->eColorFormat = OMX_COLOR_FormatUnused;

                /*Get the Frame Rate */
                pVideoPortParams->xFramerate = 0;

                /*Get the Supported CompressionFormats: only AVC is supported*/
                switch( pVideoPortParams->nIndex ) {
                    case 0 :
                        pVideoPortParams->eCompressionFormat = OMX_VIDEO_CodingAVC;
                        break;
                    default :
                        eError=OMX_ErrorNoMore;
                        break;
                }
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        /* required for Standard Video Encoder as per Spec &
        Client uses this to retrieve the Info related to the AVC rate control type*/
        case OMX_IndexParamVideoBitrate :
            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_BITRATETYPE, eError);
            if(((OMX_VIDEO_PARAM_BITRATETYPE *)pParamStruct)->nPortIndex == OMX_H264VE_OUTPUT_PORT ) {
                /*Get the Rate Control Algorithm: from the Codec Creation Time Params*/
                GET_OMX_RC_ALG(pH264VEComp, pParamStruct, eError);

                /*Get the Target Bit Rate: from the Codec Dynamic Params*/
                ((OMX_VIDEO_PARAM_BITRATETYPE *)pParamStruct)->nTargetBitrate=
                                pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.targetBitRate;

            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        /* required for Standard Video Encoder as per Spec &
        Client uses this to retrieve the Info related to the AVC structure type*/
        case OMX_IndexParamVideoAvc :
            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_AVCTYPE, eError);
            if(((OMX_VIDEO_PARAM_AVCTYPE *)pParamStruct)->nPortIndex == OMX_H264VE_OUTPUT_PORT ) {
                GET_OMX_AVC_PARAMS(pH264VEComp, pParamStruct);
                /*Get the Profile value from the Codec Creation Params*/
                GET_OMX_AVC_PROFILE(pH264VEComp, pParamStruct, eError);

                /*Get the level from the Codec Creation Params*/
                GET_OMX_AVC_LEVEL(pH264VEComp, pParamStruct, eError);

                /*get the LoopFilter mode form the Codec Creation Time Params*/
                GET_OMX_AVC_LFMODE(pH264VEComp, pParamStruct, eError);
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        /* Client uses this to retrieve the Info related to the Motion vector type*/
        case OMX_IndexParamVideoMotionVector :
            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_MOTIONVECTORTYPE, eError);
            if(((OMX_VIDEO_PARAM_MOTIONVECTORTYPE *)pParamStruct)->nPortIndex == OMX_H264VE_OUTPUT_PORT ) {
                /*Get the MV Accuracy from Codec Dynamic Params*/
                ((OMX_VIDEO_PARAM_MOTIONVECTORTYPE *)pParamStruct)->eAccuracy =
                        (OMX_VIDEO_MOTIONVECTORTYPE)pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.mvAccuracy;
                ((OMX_VIDEO_PARAM_MOTIONVECTORTYPE *)pParamStruct)->bUnrestrictedMVs = OMX_TRUE; /*by Default Codec Supports*/
                /*Number of MVs depend on the min Block size selected*/
                ((OMX_VIDEO_PARAM_MOTIONVECTORTYPE *)pParamStruct)->bFourMV =
                        (pH264VEComp->pVidEncStaticParams->interCodingParams.minBlockSizeP == IH264_BLOCKSIZE_8x8 ? OMX_TRUE : OMX_FALSE);
                /*Get the Search Range from the search Range for P Frame*/
                ((OMX_VIDEO_PARAM_MOTIONVECTORTYPE *)pParamStruct)->sXSearchRange =
                        pH264VEComp->pVidEncStaticParams->interCodingParams.searchRangeHorP;

                ((OMX_VIDEO_PARAM_MOTIONVECTORTYPE *)pParamStruct)->sYSearchRange =
                        pH264VEComp->pVidEncStaticParams->interCodingParams.searchRangeVerP;
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        /* Client uses this to configure Info related to the quantization parameter type*/
        case OMX_IndexParamVideoQuantization :
            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_QUANTIZATIONTYPE, eError);
            if(((OMX_VIDEO_PARAM_QUANTIZATIONTYPE *)pParamStruct)->nPortIndex == OMX_H264VE_OUTPUT_PORT ) {
                ((OMX_VIDEO_PARAM_QUANTIZATIONTYPE *)pParamStruct)->nQpI = pH264VEComp->pVidEncStaticParams->rateControlParams.qpI;
                ((OMX_VIDEO_PARAM_QUANTIZATIONTYPE *)pParamStruct)->nQpP = pH264VEComp->pVidEncStaticParams->rateControlParams.qpP;
                ((OMX_VIDEO_PARAM_QUANTIZATIONTYPE *)pParamStruct)->nQpB = (pH264VEComp->pVidEncStaticParams->rateControlParams.qpP)
                                                                            + (pH264VEComp->pVidEncStaticParams->rateControlParams.qpOffsetB);
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexParamVideoSliceFMO :
            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_AVCSLICEFMO, eError);
            if(((OMX_VIDEO_PARAM_AVCSLICEFMO *)pParamStruct)->nPortIndex == OMX_H264VE_OUTPUT_PORT ) {
                ((OMX_VIDEO_PARAM_AVCSLICEFMO *)pParamStruct)->nNumSliceGroups =
                        pH264VEComp->pVidEncStaticParams->fmoCodingParams.numSliceGroups;

                /*get the fmo slice grp type form the Codec Creation Time Params*/
                GET_OMX_FMO_SLIGRPMAPTYPE(pH264VEComp, pParamStruct, eError);

                /*get the slice mode from the Codec Creation Time Params*/
                GET_OMX_FMO_SLICEMODE(pH264VEComp, pParamStruct, eError);

            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexParamVideoIntraRefresh :
            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_INTRAREFRESHTYPE, eError);
            if(((OMX_VIDEO_PARAM_INTRAREFRESHTYPE *)pParamStruct)->nPortIndex == OMX_H264VE_OUTPUT_PORT ) {
                GET_OMX_INTRAREFRESHMODE(pH264VEComp, pParamStruct, eError);
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        /* required for Standard Video Encoder as per Spec*/
        case OMX_IndexParamVideoProfileLevelCurrent :
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_PROFILELEVELTYPE, eError);
            if(((OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pParamStruct)->nPortIndex == OMX_H264VE_OUTPUT_PORT ) {
                /*Get the Codec Profile */
                GET_OMX_AVC_PROFILE(pH264VEComp, LAVCParams, eError);
                ((OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pParamStruct)->eProfile = LAVCParams->eProfile;

                /*Get the Codec level */
                GET_OMX_AVC_LEVEL(pH264VEComp, LAVCParams, eError);

                ((OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pParamStruct)->eLevel = LAVCParams->eLevel;
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        /* required for Standard Video Encoder as per Spec*/
        case OMX_IndexParamVideoProfileLevelQuerySupported :
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_PROFILELEVELTYPE, eError);
            if(((OMX_VIDEO_PARAM_AVCTYPE *)pParamStruct)->nPortIndex == OMX_H264VE_OUTPUT_PORT ) {
                switch(((OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pParamStruct)->nProfileIndex ) {
                    case 0 :
                        ((OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pParamStruct)->eProfile = OMX_VIDEO_AVCProfileBaseline;
                        ((OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pParamStruct)->eLevel = OMX_VIDEO_AVCLevel51;
                        break;
                    case 1 :
                        ((OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pParamStruct)->eProfile=OMX_VIDEO_AVCProfileMain;
                        ((OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pParamStruct)->eLevel=OMX_VIDEO_AVCLevel51;
                        break;
                    case 2 :
                        ((OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pParamStruct)->eProfile=OMX_VIDEO_AVCProfileHigh;
                        ((OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pParamStruct)->eLevel=OMX_VIDEO_AVCLevel51;
                        break;
                    default :
                        eError =OMX_ErrorNoMore;
                }
                OMX_CHECK(OMX_ErrorNone == eError, eError);
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexParamVideoVBSMC :
            /* SetParameter can be invoked only when the comp is in loaded or on disabled port */
            OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
            nLCurState=pH264VEComp->sBase.tCurState;
            OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
            OMX_CHECK((nLCurState == OMX_StateLoaded), OMX_ErrorIncorrectStateOperation);
            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_VBSMCTYPE, eError);
            if(((OMX_VIDEO_PARAM_VBSMCTYPE *)pParamStruct)->nPortIndex == OMX_H264VE_OUTPUT_PORT ) {
                LVBSMC = ((OMX_VIDEO_PARAM_VBSMCTYPE *)pParamStruct);
                if( pH264VEComp->pVidEncStaticParams->interCodingParams.minBlockSizeP == IH264_BLOCKSIZE_8x8 ) {
                    /*4MV case*/
                    LVBSMC->b16x16=OMX_TRUE;
                    LVBSMC->b16x8=OMX_TRUE;
                    LVBSMC->b8x16=OMX_TRUE;
                    LVBSMC->b8x8=OMX_TRUE;
                    LVBSMC->b8x4=OMX_FALSE;
                    LVBSMC->b4x8=OMX_FALSE;
                    LVBSMC->b4x4=OMX_FALSE;
                } else if( pH264VEComp->pVidEncStaticParams->interCodingParams.minBlockSizeP == IH264_BLOCKSIZE_16x16 ) {
                    /*1 MV case*/
                    /*set the same value for both P & B frames prediction*/
                    LVBSMC->b16x16=OMX_TRUE;
                    LVBSMC->b16x8=OMX_FALSE;
                    LVBSMC->b8x16=OMX_FALSE;
                    LVBSMC->b8x8=OMX_FALSE;
                    LVBSMC->b8x4=OMX_FALSE;
                    LVBSMC->b4x8=OMX_FALSE;
                    LVBSMC->b4x4=OMX_FALSE;
                } else {
                    eError = OMX_ErrorNoMore;
                }
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        /* redirects the call to "OMXBase_GetParameter" which supports standard comp indexes */
        default :
            eError = OMXBase_GetParameter(hComponent, nParamIndex, pParamStruct);
            OMX_CHECK(OMX_ErrorNone == eError, eError);
            break;

        }

EXIT:
    if( bAllocateLocalAVC ) {
        OSAL_Free(LAVCParams);
    }

    return (eError);
}

static OMX_ERRORTYPE OMXH264VE_SetParameter(OMX_HANDLETYPE hComponent,
                                                OMX_INDEXTYPE nParamIndex,
                                                OMX_PTR pParamStruct)
{
    OMX_ERRORTYPE                              eError = OMX_ErrorNone;
    OMXH264VidEncComp                          *pH264VEComp = NULL;
    OMX_COMPONENTTYPE                          *pHandle = NULL;
    OMX_VIDEO_PARAM_PORTFORMATTYPE             *pVideoPortParams = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE               *pPortDef=NULL;
    OMX_PARAM_PORTDEFINITIONTYPE               *pLocalPortDef=NULL;
    OMX_VIDEO_PARAM_AVCTYPE                    *LAVCParams=NULL;
    OMX_VIDEO_PARAM_VBSMCTYPE                  *LVBSMC =  NULL;
    OMX_U32                                     nPortIndex;
    OMX_STATETYPE                               nLCurState;
    OMX_BOOL                                    bLCodecCreateFlag=OMX_FALSE;
    PARAMS_UPDATE_STATUS                        bLCallxDMSetParams=NO_PARAM_CHANGE;
    OMX_BOOL                                    bAllocateLocalAVC=OMX_FALSE;

    /* Check for the input parameters */
    OMX_CHECK(hComponent != NULL, OMX_ErrorBadParameter);
    OMX_CHECK(pParamStruct != NULL, OMX_ErrorBadParameter);

    /*initialize the component handle and component pvt handle*/
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pH264VEComp = (OMXH264VidEncComp *)pHandle->pComponentPrivate;

    /* Allocate memory for LAVCParams */
    LAVCParams = (OMX_VIDEO_PARAM_AVCTYPE *)OSAL_Malloc(sizeof(OMX_VIDEO_PARAM_AVCTYPE));
    OMX_CHECK(LAVCParams != NULL, OMX_ErrorInsufficientResources);
    bAllocateLocalAVC = OMX_TRUE;

    OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
    nLCurState = pH264VEComp->sBase.tCurState;
    OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);

    /* SetParameter can't be invoked incase the comp is in Invalid State  */
    OMX_CHECK(nLCurState != OMX_StateInvalid, OMX_ErrorIncorrectStateOperation);

    switch( nParamIndex ) {
        case OMX_IndexParamVideoInit :
            OSAL_Info("In OMX_IndexParamVideoInit");
            /* SetParameter can be invoked only when the comp is in loaded or on a disabled port */
            OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
            nLCurState = pH264VEComp->sBase.tCurState;
            OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
            OMX_CHECK((nLCurState == OMX_StateLoaded), OMX_ErrorIncorrectStateOperation);
            OMX_CHECK((((OMX_PORT_PARAM_TYPE *)pParamStruct)->nStartPortNumber == 0), OMX_ErrorUnsupportedSetting);
            OMX_CHECK((((OMX_PORT_PARAM_TYPE *)pParamStruct)->nPorts == 2), OMX_ErrorUnsupportedSetting);
            OSAL_Memcpy(pH264VEComp->sBase.pVideoPortParams, pParamStruct, sizeof(OMX_PORT_PARAM_TYPE));
            break;

        case OMX_IndexParamPortDefinition :
            OSAL_Info("In OMX_IndexParamPortDefinition");
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_PARAM_PORTDEFINITIONTYPE, eError);
            pPortDef = (OMX_PARAM_PORTDEFINITIONTYPE *)pParamStruct;
            nPortIndex = pPortDef->nPortIndex;

            /*check for valid port index */
            OMX_CHECK(nPortIndex < ((pH264VEComp->sBase.pVideoPortParams->nStartPortNumber) +
                        (pH264VEComp->sBase.pVideoPortParams->nPorts)), OMX_ErrorBadPortIndex);

            /*successful only when the comp is in loaded or on a disabled port*/
            OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
            nLCurState = pH264VEComp->sBase.tCurState;
            OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
            OMX_CHECK((nLCurState == OMX_StateLoaded) ||
                    (pH264VEComp->sBase.pPorts[nPortIndex]->sPortDef.bEnabled == OMX_FALSE),
                    OMX_ErrorIncorrectStateOperation);
            pLocalPortDef = &(pH264VEComp->sBase.pPorts[nPortIndex]->sPortDef);

            OMX_CHECK((pPortDef->nBufferCountActual >=
                        pH264VEComp->sBase.pPorts[nPortIndex]->sPortDef.nBufferCountMin), OMX_ErrorUnsupportedSetting);

            pH264VEComp->sBase.pPorts[nPortIndex]->sPortDef.nBufferCountActual = pPortDef->nBufferCountActual;
            /*if frame height/widht changes then change the buffer requirements accordingly*/
            if( nPortIndex == OMX_H264VE_INPUT_PORT ) {
                OMX_CHECK(((pPortDef->format.video.nFrameWidth & 0x0F) == 0), OMX_ErrorUnsupportedSetting); /*Width should be multiple of 16*/
                if( pH264VEComp->pVidEncStaticParams->videnc2Params.inputContentType == IVIDEO_PROGRESSIVE ) {
                    OMX_CHECK(((pPortDef->format.video.nFrameHeight & 0x01) == 0), OMX_ErrorUnsupportedSetting); /* Width should be multiple of 16 */
                } else {
                    OMX_CHECK(((pPortDef->format.video.nFrameHeight & 0x03) == 0), OMX_ErrorUnsupportedSetting); /* Width should be multiple of 16 */
                }
                OMX_CHECK(((pPortDef->format.video.nStride % 16 == 0) &&
                            (pPortDef->format.video.nStride >= pPortDef->format.video.nFrameWidth)), OMX_ErrorUnsupportedSetting);

                OMX_CHECK((pPortDef->format.video.eColorFormat ==  OMX_TI_COLOR_FormatYUV420PackedSemiPlanar) ||
                            (pPortDef->format.video.eColorFormat ==  OMX_COLOR_FormatAndroidOpaque),
                            OMX_ErrorUnsupportedSetting);
                if((pPortDef->format.video.nFrameWidth < 96) || (pPortDef->format.video.nFrameHeight < 80)) {
                    eError=OMX_ErrorUnsupportedSetting;
                    break;
                }
                SET_H264CODEC_CHROMAFORMAT(pPortDef, pH264VEComp, eError);

                OMX_CHECK((eError == OMX_ErrorNone), OMX_ErrorUnsupportedSetting);
                // Storing the CLient provided frame rate in internal port def structure for i/p port
                if( pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.xFramerate != pPortDef->format.video.xFramerate) {
                        pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.xFramerate =
                                                                            pPortDef->format.video.xFramerate;
                        pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.targetFrameRate =
                                                                            (pPortDef->format.video.xFramerate >> 16) * 1000;
                }

                if((pLocalPortDef->format.video.nFrameHeight != pPortDef->format.video.nFrameHeight) ||
                        (pLocalPortDef->format.video.nFrameWidth != pPortDef->format.video.nFrameWidth) ||
                        (pLocalPortDef->format.video.nStride != pPortDef->format.video.nStride)) {
                    pLocalPortDef->format.video.nStride =pPortDef->format.video.nStride;
                    pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.format.video.nFrameWidth =
                                                                                            pPortDef->format.video.nFrameWidth;
                    pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.format.video.nFrameHeight =
                                                                                            pPortDef->format.video.nFrameHeight;

                    /*Update the Sliceheight as well*/
                    pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.format.video.nSliceHeight =
                                                                                            pPortDef->format.video.nFrameHeight;

                    pH264VEComp->pVidEncStaticParams->videnc2Params.maxHeight = pPortDef->format.video.nFrameHeight;
                    pH264VEComp->pVidEncStaticParams->videnc2Params.maxWidth = pPortDef->format.video.nFrameWidth;
                    pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.inputWidth = pPortDef->format.video.nFrameWidth;

                    pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.captureWidth =
                                                                pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.inputWidth;

                    pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.inputHeight = pPortDef->format.video.nFrameHeight;
                    pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.nBufferSize =
                                                            (pPortDef->format.video.nFrameHeight * pPortDef->format.video.nFrameWidth * 3 )/2;

                    bLCallxDMSetParams = PARAMS_UPDATED_AT_OMX;
                    OSAL_Memcpy(&(pH264VEComp->sBase.pPorts[nPortIndex]->sPortDef.format), &(pPortDef->format),
                                sizeof(OMX_VIDEO_PORTDEFINITIONTYPE));

                    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.nBufferSize =
                                                            (pPortDef->format.video.nFrameHeight * pPortDef->format.video.nStride * 3) / 2;

                    if (pH264VEComp->bInputMetaDataBufferMode)
                        pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.nBufferSize = sizeof(OMX_MetaDataBuffer);

                    /* read only field value. update with the frame height.for now codec does not supports the sub frame processing*/
                    pLocalPortDef->format.video.nSliceHeight=pPortDef->format.video.nFrameHeight;

                    bLCodecCreateFlag = OMX_TRUE;
                    pH264VEComp->bSetParamInputIsDone = OMX_TRUE;
                }
            } else {
                /*OUTPUT Port*/
                OMX_CHECK((pPortDef->format.video.nFrameWidth ==
                            pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameWidth),
                            OMX_ErrorUnsupportedSetting);
                OMX_CHECK((pPortDef->format.video.nFrameHeight ==
                            pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameHeight),
                            OMX_ErrorUnsupportedSetting);

                if( pPortDef->format.video.xFramerate != 0 ) {
                    OSAL_ErrorTrace("Non-zero framerate rate set on o/p port. Setting frame rate is supported only on i/p port");
                    eError = OMX_ErrorUnsupportedSetting;
                    goto EXIT;
                }
                eError = OMXH264VE_CheckBitRateCap(pPortDef->format.video.nBitrate, hComponent);
                OMX_CHECK((eError == OMX_ErrorNone), OMX_ErrorUnsupportedSetting);
                OSAL_Memcpy(&(pH264VEComp->sBase.pPorts[nPortIndex]->sPortDef.format), &(pPortDef->format),
                            sizeof(OMX_VIDEO_PORTDEFINITIONTYPE));

                /*Not to modify the read only field value*/
                pLocalPortDef->format.video.nSliceHeight =
                                    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameHeight;
                pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.targetBitRate =
                                    pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.format.video.nBitrate;

                OMX_CHECK(pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.format.video.eCompressionFormat ==
                            OMX_VIDEO_CodingAVC, OMX_ErrorUnsupportedSetting);
                bLCallxDMSetParams=PARAMS_UPDATED_AT_OMX;
            }
            break;

        /* client uses this to modify the format type of an port  */
        case OMX_IndexParamVideoPortFormat :
            OSAL_Info("In OMX_IndexParamVideoPortFormat");
            /* SetParameter can be invoked only when the comp is in loaded or on a disabled port */
            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_PORTFORMATTYPE, eError);
            pVideoPortParams = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)pParamStruct;
            nPortIndex = pVideoPortParams->nPortIndex;
            OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
            nLCurState = pH264VEComp->sBase.tCurState;
            OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
            OMX_CHECK((nLCurState == OMX_StateLoaded) ||
                    (pH264VEComp->sBase.pPorts[nPortIndex]->sPortDef.bEnabled == OMX_FALSE),
                    OMX_ErrorIncorrectStateOperation);
            /* Specifying the Video port format type params  */
            if( nPortIndex == pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.nPortIndex ) {
                pLocalPortDef = &(pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef);
                if((pLocalPortDef->format.video.xFramerate != pVideoPortParams->xFramerate) ||
                        (pLocalPortDef->format.video.eColorFormat != pVideoPortParams->eColorFormat)) {
                    SET_H264CODEC_CHROMAFORMAT(pLocalPortDef, pH264VEComp, eError);

                    OMX_CHECK((eError == OMX_ErrorNone), OMX_ErrorUnsupportedSetting);
                    /*Set the Codec Params accordingly*/
                    pLocalPortDef->format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
                    pLocalPortDef->format.video.xFramerate = pVideoPortParams->xFramerate;
                    pLocalPortDef->format.video.eColorFormat = pVideoPortParams->eColorFormat;
                    pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.targetFrameRate =
                                    (pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.xFramerate >> 16) * 1000;
                    bLCodecCreateFlag = OMX_TRUE;
                }
            } else if( nPortIndex == pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.nPortIndex ) {
                pLocalPortDef = &(pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef);
                /*set the corresponding Portdef fields*/
                if((pLocalPortDef->format.video.xFramerate != pVideoPortParams->xFramerate) ||
                        (pLocalPortDef->format.video.eCompressionFormat != pVideoPortParams->eCompressionFormat)) {
                    if( pVideoPortParams->xFramerate != 0 ) {
                        OSAL_ErrorTrace("Non-zero framerate rate set on o/p port. Setting frame rate is supported only on i/p port");
                        eError = OMX_ErrorUnsupportedSetting;
                        goto EXIT;
                    }
                    pLocalPortDef->format.video.eCompressionFormat = pVideoPortParams->eCompressionFormat;
                    pLocalPortDef->format.video.xFramerate = pVideoPortParams->xFramerate;
                    pLocalPortDef->format.video.eColorFormat = OMX_COLOR_FormatUnused;
                    OMX_CHECK(pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.format.video.eCompressionFormat ==
                                OMX_VIDEO_CodingAVC, OMX_ErrorUnsupportedSetting);
                    bLCallxDMSetParams = PARAMS_UPDATED_AT_OMX;
                }
            } else {
                eError = OMX_ErrorBadPortIndex;
                break;
            }
            break;

        /* Client uses this to configure Video Bit rate type and target bit-rate */
        case OMX_IndexParamVideoBitrate :
            OSAL_Info("In OMX_IndexParamVideoBitrate");
            /* SetParameter can be invoked only when the comp is in loaded or on disabled port */
            OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
            nLCurState = pH264VEComp->sBase.tCurState;
            OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
            OMX_CHECK((nLCurState == OMX_StateLoaded), OMX_ErrorIncorrectStateOperation);

            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_BITRATETYPE, eError);
            if(((OMX_VIDEO_PARAM_BITRATETYPE *)pParamStruct)->nPortIndex == OMX_H264VE_OUTPUT_PORT ) {
                /*check for the rateControlPreset can be set only when it is IVIDEO_USER_DEFINED*/
                if( pH264VEComp->pVidEncStaticParams->videnc2Params.rateControlPreset != IVIDEO_USER_DEFINED ) {
                    OSAL_ErrorTrace("Rate control preset is not set to User defined");
                    eError = OMX_ErrorUnsupportedSetting;
                    goto EXIT;
                }
                /*Set the Codec Rate Control Algorithm: */
                SET_H264CODEC_RC_ALG(pParamStruct, pH264VEComp, eError);
                OMX_CHECK((eError == OMX_ErrorNone), OMX_ErrorUnsupportedSetting);
                /*Set the Preset to User Defined*/
                if((((OMX_VIDEO_PARAM_BITRATETYPE *)pParamStruct)->eControlRate) != OMX_Video_ControlRateVariable ) {
                    pH264VEComp->pVidEncStaticParams->rateControlParams.rateControlParamsPreset = 1; //UserDefined
                }
                eError = OMXH264VE_CheckBitRateCap(((OMX_VIDEO_PARAM_BITRATETYPE *)pParamStruct)->nTargetBitrate, hComponent);
                OMX_CHECK((eError == OMX_ErrorNone), OMX_ErrorUnsupportedSetting);
                /*Set the Codec Target Bit Rate: from the Codec Dynamic Params*/
                pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.targetBitRate =
                                                        ((OMX_VIDEO_PARAM_BITRATETYPE *)pParamStruct)->nTargetBitrate;

                /*Update the output port bit rate as well...for the get param to reflect the proper values*/
                pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.format.video.nBitrate =
                                                        pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.targetBitRate;
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        /* Client uses this to configure AVC structure Parameters*/
        case OMX_IndexParamVideoAvc :
            OSAL_Info("In OMX_IndexParamVideoAvc");
            /* SetParameter can be invoked only when the comp is in loaded or on disabled port */
            OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
            nLCurState = pH264VEComp->sBase.tCurState;
            OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
            OMX_CHECK((nLCurState == OMX_StateLoaded), OMX_ErrorIncorrectStateOperation);

            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_AVCTYPE, eError);
            if(((OMX_VIDEO_PARAM_AVCTYPE *)pParamStruct)->nPortIndex == OMX_H264VE_OUTPUT_PORT ) {
                /*Set the Codec Profile */
                SET_H264CODEC_PROFILE(pParamStruct, pH264VEComp, eError);
                OMX_CHECK((eError == OMX_ErrorNone), OMX_ErrorUnsupportedSetting);

                /*Set the Codec level */
                SET_H264CODEC_LEVEL(pParamStruct, pH264VEComp, eError);
                OMX_CHECK((eError == OMX_ErrorNone), OMX_ErrorUnsupportedSetting);

                eError = OMXH264VE_CheckBitRateCap(pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.targetBitRate, hComponent);
                OMX_CHECK((eError == OMX_ErrorNone), OMX_ErrorUnsupportedSetting);

                SET_H264CODEC_PARAMS_FROM_AVC(pParamStruct, pH264VEComp);

                /*Set the LoopFilter mode */
                SET_H264CODEC_LFMODE(pParamStruct, pH264VEComp, eError);
                OMX_CHECK((eError == OMX_ErrorNone), OMX_ErrorUnsupportedSetting);

                /*depending on the interframe interval (nBframes supported) update the buffer requirements */
                pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.nBufferCountMin =
                                        pH264VEComp->pVidEncStaticParams->videnc2Params.maxInterFrameInterval;
                if( pH264VEComp->bSetParamInputIsDone == OMX_TRUE ) {
                    if((pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.nBufferCountActual) <
                            (pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.nBufferCountMin)) {
                        eError=OMX_ErrorBadParameter;
                        pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.nBufferCountActual =
                                                            pH264VEComp->pVidEncStaticParams->videnc2Params.maxInterFrameInterval;
                        OSAL_ErrorTrace("need to set the no of buffers properly (buffactual < min requirement)");
                        goto EXIT;
                    }
                } else {
                    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.nBufferCountActual =
                                                            pH264VEComp->pVidEncStaticParams->videnc2Params.maxInterFrameInterval;
                }
                OSAL_Info("input port buff actual =%d", pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->nBufferCountActual);
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexParamVideoMotionVector :
            OSAL_Info("In OMX_IndexParamVideoMotionVector");
            /* SetParameter can be invoked only when the comp is in loaded or on disabled port */
            OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
            nLCurState = pH264VEComp->sBase.tCurState;
            OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
            OMX_CHECK((nLCurState == OMX_StateLoaded), OMX_ErrorIncorrectStateOperation);
            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_MOTIONVECTORTYPE, eError);
            if(((OMX_VIDEO_PARAM_MOTIONVECTORTYPE *)pParamStruct)->nPortIndex == OMX_H264VE_OUTPUT_PORT ) {
                /*Get the MV Accuracy from Codec Dynamic Params*/
                pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.mvAccuracy =
                                                ((OMX_VIDEO_PARAM_MOTIONVECTORTYPE *)pParamStruct)->eAccuracy;
                /*Number of MVs depend on the min Block size selected*/
                if(((OMX_VIDEO_PARAM_MOTIONVECTORTYPE *)pParamStruct)->bFourMV ) {
                    pH264VEComp->pVidEncStaticParams->interCodingParams.minBlockSizeP = IH264_BLOCKSIZE_8x8;
                    pH264VEComp->pVidEncStaticParams->interCodingParams.minBlockSizeB = IH264_BLOCKSIZE_8x8;
                } else {
                    pH264VEComp->pVidEncStaticParams->interCodingParams.minBlockSizeP = IH264_BLOCKSIZE_16x16;
                    pH264VEComp->pVidEncStaticParams->interCodingParams.minBlockSizeB = IH264_BLOCKSIZE_16x16;
                }
                /*Set the Search Range for P Frame*/
                pH264VEComp->pVidEncStaticParams->interCodingParams.searchRangeHorP =
                                                    ((OMX_VIDEO_PARAM_MOTIONVECTORTYPE *)pParamStruct)->sXSearchRange;
                pH264VEComp->pVidEncStaticParams->interCodingParams.searchRangeVerP =
                                                    ((OMX_VIDEO_PARAM_MOTIONVECTORTYPE *)pParamStruct)->sYSearchRange;
                /*Set the Search Range for B Frame*/
                pH264VEComp->pVidEncStaticParams->interCodingParams.searchRangeHorB =
                                                    ((OMX_VIDEO_PARAM_MOTIONVECTORTYPE *)pParamStruct)->sXSearchRange;
                pH264VEComp->pVidEncStaticParams->interCodingParams.searchRangeVerB =
                                                    OMX_H264VE_DEFAULT_VERSEARCH_BFRAME; /*the only supported value by codec*/

                /*Update the corresponding Dynamic params also*/
                /*Set the Search Range for P Frame*/
                pH264VEComp->pVidEncDynamicParams->interCodingParams.searchRangeHorP =
                                                    ((OMX_VIDEO_PARAM_MOTIONVECTORTYPE *)pParamStruct)->sXSearchRange;
                pH264VEComp->pVidEncDynamicParams->interCodingParams.searchRangeVerP =
                                                    ((OMX_VIDEO_PARAM_MOTIONVECTORTYPE *)pParamStruct)->sYSearchRange;
                /*Set the Search Range for B Frame*/
                pH264VEComp->pVidEncDynamicParams->interCodingParams.searchRangeHorB =
                                                    ((OMX_VIDEO_PARAM_MOTIONVECTORTYPE *)pParamStruct)->sXSearchRange;
                pH264VEComp->pVidEncDynamicParams->interCodingParams.searchRangeVerB =
                                                    OMX_H264VE_DEFAULT_VERSEARCH_BFRAME; /*the only supported value by codec*/
                pH264VEComp->pVidEncStaticParams->interCodingParams.interCodingPreset = 1; //User Defined
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexParamVideoQuantization :
            OSAL_Info("In OMX_IndexParamVideoQuantization");
            /* SetParameter can be invoked only when the comp is in loaded or on disabled port */
            OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
            nLCurState = pH264VEComp->sBase.tCurState;
            OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
            OMX_CHECK((nLCurState == OMX_StateLoaded), OMX_ErrorIncorrectStateOperation);
            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_QUANTIZATIONTYPE, eError);
            if(((OMX_VIDEO_PARAM_QUANTIZATIONTYPE *)pParamStruct)->nPortIndex == OMX_H264VE_OUTPUT_PORT ) {
                pH264VEComp->pVidEncStaticParams->rateControlParams.rateControlParamsPreset =
                                                                                IH264_RATECONTROLPARAMS_USERDEFINED;
                pH264VEComp->pVidEncStaticParams->rateControlParams.qpI = ((OMX_VIDEO_PARAM_QUANTIZATIONTYPE *)pParamStruct)->nQpI;
                pH264VEComp->pVidEncStaticParams->rateControlParams.qpP = ((OMX_VIDEO_PARAM_QUANTIZATIONTYPE *)pParamStruct)->nQpP;
                (pH264VEComp->pVidEncStaticParams->rateControlParams.qpOffsetB) =
                                                ((((OMX_VIDEO_PARAM_QUANTIZATIONTYPE *)pParamStruct)->nQpB) -
                                                (((OMX_VIDEO_PARAM_QUANTIZATIONTYPE *)pParamStruct)->nQpP));

                /*Update the corresponding Dynamic params also*/
                pH264VEComp->pVidEncDynamicParams->rateControlParams.rateControlParamsPreset =
                                                                            IH264_RATECONTROLPARAMS_USERDEFINED;
                pH264VEComp->pVidEncDynamicParams->rateControlParams.qpI = ((OMX_VIDEO_PARAM_QUANTIZATIONTYPE *)pParamStruct)->nQpI;
                pH264VEComp->pVidEncDynamicParams->rateControlParams.qpP = ((OMX_VIDEO_PARAM_QUANTIZATIONTYPE *)pParamStruct)->nQpP;
                (pH264VEComp->pVidEncDynamicParams->rateControlParams.qpOffsetB) =
                                            ((((OMX_VIDEO_PARAM_QUANTIZATIONTYPE *)pParamStruct)->nQpB) -
                                            (((OMX_VIDEO_PARAM_QUANTIZATIONTYPE *)pParamStruct)->nQpP));
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexParamVideoSliceFMO :
            OSAL_Info("In OMX_IndexParamVideoSliceFMO");
            /* SetParameter can be invoked only when the comp is in loaded or on disabled port */
            OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
            nLCurState = pH264VEComp->sBase.tCurState;
            OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
            OMX_CHECK((nLCurState == OMX_StateLoaded), OMX_ErrorIncorrectStateOperation);
            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_AVCSLICEFMO, eError);
            if(((OMX_VIDEO_PARAM_AVCSLICEFMO *)pParamStruct)->nPortIndex == OMX_H264VE_OUTPUT_PORT ) {
                pH264VEComp->pVidEncStaticParams->fmoCodingParams.numSliceGroups =
                                                    ((OMX_VIDEO_PARAM_AVCSLICEFMO *)pParamStruct)->nNumSliceGroups;
                /*Set the slicegrp type*/
                SET_H264CODEC_FMO_SLIGRPMAPTYPE(pParamStruct, pH264VEComp, eError);
                OMX_CHECK((eError == OMX_ErrorNone), OMX_ErrorUnsupportedSetting);

                /*Set the slicemode*/
                SET_H264CODEC_SLICEMODE(pParamStruct, pH264VEComp, eError);
                OMX_CHECK((eError == OMX_ErrorNone), OMX_ErrorUnsupportedSetting);

                /*Update the corresponding Dynamic params also*/
                pH264VEComp->pVidEncDynamicParams->sliceCodingParams.sliceMode =
                                            pH264VEComp->pVidEncStaticParams->sliceCodingParams.sliceMode;
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexParamVideoIntraRefresh :
            OSAL_Info("In OMX_IndexParamVideoIntraRefresh");
            /* SetParameter can be invoked only when the comp is in loaded or on disabled port */
            OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
            nLCurState = pH264VEComp->sBase.tCurState;
            OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
            OMX_CHECK((nLCurState == OMX_StateLoaded), OMX_ErrorIncorrectStateOperation);

            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_INTRAREFRESHTYPE, eError);
            if(((OMX_VIDEO_PARAM_INTRAREFRESHTYPE *)pParamStruct)->nPortIndex == OMX_H264VE_OUTPUT_PORT ) {
                //SET_H264CODEC_INTRAREFRESHMODE(pParamStruct, pH264VEComp, eError);
                OMX_CHECK((eError == OMX_ErrorNone), OMX_ErrorUnsupportedSetting);
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexParamVideoVBSMC :
            OSAL_Info("In OMX_IndexParamVideoVBSMC");
            /* SetParameter can be invoked only when the comp is in loaded or on disabled port */
            OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
            nLCurState = pH264VEComp->sBase.tCurState;
            OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
            OMX_CHECK((nLCurState == OMX_StateLoaded), OMX_ErrorIncorrectStateOperation);

            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_VBSMCTYPE, eError);
            if(((OMX_VIDEO_PARAM_VBSMCTYPE *)pParamStruct)->nPortIndex == OMX_H264VE_OUTPUT_PORT ) {
                LVBSMC = ((OMX_VIDEO_PARAM_VBSMCTYPE *)pParamStruct);
                if((LVBSMC->b16x16 == OMX_TRUE) && (LVBSMC->b16x8 == OMX_TRUE) && (LVBSMC->b8x16 == OMX_TRUE) &&
                        (LVBSMC->b8x8 == OMX_TRUE) && (LVBSMC->b8x4 == OMX_FALSE) && (LVBSMC->b4x8 == OMX_FALSE) &&
                        (LVBSMC->b4x4 == OMX_FALSE)) {
                    /*4MV case*/
                    /*set the same value for both P & B frames prediction*/
                    pH264VEComp->pVidEncStaticParams->interCodingParams.minBlockSizeP = IH264_BLOCKSIZE_8x8;
                    pH264VEComp->pVidEncStaticParams->interCodingParams.minBlockSizeB = IH264_BLOCKSIZE_8x8;
                } else if((LVBSMC->b16x16 == OMX_TRUE) && (LVBSMC->b16x8 == OMX_FALSE) && (LVBSMC->b8x16 == OMX_FALSE) &&
                        (LVBSMC->b8x8 == OMX_FALSE) && (LVBSMC->b8x4 == OMX_FALSE) && (LVBSMC->b4x8 == OMX_FALSE) &&
                        (LVBSMC->b4x4 == OMX_FALSE)) {
                    /*1 MV case*/
                    /*set the same value for both P & B frames prediction*/
                    pH264VEComp->pVidEncStaticParams->interCodingParams.minBlockSizeP = IH264_BLOCKSIZE_16x16;
                    pH264VEComp->pVidEncStaticParams->interCodingParams.minBlockSizeB = IH264_BLOCKSIZE_16x16;
                } else {
                    eError = OMX_ErrorUnsupportedSetting;
                }
                pH264VEComp->pVidEncStaticParams->interCodingParams.interCodingPreset = 1; //User Defined
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexParamVideoProfileLevelCurrent :
            OSAL_Info("In OMX_IndexParamVideoProfileLevelCurrent");
            /* SetParameter can be invoked only when the comp is in loaded or on disabled port */
            OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
            nLCurState = pH264VEComp->sBase.tCurState;
            OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
            OMX_CHECK((nLCurState == OMX_StateLoaded), OMX_ErrorIncorrectStateOperation);

            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_PROFILELEVELTYPE, eError);
            if(((OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pParamStruct)->nPortIndex == OMX_H264VE_OUTPUT_PORT ) {
                /*Set the Codec Profile */
                LAVCParams->eProfile = (OMX_VIDEO_AVCPROFILETYPE)((OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pParamStruct)->eProfile;
                SET_H264CODEC_PROFILE(LAVCParams, pH264VEComp, eError);
                OMX_CHECK((eError == OMX_ErrorNone), OMX_ErrorUnsupportedSetting);

                /*Set the Codec level */
                LAVCParams->eLevel = (OMX_VIDEO_AVCLEVELTYPE)((OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pParamStruct)->eLevel;
                SET_H264CODEC_LEVEL(LAVCParams, pH264VEComp, eError);
                OMX_CHECK((eError == OMX_ErrorNone), OMX_ErrorUnsupportedSetting);
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexParamStandardComponentRole :
            /*Nothing to do Right Now As the Component supports only one Role: AVC*/
            /*if it suppots multiple roles then need to set the params (Component Pvt, Codec Create & Dynamic acordingly*/
            break;
        case (OMX_INDEXTYPE)OMX_TI_IndexEncoderReceiveMetadataBuffers:
            /* SetParameter can be invoked only when the comp is in loaded or on disabled port */
            OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
            nLCurState = pH264VEComp->sBase.tCurState;
            OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
            OMX_CHECK((nLCurState == OMX_StateLoaded), OMX_ErrorIncorrectStateOperation);

            hw_module_t const* module = NULL;
            eError = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
            if (eError == 0) {
                pH264VEComp->hCC = (void *) ((IMG_gralloc_module_public_t const *)module);
                pH264VEComp->bInputMetaDataBufferMode = OMX_TRUE;
            }
            break;

        /* redirects the call to "OMXBase_SetParameter" which supports standard comp indexes  */
        default :
            OSAL_Info("In Default: Call to BASE Set Parameter");
            eError = OMXBase_SetParameter(hComponent, nParamIndex, pParamStruct);
            OMX_CHECK(OMX_ErrorNone == eError, eError);
            break;
        }

        if( bLCodecCreateFlag == OMX_TRUE ) {
            OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
            pH264VEComp->bCodecCreateSettingsChange=OMX_TRUE;
            OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
        }
        if( bLCallxDMSetParams == PARAMS_UPDATED_AT_OMX ) {
            OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
            pH264VEComp->bCallxDMSetParams=PARAMS_UPDATED_AT_OMX;
            OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
        }

EXIT:

    if( bAllocateLocalAVC ) {
        OSAL_Free(LAVCParams);
    }

    return (eError);
}


static OMX_ERRORTYPE OMXH264VE_GetConfig(OMX_HANDLETYPE hComponent,
                                             OMX_INDEXTYPE nIndex,
                                             OMX_PTR pConfigData)
{
    OMX_ERRORTYPE        eError         = OMX_ErrorNone;
    OMXH264VidEncComp    *pH264VEComp   = NULL;
    OMX_COMPONENTTYPE    *pHandle       = NULL;
    OMX_STATETYPE         nLCurState;

    /* Check the input params */
    OMX_CHECK(hComponent != NULL, OMX_ErrorBadParameter);
    OMX_CHECK(pConfigData != NULL, OMX_ErrorBadParameter);

    /*initialize the component handle and component pvt handle*/
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pH264VEComp = (OMXH264VidEncComp *)pHandle->pComponentPrivate;

    /* GetConfig can't be invoked when the comp is in Invalid state  */
    OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
    nLCurState = pH264VEComp->sBase.tCurState;
    OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
    OMX_CHECK(nLCurState != OMX_StateInvalid, OMX_ErrorIncorrectStateOperation);

    /* Take care of Supported Indexes over here   */
    switch( nIndex ) {
        /* Client uses this to retrieve the bitrate structure*/
        case  OMX_IndexConfigVideoBitrate :
            OSAL_Info("In OMX_IndexConfigVideoBitrate");
            /*required for Standard Video Encoder as per Spec*/
            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pConfigData, OMX_VIDEO_CONFIG_BITRATETYPE, eError);
            if(((OMX_VIDEO_CONFIG_BITRATETYPE *)pConfigData)->nPortIndex == OMX_H264VE_OUTPUT_PORT ) {
                ((OMX_VIDEO_CONFIG_BITRATETYPE *)pConfigData)->nEncodeBitrate =
                                                pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.targetBitRate;
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexConfigVideoFramerate :
            OSAL_Info("In OMX_IndexConfigVideoFramerate");
            /*required for Standard Video Encoder as per Spec*/
            OMX_BASE_CHK_VERSION(pConfigData, OMX_CONFIG_FRAMERATETYPE, eError);
            if(((OMX_CONFIG_FRAMERATETYPE *)pConfigData)->nPortIndex == OMX_H264VE_INPUT_PORT ) {
                ((OMX_CONFIG_FRAMERATETYPE *)pConfigData)->xEncodeFramerate =
                                    (pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.targetFrameRate << 16) / 1000; /*Q16 format*/
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        /* Client uses this to configure the intra refresh period */
        case  OMX_IndexConfigVideoAVCIntraPeriod :
            OSAL_Info("In OMX_IndexConfigVideoAVCIntraPeriod");
            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pConfigData, OMX_VIDEO_CONFIG_AVCINTRAPERIOD, eError);
            if(((OMX_VIDEO_CONFIG_AVCINTRAPERIOD *)pConfigData)->nPortIndex == OMX_H264VE_OUTPUT_PORT ) {
                ((OMX_VIDEO_CONFIG_AVCINTRAPERIOD *)pConfigData)->nIDRPeriod = pH264VEComp->pVidEncStaticParams->IDRFrameInterval;
                ((OMX_VIDEO_CONFIG_AVCINTRAPERIOD *)pConfigData)->nPFrames =
                                                        pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.intraFrameInterval;
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexConfigVideoIntraVOPRefresh :
            OSAL_Info("In OMX_IndexConfigVideoIntraVOPRefresh");
            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pConfigData, OMX_CONFIG_INTRAREFRESHVOPTYPE, eError);
            if(((OMX_CONFIG_INTRAREFRESHVOPTYPE *)pConfigData)->nPortIndex == OMX_H264VE_OUTPUT_PORT ) {
                ((OMX_CONFIG_INTRAREFRESHVOPTYPE *)pConfigData)->IntraRefreshVOP =
                            ((pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.forceFrame == IVIDEO_I_FRAME) ? OMX_TRUE : OMX_FALSE);
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexConfigVideoNalSize :
            OSAL_Info("In OMX_IndexConfigVideoNalSize");
            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pConfigData, OMX_VIDEO_CONFIG_NALSIZE, eError);
            if(((OMX_VIDEO_CONFIG_NALSIZE *)pConfigData)->nPortIndex == OMX_H264VE_OUTPUT_PORT ) {
                if( pH264VEComp->pVidEncDynamicParams->sliceCodingParams.sliceMode == IH264_SLICEMODE_BYTES ) {
                    ((OMX_VIDEO_CONFIG_NALSIZE *)pConfigData)->nNaluBytes =
                                                pH264VEComp->pVidEncDynamicParams->sliceCodingParams.sliceUnitSize;
                } else {
                    eError = OMX_ErrorUnsupportedSetting;
                }
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexConfigPriorityMgmt :
            OSAL_Info("In OMX_IndexConfigPriorityMgmt");

        default :
            OSAL_Info("In Default: Call to BASE GetConfig");
            eError = OMXBase_GetConfig(hComponent, nIndex, pConfigData);
            OMX_CHECK(OMX_ErrorNone == eError, eError);
            break;
        }

EXIT:
    return (eError);

}


static OMX_ERRORTYPE OMXH264VE_SetConfig(OMX_HANDLETYPE hComponent,
                                             OMX_INDEXTYPE nIndex,
                                             OMX_PTR pConfigData)
{
    OMX_ERRORTYPE                            eError = OMX_ErrorNone;
    OMXH264VidEncComp                        *pH264VEComp = NULL;
    OMX_COMPONENTTYPE                        *pHandle = NULL;
    OMX_STATETYPE                             nLCurState;
    OMX_BOOL                                  bLCodecCreateFlag=OMX_FALSE;
    OMX_U32                                   tStatus;
    PARAMS_UPDATE_STATUS                      bLCallxDMSetParams=NO_PARAM_CHANGE;

    /* Check the input params */
    OMX_CHECK(hComponent != NULL, OMX_ErrorBadParameter);
    OMX_CHECK(pConfigData != NULL, OMX_ErrorBadParameter);

    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pH264VEComp = (OMXH264VidEncComp *)pHandle->pComponentPrivate;

    /* SetConfig can't be invoked when the comp is in Invalid state */
    OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
    nLCurState = pH264VEComp->sBase.tCurState;
    OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
    OMX_CHECK(nLCurState != OMX_StateInvalid, OMX_ErrorIncorrectStateOperation);

    /* Take care of Supported Indices over here  */
    switch( nIndex ) {
        case  OMX_IndexConfigVideoBitrate :
            OSAL_Info("In OMX_IndexConfigVideoBitrate");
            /*required for Standard Video Encoder as per Spec*/
            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pConfigData, OMX_VIDEO_CONFIG_BITRATETYPE, eError);
            if(((OMX_VIDEO_CONFIG_BITRATETYPE *)pConfigData)->nPortIndex == OMX_H264VE_OUTPUT_PORT ) {
                eError = OMXH264VE_CheckBitRateCap(((OMX_VIDEO_CONFIG_BITRATETYPE *)pConfigData)->nEncodeBitrate, hComponent);
                OMX_CHECK((eError == OMX_ErrorNone), OMX_ErrorUnsupportedSetting);
                pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.targetBitRate =
                                                    ((OMX_VIDEO_CONFIG_BITRATETYPE *)pConfigData)->nEncodeBitrate;
                /*Update the output port bit rate as well...for the get param to reflect the proper values*/
                pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.format.video.nBitrate =
                                                    pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.targetBitRate;
                /*set the HRD biffer size appropriately*/
                if( pH264VEComp->pVidEncStaticParams->rateControlParams.rcAlgo == IH264_RATECONTROL_PRC_LOW_DELAY ) {
                    pH264VEComp->pVidEncDynamicParams->rateControlParams.HRDBufferSize =
                                                    (pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.targetBitRate) / 2;
                } else {
                    pH264VEComp->pVidEncDynamicParams->rateControlParams.HRDBufferSize =
                                                    (pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.targetBitRate);
                }
                bLCallxDMSetParams=PARAMS_UPDATED_AT_OMX;

            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexConfigVideoFramerate :
            OSAL_Info("In OMX_IndexConfigVideoFramerate");
            /*required for Standard Video Encoder as per Spec*/
            OMX_BASE_CHK_VERSION(pConfigData, OMX_CONFIG_FRAMERATETYPE, eError);
            if(((OMX_CONFIG_FRAMERATETYPE *)pConfigData)->nPortIndex == OMX_H264VE_INPUT_PORT ) {
                if( pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.xFramerate !=
                        (((OMX_CONFIG_FRAMERATETYPE *)pConfigData)->xEncodeFramerate)) {
                    pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.targetFrameRate =
                                        ((((OMX_CONFIG_FRAMERATETYPE *)pConfigData)->xEncodeFramerate) >> 16) * 1000;
                    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.xFramerate =
                                        (((OMX_CONFIG_FRAMERATETYPE *)pConfigData)->xEncodeFramerate);
                    bLCallxDMSetParams=PARAMS_UPDATED_AT_OMX;
                }
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        case  OMX_IndexConfigVideoAVCIntraPeriod :
            OSAL_Info("In OMX_IndexConfigVideoAVCIntraPeriod");
            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pConfigData, OMX_VIDEO_CONFIG_AVCINTRAPERIOD, eError);
            if(((OMX_VIDEO_CONFIG_AVCINTRAPERIOD *)pConfigData)->nPortIndex == OMX_H264VE_OUTPUT_PORT ) {
                OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
                nLCurState = pH264VEComp->sBase.tCurState;
                OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
                /*If Client want to set the IDR frame Interval */
                if(((OMX_VIDEO_CONFIG_AVCINTRAPERIOD *)pConfigData)->nIDRPeriod != 1 ) {
                    /*IDR frame Interval is other than the Component default settings -
                    it is possible only when the component is in loaded state*/
                    OMX_CHECK((nLCurState == OMX_StateLoaded), OMX_ErrorIncorrectStateOperation);
                    pH264VEComp->pVidEncStaticParams->IDRFrameInterval = ((OMX_VIDEO_CONFIG_AVCINTRAPERIOD *)pConfigData)->nIDRPeriod;
                    bLCodecCreateFlag = OMX_TRUE;
                }
                pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.intraFrameInterval =
                                                        ((OMX_VIDEO_CONFIG_AVCINTRAPERIOD *)pConfigData)->nPFrames;
                bLCallxDMSetParams = PARAMS_UPDATED_AT_OMX;
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexConfigVideoIntraVOPRefresh :
            OSAL_Info("In OMX_IndexConfigVideoIntraVOPRefresh");
            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pConfigData, OMX_CONFIG_INTRAREFRESHVOPTYPE, eError);
            if(((OMX_CONFIG_INTRAREFRESHVOPTYPE *)pConfigData)->nPortIndex == OMX_H264VE_OUTPUT_PORT ) {
                if(((OMX_CONFIG_INTRAREFRESHVOPTYPE *)pConfigData)->IntraRefreshVOP ) {
                    pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.forceFrame = IVIDEO_IDR_FRAME;
                } else {
                    pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.forceFrame = IVIDEO_NA_FRAME;
                }
                bLCallxDMSetParams = PARAMS_UPDATED_AT_OMX;
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexConfigVideoNalSize :
            OSAL_Info("In OMX_IndexConfigVideoNalSize");
            /* Check for the correct nSize & nVersion information */
            OMX_BASE_CHK_VERSION(pConfigData, OMX_VIDEO_CONFIG_NALSIZE, eError);
            if(((OMX_VIDEO_CONFIG_NALSIZE *)pConfigData)->nPortIndex == OMX_H264VE_OUTPUT_PORT ) {
                if( pH264VEComp->pVidEncDynamicParams->sliceCodingParams.sliceMode == IH264_SLICEMODE_DEFAULT ) {
                    pH264VEComp->pVidEncDynamicParams->sliceCodingParams.sliceMode = IH264_SLICEMODE_BYTES;
                    pH264VEComp->pVidEncDynamicParams->sliceCodingParams.sliceCodingPreset = IH264_SLICECODING_USERDEFINED;
                }
                if( pH264VEComp->pVidEncDynamicParams->sliceCodingParams.sliceMode == IH264_SLICEMODE_BYTES ) {
                    OMX_CHECK(pH264VEComp->pVidEncStaticParams->videnc2Params.inputContentType != IVIDEO_INTERLACED,
                                OMX_ErrorUnsupportedSetting);
                    OMX_CHECK(pH264VEComp->pVidEncStaticParams->entropyCodingMode != IH264_ENTROPYCODING_CABAC,
                                OMX_ErrorUnsupportedSetting);
                    OMX_CHECK(pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.inputWidth >= 128,
                                OMX_ErrorUnsupportedSetting);
                    OMX_CHECK(pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.interFrameInterval == 1,
                                OMX_ErrorUnsupportedSetting);
                    pH264VEComp->pVidEncDynamicParams->sliceCodingParams.sliceUnitSize =
                                                                ((OMX_VIDEO_CONFIG_NALSIZE *)pConfigData)->nNaluBytes;

                    bLCallxDMSetParams = PARAMS_UPDATED_AT_OMX;
                   if((nLCurState == OMX_StateLoaded) && (eError == OMX_ErrorNone)) {
                        if( pH264VEComp->pVidEncStaticParams->sliceCodingParams.sliceMode == IH264_SLICEMODE_DEFAULT ) {
                            pH264VEComp->pVidEncStaticParams->sliceCodingParams.sliceMode = IH264_SLICEMODE_BYTES;
                            pH264VEComp->pVidEncStaticParams->sliceCodingParams.sliceCodingPreset = IH264_SLICECODING_USERDEFINED;
                        }
                        pH264VEComp->pVidEncStaticParams->sliceCodingParams.sliceUnitSize =
                                                                pH264VEComp->pVidEncDynamicParams->sliceCodingParams.sliceUnitSize;
                    }
                } else {
                    eError = OMX_ErrorUnsupportedSetting;
                }
            } else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexConfigPriorityMgmt :
            OSAL_ErrorTrace("In OMX_IndexConfigPriorityMgmt");

            break;

        default :
            OSAL_Info("In Default: Call to BASE SetConfig");
            eError = OMXBase_SetConfig(hComponent, nIndex, pConfigData);
            OMX_CHECK(OMX_ErrorNone == eError, eError);
            break;
        }

        if( bLCodecCreateFlag == OMX_TRUE ) {
            OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
            pH264VEComp->bCodecCreateSettingsChange = OMX_TRUE;
            OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
        }

        if( bLCallxDMSetParams == PARAMS_UPDATED_AT_OMX ) {
            OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
            pH264VEComp->bCallxDMSetParams = PARAMS_UPDATED_AT_OMX;
            OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
        }

EXIT:

    return (eError);
}


static OMX_ERRORTYPE OMXH264VE_CommandNotify(OMX_HANDLETYPE hComponent,
                                                         OMX_COMMANDTYPE Cmd,
                                                         OMX_U32 nParam,
                                                         OMX_PTR pCmdData)
{
    OMX_ERRORTYPE       eError      = OMX_ErrorNone;
    OMXH264VidEncComp  *pH264VEComp = NULL;
    OMX_COMPONENTTYPE  *pHandle     = NULL;
    OMX_U32               i;
    OMX_STATETYPE         tCurState, tNewState;
    XDAS_Int32            retval = 0;

    /* Check the input parameters */
    OMX_CHECK(hComponent != NULL, OMX_ErrorBadParameter);
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pH264VEComp = (OMXH264VidEncComp *)pHandle->pComponentPrivate;


    /* Complete all the operations like Alg Instance create or
    *  allocation of any resources which are specific to the Component, Notify this
    *  Asynchronous event  completion to the Base Comp  via ReturnEventNotify call*/

    OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
    tCurState = pH264VEComp->sBase.tCurState;
    tNewState = pH264VEComp->sBase.tNewState;
    OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);

    switch( Cmd ) {
        case OMX_CommandStateSet :
            /* Incase if the comp is moving from loaded to idle */
            if((tCurState == OMX_StateLoaded) && (tNewState == OMX_StateIdle)) {
                OSAL_Info("In OMX_CommandStateSet:Loaded to Idle");
                eError = OMXH264VE_SetEncCodecReady(hComponent);
                OMX_CHECK(eError == OMX_ErrorNone, eError);
            }

            /* Incase if the comp is moving from idle to executing, process buffers if an supplier port */
            else if(((tCurState == OMX_StateIdle) && (tNewState == OMX_StateExecuting)) ||
                    ((tCurState == OMX_StateIdle) && (tNewState == OMX_StatePause))) {
                OSAL_Info("In OMX_CommandStateSet:Idle to Executing");

                pH264VEComp->pCodecInBufferArray = (OMX_BUFFERHEADERTYPE **)OSAL_Malloc(sizeof(OMX_BUFFERHEADERTYPE*) * 
                                                    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.nBufferCountActual);
                OMX_CHECK(pH264VEComp->pCodecInBufferArray != NULL, OMX_ErrorInsufficientResources);
                /*allocate the memory for the bufferhdrs*/
                for (i = 0; i < pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.nBufferCountActual; i++ ) {
                    pH264VEComp->pCodecInBufferArray[i] = NULL;
                }

                pH264VEComp->pCodecInBufferBackupArray = (OMXBase_BufHdrPvtData *)OSAL_Malloc(sizeof(OMXBase_BufHdrPvtData) *
                                                        pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.nBufferCountActual);

                OMX_CHECK(pH264VEComp->pCodecInBufferBackupArray != NULL, OMX_ErrorInsufficientResources);

                if ( ! pH264VEComp->bInputMetaDataBufferMode) {
                    for (i = 0; i < pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.nBufferCountActual; i++) {
                        MemHeader *h = &(pH264VEComp->pCodecInBufferBackupArray[i].sMemHdr[0]);
                        h->ptr = memplugin_alloc_noheader(h, pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameWidth, 
                                                        pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameHeight,
                                                        MEM_TILER8_2D, 0 ,0);

                        h = &(pH264VEComp->pCodecInBufferBackupArray[i].sMemHdr[1]);
                        h->ptr = memplugin_alloc_noheader(h, pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameWidth,
                                                        pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameHeight / 2,
                                                        MEM_TILER16_2D, 0 ,0);
                    }
                }
            }
            /* Incase If the comp is moving to Idle from executing, return all the buffers back to the IL client*/
            else if(((tCurState == OMX_StateExecuting) && (tNewState == OMX_StateIdle)) ||
                    ((tCurState == OMX_StatePause) && (tNewState == OMX_StateIdle))) {
                OSAL_Info("In OMX_CommandStateSet:Executing/Pause to Idle");

                /*Flushout all the locked buffers*/
                eError=OMXH264VE_FLUSHLockedBuffers(pHandle);
                OMX_CHECK(eError == OMX_ErrorNone, eError);
                if( pH264VEComp->bCodecCreate ) {
                    /*Codec Call: control call with command XDM_RESET*/
                    OSAL_Info("Call Codec_RESET ");
                    eError = OMXH264VE_VISACONTROL(pH264VEComp->pVidEncHandle, XDM_RESET,
                                                        (VIDENC2_DynamicParams *)(pH264VEComp->pVidEncDynamicParams),
                                                        (IVIDENC2_Status *)(pH264VEComp->pVidEncStatus), hComponent, &retval);
                    if( retval != VIDENC2_EOK ) {
                        OSAL_ErrorTrace("Got error from the Codec_RESET call");
                        OMX_TI_GET_ERROR(pH264VEComp, pH264VEComp->pVidEncStatus->videnc2Status.extendedError, eError);
                        OMX_CHECK(eError == OMX_ErrorNone, eError);
                    }
                    OMX_CHECK(eError == OMX_ErrorNone, eError);
                }

                if( pH264VEComp->pCodecInBufferArray ) {
                    OSAL_Free(pH264VEComp->pCodecInBufferArray);
                }

                if (!pH264VEComp->bInputMetaDataBufferMode) {
                    for (i = 0; i < pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.nBufferCountActual; i++) {
                        MemHeader *h = &(pH264VEComp->pCodecInBufferBackupArray[i].sMemHdr[0]);
                        memplugin_free_noheader(h);

                        h = &(pH264VEComp->pCodecInBufferBackupArray[i].sMemHdr[1]);
                        memplugin_free_noheader(h);
                    }
                } else if (pH264VEComp->pBackupBuffers) {
                    for (i = 0; i < pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.nBufferCountActual; i++) {
                        if(pH264VEComp->pBackupBuffers[i]) {
                            pH264VEComp->mAllocDev->free(pH264VEComp->mAllocDev, (buffer_handle_t)(pH264VEComp->pBackupBuffers[i]));
                            pH264VEComp->pBackupBuffers[i] = NULL;
                        }
                    }
                    OSAL_Free(pH264VEComp->pBackupBuffers);
                    pH264VEComp->pBackupBuffers = NULL;
                }

                if( pH264VEComp->pCodecInBufferBackupArray ) {
                    OSAL_Free(pH264VEComp->pCodecInBufferBackupArray);
                }

                if(pH264VEComp && pH264VEComp->mAllocDev) {
                    gralloc_close(pH264VEComp->mAllocDev);
                    pH264VEComp->mAllocDev = NULL;
                }
                OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
                pH264VEComp->nCodecConfigSize = 0;
                pH264VEComp->bAfterGenHeader  = OMX_FALSE;
                OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);

                /* Update the Generate Header Params : to continue with New stream w/o codec create */
                pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.generateHeader = XDM_GENERATE_HEADER;
                OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
                pH264VEComp->bCallxDMSetParams = PARAMS_UPDATED_AT_OMX;
                pH264VEComp->bSendCodecConfig = OMX_TRUE;
                OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
            }
            /* State transition from pause to executing state */
            else if((tCurState == OMX_StatePause) &&
                    (tNewState == OMX_StateExecuting)) {
                OSAL_Info("In OMX_CommandStateSet:Pause to Executing");
            } else if((tCurState == OMX_StateExecuting) &&
                    (tNewState == OMX_StatePause)) {
            } else if((tCurState == OMX_StateIdle) &&
                    (tNewState == OMX_StateLoaded)) {
                OSAL_Info("In OMX_CommandStateSet:Idle to Loaded");
                /* Delete the Codec Instance */
                if( pH264VEComp->bCodecCreate && pH264VEComp->pVidEncHandle) {
                    VIDENC2_delete(pH264VEComp->pVidEncHandle);
                    pH264VEComp->pVidEncHandle = NULL;
                }
                pH264VEComp->bCodecCreate=OMX_FALSE;
                OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
                pH264VEComp->bCallxDMSetParams=PARAMS_UPDATED_AT_OMX;
                pH264VEComp->bSendCodecConfig=OMX_TRUE;
                pH264VEComp->bSetParamInputIsDone = OMX_FALSE;
                OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
            } else if( tNewState == OMX_StateInvalid ) {
                OSAL_Info("In OMX_CommandStateSet:Invalid state");
                /* Delete the Codec Instance */
                if( pH264VEComp->bCodecCreate && pH264VEComp->pVidEncHandle) {
                    VIDENC2_delete(pH264VEComp->pVidEncHandle);
                    pH264VEComp->pVidEncHandle = NULL;
                }
                pH264VEComp->bCodecCreate=OMX_FALSE;
            }
            break;

        case OMX_CommandFlush :
            OSAL_Info("In OMX_CommandFlush");
            OMX_CHECK(((nParam == OMX_H264VE_OUTPUT_PORT) || (nParam == OMX_H264VE_INPUT_PORT) || (nParam == OMX_ALL)),
                        OMX_ErrorBadParameter);
            if((nParam == OMX_H264VE_INPUT_PORT) || (nParam == OMX_ALL)) {
                if( pH264VEComp->bCodecCreate ) {
                    /*Codec Call: control call with command XDM_FLUSH*/
                    OSAL_Info("Call CodecFlush ");
                    eError = OMXH264VE_VISACONTROL(pH264VEComp->pVidEncHandle, XDM_FLUSH,
                                                        (VIDENC2_DynamicParams *)(pH264VEComp->pVidEncDynamicParams),
                                                        (IVIDENC2_Status *)(pH264VEComp->pVidEncStatus), hComponent, &retval);
                    if( retval != VIDENC2_EOK ) {
                        OSAL_ErrorTrace("Got error from the CodecFlush call");
                        OMX_TI_GET_ERROR(pH264VEComp, pH264VEComp->pVidEncStatus->videnc2Status.extendedError, eError);
                        OMX_CHECK(eError == OMX_ErrorNone, eError);
                    }
                    OMX_CHECK(eError == OMX_ErrorNone, eError);
                }
                /*Flushout all the locked buffers*/
                eError = OMXH264VE_FLUSHLockedBuffers(pHandle);
                OMX_CHECK(eError == OMX_ErrorNone, eError);
                /* Reset the Codec : to continue with New stream w/o codec create */
                if( pH264VEComp->bCodecCreate ) {
                    /*Codec Call: control call with command XDM_RESET*/
                    OSAL_Info("Call Codec_RESET ");
                    eError = OMXH264VE_VISACONTROL(pH264VEComp->pVidEncHandle, XDM_RESET,
                                                        (VIDENC2_DynamicParams *)(pH264VEComp->pVidEncDynamicParams),
                                                        (IVIDENC2_Status *)(pH264VEComp->pVidEncStatus), hComponent, &retval);
                    if( retval != VIDENC2_EOK ) {
                        OSAL_ErrorTrace("Got error from the Codec_RESET call");
                        OMX_TI_GET_ERROR(pH264VEComp, pH264VEComp->pVidEncStatus->videnc2Status.extendedError, eError);
                        OMX_CHECK(eError == OMX_ErrorNone, eError);
                    }
                    OMX_CHECK(eError == OMX_ErrorNone, eError);
                }

                pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.generateHeader = XDM_GENERATE_HEADER;
                OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
                pH264VEComp->bCallxDMSetParams = PARAMS_UPDATED_AT_OMX;
                pH264VEComp->bSendCodecConfig = OMX_TRUE;
                pH264VEComp->nCodecConfigSize = 0;
                pH264VEComp->bAfterGenHeader  = OMX_FALSE;
                OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
            }
            if(nParam == OMX_H264VE_OUTPUT_PORT) {
                /*do nothing*/
            }
            break;

        case OMX_CommandPortDisable :
            OSAL_Info("In OMX_CommandPortDisable");
            OMX_CHECK(((nParam == OMX_H264VE_OUTPUT_PORT) || (nParam == OMX_H264VE_INPUT_PORT) || (nParam == OMX_ALL)),
                        OMX_ErrorBadParameter);
            if((nParam == OMX_H264VE_INPUT_PORT) || (nParam == OMX_ALL)) {
                if( pH264VEComp->bCodecCreate ) {
                    /*control call with command XDM_FLUSH*/
                    OSAL_Info("Call CodecFlush ");
                    eError = OMXH264VE_VISACONTROL(pH264VEComp->pVidEncHandle, XDM_FLUSH,
                                                        (VIDENC2_DynamicParams *)(pH264VEComp->pVidEncDynamicParams),
                                                        (IVIDENC2_Status *)(pH264VEComp->pVidEncStatus), hComponent, &retval);
                    if( retval != VIDENC2_EOK ) {
                        OSAL_ErrorTrace("Got error from the CodecFlush call");
                        OMX_TI_GET_ERROR(pH264VEComp, pH264VEComp->pVidEncStatus->videnc2Status.extendedError, eError);
                        OMX_CHECK(eError == OMX_ErrorNone, eError);
                    }
                    OMX_CHECK(eError == OMX_ErrorNone, eError);
                }
                /*Flushout all the locked buffers*/
                eError= OMXH264VE_FLUSHLockedBuffers(pHandle);
                OMX_CHECK(eError == OMX_ErrorNone, eError);
                /* Reset the Codec : to continue with New stream w/o codec create */
                if( pH264VEComp->bCodecCreate ) {
                    /*Codec Call: control call with command XDM_RESET*/
                    OSAL_Info("Call Codec_RESET ");
                    eError = OMXH264VE_VISACONTROL(pH264VEComp->pVidEncHandle, XDM_RESET,
                                                        (VIDENC2_DynamicParams *)(pH264VEComp->pVidEncDynamicParams),
                                                        (IVIDENC2_Status *)(pH264VEComp->pVidEncStatus), hComponent, &retval);
                    if( retval != VIDENC2_EOK ) {
                        OSAL_ErrorTrace("Got error from the Codec_RESET call");
                        OMX_TI_GET_ERROR(pH264VEComp, pH264VEComp->pVidEncStatus->videnc2Status.extendedError, eError);
                        OMX_CHECK(eError == OMX_ErrorNone, eError);
                    }
                    OMX_CHECK(eError == OMX_ErrorNone, eError);
                }
                pH264VEComp->bInputPortDisable = OMX_TRUE;
                pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.generateHeader = XDM_GENERATE_HEADER;
                OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
                pH264VEComp->bCallxDMSetParams = PARAMS_UPDATED_AT_OMX;
                pH264VEComp->bSendCodecConfig = OMX_TRUE;
                pH264VEComp->bSetParamInputIsDone = OMX_FALSE;
                pH264VEComp->nCodecConfigSize = 0;
                pH264VEComp->bAfterGenHeader  = OMX_FALSE;
                OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
            }
            if( nParam == OMX_H264VE_OUTPUT_PORT ) {
                /*do nothing*/
            }
            break;

        case OMX_CommandPortEnable :
            OSAL_Info("In OMX_CommandPortEnable");
            /*base is taking care of allocating all the resources*/
            OMX_CHECK(((nParam == OMX_H264VE_OUTPUT_PORT) || (nParam == OMX_H264VE_INPUT_PORT) || (nParam == OMX_ALL)),
                        OMX_ErrorBadParameter);
            if((nParam == OMX_H264VE_INPUT_PORT) || (nParam == OMX_ALL)) {
                if((pH264VEComp->bCodecCreate) & (pH264VEComp->bInputPortDisable) & (pH264VEComp->bCodecCreateSettingsChange)) {
                    /* Delete the old Codec Instance */
                    if( pH264VEComp->pVidEncHandle ) {
                        VIDENC2_delete(pH264VEComp->pVidEncHandle);
                        pH264VEComp->pVidEncHandle = NULL;
                    }

                    /* Create a New Codec Instance */
                    eError = OMXH264VE_SetEncCodecReady(hComponent);
                    OMX_CHECK(eError == OMX_ErrorNone, eError);
                } /*end if codec create*/
                /*Reset the port disable flag */
                pH264VEComp->bInputPortDisable = OMX_FALSE;
            } /*end if(i/p or ALL )*/
            if( nParam == OMX_H264VE_OUTPUT_PORT ) {
                /*do nothing*/
            }
            break;

        default :
            OSAL_Info("In Default");
            eError = OMX_ErrorBadParameter;
            OMX_CHECK(eError == OMX_ErrorNone, eError);
            break;
        }

        /* Note: Notify this completion to the Base comp via ReturnEventNotify call */
        OSAL_Info("Notify Base via ReturnEventNotify ");
        eError = pH264VEComp->sBase.fpReturnEventNotify(hComponent, OMX_EventCmdComplete, Cmd, nParam, NULL);
        OMX_CHECK(eError == OMX_ErrorNone, eError);

EXIT:

    return (eError);
}


static OMX_ERRORTYPE OMXH264VE_DataNotify(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE                       eError = OMX_ErrorNone;
    OMXH264VidEncComp                 *pH264VEComp = NULL;
    OMX_COMPONENTTYPE                  *pComp = NULL;
    OMX_BUFFERHEADERTYPE               *pOutBufHeader = NULL;
    OMX_U32                             nInMsgCount = 0, nOutMsgCount = 0, i, j;
    XDAS_Int32                          retval = 0;
    OMX_STATETYPE                       tCurState;
    PARAMS_UPDATE_STATUS                bLCallxDMSetParams;
    OMX_BOOL                            bLEOS=OMX_FALSE;
    OMX_BOOL                            bLCodecFlush=OMX_FALSE;
    OMX_S32     InBufferHdrIndex = -1;
    OMX_U32     LCodecLockedBufferCount = 0;
    OMX_BOOL    bLCallCodecProcess = OMX_FALSE;
    OMXBase_CodecConfigBuf                   AttrParams;
    OMX_BOOL                                 bLSendCodecConfig;
    void *srcPtr = NULL, *dstPtr = NULL;
    OMX_U32 step, stride;

    /* Check the input parameters */
    OMX_CHECK(hComponent != NULL, OMX_ErrorBadParameter);
    pComp = (OMX_COMPONENTTYPE *)hComponent;
    pH264VEComp = (OMXH264VidEncComp *)pComp->pComponentPrivate;

    /* Strat buffer processing only when the comp is in Executing state and the Port are Enabled*/
    OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
    tCurState = pH264VEComp->sBase.tCurState;
    OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
    if((tCurState == OMX_StateExecuting) && (pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.bEnabled)
            && (pH264VEComp->sBase.pPorts[OMX_H264VE_OUTPUT_PORT]->sPortDef.bEnabled)) {

        eError = pH264VEComp->sBase.pPvtData->fpDioGetCount(hComponent, OMX_H264VE_INPUT_PORT, (OMX_PTR)&nInMsgCount);
        OMX_CHECK(((eError == OMX_ErrorNone) || (eError == OMX_TI_WarningEosReceived)), OMX_ErrorInsufficientResources);

        eError = pH264VEComp->sBase.pPvtData->fpDioGetCount(hComponent, OMX_H264VE_OUTPUT_PORT, (OMX_PTR)&nOutMsgCount);
        OMX_CHECK((eError == OMX_ErrorNone), OMX_ErrorInsufficientResources);

        OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
        bLSendCodecConfig = pH264VEComp->bSendCodecConfig;
        OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);

        if( bLSendCodecConfig ) {
            if((nOutMsgCount > 0) && (nInMsgCount > 0)) {
                OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
                bLCallxDMSetParams=pH264VEComp->bCallxDMSetParams;
                OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
                if( bLCallxDMSetParams == PARAMS_UPDATED_AT_OMX ) {
                    eError = OMXH264VE_SetDynamicParamsToCodec(hComponent);
                    OMX_CHECK(eError == OMX_ErrorNone, eError);
                }

                pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.generateHeader = XDM_GENERATE_HEADER;
                /* Update the OutBuf details before the Codec Process call */
                pH264VEComp->pVedEncOutBufs->descs[0].memType = XDM_MEMTYPE_RAW;
                pH264VEComp->pVedEncOutBufs->descs[0].buf = (XDAS_Int8 *)(pH264VEComp->sCodecConfigData.sBuffer);
                pH264VEComp->pVedEncOutBufs->descs[0].bufSize.bytes = SPS_PPS_HEADER_DATA_SIZE;

                /* Update the InBuf details before the Codec Process call */
                for( i = 0; i < pH264VEComp->pVedEncInBufs->numPlanes; i++ ) {
                    pH264VEComp->pVedEncInBufs->planeDesc[i].buf = (XDAS_Int8 *)(pH264VEComp->pTempBuffer[i]);
                    pH264VEComp->pVedEncInBufs->planeDesc[i].memType = XDM_MEMTYPE_RAW;
                    pH264VEComp->pVedEncInBufs->planeDesc[i].bufSize.bytes =
                                            (pH264VEComp->pVidEncStatus->videnc2Status.bufInfo.minInBufSize[i].bytes);
                }

                /* Update the InArgs details before the Codec Process call */
                pH264VEComp->pVidEncInArgs->videnc2InArgs.size = sizeof(IVIDENC2_InArgs);
                pH264VEComp->pVidEncInArgs->videnc2InArgs.control=IVIDENC2_CTRL_NONE;
                pH264VEComp->pVidEncInArgs->videnc2InArgs.inputID = 1000; /*to overcome the limitation inside the Codec- codec checks for NULL*/

                /* Update the OutArgs details before the Codec Process call */
                pH264VEComp->pVidEncOutArgs->videnc2OutArgs.size = sizeof(IVIDENC2_OutArgs);

                /* Call the Codec Process call */
                eError = OMXH264VE_VISAPROCESS_AND_UPDATEPARAMS(hComponent, &retval);
                OMX_CHECK(eError == OMX_ErrorNone, eError);

                ALOGE("BytesGenerated=%d", pH264VEComp->pVidEncOutArgs->videnc2OutArgs.bytesGenerated);
                ALOGE("freed ID=%d", pH264VEComp->pVidEncOutArgs->videnc2OutArgs.freeBufID[0]);

                /* Send the Condec Config Data to the Client */
                AttrParams.sBuffer = pH264VEComp->sCodecConfigData.sBuffer;
                AttrParams.sBuffer->size = pH264VEComp->pVidEncOutArgs->videnc2OutArgs.bytesGenerated;
                pH264VEComp->sBase.pPvtData->fpDioControl(pComp, OMX_H264VE_OUTPUT_PORT, OMX_DIO_CtrlCmd_SetCtrlAttribute, &AttrParams);
                pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.generateHeader = XDM_ENCODE_AU;

                OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
                pH264VEComp->bSendCodecConfig =OMX_FALSE;
                pH264VEComp->nCodecConfigSize = pH264VEComp->pVidEncOutArgs->videnc2OutArgs.bytesGenerated;
                pH264VEComp->bAfterGenHeader  = OMX_TRUE;
                OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);

            } else {
                goto EXIT;
            }
        }

        /* check for both input and output */
        eError=pH264VEComp->sBase.pPvtData->fpDioGetCount(hComponent, OMX_H264VE_INPUT_PORT, (OMX_PTR)&nInMsgCount);
        OMX_CHECK(((eError == OMX_ErrorNone) || (eError == OMX_TI_WarningEosReceived)), OMX_ErrorInsufficientResources);

        eError=pH264VEComp->sBase.pPvtData->fpDioGetCount(hComponent, OMX_H264VE_OUTPUT_PORT, (OMX_PTR)&nOutMsgCount);
        OMX_CHECK((eError == OMX_ErrorNone), OMX_ErrorInsufficientResources);

        /* if both are ready-> process data */
        while(((nInMsgCount > 0) && (nOutMsgCount > 0)) || ((pH264VEComp->bAfterEOSReception) && (nOutMsgCount > 0))) {
            /*dequeue the output buffer*/
            eError = pH264VEComp->sBase.pPvtData->fpDioDequeue(hComponent, OMX_H264VE_OUTPUT_PORT, (OMX_PTR*)(&pOutBufHeader));
            OMX_CHECK((eError == OMX_ErrorNone), OMX_ErrorInsufficientResources);
            ((OMXBase_BufHdrPvtData *)(pOutBufHeader->pPlatformPrivate))->bufSt = OWNED_BY_CODEC;

            /*branch the control flow based on the Before EOS /After EOS processing*/
            if( !pH264VEComp->bAfterEOSReception ) {
                OSAL_Info("Before EOS reception Case");
                /*get the free bufhdr from the inputbufhdrarray*/
                eError = OMXH264VE_GetNextFreeBufHdr(pComp, &InBufferHdrIndex, OMX_H264VE_INPUT_PORT);
                OMX_CHECK(((InBufferHdrIndex != -1) && (eError == OMX_ErrorNone)), OMX_ErrorInsufficientResources);

                /*dequeue the input buffer*/
                eError = pH264VEComp->sBase.pPvtData->fpDioDequeue(hComponent, OMX_H264VE_INPUT_PORT,
                                                            (OMX_PTR*)&(pH264VEComp->pCodecInBufferArray[InBufferHdrIndex]));
                OMX_CHECK((eError == OMX_ErrorNone), OMX_ErrorInsufficientResources);
                ((OMXBase_BufHdrPvtData *)(pH264VEComp->pCodecInBufferArray[InBufferHdrIndex]->pPlatformPrivate))->bufSt = OWNED_BY_CODEC;

                if((pH264VEComp->pCodecInBufferArray[InBufferHdrIndex])->nFlags & OMX_BUFFERFLAG_EOS ) {
                    bLEOS = OMX_TRUE;
                    pH264VEComp->bPropagateEOSToOutputBuffer = OMX_TRUE;
                    bLCodecFlush = OMX_TRUE;
                }

                if(((pH264VEComp->pCodecInBufferArray[InBufferHdrIndex])->nFilledLen) == 0 ) {
                    /*update the buffer status to free & return the buffer to the client*/
                    ((OMXBase_BufHdrPvtData *)(pH264VEComp->pCodecInBufferArray[InBufferHdrIndex]->pPlatformPrivate))->bufSt = OWNED_BY_US;

                    pH264VEComp->sBase.pPvtData->fpDioSend(hComponent, OMX_H264VE_INPUT_PORT,
                                                        (pH264VEComp->pCodecInBufferArray[InBufferHdrIndex]));

                    ((OMXBase_BufHdrPvtData *)(pOutBufHeader->pPlatformPrivate))->bufSt = OWNED_BY_US;

                    pH264VEComp->sBase.pPvtData->fpDioCancel(hComponent, OMX_H264VE_OUTPUT_PORT, pOutBufHeader);
                    if( bLEOS ) {
                        bLCallCodecProcess = OMX_FALSE;
                    } else {
                        OSAL_ErrorTrace("++++++++Input Buffer Sent with no DATA & no EOS flag++++++++++++");
                        goto CHECKCOUNT;
                    }
                } else {
                    /* Update the Input buffer details before the Codec Process call */
                    for( i = 0; i < pH264VEComp->pVedEncInBufs->numPlanes; i++ ) {
                        if( i == 0 ) {
#ifdef ENCODE_RAM_BUFFERS
                            ((OMXBase_BufHdrPvtData *)(pH264VEComp->pCodecInBufferArray[InBufferHdrIndex]->pPlatformPrivate))->sMemHdr[0].offset = (pH264VEComp->pCodecInBufferArray[InBufferHdrIndex]->nOffset);

                            pH264VEComp->pVedEncInBufs->planeDesc[i].buf = (XDAS_Int8 *)&(((OMXBase_BufHdrPvtData *)(pH264VEComp->pCodecInBufferArray[InBufferHdrIndex]->pPlatformPrivate))->sMemHdr[0]);

                            pH264VEComp->pVedEncInBufs->planeDesc[i].bufSize.bytes = (pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameWidth) * (pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameHeight);

                            pH264VEComp->pVedEncInBufs->planeDesc[i].memType = XDM_MEMTYPE_RAW;

                            pH264VEComp->pVedEncInBufs->imagePitch[0] = pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameWidth;
#else
                            if (pH264VEComp->bInputMetaDataBufferMode) {
                                OMX_U32 *pTempBuffer;
                                OMX_U32 nMetadataBufferType;
                                IMG_native_handle_t* pGrallocHandle=NULL;
                                pTempBuffer = (OMX_U32 *) (pH264VEComp->pCodecInBufferArray[InBufferHdrIndex]->pBuffer);
                                nMetadataBufferType = *pTempBuffer;
                                if(nMetadataBufferType == kMetadataBufferTypeGrallocSource){
                                    buffer_handle_t  tBufHandle;
                                    pTempBuffer++;
                                    tBufHandle =  *((buffer_handle_t *)pTempBuffer);
                                    pGrallocHandle = (IMG_native_handle_t*) tBufHandle;
                                    if (pGrallocHandle->iFormat != HAL_PIXEL_FORMAT_TI_NV12) {
                                        if (pH264VEComp->pBackupBuffers == NULL) {
                                            /* Open gralloc allocator and allocate the backup buffers */
                                            gralloc_open(pH264VEComp->hCC, &(pH264VEComp->mAllocDev));
                                            OMX_CHECK(pH264VEComp->mAllocDev != NULL, OMX_ErrorInsufficientResources);

                                            pH264VEComp->pBackupBuffers = (IMG_native_handle_t **)OSAL_Malloc(sizeof(IMG_native_handle_t*) *
                                                                    pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.nBufferCountActual);
                                            OMX_CHECK(pH264VEComp->pBackupBuffers != NULL, OMX_ErrorInsufficientResources);

                                            for (j = 0; j < pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.nBufferCountActual; j++ ) {
                                                pH264VEComp->pBackupBuffers[j] = NULL;
                                                int err = pH264VEComp->mAllocDev->alloc(pH264VEComp->mAllocDev,
                                                            (int) pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameWidth,
                                                            (int) pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameHeight,
                                                            (int) HAL_PIXEL_FORMAT_TI_NV12, (int) GRALLOC_USAGE_HW_RENDER,
                                                            (const struct native_handle_t * *)(&(pH264VEComp->pBackupBuffers[j])), (int *) &stride);
                                                OMX_CHECK(err == 0, err);

                                                //Get the DMA BUFF_FDs from the gralloc pointers
                                                pH264VEComp->pCodecInBufferBackupArray[j].sMemHdr[0].dma_buf_fd = (OMX_U32)((pH264VEComp->pBackupBuffers[j])->fd[0]);
                                                pH264VEComp->pCodecInBufferBackupArray[j].sMemHdr[1].dma_buf_fd = (OMX_U32)((pH264VEComp->pBackupBuffers[j])->fd[1]);
                                            }
                                        }

                                        /* Invoke color conversion routine here */
                                        COLORCONVERT_PlatformOpaqueToNV12(pH264VEComp->hCC, (void **) &pGrallocHandle,
                                                                (void **) &pH264VEComp->pBackupBuffers[InBufferHdrIndex],
                                                                pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameWidth,
                                                                pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameHeight,
                                                                4096, COLORCONVERT_BUFTYPE_GRALLOCOPAQUE,
                                                                COLORCONVERT_BUFTYPE_GRALLOCOPAQUE );
                                    } else {
                                        pH264VEComp->pCodecInBufferBackupArray[i].sMemHdr[0].dma_buf_fd = (OMX_U32)(pGrallocHandle->fd[0]);
                                        pH264VEComp->pCodecInBufferBackupArray[i].sMemHdr[1].dma_buf_fd = (OMX_U32)(pGrallocHandle->fd[1]);
                                    }
                                }
                            } else {
                                srcPtr = ((OMXBase_BufHdrPvtData *)(pH264VEComp->pCodecInBufferArray[InBufferHdrIndex]->pPlatformPrivate))->sMemHdr[0].ptr;
                                dstPtr = pH264VEComp->pCodecInBufferBackupArray[InBufferHdrIndex].sMemHdr[0].ptr;
                                step = 4096;
                                for (j = 0; j < pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameHeight;
                                        j++) {
                                    memcpy(dstPtr + j * step,
                                        srcPtr + j * pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameWidth,
                                        pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameWidth);
                                }
                            }

                            pH264VEComp->pVedEncInBufs->planeDesc[i].memType = XDM_MEMTYPE_TILED8;

                            pH264VEComp->pCodecInBufferBackupArray[InBufferHdrIndex].sMemHdr[0].offset =
                                                    (pH264VEComp->pCodecInBufferArray[InBufferHdrIndex]->nOffset);

                            pH264VEComp->pVedEncInBufs->planeDesc[i].buf =
                                                    (XDAS_Int8 *)&(pH264VEComp->pCodecInBufferBackupArray[InBufferHdrIndex].sMemHdr[0]);

                            pH264VEComp->pVedEncInBufs->planeDesc[i].bufSize.tileMem.width =
                                                    (pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameWidth);

                            pH264VEComp->pVedEncInBufs->planeDesc[i].bufSize.tileMem.height =
                                                    (pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameHeight);

                            pH264VEComp->pVedEncInBufs->imagePitch[0] = 4096;
#endif
                        } else if( i == 1 ) {
#ifdef ENCODE_RAM_BUFFERS
                            memcpy(&((OMXBase_BufHdrPvtData *)(pH264VEComp->pCodecInBufferArray[InBufferHdrIndex]->pPlatformPrivate))->sMemHdr[1], &((OMXBase_BufHdrPvtData *)(pH264VEComp->pCodecInBufferArray[InBufferHdrIndex]->pPlatformPrivate))->sMemHdr[0], sizeof(MemHeader));

                            ((OMXBase_BufHdrPvtData *)(pH264VEComp->pCodecInBufferArray[InBufferHdrIndex]->pPlatformPrivate))->sMemHdr[1].offset = ((pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nStride) * (pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameHeight));

                            pH264VEComp->pVedEncInBufs->planeDesc[i].buf = (XDAS_Int8 *)&(((OMXBase_BufHdrPvtData *)(pH264VEComp->pCodecInBufferArray[InBufferHdrIndex]->pPlatformPrivate))->sMemHdr[1]);

                            pH264VEComp->pVedEncInBufs->planeDesc[i].bufSize.bytes = (pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameWidth) * (pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameHeight) / 2;

                            pH264VEComp->pVedEncInBufs->planeDesc[i].memType = XDM_MEMTYPE_RAW;

                            pH264VEComp->pVedEncInBufs->imagePitch[1] = pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameWidth;

#else
                            if (pH264VEComp->bInputMetaDataBufferMode) {
                            //Nothing to be done; color conversion and fd xlation is done during the plane0 processing
                            } else {
                                srcPtr =((OMXBase_BufHdrPvtData *)(pH264VEComp->pCodecInBufferArray[InBufferHdrIndex]->pPlatformPrivate))->sMemHdr[0].ptr;
                                dstPtr = pH264VEComp->pCodecInBufferBackupArray[InBufferHdrIndex].sMemHdr[1].ptr;
                                for (j = 0; j < pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameHeight / 2; j++) {
                                    memcpy(dstPtr + j * step,
                                            srcPtr + (j + pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameHeight) * pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameWidth,
                                            pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameWidth);
                                }
                            }

                            pH264VEComp->pVedEncInBufs->planeDesc[i].memType = XDM_MEMTYPE_TILED16;

                            pH264VEComp->pCodecInBufferBackupArray[InBufferHdrIndex].sMemHdr[1].offset = 0;

                            pH264VEComp->pVedEncInBufs->planeDesc[i].buf =
                                                    (XDAS_Int8 *)&(pH264VEComp->pCodecInBufferBackupArray[InBufferHdrIndex].sMemHdr[1]);

                            pH264VEComp->pVedEncInBufs->planeDesc[i].bufSize.tileMem.width =
                                                    (pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameWidth);

                            pH264VEComp->pVedEncInBufs->planeDesc[i].bufSize.tileMem.height =
                                                    (pH264VEComp->sBase.pPorts[OMX_H264VE_INPUT_PORT]->sPortDef.format.video.nFrameHeight/ 2);

                            pH264VEComp->pVedEncInBufs->imagePitch[1] = 4096;
#endif
                        } else {
                            eError = OMX_ErrorUnsupportedSetting;
                            OSAL_ErrorTrace("only NV12 is supproted currently; wrong param from Codec");
                            OMX_CHECK(eError == OMX_ErrorNone, eError);
                        }
                    }
                    bLCallCodecProcess=OMX_TRUE;
                }
            } else {
                OSAL_Info("After EOS reception Case");
                eError = OMXH264VE_GetNumCodecLockedBuffers(pComp, &LCodecLockedBufferCount);
                OMX_CHECK(eError == OMX_ErrorNone, eError);
                if( LCodecLockedBufferCount > 0 ) {
                    /*After EOS reception No need to provide Input for the Process, hence passing tempbuffers*/
                    for( i = 0; i < pH264VEComp->pVedEncInBufs->numPlanes; i++ ) {
                        pH264VEComp->pVedEncInBufs->planeDesc[i].buf = (XDAS_Int8 *)(pH264VEComp->pTempBuffer[i]);
                        pH264VEComp->pVedEncInBufs->planeDesc[i].memType = XDM_MEMTYPE_RAW;
                        pH264VEComp->pVedEncInBufs->planeDesc[i].bufSize.bytes =
                                                (pH264VEComp->pVidEncStatus->videnc2Status.bufInfo.minInBufSize[i].bytes);
                    }
                    bLCallCodecProcess=OMX_TRUE;
                } else {
                    /*Update the OutBufHeader*/
                    pOutBufHeader->nOffset = 0;
                    pOutBufHeader->nFilledLen = 0;
                    pOutBufHeader->nFlags |= OMX_BUFFERFLAG_EOS;
                    pH264VEComp->sBase.pPvtData->fpDioSend(hComponent, OMX_H264VE_OUTPUT_PORT, pOutBufHeader);
                    pH264VEComp->bPropagateEOSToOutputBuffer = OMX_FALSE;
                    /*notify the EOSEvent to the client*/
                    pH264VEComp->bNotifyEOSEventToClient = OMX_TRUE;
                    bLCallCodecProcess = OMX_FALSE;
                }
            }
            if( bLCallCodecProcess ) {
                if( !pH264VEComp->bAfterEOSReception ) {
                    OSAL_Info("update the Dynamic params before Process");
                    /* Call to xDM Set Params depending on the pH264VEComp->bCallxDMSetParams flag status before the process call */
                    OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
                    bLCallxDMSetParams = pH264VEComp->bCallxDMSetParams;
                    OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
                    if( bLCallxDMSetParams == PARAMS_UPDATED_AT_OMX ) {
                        eError = OMXH264VE_SetDynamicParamsToCodec(hComponent);
                        OMX_CHECK(eError == OMX_ErrorNone, eError);
                    }
                }
                /* Update the Output buffer details before the Codec Process call */
                OSAL_Info("Update the Output buffer details before the Codec Process call");
                /*Note: this implementation assumes
                a) output buffer is always 1D
                b) if in case of multiple output buffers that need to given to codec for each process call
                they are allocated in contiguous memory
                */
                if( pH264VEComp->pVedEncOutBufs->numBufs != 1 ) {
                    eError = OMX_ErrorUnsupportedSetting;
                    OSAL_ErrorTrace("Encoder Output Buffer is assigned as 2D Buffer");
                    goto EXIT;
                }

                pH264VEComp->pVedEncOutBufs->descs[0].buf =
                                        (XDAS_Int8 *)&(((OMXBase_BufHdrPvtData*)pOutBufHeader->pPlatformPrivate)->sMemHdr[0]);

                pH264VEComp->pVedEncOutBufs->descs[0].bufSize.bytes = pOutBufHeader->nAllocLen;

                /* Update the InArgs details before the Codec Process call */
                OSAL_Info("Update the InArgs before the Codec Process call");
                pH264VEComp->pVidEncInArgs->videnc2InArgs.size = sizeof(IH264ENC_InArgs);
                pH264VEComp->pVidEncInArgs->videnc2InArgs.control = IVIDENC2_CTRL_NONE;

                if( !pH264VEComp->bAfterEOSReception ) {
                    pH264VEComp->pVidEncInArgs->videnc2InArgs.inputID = InBufferHdrIndex + 1;
                    /*Zero is not a valid input so increasing the bufferIndex value by 1*/
                } else {
                    pH264VEComp->pVidEncInArgs->videnc2InArgs.inputID = 0; /*Zero is not a valid input */
                }

                pH264VEComp->pVidEncOutArgs->videnc2OutArgs.freeBufID[0] = 0;
                /* Update the OutArgs details before the Codec Process call */
                OSAL_Info("Update the OutArgs before the Codec Process call");
                pH264VEComp->pVidEncOutArgs->videnc2OutArgs.size = sizeof(IVIDENC2_OutArgs);

                /* Codec Process call */
                eError = OMXH264VE_VISAPROCESS_AND_UPDATEPARAMS(hComponent, &retval);
                OMX_CHECK(eError == OMX_ErrorNone, eError);

                /* Send the input & corresponding output buffers Back to the Client when the codec frees */
                /*Note: implementation is based on
                a) after every process, there will be at max one input buffer that can be freed
                b)for every input buffer that is freed ......there will be corresponding o/p buffer
                */
                if( pH264VEComp->pVidEncOutArgs->videnc2OutArgs.freeBufID[0]) {
                    /*Non Zero ID : valid free buffer ID*/
                    OSAL_Info("Codec freed input buffer with ID =%d", pH264VEComp->pVidEncOutArgs->videnc2OutArgs.freeBufID[0]);

                    /* Propagate the Time Stamps from input to Corresponding Output */
                    OSAL_Info("Propagate the timestamp");
                    pOutBufHeader->nTimeStamp =
                            (pH264VEComp->pCodecInBufferArray[(pH264VEComp->pVidEncOutArgs->videnc2OutArgs.freeBufID[0] - 1)])->nTimeStamp;

                    /* Send the input buffers Back to the Client */
                    pH264VEComp->pCodecInBufferArray[(pH264VEComp->pVidEncOutArgs->videnc2OutArgs.freeBufID[0] - 1)]->nFilledLen = 0;
                    /*Completely Consumed*/
                    pH264VEComp->pCodecInBufferArray[(pH264VEComp->pVidEncOutArgs->videnc2OutArgs.freeBufID[0] - 1)]->nOffset = 0;

                    ((OMXBase_BufHdrPvtData *)((pH264VEComp->pCodecInBufferArray[(pH264VEComp->pVidEncOutArgs->videnc2OutArgs.freeBufID[0] - 1)])->pPlatformPrivate))->bufSt = OWNED_BY_US;

                    pH264VEComp->sBase.pPvtData->fpDioSend(hComponent, OMX_H264VE_INPUT_PORT,
                                           pH264VEComp->pCodecInBufferArray[(pH264VEComp->pVidEncOutArgs->videnc2OutArgs.freeBufID[0] - 1)]);


                    /*check for the EOS */
                    if( pH264VEComp->bPropagateEOSToOutputBuffer ) {
                        /*Send the Output with EOS after sending all the previous buffers*/
                        eError = OMXH264VE_GetNumCodecLockedBuffers(pComp, &LCodecLockedBufferCount);
                        OMX_CHECK(eError == OMX_ErrorNone, eError);
                        if(LCodecLockedBufferCount == 0) {
                            /*No locked buffers*/
                            pOutBufHeader->nFlags |= OMX_BUFFERFLAG_EOS;
                            pH264VEComp->bPropagateEOSToOutputBuffer = OMX_FALSE;
                        }
                    }

                    /* Check for IDR frame & update the Output BufferHdr Flags */
                    if(((pH264VEComp->pVidEncOutArgs->videnc2OutArgs.encodedFrameType) == IVIDEO_IDR_FRAME) ||
                            ((pH264VEComp->pVidEncOutArgs->videnc2OutArgs.encodedFrameType) == IVIDEO_I_FRAME)) {
                        pOutBufHeader->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;
                    }

                    /* Send the output buffers Back to the Client */
                    OSAL_Info("Send the output buffers Back to the Client");
                    /*Update the OutBufHeader*/
                    if( pH264VEComp->bAfterGenHeader ) {
                        pOutBufHeader->nOffset = pH264VEComp->nCodecConfigSize;
                        pOutBufHeader->nFilledLen = ((pH264VEComp->pVidEncOutArgs->videnc2OutArgs.bytesGenerated) -
                                                        (pH264VEComp->nCodecConfigSize));
                        pH264VEComp->bAfterGenHeader = OMX_FALSE;
                        pH264VEComp->nCodecConfigSize = 0;
                    } else {
                            pOutBufHeader->nOffset = 0;
                            pOutBufHeader->nFilledLen = pH264VEComp->pVidEncOutArgs->videnc2OutArgs.bytesGenerated;
                        }
                        /* Return this output buffer to the Base comp via ReturnDataNotify call
                        * to communciate with the IL client incase of Non-Tunneling or tunneled
                        * component in case of tunneling*/
                        ((OMXBase_BufHdrPvtData *)(pOutBufHeader->pPlatformPrivate))->bufSt = OWNED_BY_US;
                        pH264VEComp->sBase.pPvtData->fpDioSend(hComponent, OMX_H264VE_OUTPUT_PORT, pOutBufHeader);
                } else {
                    if( pH264VEComp->pVidEncOutArgs->videnc2OutArgs.bytesGenerated == 0 ) {
                        /*free ID ==0 so no inputbuffer is freed & thus no o/p should be generated*/
                        OSAL_Info("codec locked the buffer so do the DIO_Cancel for the outputbuffer");
                        OSAL_Info("bytes generated is Zero & retain the output buffers via DIO_Cancel");
                        ((OMXBase_BufHdrPvtData *)(pOutBufHeader->pPlatformPrivate))->bufSt = OWNED_BY_US;
                        pH264VEComp->sBase.pPvtData->fpDioCancel(hComponent, OMX_H264VE_OUTPUT_PORT, pOutBufHeader);
                    } else {
                        if((!pH264VEComp->bAfterEOSReception) ||
                                ((pH264VEComp->pVidEncStaticParams->nalUnitControlParams.naluPresentMaskEndOfSequence) == 0x0)) {
                            OSAL_ErrorTrace("***********something gone wrong***********");
                            goto EXIT;
                        }
                        /* Send the output buffers Back to the Client */
                        OSAL_Info("Send the output buffers Back to the Client");
                        /*Update the OutBufHeader*/
                        pOutBufHeader->nOffset = 0;
                        pOutBufHeader->nFilledLen = pH264VEComp->pVidEncOutArgs->videnc2OutArgs.bytesGenerated;
                        pOutBufHeader->nFlags |= OMX_BUFFERFLAG_EOS;
                        pH264VEComp->bPropagateEOSToOutputBuffer = OMX_FALSE;
                        /*notify the EOSEvent to the client*/
                        pH264VEComp->bNotifyEOSEventToClient = OMX_TRUE;
                        /* Return this output buffer to the Base comp via ReturnDataNotify call
                        * to communciate with the IL client incase of Non-Tunneling or tunneled
                        * component in case of tunneling*/
                        ((OMXBase_BufHdrPvtData *)(pOutBufHeader->pPlatformPrivate))->bufSt = OWNED_BY_US;
                        pH264VEComp->sBase.pPvtData->fpDioSend(hComponent, OMX_H264VE_OUTPUT_PORT, pOutBufHeader);
                    }
                }
            }
            /*Call to xDM_FLUSH when Input with EOS flag has been received*/
            if( bLCodecFlush ) {
                /*control call with command XDM_FLUSH*/
                OSAL_Info("Call CodecFlush ");
                eError = OMXH264VE_VISACONTROL(pH264VEComp->pVidEncHandle, XDM_FLUSH,
                                                    (VIDENC2_DynamicParams *)(pH264VEComp->pVidEncDynamicParams),
                                                    (IVIDENC2_Status *)(pH264VEComp->pVidEncStatus), hComponent, &retval);
                if( retval == VIDENC2_EFAIL ) {
                    OSAL_ErrorTrace("Got error from the CodecControl call");
                    OMX_TI_GET_ERROR(pH264VEComp, pH264VEComp->pVidEncStatus->videnc2Status.extendedError, eError);
                    OMX_CHECK(eError == OMX_ErrorNone, eError);
                }
                OMX_CHECK(eError == OMX_ErrorNone, eError);
                /*Set the bCodecFlush to True ....no need to call codec flush during idle->Loaded Transition*/
                pH264VEComp->bCodecFlush = OMX_TRUE;
                bLEOS= OMX_FALSE;
                bLCodecFlush = OMX_FALSE;
                /* after EOS no need to provide extra input buffer */
                pH264VEComp->bAfterEOSReception = OMX_TRUE;
            }
            if( pH264VEComp->bNotifyEOSEventToClient ) {
                /* Reset the Codec : to continue with New stream w/o codec create */
                if( pH264VEComp->bCodecCreate ) {
                    /*Codec Call: control call with command XDM_RESET*/
                    eError = OMXH264VE_VISACONTROL(pH264VEComp->pVidEncHandle, XDM_RESET,
                                                        (VIDENC2_DynamicParams *)(pH264VEComp->pVidEncDynamicParams),
                                                        (IVIDENC2_Status *)(pH264VEComp->pVidEncStatus), hComponent, &retval);
                    if( retval != VIDENC2_EOK ) {
                        OSAL_ErrorTrace("Got error from the Codec_RESET call");
                        OMX_TI_GET_ERROR(pH264VEComp, pH264VEComp->pVidEncStatus->videnc2Status.extendedError, eError);
                        OMX_CHECK(eError == OMX_ErrorNone, eError);
                    }
                    OMX_CHECK(eError == OMX_ErrorNone, eError);
                }
                /* Notify EOS event flag to the Base Component via ReturnEventNotify
                * call , which communciates with the IL Client    */
                pH264VEComp->sBase.fpReturnEventNotify(hComponent, OMX_EventBufferFlag,
                                                        OMX_H264VE_OUTPUT_PORT,
                                                        OMX_BUFFERFLAG_EOS, NULL);
                /*reset the flags*/
                pH264VEComp->bAfterEOSReception = OMX_FALSE;
                pH264VEComp->bNotifyEOSEventToClient = OMX_FALSE;
                pH264VEComp->pVidEncDynamicParams->videnc2DynamicParams.generateHeader = XDM_GENERATE_HEADER;
                OSAL_ObtainMutex(pH264VEComp->sBase.pMutex, OSAL_SUSPEND);
                pH264VEComp->bSendCodecConfig = OMX_TRUE;
                pH264VEComp->bCallxDMSetParams = PARAMS_UPDATED_AT_OMX;
                pH264VEComp->nCodecConfigSize = 0;
                pH264VEComp->bAfterGenHeader = OMX_FALSE;
                OSAL_ReleaseMutex(pH264VEComp->sBase.pMutex);
                goto EXIT;
            }

CHECKCOUNT:
            /*get the buffer count for the next loop execution*/
            pH264VEComp->sBase.pPvtData->fpDioGetCount(hComponent, OMX_H264VE_INPUT_PORT, (OMX_PTR)&nInMsgCount);
            pH264VEComp->sBase.pPvtData->fpDioGetCount(hComponent, OMX_H264VE_OUTPUT_PORT, (OMX_PTR)&nOutMsgCount);
            ALOGE("Input Msg Count=%d, Output Msg Count=%d for the Next process loop", nInMsgCount, nOutMsgCount);
        }  /*end of process call loop*/
    } else {
        OSAL_WarningTrace("!!!!!!!!!!!Incorrect State operation/ ports need to be enabled!!!!!!!!");
    }

EXIT:

    return (eError);

}

static OMX_ERRORTYPE OMXH264VE_ComponentDeinit(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE         eError    = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pComp    = NULL;
    OMXH264VidEncComp  *pH264VEComp = NULL;
    OMX_U32               i;


    /* Check the input parameters */
    OMX_CHECK(hComponent != NULL, OMX_ErrorBadParameter);
    pComp = (OMX_COMPONENTTYPE *)hComponent;
    pH264VEComp = (OMXH264VidEncComp *)pComp->pComponentPrivate;

    OMXBase_UtilCleanupIfError(hComponent);

    /* Calling OMX Base Component Deinit  */
    OSAL_Info("Call BaseComponent Deinit");
    eError = OMXBase_ComponentDeinit(hComponent);
    OMX_CHECK(eError == OMX_ErrorNone, eError);
    /*Allocating memory for port properties before calling SetDefaultProperties*/
    OSAL_Info("DeInitialize DerToBase.PortProperties");

    pH264VEComp->bInputMetaDataBufferMode = OMX_FALSE;

    /*Add CE deinit related stuff here*/
    if( pH264VEComp->pCEhandle ) {
        OSAL_Info("Call Engine_Close");
        Engine_close(pH264VEComp->pCEhandle);
    }

    /* Free up the H264VE component's private area */
    OSAL_Free(pH264VEComp->sBase.pAudioPortParams);
    OSAL_Free(pH264VEComp->sBase.pVideoPortParams);
    OSAL_Free(pH264VEComp->sBase.pImagePortParams);
    OSAL_Free(pH264VEComp->sBase.pOtherPortParams);
    memplugin_free_noheader(pH264VEComp->sCodecConfigData.sBuffer);
    memplugin_free_noheader(pH264VEComp->pTempBuffer[0]);
    memplugin_free_noheader(pH264VEComp->pTempBuffer[1]);

    OSAL_Free(pH264VEComp);
    pH264VEComp = NULL;

EXIT:
    OSAL_Info("At the End of Component DeInit");
    return (eError);
}


OMX_ERRORTYPE OMXH264VE_GetExtensionIndex(OMX_HANDLETYPE hComponent,
                                              OMX_STRING cParameterName,
                                              OMX_INDEXTYPE *pIndexType)
{
    OMX_ERRORTYPE         eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE    *pComp;
    OMXH264VidEncComp   *pH264VEComp = NULL;

    /* Check the input parameters */
    OMX_CHECK(hComponent != NULL, OMX_ErrorBadParameter);

    pComp = (OMX_COMPONENTTYPE *)hComponent;
    pH264VEComp = (OMXH264VidEncComp *)pComp->pComponentPrivate;

    OMX_CHECK(pH264VEComp != NULL, OMX_ErrorBadParameter);

    OMX_CHECK(cParameterName != NULL, OMX_ErrorBadParameter);
    OMX_CHECK(pIndexType != NULL, OMX_ErrorBadParameter);
    if( pH264VEComp->sBase.tCurState == OMX_StateInvalid ) {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
    if( strlen(cParameterName) > 127 ) {
        //strlen does not include �\0� size, hence 127
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if(strcmp(cParameterName, "OMX.google.android.index.storeMetaDataInBuffers") == 0) {
        *pIndexType = (OMX_INDEXTYPE) OMX_TI_IndexEncoderReceiveMetadataBuffers;
        goto EXIT;
    }

    eError = OMX_ErrorUnsupportedIndex;

EXIT:
    return (eError);
}


static OMX_ERRORTYPE OMXH264VE_ComponentTunnelRequest(OMX_HANDLETYPE hComponent,
                                                          OMX_U32 nPort,
                                                          OMX_HANDLETYPE hTunneledComp,
                                                          OMX_U32 nTunneledPort,
                                                          OMX_TUNNELSETUPTYPE *pTunnelSetup)
{
    OMX_ERRORTYPE         eError    = OMX_ErrorNone;
    OMXH264VidEncComp  *pH264VEComp = NULL;
    OMX_COMPONENTTYPE    *pHandle   = NULL;

    /* Check all the input parametrs */
    OMX_CHECK((hComponent != NULL), OMX_ErrorBadParameter);
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pH264VEComp = (OMXH264VidEncComp *)pHandle->pComponentPrivate;
    OMX_CHECK((pH264VEComp != NULL), OMX_ErrorBadParameter);

    eError = OMX_ErrorNotImplemented;
    OSAL_ErrorTrace("in omx-h264e ComponentTunnelRequest :: enable input subframe processing first");
    OMX_CHECK(OMX_ErrorNone == eError, eError);

EXIT:
    return (eError);
}


