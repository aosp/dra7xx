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

#ifndef _OMX_VIDEO_DECODER_INTERNAL_H_
#define _OMX_VIDEO_DECODER_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <osal_trace.h>
#include <OMX_Core.h>
#include <OMX_Component.h>
#include <omx_base.h>
#include <OMX_Types.h>
#include <omx_base_utils.h>
#include <omx_video_decoder.h>
#include <omx_video_decoder_componenttable.h>

static OMX_STRING   engineName   = "ivahd_vidsvr";

#define OMX_VIDDEC_COMP_VERSION_MAJOR 1
#define OMX_VIDDEC_COMP_VERSION_MINOR 1
#define OMX_VIDDEC_COMP_VERSION_REVISION 0
#define OMX_VIDDEC_COMP_VERSION_STEP 0

#define OMX_VIDEODECODER_DEFAULT_FRAMERATE 30

/*! Minimum Input Buffer Count */
#define OMX_VIDDEC_MIN_IN_BUF_COUNT 1
/*! Default Input Buffer Count */
#define OMX_VIDDEC_DEFAULT_IN_BUF_COUNT 2
/*! Minimum Input Buffer Count in Data Sync mode
* codec releases input buffer of Nth Data Sync call during (N+2)th Data Sync call
* So minimum number of input buffers required is 3 */
#define OMX_VIDDEC_DATASYNC_MIN_IN_BUF_COUNT 3
/*! Default Input Buffer Count in Data Sync mode*/
#define OMX_VIDDEC_DATASYNC_DEFAULT_IN_BUF_COUNT 4
/*! Default Frame Width */
#define OMX_VIDDEC_DEFAULT_FRAME_WIDTH 176
/*! Default Frame Height */
#define OMX_VIDDEC_DEFAULT_FRAME_HEIGHT 144
/*! Default 1D-Buffer Alignment */
#define OMX_VIDDEC_DEFAULT_1D_INPUT_BUFFER_ALIGNMENT 1
/*! Default Stride value for 2D buffer */
#define OMX_VIDDEC_TILER_STRIDE  (4096)
/*! Max Image Width supported */
#define OMX_VIDDEC_MAX_WIDTH  (2048)
/*! Max Image Height supported */
#define OMX_VIDDEC_MAX_HEIGHT  (2048)
/*! Max Number of MBs supported */
#define OMX_VIDDEC_MAX_MACROBLOCK (8160)

#define OMX_VIDEODECODER_COMPONENT_NAME "OMX.TI.DUCATI1.VIDEO.DECODER"

/* external definition for the Video Params Init function */
extern OMX_ERRORTYPE OMXVidDec_InitFields(OMXVidDecComp *pVidDecComp);
extern void OMXVidDec_InitPortDefs(OMX_HANDLETYPE hComponent, OMXVidDecComp *pVidDecComp);
extern void OMXVidDec_InitPortParams(OMXVidDecComp *pVidDecComp);
extern void OMXVidDec_InitDecoderParams(OMX_HANDLETYPE hComponent, OMXVidDecComp *pVidDecComp);

extern OMX_ERRORTYPE OMXVidDec_HandleFLUSH_EOS(OMX_HANDLETYPE hComponent, OMX_BUFFERHEADERTYPE *pOutBufHeader,
                                        OMX_BUFFERHEADERTYPE *pInBufHeader);
extern OMX_ERRORTYPE OMXVidDec_SetInPortDef(OMX_HANDLETYPE hComponent,
                                        OMX_PARAM_PORTDEFINITIONTYPE *pPortDefs);
extern OMX_ERRORTYPE OMXVidDec_SetOutPortDef(OMXVidDecComp *pVidDecComp,
                                        OMX_PARAM_PORTDEFINITIONTYPE *pPortDefs);
extern OMX_ERRORTYPE OMXVidDec_HandleFirstFrame(OMX_HANDLETYPE hComponent,
                                        OMX_BUFFERHEADERTYPE * *ppInBufHeader);
extern OMX_ERRORTYPE OMXVidDec_HandleCodecProcError(OMX_HANDLETYPE hComponent,
                                        OMX_BUFFERHEADERTYPE * *ppInBufHeader,
                                        OMX_BUFFERHEADERTYPE * *ppOutBufHeader);
extern OMX_ERRORTYPE  OMXVidDec_HandleLockedBuffer(OMX_HANDLETYPE hComponent,
                                        OMX_BUFFERHEADERTYPE *pOutBufHeader);

XDAS_Int32 OMXVidDec_DataSync_GetInputData(XDM_DataSyncHandle dataSyncHandle, XDM_DataSyncDesc *dataSyncDesc);

XDAS_Int32 OMXVidDec_DataSync_PutBuffer(XDM_DataSyncHandle dataSyncHandle, XDM_DataSyncDesc *dataSyncDesc);

extern void OMXVidDec_Set2DBuffParams(OMX_HANDLETYPE hComponent, OMXVidDecComp *pVidDecComp);
extern OMX_U32 Calc_InbufSize(OMX_U32 width, OMX_U32 height);

#ifdef __cplusplus
}
#endif

#endif /* _OMX_VIDEO_DECODER_H_ */

