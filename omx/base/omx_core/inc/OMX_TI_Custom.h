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

#ifndef OMX_TI_Custom_H
#define OMX_TI_Custom_H

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_IVCommon.h>

/**
* A pointer to this struct is passed to the OMX_SetParameter when the extension
* index for the 'OMX.google.android.index.enableAndroidNativeBuffers' extension
* is given.
* The corresponding extension Index is OMX_TI_IndexUseNativeBuffers.
* This will be used to inform OMX about the presence of gralloc pointers instead
* of virtual pointers
*/
typedef struct OMX_TI_PARAMUSENATIVEBUFFER {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnable;
} OMX_TI_PARAMUSENATIVEBUFFER;

/**
* A pointer to this struct is passed to OMX_GetParameter when the extension
* index for the 'OMX.google.android.index.getAndroidNativeBufferUsage'
* extension is given.
* The corresponding extension Index is OMX_TI_IndexAndroidNativeBufferUsage.
* The usage bits returned from this query will be used to allocate the Gralloc
* buffers that get passed to the useAndroidNativeBuffer command.
*/
typedef struct OMX_TI_PARAMNATIVEBUFFERUSAGE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nUsage;
} OMX_TI_PARAMNATIVEBUFFERUSAGE;

typedef enum OMX_TI_INDEXTYPE {
    OMX_TI_IndexUseNativeBuffers = ((OMX_INDEXTYPE)OMX_IndexVendorStartUnused + 1),
    OMX_TI_IndexAndroidNativeBufferUsage,
    OMX_TI_IndexParamTimeStampInDecodeOrder,
    OMX_TI_IndexEncoderReceiveMetadataBuffers,
 } OMX_TI_INDEXTYPE;

typedef enum OMX_TI_ERRORTYPE
{
    /*Control attribute is pending - Dio_Dequeue will not work until attribute
    is retreived*/
    OMX_TI_WarningAttributePending = (OMX_S32)((OMX_ERRORTYPE)OMX_ErrorVendorStartUnused + 1),
    /*Attribute buffer size is insufficient - reallocate the attribute buffer*/
    OMX_TI_WarningInsufficientAttributeSize,
    /*EOS buffer has been received*/
    OMX_TI_WarningEosReceived,
    /*Port enable is called on an already enabled port*/
    OMX_TI_ErrorPortIsAlreadyEnabled,
    /*Port disable is called on an already disabled port*/
    OMX_TI_ErrorPortIsAlreadyDisabled
} OMX_TI_ERRORTYPE;


/**
* OMX_TI_VIDEO_CONFIG_AVC_LTRP_INTERVAL : Structure to enable timestamps in decode order
*                                        at i/p of decoders.
*/
typedef struct OMX_TI_PARAM_TIMESTAMP_IN_DECODE_ORDER {
    OMX_U32         nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_BOOL        bEnabled;
} OMX_TI_PARAM_TIMESTAMP_IN_DECODE_ORDER;

#ifdef __cplusplus
}
#endif

#endif

