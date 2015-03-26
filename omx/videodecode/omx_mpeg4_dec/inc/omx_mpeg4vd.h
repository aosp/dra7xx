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

#ifndef _OMX_MPEG4VD_H_
#define _OMX_MPEG4VD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <omx_video_decoder.h>
#include <ti/sdo/codecs/mpeg4vdec/impeg4vdec.h>

/*! Padding for width as per Codec Requirement */
#define PADX (32)
/*! Padding for height as per Codec requirement */
#define PADY (32)


void OMXMPEG4VD_SetStaticParams(OMX_HANDLETYPE hComponent, void *staticparams);

void OMXMPEG4VD_SetDynamicParams(OMX_HANDLETYPE hComponent, void *dynamicParams);

void OMXMPEG4VD_SetStatus(OMX_HANDLETYPE hComponent, void *status);

PaddedBuffParams CalculateMPEG4VD_outbuff_details(OMX_HANDLETYPE hComponent,
                                                 OMX_U32 width, OMX_U32 height);

OMX_ERRORTYPE OMXMPEG4VD_Init(OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE OMXH263VD_Init(OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE OMXMPEG4_H263VD_Init(OMX_HANDLETYPE hComponent);
void OMXMPEG4VD_DeInit(OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE OMXMPEG4VD_HandleError(OMX_HANDLETYPE hComponent);

extern OMX_ERRORTYPE OMXMPEG4VD_SetParameter(OMX_HANDLETYPE hComponent,
                                                 OMX_INDEXTYPE nIndex, OMX_PTR pParamStruct);
extern OMX_ERRORTYPE OMXMPEG4VD_GetParameter(OMX_HANDLETYPE hComponent,
                                                 OMX_INDEXTYPE nIndex, OMX_PTR pParamStruct);

extern OMX_ERRORTYPE OMXVidDec_SetParameter(OMX_HANDLETYPE hComponent,
                                                      OMX_INDEXTYPE nIndex, OMX_PTR pParamStruct);
extern OMX_ERRORTYPE OMXVidDec_GetParameter(OMX_HANDLETYPE hComponent,
                                                      OMX_INDEXTYPE nIndex, OMX_PTR pParamStruct);


typedef struct OMXMPEG4VidDecComp {
    OMX_PARAM_DEBLOCKINGTYPE         tDeblockingParam;
    OMX_VIDEO_PARAM_MPEG4TYPE        tMPEG4VideoParam;
    OMX_BOOL                         bIsSorensonSpark;
} OMXMPEG4VidDecComp;


#ifdef __cplusplus
}
#endif

#endif /* _OMX_MPEG4VD_H_ */

