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

#ifndef _OMX_H264VD_H_
#define _OMX_H264VD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <omx_video_decoder.h>
#include <ti/sdo/codecs/h264vdec/ih264vdec.h>

/*! Padding for width as per Codec Requirement */
 #define PADX  (32)
/*! Padding for height as per Codec requirement */
 #define PADY  (24)

/** Default Frame skip H264 Decoder */
#define H264VD_DEFAULT_FRAME_SKIP              IVIDEO_SKIP_DEFAULT


static OMX_ERRORTYPE OMXH264VD_GetParameter(OMX_HANDLETYPE hComponent,
                        OMX_INDEXTYPE nIndex, OMX_PTR pParamStruct);

static OMX_ERRORTYPE OMXH264VD_SetParameter(OMX_HANDLETYPE hComponent,
                        OMX_INDEXTYPE nIndex, OMX_PTR pParamStruct);

void OMXH264VD_Set_StaticParams(OMX_HANDLETYPE hComponent, void *staticparams);

void OMXH264VD_Set_DynamicParams(OMX_HANDLETYPE hComponent, void *dynamicParams);

void OMXH264VD_Set_Status(OMX_HANDLETYPE hComponent, void *status);

OMX_ERRORTYPE OMXH264VD_HandleError(OMX_HANDLETYPE hComponent);

PaddedBuffParams CalculateH264VD_outbuff_details(OMX_HANDLETYPE hComponent,
                        OMX_U32 width, OMX_U32 height);

extern OMX_ERRORTYPE OMXH264VD_Init(OMX_HANDLETYPE hComponent);
extern void OMXH264VD_DeInit(OMX_HANDLETYPE hComponent);
extern OMX_U32 OMXH264VD_Calculate_TotalRefFrames(OMX_U32 nWidth, OMX_U32 nHeight, OMX_VIDEO_AVCLEVELTYPE eLevel);

typedef struct OMXH264VidDecComp {
    OMX_VIDEO_PARAM_AVCTYPE tH264VideoParam;
} OMXH264VidDecComp;


#ifdef __cplusplus
}
#endif

#endif /* _OMX_H2644VD_NEW_H_ */

