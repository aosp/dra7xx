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

#ifndef _OMX_H264VE_COMPONENT_H
#define _OMX_H264VE_COMPONENT_H

#ifdef _cplusplus
extern "C"   {
#endif /* _cplusplus */

#define ENCODE_RAM_BUFFERS

#include "native_handle.h"
#include <hal_public.h>

#include "omx_H264videoencoderutils.h"

#define  OMX_VIDENC_NUM_PORTS     (2)

#define OMX_ENGINE_NAME "ivahd_vidsvr"

typedef struct OMX_H264_LVL_BITRATE {
    OMX_VIDEO_AVCLEVELTYPE  eLevel;
    OMX_U32                 nMaxBitRateSupport;
}OMX_H264_LVL_BITRATE;

typedef struct OMX_MetaDataBuffer {
    int type;
    void *handle;
    int offset;
}OMX_MetaDataBuffer;

/* OMX H264 Encoder Component */
typedef struct OMXH264VideoEncoderComponent {
    /* base component handle */
    OMXBaseComp         sBase;

    /* codec and engine handles */
    Engine_Handle       pCEhandle;
    Engine_Error        tCEerror;
    VIDENC2_Handle      pVidEncHandle;
    OMX_BOOL            bCodecCreate;
    OMX_BOOL            bCodecCreateSettingsChange;

    /* Encoder static/dynamic/buf args */
    IH264ENC_Params         *pVidEncStaticParams;
    IH264ENC_DynamicParams  *pVidEncDynamicParams;
    IH264ENC_Status         *pVidEncStatus;
    IH264ENC_InArgs         *pVidEncInArgs;
    IH264ENC_OutArgs        *pVidEncOutArgs;
    IVIDEO2_BufDesc         *pVedEncInBufs;
    XDM2_BufDesc            *pVedEncOutBufs;

    /* omx component statemachine variables */
    OMX_BOOL                bInputPortDisable;
    OMX_BOOL                bCodecFlush;
    PARAMS_UPDATE_STATUS    bCallxDMSetParams;
    OMX_BOOL                bAfterEOSReception;
    OMX_BOOL                bNotifyEOSEventToClient;
    OMX_BOOL                bPropagateEOSToOutputBuffer;
    OMX_BOOL                bSetParamInputIsDone;

    /* codec config handling variables*/
    OMXBase_CodecConfigBuf  sCodecConfigData;
    OMX_BOOL                bSendCodecConfig;
    OMX_U32                 nCodecConfigSize;
    OMX_BOOL                bAfterGenHeader;

    /* internal buffer tracking arrays */
    OMX_BUFFERHEADERTYPE    **pCodecInBufferArray;
    OMXBase_BufHdrPvtData   *pCodecInBufferBackupArray;

    /* temporary memory to meet and codec and dce requirements */
    MemHeader               *pTempBuffer[2];

    OMX_BOOL                bInputMetaDataBufferMode;
    OMX_PTR                 hCC;
    IMG_native_handle_t     **pBackupBuffers;
    alloc_device_t          *mAllocDev;

} OMXH264VidEncComp;

OMX_ERRORTYPE OMXH264VE_ComponentInit(OMX_HANDLETYPE hComponent);

#ifdef _cplusplus
}
#endif /* __cplusplus */

#endif /* _OMX_H264VE_COMPONENT_H */

