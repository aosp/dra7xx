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

#include <dirent.h>

#define MAX_ROLES 20
#define MAX_TABLE_SIZE 50

typedef struct _ComponentTable {
    OMX_STRING name;
    OMX_U16 nRoles;
    OMX_STRING pRoleArray[MAX_ROLES];
}ComponentTable;

OMX_API OMX_ERRORTYPE OMX_GetRolesOfComponent (
                    OMX_IN      OMX_STRING compName,
                    OMX_INOUT   OMX_U32 *pNumRoles,
                    OMX_OUT     OMX_U8 **roles);

OMX_ERRORTYPE OMX_PrintComponentTable();
OMX_ERRORTYPE OMX_BuildComponentTable();
OMX_ERRORTYPE ComponentTable_EventHandler(
                    OMX_IN OMX_HANDLETYPE hComponent,
                    OMX_IN OMX_PTR pAppData,
                    OMX_IN OMX_EVENTTYPE eEvent,
                    OMX_IN OMX_U32 nData1,
                    OMX_IN OMX_U32 nData2,
                    OMX_IN OMX_PTR pEventData);

OMX_ERRORTYPE ComponentTable_EmptyBufferDone(
                    OMX_OUT OMX_HANDLETYPE hComponent,
                    OMX_OUT OMX_PTR pAppData,
                    OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer);

OMX_ERRORTYPE ComponentTable_FillBufferDone(
                    OMX_OUT OMX_HANDLETYPE hComponent,
                    OMX_OUT OMX_PTR pAppData,
                    OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer);
