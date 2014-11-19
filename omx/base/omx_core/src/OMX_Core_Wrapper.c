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

#include <OMX_Component.h>
#include <OMX_Core.h>
#include <osal_trace.h>

#include "OMX_ComponentRegistry.h"
#include "OMX_Core_Wrapper.h"

#define LOG_TAG "OMX_CORE_WRAPPER"

/** determine capabilities of a component before acually using it */
extern OMX_BOOL TIOMXConfigParser(OMX_PTR aInputParameters,
    OMX_PTR aOutputParameters);

OMX_ERRORTYPE TIComponentTable_EventHandler(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_PTR pAppData,
    OMX_IN OMX_EVENTTYPE eEvent,
    OMX_IN OMX_U32 nData1, OMX_IN OMX_U32 nData2, OMX_IN OMX_PTR pEventData)
{
    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE TIComponentTable_EmptyBufferDone(OMX_OUT OMX_HANDLETYPE
    hComponent, OMX_OUT OMX_PTR pAppData,
    OMX_OUT OMX_BUFFERHEADERTYPE * pBuffer)
{
    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE TIComponentTable_FillBufferDone(OMX_OUT OMX_HANDLETYPE
    hComponent, OMX_OUT OMX_PTR pAppData,
    OMX_OUT OMX_BUFFERHEADERTYPE * pBuffer)
{
    return OMX_ErrorNotImplemented;
}


OMX_API OMX_ERRORTYPE TIOMX_Init(void)
{
    OSAL_Entering("TIOMX_Init\n");
    return OMX_Init();
}

OMX_API OMX_ERRORTYPE TIOMX_Deinit(void)
{
    OSAL_Entering("TIOMX_Deinit\n");
    return OMX_Deinit();
}

OMX_API OMX_ERRORTYPE TIOMX_ComponentNameEnum(OMX_OUT OMX_STRING
            cComponentName, OMX_IN OMX_U32 nNameLength, OMX_IN OMX_U32 nIndex)
{
    OSAL_Entering("TIOMX_ComponentNameEnum\n");
    return OMX_ComponentNameEnum(cComponentName, nNameLength, nIndex);
}

OMX_API OMX_ERRORTYPE TIOMX_GetHandle(OMX_OUT OMX_HANDLETYPE * pHandle,
            OMX_IN OMX_STRING cComponentName,
            OMX_IN OMX_PTR pAppData, OMX_IN OMX_CALLBACKTYPE * pCallBacks)
{
    OSAL_Entering("TIOMX_GetHandle\n");
    return OMX_GetHandle(pHandle, cComponentName, pAppData, pCallBacks);
}

OMX_API OMX_ERRORTYPE TIOMX_FreeHandle(OMX_IN OMX_HANDLETYPE hComponent)
{
    OSAL_Entering("TIOMX_FreeHandle\n");
    return OMX_FreeHandle(hComponent);
}

OMX_API OMX_ERRORTYPE TIOMX_GetComponentsOfRole(OMX_IN OMX_STRING role,
                    OMX_INOUT OMX_U32 * pNumComps, OMX_INOUT OMX_U8 ** compNames)
{
    OSAL_Entering("TIOMX_GetComponentsOfRole\n");
    return OMX_GetComponentsOfRole(role, pNumComps, compNames);
}

OMX_API OMX_ERRORTYPE TIOMX_GetRolesOfComponent(OMX_IN OMX_STRING compName,
            OMX_INOUT OMX_U32 * pNumRoles, OMX_OUT OMX_U8 ** roles)
{
    OSAL_Entering("TIOMX_GetRolesOfComponent\n");
    return OMX_GetRolesOfComponent(compName, pNumRoles, roles);
}

OMX_API OMX_ERRORTYPE TIOMX_SetupTunnel(OMX_IN OMX_HANDLETYPE hOutput,
            OMX_IN OMX_U32 nPortOutput,
            OMX_IN OMX_HANDLETYPE hInput, OMX_IN OMX_U32 nPortInput)
{
    OSAL_Entering("TIOMX_SetupTunnel\n");
    return OMX_SetupTunnel(hOutput, nPortOutput, hInput, nPortInput);
}

OMX_API OMX_ERRORTYPE TIOMX_GetContentPipe(OMX_OUT OMX_HANDLETYPE * hPipe,
    OMX_IN OMX_STRING szURI)
{

    OSAL_Entering("TIOMX_GetContentPipe\n");
    return 0;
}

