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

#ifndef _OMX_BASE_UTILS_H_
#define _OMX_BASE_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <osal_trace.h>

#define OMX_CHECK(_COND_, _ERRORCODE_) do { \
                    if( !(_COND_)) { eError = _ERRORCODE_; \
                        OSAL_ErrorTrace("Failed check: " # _COND_); \
                        OSAL_ErrorTrace("Returning error: " # _ERRORCODE_); \
                        goto EXIT; } \
                } while( 0 )

#define OMX_BASE_CHK_VERSION(_pStruct_, _sName_, _e_) do { \
                if(((_sName_ *)_pStruct_)->nSize != sizeof(_sName_)) { \
                    _e_ = OMX_ErrorBadParameter; \
                    OSAL_ErrorTrace("Incorrect 'nSize' field. Returning OMX_ErrorBadParameter"); \
                    goto EXIT; } \
                if((((_sName_ *)_pStruct_)->nVersion.s.nVersionMajor != 0x1) || \
                    ((((_sName_ *)_pStruct_)->nVersion.s.nVersionMinor != 0x1) && \
                    ((_sName_ *)_pStruct_)->nVersion.s.nVersionMinor != 0x0 )) { \
                    _e_ = OMX_ErrorVersionMismatch; \
                    OSAL_ErrorTrace("Version mismatch. Returning OMX_ErrorVersionMismatch"); \
                    goto EXIT; } \
                } while( 0 )

#define OMX_BASE_INIT_STRUCT_PTR(_pStruct_, _sName_) do { \
                        OSAL_Memset((_pStruct_), 0x0, sizeof(_sName_)); \
                        (_pStruct_)->nSize = sizeof(_sName_); \
                        (_pStruct_)->nVersion.s.nVersionMajor = 0x1; \
                        (_pStruct_)->nVersion.s.nVersionMinor = 0x1; \
                        (_pStruct_)->nVersion.s.nRevision     = 0x2; \
                        (_pStruct_)->nVersion.s.nStep         = 0x0; \
                } while( 0 )

#ifdef __cplusplus
}
#endif

#endif /* _OMX_BASE_UTILS_H_ */

