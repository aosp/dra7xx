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

#ifndef _OMX_VIDEO_DECODER_H_
#define _OMX_VIDEO_DECODER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <OMX_Types.h>
#include <omx_base.h>
#include <xdc/std.h>
#include <ti/xdais/xdas.h>
#include <ti/sdo/ce/video3/viddec3.h>
#include <omx_base_utils.h>

#include <hardware/gralloc.h>
#include <hardware/hardware.h>
#include <hal_public.h>

#define OMX_VIDDEC_NUM_OF_PORTS             OMX_BASE_NUM_OF_PORTS
#define OMX_VIDDEC_INPUT_PORT               OMX_BASE_INPUT_PORT
#define OMX_VIDDEC_OUTPUT_PORT              OMX_BASE_OUTPUT_PORT
#define OMX_VIDDEC_DEFAULT_START_PORT_NUM   OMX_BASE_DEFAULT_START_PORT_NUM

/*
* Padded Buffer Parameters
*/
typedef struct PaddedBuffParams {
    OMX_U32 nBufferSize;              /*! Buffer size */
    OMX_U32 nPaddedWidth;             /*! Padded Width of the buffer */
    OMX_U32 nPaddedHeight;            /*! Padded Height */
    OMX_U32 nBufferCountMin;          /*! Min number of buffers required */
    OMX_U32 nBufferCountActual;       /*! Actual number of buffers */
    OMX_U32 n1DBufferAlignment;       /*! 1D Buffer Alignment value */
    OMX_U32 n2DBufferYAlignment;      /*! Y axis alignment value in 2Dbuffer */
    OMX_U32 n2DBufferXAlignment;      /*! X Axis alignment value in 2Dbuffer */
} PaddedBuffParams;

/*
* OMX Video decoder component
*/
typedef struct OMXVideoDecoderComponent {
    OMXBaseComp                     sBase;
    /* codec related fields */
    OMX_STRING                      cDecoderName;
    VIDDEC3_Handle                  pDecHandle;
    Engine_Handle                   ce;
    IVIDDEC3_Params                 *pDecStaticParams;        /*! Pointer to Decoder Static Params */
    IVIDDEC3_DynamicParams          *pDecDynParams;           /*! Pointer to Decoder Dynamic Params */
    IVIDDEC3_Status                 *pDecStatus;              /*! Pointer to Decoder Status struct */
    IVIDDEC3_InArgs                 *pDecInArgs;              /*! Pointer to Decoder InArgs */
    IVIDDEC3_OutArgs                *pDecOutArgs;             /*! Pointer to Decoder OutArgs */
    XDM2_BufDesc                    *tInBufDesc;
    XDM2_BufDesc                    *tOutBufDesc;

    /* OMX params */
    OMX_VIDEO_PARAM_PORTFORMATTYPE  tVideoParams[OMX_VIDDEC_NUM_OF_PORTS];
    OMX_CONFIG_RECTTYPE             tCropDimension;
    OMX_CONFIG_SCALEFACTORTYPE      tScaleParams;
    OMX_PARAM_COMPONENTROLETYPE     tComponentRole;
    OMX_CONFIG_RECTTYPE             t2DBufferAllocParams[OMX_VIDDEC_NUM_OF_PORTS];

    /* local params */
    gralloc_module_t                const *grallocModule;
    OMXBase_CodecConfigBuf          sCodecConfig;
    OMX_U32                         nOutPortReconfigRequired;
    OMX_U32                         nCodecRecreationRequired;
    OMX_U32                         bInputBufferCancelled;

    OMX_U32                         bIsFlushRequired;
    OMX_BOOL                        bUsePortReconfigForCrop;
    OMX_BOOL                        bUsePortReconfigForPadding;
    OMX_BOOL                        bSupportDecodeOrderTimeStamp;
    OMX_BOOL                        bSupportSkipGreyOutputFrames;

    OMX_U32                         nFrameCounter;
    OMX_BOOL                        bSyncFrameReady;
    OMX_U32                         nOutbufInUseFlag;
    OMX_PTR                         pCodecSpecific;
    OMX_U32                         nDecoderMode;
    OMX_U32                         nFatalErrorGiven;
    OMX_PTR                         pTimeStampStoragePipe;
    OMX_U32                         nFrameRateDivisor;
    OMX_BOOL                        bFirstFrameHandled;

    void (*fpSet_StaticParams)(OMX_HANDLETYPE hComponent, void *params);
    void (*fpSet_DynamicParams)(OMX_HANDLETYPE hComponent, void *dynamicparams);
    void (*fpSet_Status)(OMX_HANDLETYPE hComponent, void *status);
    void (*fpDeinit_Codec)(OMX_HANDLETYPE hComponent);
    OMX_ERRORTYPE (*fpHandle_ExtendedError)(OMX_HANDLETYPE hComponent);
    OMX_ERRORTYPE (*fpHandle_CodecGetStatus)(OMX_HANDLETYPE hComponent);
    PaddedBuffParams (*fpCalc_OubuffDetails)(OMX_HANDLETYPE hComponent, OMX_U32 width, OMX_U32 height);

}OMXVidDecComp;

/* Component Entry Method */
OMX_ERRORTYPE OMX_ComponentInit(OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE OMXVidDec_ComponentInit(OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE OMXVidDec_ComponentDeinit(OMX_HANDLETYPE hComponent);

OMX_ERRORTYPE OMXVidDec_SetParameter(OMX_HANDLETYPE hComponent,
                            OMX_INDEXTYPE nIndex, OMX_PTR pParamStruct);

OMX_ERRORTYPE OMXVidDec_GetParameter(OMX_HANDLETYPE hComponent,
                            OMX_INDEXTYPE nIndex, OMX_PTR pParamStruct);

OMX_ERRORTYPE OMXVidDec_GetConfig(OMX_HANDLETYPE hComponent,
                            OMX_INDEXTYPE nIndex, OMX_PTR pComponentConfigStructure);

OMX_ERRORTYPE OMXVidDec_SetConfig(OMX_HANDLETYPE hComponent,
                            OMX_INDEXTYPE nIndex, OMX_PTR pComponentConfigStructure);

OMX_ERRORTYPE OMXVidDec_CommandNotify(OMX_HANDLETYPE hComponent, OMX_COMMANDTYPE Cmd,
                            OMX_U32 nParam, OMX_PTR pCmdData);

OMX_ERRORTYPE OMXVidDec_DataNotify(OMX_HANDLETYPE hComponent);

OMX_ERRORTYPE OMXVidDec_XlateBuffHandle(OMX_HANDLETYPE hComponent, OMX_PTR pBufferHdr, OMX_BOOL bRegister);

OMX_ERRORTYPE OMXVidDec_GetExtensionIndex(OMX_HANDLETYPE hComponent, OMX_STRING cParameterName,
                            OMX_INDEXTYPE *pIndexType);


#ifdef __cplusplus
}
#endif

#endif /* _OMX_VIDEO_DECODER_H_ */

