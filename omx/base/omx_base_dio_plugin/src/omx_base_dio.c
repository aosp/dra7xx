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
#define LOG_TAG "OMX_BASE_DIO"

#include <string.h>
#include <omx_base.h>

static OMX_PTR OMXBase_DIO_GetPort(OMX_HANDLETYPE hComponent,
                                    OMX_U32 nPortIndex);
/*
* OMXBase DIO Init
*/
OMX_ERRORTYPE OMXBase_DIO_Init (OMX_HANDLETYPE hComponent,
                                 OMX_U32 nPortIndex,
                                 OMX_STRING cChannelType,
                                 OMX_PTR pCreateParams)
{
    OMX_ERRORTYPE   eError = OMX_ErrorNone;
    OMX_DIO_Object  *hDIO = NULL;
    OMXBase_Port    *pPort = NULL;
    OMX_BOOL        bFound = OMX_FALSE;
    OMX_U32         i = 0;

    OMX_COMPONENTTYPE   *pComp = (OMX_COMPONENTTYPE *)hComponent;
    OMXBaseComp         *pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;

    OMX_CHECK(NULL != cChannelType, OMX_ErrorBadParameter);

    pPort = OMXBase_DIO_GetPort(hComponent, nPortIndex);
    OMX_CHECK(pPort != NULL, OMX_ErrorBadParameter);

    while( NULL != OMX_DIO_Registered[i].cChannelType ) {
        if( strcmp(cChannelType, OMX_DIO_Registered[i].cChannelType) == 0 ) {
            bFound = OMX_TRUE;
            break;
        }
        i++;
    }

    if( bFound ) {
        hDIO = (OMX_DIO_Object *) OSAL_Malloc(sizeof(OMX_DIO_Object));
        OMX_CHECK(NULL != hDIO, OMX_ErrorInsufficientResources);
        OSAL_Memset(hDIO, 0x0, sizeof(OMX_DIO_Object));

        /* Initialize the DIO object depending on the ChannelType */
        eError = OMX_DIO_Registered[i].pInitialize(hDIO, pCreateParams);
        OMX_CHECK(OMX_ErrorNone == eError, eError);

        /* Assign DIO handle to port */
        pPort->hDIO = hDIO;
    } else {
        OMX_CHECK(OMX_FALSE, OMX_ErrorUndefined);
    }

EXIT:
    return (eError);
}

/*
* OMXBase DIO DeInit
*/
OMX_ERRORTYPE OMXBase_DIO_Deinit (OMX_HANDLETYPE hComponent,
                                   OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE   eError = OMX_ErrorNone;
    OMX_DIO_Object  *hDIO = NULL;
    OMXBase_Port    *pPort = NULL;

    pPort = OMXBase_DIO_GetPort(hComponent, nPortIndex);
    OMX_CHECK(pPort != NULL, OMX_ErrorBadParameter);

    hDIO = (OMX_DIO_Object *)pPort->hDIO;
    OMX_CHECK(hDIO != NULL, OMX_ErrorBadParameter);

    eError = hDIO->deinit(hDIO);

    OSAL_Free(pPort->hDIO);
    pPort->hDIO = NULL;

EXIT:
    return (eError);
}

/*
* OMXBase DIO Open
*/
OMX_ERRORTYPE OMXBase_DIO_Open (OMX_HANDLETYPE hComponent,
                                 OMX_U32 nPortIndex,
                                 OMX_PTR pOpenParams)
{
    OMX_ERRORTYPE   eError = OMX_ErrorNone;
    OMX_DIO_Object  *hDIO = NULL;
    OMXBase_Port    *pPort = NULL;

    pPort = OMXBase_DIO_GetPort(hComponent, nPortIndex);
    OMX_CHECK(pPort != NULL, OMX_ErrorBadParameter);

    hDIO = (OMX_DIO_Object *)pPort->hDIO;
    OMX_CHECK(hDIO != NULL, OMX_ErrorBadParameter);

    eError = hDIO->open(hDIO, (OMX_DIO_OpenParams *)pOpenParams);

EXIT:
    if( eError == OMX_ErrorNone ) {
        hDIO->bOpened = OMX_TRUE;
    }
    return (eError);
}

/*
* OMX Base DIO close
*/
OMX_ERRORTYPE OMXBase_DIO_Close (OMX_HANDLETYPE hComponent,
                                  OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE   eError = OMX_ErrorNone;
    OMX_DIO_Object  *hDIO = NULL;
    OMXBase_Port    *pPort = NULL;

    pPort = OMXBase_DIO_GetPort(hComponent, nPortIndex);
    OMX_CHECK(pPort != NULL, OMX_ErrorBadParameter);

    hDIO = (OMX_DIO_Object *)pPort->hDIO;
    OMX_CHECK(hDIO != NULL, OMX_ErrorBadParameter);

    hDIO->bOpened = OMX_FALSE;
    eError =  hDIO->close(hDIO);

EXIT:
    return (eError);
}

/*
* OMX Base DIO Queue
*/
OMX_ERRORTYPE OMXBase_DIO_Queue (OMX_HANDLETYPE hComponent,
                                  OMX_U32 nPortIndex,
                                  OMX_PTR pBuffHeader)
{
    OMX_ERRORTYPE   eError = OMX_ErrorNone;
    OMX_DIO_Object  *hDIO = NULL;
    OMXBase_Port    *pPort = NULL;

    pPort = OMXBase_DIO_GetPort(hComponent, nPortIndex);
    OMX_CHECK(pPort != NULL, OMX_ErrorBadParameter);

    hDIO = (OMX_DIO_Object *)pPort->hDIO;
    OMX_CHECK(hDIO != NULL, OMX_ErrorBadParameter);

    eError = hDIO->queue(hDIO, pBuffHeader);

EXIT:
    return (eError);
}

/*
* OMXBase DIO Dequeue
*/
OMX_ERRORTYPE OMXBase_DIO_Dequeue (OMX_HANDLETYPE hComponent,
                                    OMX_U32 nPortIndex,
                                    OMX_PTR *pBuffHeader)
{
    OMX_ERRORTYPE   eError = OMX_ErrorNone;
    OMX_DIO_Object  *hDIO = NULL;
    OMXBase_Port    *pPort = NULL;

    pPort = OMXBase_DIO_GetPort(hComponent, nPortIndex);
    OMX_CHECK(pPort != NULL, OMX_ErrorBadParameter);

    hDIO = (OMX_DIO_Object *)pPort->hDIO;
    OMX_CHECK(hDIO != NULL, OMX_ErrorBadParameter);

    eError =  hDIO->dequeue(hDIO, pBuffHeader);

EXIT:
    return (eError);
}

/*
* OMXBase DIO Send
*/
OMX_ERRORTYPE OMXBase_DIO_Send (OMX_HANDLETYPE hComponent,
                                 OMX_U32 nPortIndex,
                                 OMX_PTR pBuffHeader)
{
    OMX_ERRORTYPE              eError = OMX_ErrorNone;
    OMX_DIO_Object   *hDIO = NULL;
    OMXBase_Port         *pPort = NULL;

    pPort = OMXBase_DIO_GetPort(hComponent, nPortIndex);
    OMX_CHECK(pPort != NULL, OMX_ErrorBadParameter);

    hDIO = (OMX_DIO_Object *)pPort->hDIO;
    OMX_CHECK(hDIO != NULL, OMX_ErrorBadParameter);

    eError = hDIO->send(hDIO, pBuffHeader);

EXIT:
    return (eError);
}

/*
* OMXBase DIO Cancel
*/
OMX_ERRORTYPE OMXBase_DIO_Cancel (OMX_HANDLETYPE hComponent,
                                   OMX_U32 nPortIndex,
                                   OMX_PTR pBuffHeader)
{
    OMX_ERRORTYPE   eError = OMX_ErrorNone;
    OMX_DIO_Object  *hDIO = NULL;
    OMXBase_Port    *pPort = NULL;

    pPort = OMXBase_DIO_GetPort(hComponent, nPortIndex);
    OMX_CHECK(pPort != NULL, OMX_ErrorBadParameter);

    hDIO = (OMX_DIO_Object *)pPort->hDIO;
    OMX_CHECK(hDIO != NULL, OMX_ErrorBadParameter);

    eError = hDIO->cancel(hDIO, pBuffHeader);

EXIT:
    return (eError);
}

/*
* OMXBase DIO Control
*/
OMX_ERRORTYPE OMXBase_DIO_Control (OMX_HANDLETYPE hComponent,
                                    OMX_U32 nPortIndex,
                                    OMX_DIO_CtrlCmdType nCmdType,
                                    OMX_PTR pParams)
{
    OMX_ERRORTYPE   eError = OMX_ErrorNone;
    OMX_DIO_Object  *hDIO = NULL;
    OMXBase_Port    *pPort = NULL;

    pPort = OMXBase_DIO_GetPort(hComponent, nPortIndex);
    OMX_CHECK(pPort != NULL, OMX_ErrorBadParameter);

    hDIO = (OMX_DIO_Object *)pPort->hDIO;
    OMX_CHECK(hDIO != NULL, OMX_ErrorBadParameter);

    eError = hDIO->control(hDIO, nCmdType, pParams);

EXIT:
    return (eError);
}

/*
* OMX Base DIO GetCount
*/
OMX_ERRORTYPE OMXBase_DIO_GetCount (OMX_HANDLETYPE hComponent,
                                     OMX_U32 nPortIndex,
                                     OMX_U32 *pCount)
{
    OMX_ERRORTYPE   eError = OMX_ErrorNone;
    OMX_DIO_Object  *hDIO = NULL;
    OMXBase_Port    *pPort = NULL;

    /*Resetting count to 0 initially*/
    *pCount = 0;
    pPort = OMXBase_DIO_GetPort(hComponent, nPortIndex);
    OMX_CHECK(pPort != NULL, OMX_ErrorBadParameter);

    hDIO = (OMX_DIO_Object *)pPort->hDIO;
    OMX_CHECK(hDIO != NULL, OMX_ErrorBadParameter);

    eError = hDIO->getcount(hDIO, pCount);

EXIT:
    return (eError);
}


/*
* OMX Base DIO GetPort from the PortIndex
*/
static OMX_PTR OMXBase_DIO_GetPort(OMX_HANDLETYPE hComponent, OMX_U32 nPortIndex)
{
    OMX_COMPONENTTYPE   *pComp = (OMX_COMPONENTTYPE *)hComponent;
    OMXBaseComp         *pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    OMXBase_Port        *pPort = NULL;
    OMX_U32             nStartPortNumber = 0;

    nStartPortNumber = pBaseComp->nMinStartPortIndex;
    if( pBaseComp->pPorts == NULL ) {
        pPort = NULL;
        goto EXIT;
    }
    pPort = (OMXBase_Port *)pBaseComp->pPorts[nPortIndex - nStartPortNumber];

EXIT:
    return (pPort);
}




