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

/** OMX_Core_Wrapper.h
*/

#ifndef OMX_Core_Wrapper_h
#define OMX_Core_Wrapper_h

#ifdef __cplusplus
extern "C" {
#endif


/* Each OMX header shall include all required header files to allow the
 *  header to compile without errors.  The includes below are required
 *  for this header file to compile successfully
 */

#include <OMX_Core.h>

OMX_API OMX_ERRORTYPE OMX_APIENTRY TIOMX_Init(void);
OMX_API OMX_ERRORTYPE OMX_APIENTRY TIOMX_Deinit(void);
OMX_API OMX_ERRORTYPE OMX_APIENTRY TIOMX_ComponentNameEnum(
                        OMX_OUT OMX_STRING cComponentName,
                        OMX_IN  OMX_U32 nNameLength,
                        OMX_IN  OMX_U32 nIndex);
OMX_API OMX_ERRORTYPE OMX_APIENTRY TIOMX_GetHandle(
                        OMX_OUT OMX_HANDLETYPE* pHandle,
                        OMX_IN  OMX_STRING cComponentName,
                        OMX_IN  OMX_PTR pAppData,
                        OMX_IN  OMX_CALLBACKTYPE* pCallBacks);
OMX_API OMX_ERRORTYPE OMX_APIENTRY TIOMX_FreeHandle(
                        OMX_IN  OMX_HANDLETYPE hComponent);
OMX_API OMX_ERRORTYPE TIOMX_GetComponentsOfRole (
                        OMX_IN      OMX_STRING role,
                        OMX_INOUT   OMX_U32 *pNumComps,
                        OMX_INOUT   OMX_U8  **compNames);
OMX_API OMX_ERRORTYPE OMX_APIENTRY TIOMX_SetupTunnel(
                        OMX_IN  OMX_HANDLETYPE hOutput,
                        OMX_IN  OMX_U32 nPortOutput,
                        OMX_IN  OMX_HANDLETYPE hInput,
                        OMX_IN  OMX_U32 nPortInput);
OMX_API OMX_ERRORTYPE   OMX_APIENTRY  TIOMX_GetContentPipe(
                        OMX_OUT OMX_HANDLETYPE *hPipe,
                        OMX_IN OMX_STRING szURI);

#ifdef __cplusplus
}
#endif

#endif

