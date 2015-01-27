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

#define LOG_TAG "OMX_BASE"

#include <string.h>
#include <OMX_Core.h>
#include <OMX_Component.h>
#include <omx_base.h>
#include <OMX_TI_Custom.h>

#define HAL_NV12_PADDED_PIXEL_FORMAT (OMX_TI_COLOR_FormatYUV420PackedSemiPlanar - OMX_COLOR_FormatVendorStartUnused)


/**
* OMX BaseComponent Init
*/
OMX_ERRORTYPE OMXBase_ComponentInit(OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone, eTmpError = OMX_ErrorNone;
    OSAL_ERROR          tStatus = OSAL_ErrNone;
    OMX_COMPONENTTYPE   *pComp = NULL;
    OMXBaseComp         *pBaseComp = NULL;

    OMX_CHECK(hComponent != NULL, OMX_ErrorBadParameter);
    pComp = (OMX_COMPONENTTYPE *)hComponent;
    pBaseComp = (OMXBaseComp*)pComp->pComponentPrivate;
    OMX_CHECK(pBaseComp != NULL, OMX_ErrorBadParameter);

    /* populate component specific function pointers  */
    pComp->GetComponentVersion      =  OMXBase_GetComponentVersion;
    pComp->SendCommand              =  OMXBase_SendCommand;
    pComp->GetParameter             =  OMXBase_GetParameter;
    pComp->SetParameter             =  OMXBase_SetParameter;
    pComp->GetConfig                =  OMXBase_GetConfig;
    pComp->SetConfig                =  OMXBase_SetConfig;
    pComp->GetExtensionIndex        =  OMXBase_GetExtensionIndex;
    pComp->GetState                 =  OMXBase_GetState;
    pComp->ComponentTunnelRequest   =  NULL;
    pComp->UseBuffer                =  OMXBase_UseBuffer;
    pComp->AllocateBuffer           =  OMXBase_AllocateBuffer;
    pComp->FreeBuffer               =  OMXBase_FreeBuffer;
    pComp->EmptyThisBuffer          =  OMXBase_EmptyThisBuffer;
    pComp->FillThisBuffer           =  OMXBase_FillThisBuffer;
    pComp->SetCallbacks             =  OMXBase_SetCallbacks;
    pComp->ComponentDeInit          =  OMXBase_ComponentDeinit;
    pComp->UseEGLImage              =  OMXBase_UseEGLImage;
    pComp->ComponentRoleEnum        =  OMXBase_ComponentRoleEnum;

    pBaseComp->fpReturnEventNotify    =  OMXBase_CB_ReturnEventNotify;

    /* create a mutex to handle race conditions */
    tStatus = OSAL_CreateMutex(&(pBaseComp->pMutex));
    if(OSAL_ErrNone != tStatus) {
        return OMX_ErrorInsufficientResources;
    }

    /* Allocate internal area for Base component */
    pBaseComp->pPvtData = (OMXBaseComp_Pvt*)OSAL_Malloc(sizeof(OMXBaseComp_Pvt));
    OMX_CHECK(pBaseComp->pPvtData != NULL, OMX_ErrorInsufficientResources);

    OSAL_Memset(pBaseComp->pPvtData, 0, sizeof(OMXBaseComp_Pvt));
    eError = OMXBase_PrivateInit(hComponent);
    OMX_CHECK(OMX_ErrorNone == eError, eError);

    /* Initialize ports */
    eError = OMXBase_InitializePorts(hComponent);
    OMX_CHECK(OMX_ErrorNone == eError, eError);

    /* Component is initialized successfully, set the
    * component to loaded state */
    pBaseComp->tCurState =  OMX_StateLoaded;
    pBaseComp->tNewState =  OMX_StateMax;


EXIT:
    /* incase of an error, deinitialize the component */
    if((OMX_ErrorNone != eError) && (pComp != NULL)) {
        eTmpError = eError;
        eError = pComp->ComponentDeInit(hComponent);
        eError = eTmpError;
    }
    return (eError);
}

/*
* SetCallbacks
*/
OMX_ERRORTYPE OMXBase_SetCallbacks(OMX_HANDLETYPE hComponent,
                                    OMX_CALLBACKTYPE *pCallbacks,
                                    OMX_PTR pAppData)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE   *pComp = NULL;
    OMXBaseComp         *pBaseComp = NULL;
    OMXBaseComp_Pvt     *pBaseCompPvt = NULL;
    OMX_U32             i = 0;

    OMX_CHECK((hComponent != NULL) &&
             (pCallbacks != NULL), OMX_ErrorBadParameter);

    pComp = (OMX_COMPONENTTYPE *)hComponent;
    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;

    pComp->pApplicationPrivate = pAppData;
    pBaseCompPvt->sAppCallbacks = *pCallbacks;

    for( i = 0; i < pBaseComp->nNumPorts; i++ ) {
        pBaseComp->pPorts[i]->bIsBufferAllocator = OMX_FALSE;
    }

EXIT:
    return (eError);
}

/*
* GetComponent Version
*/
OMX_ERRORTYPE OMXBase_GetComponentVersion(OMX_HANDLETYPE hComponent,
                                           OMX_STRING pComponentName,
                                           OMX_VERSIONTYPE *pComponentVersion,
                                           OMX_VERSIONTYPE *pSpecVersion,
                                           OMX_UUIDTYPE *pComponentUUID)
{
    OMX_ERRORTYPE       eError  = OMX_ErrorNone;
    OMX_COMPONENTTYPE   *pComp = NULL;
    OMXBaseComp         *pBaseComp = NULL;

    OMX_CHECK((hComponent != NULL) &&
                (pComponentName != NULL) &&
                (pComponentVersion != NULL) &&
                (pSpecVersion != NULL), OMX_ErrorBadParameter);

    pComp = (OMX_COMPONENTTYPE *)hComponent;
    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;

    /*Can't be invoked when the comp is in invalid state*/
    OMX_CHECK(OMX_StateInvalid != pBaseComp->tCurState, OMX_ErrorInvalidState);

    OSAL_Memcpy(pComponentName, pBaseComp->cComponentName, OMX_MAX_STRINGNAME_SIZE);

    *pComponentVersion = pBaseComp->nComponentVersion;
    *pSpecVersion      = pComp->nVersion;

EXIT:
    return (eError);
}

/*
* GetState
*/
OMX_ERRORTYPE OMXBase_GetState(OMX_HANDLETYPE hComponent,
                                OMX_STATETYPE *pState)
{
    OMX_ERRORTYPE           eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE      *pComp = NULL;
    OMXBaseComp   *pBaseComp = NULL;

    OMX_CHECK((hComponent != NULL) &&
                (pState != NULL), OMX_ErrorBadParameter);

    pComp = (OMX_COMPONENTTYPE *)hComponent;
    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;

    *pState = pBaseComp->tCurState;

EXIT:
    return (eError);
}

/*
* Base Component DeInit
*/
OMX_ERRORTYPE OMXBase_ComponentDeinit(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE       eError  = OMX_ErrorNone, eTmpError = OMX_ErrorNone;
    OSAL_ERROR          tStatus = OSAL_ErrNone;
    OMX_COMPONENTTYPE   *pComp = NULL;
    OMXBaseComp         *pBaseComp = NULL;
    OMXBaseComp_Pvt     *pBaseCompPvt = NULL;
    OMXBase_Port        *pPort = NULL;
    OMX_U32             i = 0;

    OMX_CHECK(hComponent != NULL, OMX_ErrorBadParameter);
    pComp = (OMX_COMPONENTTYPE *)hComponent;
    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    OMX_CHECK(pBaseComp != NULL, OMX_ErrorBadParameter);
    pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;

    OSAL_ObtainMutex(pBaseComp->pMutex, OSAL_SUSPEND);

    /*If component transitioned to invalid state suddenly then dio close and
    deinit might have not been called - in that case calle them now */
    if( pBaseComp->tCurState != OMX_StateLoaded ) {
        for( i=0; i < pBaseComp->nNumPorts; i++ ) {
            pPort = pBaseComp->pPorts[i];
            if( pPort != NULL ) {
                if( pPort->hDIO != NULL ) {
                    eTmpError = OMXBase_DIO_Close(hComponent,
                                       (i + pBaseComp->nMinStartPortIndex));
                    if( eTmpError != OMX_ErrorNone ) {
                        eError = eTmpError;
                    }
                    eTmpError = OMXBase_DIO_Deinit(hComponent,
                                (i + pBaseComp->nMinStartPortIndex));
                    if( eTmpError != OMX_ErrorNone ) {
                        eError = eTmpError;
                    }
                }
            }
        }
    }

    /* deinitialize ports and freeup the memory */
    eTmpError = OMXBase_DeinitializePorts(hComponent);
    if( eTmpError != OMX_ErrorNone ) {
        eError = eTmpError;
    }

    if (pBaseCompPvt) {
        OMXBase_PrivateDeInit(hComponent);
    }

    OSAL_ReleaseMutex(pBaseComp->pMutex);
    tStatus = OSAL_DeleteMutex(pBaseComp->pMutex);
    if( tStatus != OSAL_ErrNone ) {
        eError = OMX_ErrorUndefined;
    }
    pBaseComp->pMutex = NULL;

    OSAL_Free(pBaseCompPvt);
    pBaseCompPvt = NULL;

EXIT:
    return (eError);
}

/*
* OMXBase SendCommand
*/
OMX_ERRORTYPE OMXBase_SendCommand(OMX_HANDLETYPE hComponent,
                                   OMX_COMMANDTYPE Cmd,
                                   OMX_U32 nParam1,
                                   OMX_PTR pCmdData)
{
    OMX_ERRORTYPE       eError  = OMX_ErrorNone;
    OSAL_ERROR          tStatus = OSAL_ErrNone;
    OMX_COMPONENTTYPE   *pComp = NULL;
    OMXBaseComp         *pBaseComp = NULL;
    OMXBaseComp_Pvt     *pBaseCompPvt = NULL;
    OMXBase_Port        *pPort = NULL;
    OMXBase_CmdParams   sCmdParams, sErrorCmdParams;
    OMX_U32             nPorts, nStartPortNumber, nIndex;
    OMX_PTR             pLocalCmdData = NULL;
    OMX_BOOL            bFreeCmdDataIfError = OMX_TRUE;
    OMX_U32             i = 0;
    uint32_t            nActualSize = 0, nCmdCount = 0;

    OMX_CHECK(hComponent != NULL, OMX_ErrorBadParameter);
    if( OMX_CommandMarkBuffer == Cmd ) {
        OMX_CHECK(pCmdData != NULL, OMX_ErrorBadParameter);
    }
    pComp = (OMX_COMPONENTTYPE *)hComponent;
    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;

    nPorts = pBaseComp->nNumPorts;
    nStartPortNumber = pBaseComp->nMinStartPortIndex;

    /*Can't be invoked when the comp is in invalid state*/
    OMX_CHECK(OMX_StateInvalid != pBaseComp->tCurState, OMX_ErrorInvalidState);

    switch( Cmd ) {
        case OMX_CommandStateSet :
            /*Return error if unknown state is provided*/
            OMX_CHECK(((OMX_STATETYPE)nParam1 <=
            OMX_StateWaitForResources), OMX_ErrorBadParameter);

            /*Mutex protection is for multiple SendCommands on the same
            component parallely. This can especially happen in error handling
            scenarios. Mutex protection ensure that the NewState variable is not
            overwritten.*/
            OSAL_ObtainMutex(pBaseCompPvt->pNewStateMutex,
            OSAL_SUSPEND);
            /*Multiple state transitions at the same time is not allowed.
            Though it is allowed by the spec, we prohibit it in our
            implementation*/
            if( OMX_StateMax != pBaseComp->tNewState ) {
                eError = OMX_ErrorIncorrectStateTransition;
                OSAL_ReleaseMutex(pBaseCompPvt->pNewStateMutex);
                goto EXIT;
            }
            pBaseComp->tNewState = (OMX_STATETYPE)nParam1;
            OSAL_ReleaseMutex(pBaseCompPvt->pNewStateMutex);
        break;

        case OMX_CommandPortDisable :
            /* Index of the port to disable should be less than
            * no of ports or equal to OMX_ALL */
            OMX_CHECK((nParam1 < (nStartPortNumber + nPorts)) ||
                    (nParam1 == OMX_ALL), OMX_ErrorBadPortIndex);
            if( OMX_ALL == nParam1 ) {
                /*Dont want to return same port state error if it never enters
                the for loop*/
                if( nPorts > 0 ) {
                    eError = (OMX_ERRORTYPE)OMX_ErrorNone;
                }

                for( nIndex = 0; nIndex < nPorts; nIndex++ ) {
                    pPort = pBaseComp->pPorts[nIndex];
                    /*Atleast 1 port is enabled - dont send same port state error*/
                    if( pPort->sPortDef.bEnabled ) {
                        pPort->sPortDef.bEnabled = OMX_FALSE;
                        pPort->bIsInTransition = OMX_TRUE;
                        eError = OMX_ErrorNone;
                    }
                }
            } else {
                nIndex = nParam1 - nStartPortNumber;
                pPort = pBaseComp->pPorts[nIndex];
                if( pPort->sPortDef.bEnabled ) {
                    pPort->sPortDef.bEnabled = OMX_FALSE;
                    pPort->bIsInTransition = OMX_TRUE;
                } else {
                    eError = (OMX_ERRORTYPE)OMX_ErrorNone;
                }
            }
        break;

        case OMX_CommandPortEnable :
            /* Index of the port to enable should be less than
            * no of ports or equal to OMX_ALL */
            OMX_CHECK((nParam1 < (nStartPortNumber + nPorts)) ||
                (nParam1 == OMX_ALL), OMX_ErrorBadPortIndex);
            if( OMX_ALL == nParam1 ) {
                /*Dont want to return same port state error if it never enters
                the for loop*/
                if( nPorts > 0 ) {
                    eError = (OMX_ERRORTYPE)OMX_ErrorNone;
                }

                for( nIndex = 0; nIndex < nPorts; nIndex++ ) {
                    pPort = pBaseComp->pPorts[nIndex];
                    /*Atleast 1 port is disabled - dont send same port state error*/
                    if( !pPort->sPortDef.bEnabled ) {
                        eError = OMX_ErrorNone;
                        pPort->sPortDef.bEnabled = OMX_TRUE;
                        pPort->bIsInTransition = OMX_TRUE;
                    }
                }
            } else {
                nIndex = nParam1 - nStartPortNumber;
                pPort = pBaseComp->pPorts[nIndex];
                if( !pPort->sPortDef.bEnabled ) {
                    pPort->sPortDef.bEnabled = OMX_TRUE;
                    pPort->bIsInTransition = OMX_TRUE;
                } else {
                    eError = (OMX_ERRORTYPE)OMX_ErrorNone;
                }
            }
        break;

        case OMX_CommandMarkBuffer :
            /*For mark buffer pCmdData points to a structure of type OMX_MARKTYPE.
            This may not be valid once send commmand returns so allocate memory and
            copy this info there. This memory will be freed up during the command
            complete callback for mark buffer.*/
            OMX_CHECK((nParam1 < (nStartPortNumber + nPorts)) ||
                (nParam1 == OMX_ALL), OMX_ErrorBadPortIndex);
            pLocalCmdData = OSAL_Malloc(sizeof(OMX_MARKTYPE));
            OMX_CHECK(pLocalCmdData != NULL, OMX_ErrorInsufficientResources);
            OSAL_Memcpy(pLocalCmdData, pCmdData, sizeof(OMX_MARKTYPE));
        break;

        case OMX_CommandFlush :
            OMX_CHECK((nParam1 < (nStartPortNumber + nPorts)) ||
                        (nParam1 == OMX_ALL), OMX_ErrorBadPortIndex);
        break;

        default :
            OMX_CHECK(OMX_FALSE, OMX_ErrorBadParameter);
    }

    /*Return error if port enable/disable command is sent on an already
    enabled/disabled port*/
    OMX_CHECK(eError == OMX_ErrorNone, eError);
    /*For mark buffer store pCmdData in a separate pipe - to be freed up during
    command complete callback*/
    if( Cmd == OMX_CommandMarkBuffer ) {
        /*If pipe is full, return error - thus a limitation that currently
        cannot queue up more than OMXBase_MAXCMDS mark buffer commands*/
        tStatus = OSAL_WriteToPipe(pBaseCompPvt->pCmdDataPipe,
                                &pLocalCmdData, sizeof(OMX_PTR), OSAL_NO_SUSPEND);
        OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorInsufficientResources);
    }
    /*Obtain mutex for writing to command pipe*/
    tStatus = OSAL_ObtainMutex(pBaseCompPvt->pCmdPipeMutex, OSAL_SUSPEND);
    if((tStatus != OSAL_ErrNone) && (Cmd == OMX_CommandMarkBuffer)) {
        bFreeCmdDataIfError = OMX_FALSE;
    }
    OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorUndefined);

    /* keep this info, and process later */
    sCmdParams.eCmd     = Cmd;
    sCmdParams.unParam  = nParam1;
    if( Cmd == OMX_CommandMarkBuffer ) {
        sCmdParams.pCmdData = pLocalCmdData;
    } else {
        sCmdParams.pCmdData = pCmdData;
    }
    tStatus = OSAL_WriteToPipe(pBaseCompPvt->pCmdPipe, &sCmdParams,
    sizeof(sCmdParams), OSAL_SUSPEND);
    if( tStatus != OSAL_ErrNone ) {
        /*Do not free pLocalCmdData in this case since it has been pushed to
        pipe and will now be freed during deinit when pipe is deleted*/
        if( Cmd == OMX_CommandMarkBuffer ) {
            bFreeCmdDataIfError = OMX_FALSE;
        }
        /*Release the locked mutex and exit with error*/
        eError = OMX_ErrorInsufficientResources;
        tStatus = OSAL_ReleaseMutex(pBaseCompPvt->pCmdPipeMutex);
        goto EXIT;
    }
    /* This call invokes the process function directly incase if comp
    * does process in client context, otherwise triggers compo thread */
    eError = pBaseCompPvt->fpInvokeProcessFunction(hComponent, CMDEVENT);
    if( eError != OMX_ErrorNone ) {
        if( Cmd == OMX_CommandMarkBuffer ) {
            /*Do not free pLocalCmdData in this case since it has been pushed to
            pipe and will now be freed during deinit when pipe is deleted*/
            bFreeCmdDataIfError = OMX_FALSE;
        }
        /*Get the count in cmd pipe - this is to be used for popping the
        recently added cmd to the pipe since that is no longer valid*/
        tStatus = OSAL_GetPipeReadyMessageCount(pBaseCompPvt->pCmdPipe,
                                            &nCmdCount);
        if( tStatus != OSAL_ErrNone ) {
            /*Release mutex and return error*/
            eError = OMX_ErrorUndefined;
            tStatus = OSAL_ReleaseMutex(pBaseCompPvt->pCmdPipeMutex);
            goto EXIT;
        }

        for( i = 0; i < nCmdCount; i++ ) {
            /*Clear the last command from pipe since error has occured*/
            tStatus = OSAL_ReadFromPipe(pBaseCompPvt->pCmdPipe, &sErrorCmdParams,
                                        sizeof(sErrorCmdParams), &nActualSize,
                                        OSAL_SUSPEND);
            if( tStatus != OSAL_ErrNone ) {
                eError = OMX_ErrorUndefined;
                break;
            }
            if( OSAL_Memcmp(&sErrorCmdParams, &sCmdParams, sizeof(OMXBase_CmdParams)) == 0 ) {
                OSAL_ErrorTrace("Found the command to discard");
                break;
            } else {
                /*This is not the command to be discarded - write it back to
                pipe*/
                tStatus = OSAL_WriteToPipe(pBaseCompPvt->pCmdPipe,
                                        &sErrorCmdParams, sizeof(sErrorCmdParams),
                                        OSAL_SUSPEND);
                if( tStatus != OSAL_ErrNone ) {
                    OSAL_ErrorTrace("Write to pipe failed");
                    eError = OMX_ErrorUndefined;
                    break;
                }
            }
        }

        if( i == nCmdCount ) {
            /*The command to discard was not found even after going through the
            pipe*/
            OSAL_ErrorTrace("Command to be discarded not found in pipe");
            eError = OMX_ErrorUndefined;
        }
    }
    tStatus = OSAL_ReleaseMutex(pBaseCompPvt->pCmdPipeMutex);
    OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorUndefined);

EXIT:
    if( eError != OMX_ErrorNone ) {
        if( pLocalCmdData && bFreeCmdDataIfError ) {
            OSAL_Free(pLocalCmdData);
            pLocalCmdData = NULL;
        }
    }
    return (eError);
}

/*
* OMX Base Get Parameter
*/
OMX_ERRORTYPE OMXBase_GetParameter(OMX_HANDLETYPE hComponent,
                                    OMX_INDEXTYPE nParamIndex,
                                    OMX_PTR pParamStruct)
{
    OMX_ERRORTYPE           eError  = OMX_ErrorNone;
    OMX_COMPONENTTYPE       *pComp = NULL;
    OMXBaseComp             *pBaseComp = NULL;
    OMXBaseComp_Pvt         *pBaseCompPvt = NULL;
    OMXBase_Port            *pPort = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE    *pPortDef = NULL;
    OMX_PRIORITYMGMTTYPE            *pPriorityMgmt = NULL;
    OMX_PARAM_BUFFERSUPPLIERTYPE    *pBufSupplier = NULL;
    OMX_U32                         nStartPortNumber, nPorts, nPortIndex;


    OMX_CHECK((hComponent != NULL) && (pParamStruct != NULL), OMX_ErrorBadParameter);

    pComp = (OMX_COMPONENTTYPE *)hComponent;
    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;

    nPorts = pBaseComp->nNumPorts;
    nStartPortNumber = pBaseComp->nMinStartPortIndex;

    /*Can't be invoked when the comp is in invalid state*/
    OMX_CHECK(OMX_StateInvalid != pBaseComp->tCurState, OMX_ErrorInvalidState);

    switch( nParamIndex ) {
        case OMX_IndexParamAudioInit :
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_PORT_PARAM_TYPE, eError);
            if( pBaseComp->pAudioPortParams == NULL ) {
                OMX_BASE_INIT_STRUCT_PTR((OMX_PORT_PARAM_TYPE *)(pParamStruct),
                                        OMX_PORT_PARAM_TYPE);
                ((OMX_PORT_PARAM_TYPE *)pParamStruct)->nPorts           = 0;
                ((OMX_PORT_PARAM_TYPE *)pParamStruct)->nStartPortNumber = 0;
                break;
            }
            *(OMX_PORT_PARAM_TYPE *)pParamStruct = *(pBaseComp->pAudioPortParams);
        break;

        case OMX_IndexParamImageInit :
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_PORT_PARAM_TYPE, eError);
            if( pBaseComp->pImagePortParams == NULL ) {
                OMX_BASE_INIT_STRUCT_PTR((OMX_PORT_PARAM_TYPE *)(pParamStruct),
                                        OMX_PORT_PARAM_TYPE);
                ((OMX_PORT_PARAM_TYPE *)pParamStruct)->nPorts           = 0;
                ((OMX_PORT_PARAM_TYPE *)pParamStruct)->nStartPortNumber = 0;
                break;
            }
            *(OMX_PORT_PARAM_TYPE *)pParamStruct = *(pBaseComp->pImagePortParams);
        break;

        case OMX_IndexParamVideoInit :
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_PORT_PARAM_TYPE, eError);
            if( pBaseComp->pVideoPortParams == NULL ) {
                OMX_BASE_INIT_STRUCT_PTR((OMX_PORT_PARAM_TYPE *)(pParamStruct),
                                        OMX_PORT_PARAM_TYPE);
                ((OMX_PORT_PARAM_TYPE *)pParamStruct)->nPorts           = 0;
                ((OMX_PORT_PARAM_TYPE *)pParamStruct)->nStartPortNumber = 0;
                break;
            }
            *(OMX_PORT_PARAM_TYPE *)pParamStruct =
            *(pBaseComp->pVideoPortParams);
        break;

        case OMX_IndexParamOtherInit :
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_PORT_PARAM_TYPE, eError);
            if( pBaseComp->pOtherPortParams == NULL ) {
                OMX_BASE_INIT_STRUCT_PTR((OMX_PORT_PARAM_TYPE *)(pParamStruct),
                                        OMX_PORT_PARAM_TYPE);
                ((OMX_PORT_PARAM_TYPE *)pParamStruct)->nPorts           = 0;
                ((OMX_PORT_PARAM_TYPE *)pParamStruct)->nStartPortNumber = 0;
                break;
            }
            *(OMX_PORT_PARAM_TYPE *)pParamStruct = *(pBaseComp->pOtherPortParams);
        break;

        case OMX_IndexParamPortDefinition :
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_PARAM_PORTDEFINITIONTYPE,
            eError);
            pPortDef = (OMX_PARAM_PORTDEFINITIONTYPE *)pParamStruct;
            nPortIndex = pPortDef->nPortIndex - nStartPortNumber;
            /* check for valid port index */
            OMX_CHECK(nPortIndex < nPorts,
            OMX_ErrorBadPortIndex);
            *pPortDef = pBaseComp->pPorts[nPortIndex]->sPortDef;
            if (pPortDef->eDir == OMX_DirOutput &&
                    pBaseComp->pPorts[nPortIndex]->sProps.eBufMemoryType == MEM_GRALLOC) {
                pPortDef->format.video.eColorFormat = HAL_NV12_PADDED_PIXEL_FORMAT;
            }
        break;

        case OMX_IndexParamCompBufferSupplier :
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_PARAM_BUFFERSUPPLIERTYPE, eError);
            pBufSupplier = (OMX_PARAM_BUFFERSUPPLIERTYPE *)pParamStruct;
            nPortIndex = pBufSupplier->nPortIndex - nStartPortNumber;
            /* check for valid port index */
            OMX_CHECK(nPortIndex < nPorts, OMX_ErrorBadPortIndex);
            pPort = pBaseComp->pPorts[nPortIndex];
            pBufSupplier->eBufferSupplier = 0x0;
        break;

        case OMX_IndexParamPriorityMgmt :
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_PRIORITYMGMTTYPE, eError);
            pPriorityMgmt = (OMX_PRIORITYMGMTTYPE *)pParamStruct;
        break;

        default :
            OSAL_ErrorTrace("Unknown Index received ");
            eError = OMX_ErrorUnsupportedIndex;
        break;
    }

EXIT:
    return (eError);
}

/*
* OMX Base SetParameter
*/
OMX_ERRORTYPE OMXBase_SetParameter(OMX_HANDLETYPE hComponent,
                                    OMX_INDEXTYPE nIndex,
                                    OMX_PTR pParamStruct)
{
    OMX_ERRORTYPE               eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE           *pComp = NULL;
    OMXBaseComp                 *pBaseComp = NULL;
    OMXBaseComp_Pvt             *pBaseCompPvt = NULL;
    OMXBase_Port                *pPort = NULL;
    OMX_PRIORITYMGMTTYPE        *pPriorityMgmt = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE    *pPortDef = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE    *pLocalPortDef;
    OMX_PARAM_BUFFERSUPPLIERTYPE    *pBufSupplier = NULL;
    OMX_U32                         nStartPortNumber, nPorts, nPortIndex;
    OMX_PARAM_BUFFERSUPPLIERTYPE    sTunBufSupplier;
    OMX_TI_PARAMUSENATIVEBUFFER     *pParamNativeBuffer = NULL;

    OMX_CHECK((hComponent != NULL) &&
                (pParamStruct != NULL), OMX_ErrorBadParameter);

    pComp = (OMX_COMPONENTTYPE *)hComponent;
    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;
    nPorts = pBaseComp->nNumPorts;
    nStartPortNumber = pBaseComp->nMinStartPortIndex;

    /*Can't be invoked when the comp is in invalid state*/
    OMX_CHECK(OMX_StateInvalid != pBaseComp->tCurState,
                OMX_ErrorInvalidState);
    /* This method is not allowed when the component is not in the loaded
    * state or the the port is not disabled */
    if((OMX_IndexParamPriorityMgmt == nIndex
        || OMX_IndexParamAudioInit == nIndex
        || OMX_IndexParamVideoInit == nIndex
        || OMX_IndexParamImageInit == nIndex
        || OMX_IndexParamOtherInit == nIndex)
        && (pBaseComp->tCurState != OMX_StateLoaded)) {
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    switch( nIndex ) {
        case OMX_IndexParamPriorityMgmt :
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_PRIORITYMGMTTYPE, eError);
            pPriorityMgmt = (OMX_PRIORITYMGMTTYPE *)pParamStruct;
        break;

        case OMX_IndexParamPortDefinition :
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_PARAM_PORTDEFINITIONTYPE,
                                eError);
            pPortDef = (OMX_PARAM_PORTDEFINITIONTYPE *)pParamStruct;

            nPortIndex = pPortDef->nPortIndex - nStartPortNumber;
            /* check for valid port index */
            OMX_CHECK(nPortIndex < nPorts, OMX_ErrorBadPortIndex);
            pPort = pBaseComp->pPorts[nPortIndex];
            /* successfully only when the comp is in loaded or disabled port */
            OMX_CHECK((pBaseComp->tCurState == OMX_StateLoaded) ||
                        (pPort->sPortDef.bEnabled == OMX_FALSE),
                         OMX_ErrorIncorrectStateOperation);

            pLocalPortDef = (OMX_PARAM_PORTDEFINITIONTYPE *)pParamStruct;
            /*Copying only the modifiabke fields. Rest are all read only fields*/
            pBaseComp->pPorts[nPortIndex]->sPortDef.nBufferCountActual =
                                                    pLocalPortDef->nBufferCountActual;
            pBaseComp->pPorts[nPortIndex]->sPortDef.format = pLocalPortDef->format;
        break;

        /*These are compulsory parameters hence being supported by base*/
        case OMX_IndexParamAudioInit :
        case OMX_IndexParamVideoInit :
        case OMX_IndexParamImageInit :
        case OMX_IndexParamOtherInit :
            /*All fields of OMX_PORT_PARAM_TYPE are read only so SetParam will just
            return and not overwrite anything*/
            OMX_BASE_CHK_VERSION(pParamStruct, OMX_PORT_PARAM_TYPE, eError);
        break;

        case OMX_TI_IndexUseNativeBuffers:
            pParamNativeBuffer = (OMX_TI_PARAMUSENATIVEBUFFER* )pParamStruct;
            if(pParamNativeBuffer->bEnable == OMX_TRUE) {
                pBaseComp->pPorts[pParamNativeBuffer->nPortIndex - nStartPortNumber]->sProps.eBufMemoryType = MEM_GRALLOC;
            } else {
                pBaseComp->pPorts[pParamNativeBuffer->nPortIndex - nStartPortNumber]->sProps.eBufMemoryType = MEM_CARVEOUT;
            }
        break;

        default :
            eError = OMX_ErrorUnsupportedIndex;
        break;
        }

EXIT:
    return (eError);
}

/*
* OMX Base AllocatBuffer
*/
OMX_ERRORTYPE OMXBase_AllocateBuffer(OMX_HANDLETYPE hComponent,
                                      OMX_BUFFERHEADERTYPE * *ppBufferHdr,
                                      OMX_U32 nPortIndex,
                                      OMX_PTR pAppPrivate,
                                      OMX_U32 nSizeBytes)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OSAL_ERROR          tStatus = OSAL_ErrNone;
    OMX_COMPONENTTYPE   *pComp = NULL;
    OMXBaseComp         *pBaseComp = NULL;
    OMXBaseComp_Pvt     *pBaseCompPvt = NULL;
    OMXBase_Port        *pPort = NULL;
    OMX_DIO_CreateParams    sDIOCreateParams;
    OMX_DIO_OpenParams  sDIOOpenParams;
    OMX_U32             nStartPortNumber = 0, nPorts = 0;

    OMX_CHECK((hComponent != NULL) && (nSizeBytes > 0), OMX_ErrorBadParameter);

    pComp = (OMX_COMPONENTTYPE *)hComponent;
    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;
    nStartPortNumber = pBaseComp->nMinStartPortIndex;
    nPorts = pBaseComp->nNumPorts;
    OMX_CHECK(nPortIndex >= nStartPortNumber && nPortIndex < nPorts,
                                        OMX_ErrorBadPortIndex);
    /*Can't be invoked when the comp is in invalid state*/
    OMX_CHECK(OMX_StateInvalid != pBaseComp->tCurState,
                                OMX_ErrorInvalidState);

    pPort = pBaseComp->pPorts[nPortIndex - nStartPortNumber];

    /*Buffer size should be >= the minimum buffer size specified in
    port definition*/
    OMX_CHECK(nSizeBytes >= pPort->sPortDef.nBufferSize,
                                OMX_ErrorBadParameter);
    if((pBaseComp->tCurState == OMX_StateLoaded ||
            pBaseComp->tCurState == OMX_StateWaitForResources) &&
            (pBaseComp->tNewState == OMX_StateIdle) &&
            (pPort->sPortDef.bEnabled)) {
        /*Allowed during loaded/waitforresources --> idle transition if port is
        enabled*/
    } else if((pPort->bIsInTransition) &&
            (pBaseComp->tCurState != OMX_StateLoaded) &&
            (pBaseComp->tCurState != OMX_StateWaitForResources)) {
        /*Allowed when port is transitioning to enabled if current state is not
        loaded/waitforresources*/
    } else {
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }
    /*Port should not be already populated*/
    OMX_CHECK(pPort->sPortDef.bPopulated == OMX_FALSE,
    OMX_ErrorBadParameter);
    if( pPort->nBufferCnt == 0 ) {
        /* Initialize DIO and open as a supplier  */
        pPort->bIsBufferAllocator          = OMX_TRUE;
        sDIOCreateParams.hComponent        = hComponent;
        sDIOCreateParams.nPortIndex        = pPort->sPortDef.nPortIndex;
        sDIOCreateParams.pAppCallbacks     = &(pBaseCompPvt->sAppCallbacks);
        if( pPort->sPortDef.eDir == OMX_DirOutput ) {
            sDIOOpenParams.nMode = OMX_DIO_WRITER;
        } else {
            sDIOOpenParams.nMode = OMX_DIO_READER;
        }

        sDIOOpenParams.nBufSize = nSizeBytes;
        eError = OMXBase_DIO_Init(hComponent, nPortIndex, "OMX.DIO.NONTUNNEL",
                                &sDIOCreateParams);
        OMX_CHECK(eError == OMX_ErrorNone, eError);
        eError = OMXBase_DIO_Open(hComponent, nPortIndex, &sDIOOpenParams);
        OMX_CHECK(eError == OMX_ErrorNone, eError);
    }
    /*update buffer header from buffer list  */
    *ppBufferHdr = pPort->pBufferlist[pPort->nBufferCnt];
    if( pPort->sPortDef.eDir == OMX_DirInput ) {
        (*ppBufferHdr)->nInputPortIndex  = pPort->sPortDef.nPortIndex;
        (*ppBufferHdr)->nOutputPortIndex = OMX_NOPORT;
    } else if( pPort->sPortDef.eDir == OMX_DirOutput ) {
        (*ppBufferHdr)->nOutputPortIndex  = pPort->sPortDef.nPortIndex;
        (*ppBufferHdr)->nInputPortIndex   = OMX_NOPORT;
    }
    (*ppBufferHdr)->pAppPrivate = pAppPrivate;
    (*ppBufferHdr)->nAllocLen = nSizeBytes;

    pPort->nBufferCnt++;
    if( pPort->sPortDef.nBufferCountActual == pPort->nBufferCnt ) {
        pPort->sPortDef.bPopulated = OMX_TRUE;
        tStatus = OSAL_SetEvent(pPort->pBufAllocFreeEvent, BUF_ALLOC_EVENT,
                                                            OSAL_EVENT_OR);
        OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorInsufficientResources);
    }

EXIT:
    return (eError);
}

/*
* OMX Base UseBuffer
*/
OMX_ERRORTYPE OMXBase_UseBuffer(OMX_HANDLETYPE hComponent,
                                 OMX_BUFFERHEADERTYPE * *ppBufferHdr,
                                 OMX_U32 nPortIndex,
                                 OMX_PTR pAppPrivate,
                                 OMX_U32 nSizeBytes,
                                 OMX_U8 *pBuffer)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OSAL_ERROR          tStatus = OSAL_ErrNone;
    OMX_COMPONENTTYPE   *pComp = NULL;
    OMXBaseComp         *pBaseComp = NULL;
    OMXBaseComp_Pvt     *pBaseCompPvt = NULL;
    OMXBase_Port        *pPort = NULL;
    OMX_DIO_CreateParams    sDIOCreateParams;
    OMX_DIO_OpenParams      sDIOOpenParams;
    OMX_U32                 nStartPortNumber = 0, nPorts = 0;

    OMX_CHECK((hComponent != NULL), OMX_ErrorBadParameter);

    pComp = (OMX_COMPONENTTYPE *)hComponent;
    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;
    nStartPortNumber = pBaseComp->nMinStartPortIndex;
    nPorts = pBaseComp->nNumPorts;
    OMX_CHECK(nPortIndex >= nStartPortNumber && nPortIndex < nPorts,
                                    OMX_ErrorBadPortIndex);
    /*Can't be invoked when the comp is in invalid state*/
    OMX_CHECK(OMX_StateInvalid != pBaseComp->tCurState,
                                OMX_ErrorInvalidState);

    pPort = pBaseComp->pPorts[nPortIndex - nStartPortNumber];

    if((pBaseComp->tCurState == OMX_StateLoaded ||
            pBaseComp->tCurState == OMX_StateWaitForResources) &&
            (pBaseComp->tNewState == OMX_StateIdle) &&
            (pPort->sPortDef.bEnabled)) {
        /*Allowed during loaded/waitforresources --> idle transition if port is
        enabled*/
    } else if((pPort->bIsInTransition) &&
            (pBaseComp->tCurState != OMX_StateLoaded) &&
            (pBaseComp->tCurState != OMX_StateWaitForResources)) {
        /*Allowed when port is transitioning to enabled if current state is not
        loaded/waitforresources*/
    } else {
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }
    OMX_CHECK(pPort->sPortDef.bPopulated == OMX_FALSE,
                    OMX_ErrorBadParameter);

    if( pPort->nBufferCnt == 0 ) {
        /* Initialize DIO if not initialized and open as a non supplier  */
        pPort->bIsBufferAllocator = OMX_FALSE;
        if( pPort->hDIO == NULL) {
            sDIOCreateParams.hComponent        = hComponent;
            sDIOCreateParams.nPortIndex         = pPort->sPortDef.nPortIndex;
            sDIOCreateParams.pAppCallbacks     = &(pBaseCompPvt->sAppCallbacks);

            if( pPort->sPortDef.eDir == OMX_DirOutput ) {
                sDIOOpenParams.nMode = OMX_DIO_WRITER;
            } else {
                sDIOOpenParams.nMode = OMX_DIO_READER;
            }

            sDIOOpenParams.nBufSize = nSizeBytes;
            eError = OMXBase_DIO_Init(hComponent, nPortIndex,
                                        "OMX.DIO.NONTUNNEL", &sDIOCreateParams);
            OMX_CHECK(eError == OMX_ErrorNone, eError);
            eError = OMXBase_DIO_Open(hComponent, nPortIndex, &sDIOOpenParams);
            OMX_CHECK(eError == OMX_ErrorNone, eError);
        }
    }
    /*update buffer header from buffer list  */
    pPort->pBufferlist[pPort->nBufferCnt]->pBuffer = pBuffer;
    *ppBufferHdr = pPort->pBufferlist[pPort->nBufferCnt];
    if( pPort->sPortDef.eDir == OMX_DirInput ) {
        (*ppBufferHdr)->nInputPortIndex  = pPort->sPortDef.nPortIndex;
        (*ppBufferHdr)->nOutputPortIndex = OMX_NOPORT;
    } else if( pPort->sPortDef.eDir == OMX_DirOutput ) {
        (*ppBufferHdr)->nOutputPortIndex  = pPort->sPortDef.nPortIndex;
        (*ppBufferHdr)->nInputPortIndex   = OMX_NOPORT;
    }
    (*ppBufferHdr)->pAppPrivate = pAppPrivate;
    (*ppBufferHdr)->nAllocLen = nSizeBytes;

    if (pBaseComp->fpXlateBuffHandle != NULL) {
        eError = pBaseComp->fpXlateBuffHandle(hComponent, (OMX_PTR)(*ppBufferHdr), OMX_TRUE);
    }

    pPort->nBufferCnt++;
    if( pPort->sPortDef.nBufferCountActual == pPort->nBufferCnt ) {
        pPort->sPortDef.bPopulated = OMX_TRUE;
        tStatus = OSAL_SetEvent(pPort->pBufAllocFreeEvent, BUF_ALLOC_EVENT,
                                                        OSAL_EVENT_OR);
        OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorInsufficientResources);
    }
EXIT:
    return (eError);
}

/*
* OMX Base FreeBuffer
*/
OMX_ERRORTYPE OMXBase_FreeBuffer(OMX_HANDLETYPE hComponent,
                                  OMX_U32 nPortIndex,
                                  OMX_BUFFERHEADERTYPE *pBuffHeader)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone, eTmpError = OMX_ErrorNone;
    OSAL_ERROR          tStatus = OSAL_ErrNone;
    OMX_COMPONENTTYPE   *pComp = NULL;
    OMXBaseComp         *pBaseComp = NULL;
    OMXBaseComp_Pvt     *pBaseCompPvt = NULL;
    OMXBase_Port        *pPort = NULL;
    OMX_U32             nStartPortNumber = 0, nPorts = 0;

    OMX_CHECK((hComponent != NULL) &&
                (pBuffHeader != NULL), OMX_ErrorBadParameter);

    pComp = (OMX_COMPONENTTYPE *)hComponent;
    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;
    nStartPortNumber = pBaseComp->nMinStartPortIndex;
    nPorts = pBaseComp->nNumPorts;
    OMX_CHECK(nPortIndex >= nStartPortNumber && nPortIndex < nPorts,
                    OMX_ErrorBadPortIndex);

    pPort = pBaseComp->pPorts[nPortIndex - nStartPortNumber];

    OMX_BASE_CHK_VERSION(pBuffHeader, OMX_BUFFERHEADERTYPE, eError);
    /*Free buffer should not be called on a port after all buffers have been
    freed*/
    OMX_CHECK(pPort->nBufferCnt != 0, OMX_ErrorBadParameter);

    /* unregister the buffer with decoder for non-allocator port*/
    if (!pPort->bIsBufferAllocator && pBaseComp->fpXlateBuffHandle != NULL) {
        eError = pBaseComp->fpXlateBuffHandle(hComponent, (OMX_PTR)(pBuffHeader), OMX_FALSE);
    }

    /* Just decrement the buffer count and unpopulate the port,
    * buffer header pool is freed up once all the buffers are received */
    pPort->nBufferCnt--;
    pPort->sPortDef.bPopulated = OMX_FALSE;
    if( pBaseComp->tCurState == OMX_StateIdle &&
            pBaseComp->tNewState == OMX_StateLoaded && pPort->sPortDef.bEnabled ) {
        /*Allowed on idle --> loaded transition if port is enabled*/
    } else if((pPort->bIsInTransition) &&
            (pBaseComp->tCurState != OMX_StateLoaded) &&
            (pBaseComp->tCurState != OMX_StateWaitForResources)) {
        /*Allowed during port disable if current state is not
        loaded/waitforresources*/
    } else {
        eError = pBaseCompPvt->sAppCallbacks.EventHandler(hComponent,
                                        pComp->pApplicationPrivate,
                                        OMX_EventError, (OMX_U32)OMX_ErrorPortUnpopulated,
                                        nPortIndex, " PortUnpopulated ");
        /*Ideally callback should never return error*/
        OMX_CHECK(OMX_ErrorNone == eError, eError);
    }
    if( pPort->nBufferCnt == 0 ) {
        tStatus = OSAL_SetEvent(pPort->pBufAllocFreeEvent, BUF_FREE_EVENT,
                                    OSAL_EVENT_OR);
        OMX_CHECK(OSAL_ErrNone == tStatus, OMX_ErrorUndefined);
    }

EXIT:
    if((eTmpError != OMX_ErrorNone) && (eError == OMX_ErrorNone)) {
        /*An error occured earlier but we still continued cleanup to avoid leaks.
        Setting the return value to the error code now*/
        eError = eTmpError;
    }
    return (eError);
}

/*
* Empty This Buffer
*/
OMX_ERRORTYPE OMXBase_EmptyThisBuffer(OMX_HANDLETYPE hComponent,
                                       OMX_BUFFERHEADERTYPE *pBuffer)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE   *pComp = NULL;
    OMXBaseComp         *pBaseComp = NULL;
    OMXBaseComp_Pvt     *pBaseCompPvt = NULL;
    OMXBase_Port        *pPort = NULL;
    OMX_U32             nPorts, nStartPortNumber;

    OMX_CHECK((hComponent != NULL) &&
                (pBuffer != NULL), OMX_ErrorBadParameter);

    pComp = (OMX_COMPONENTTYPE *)hComponent;
    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;
    nPorts = pBaseComp->nNumPorts;
    nStartPortNumber = pBaseComp->nMinStartPortIndex;

    /*Can't be invoked when the comp is in invalid state*/
    OMX_CHECK(OMX_StateInvalid != pBaseComp->tCurState, OMX_ErrorInvalidState);
    /* check for valid port index */
    OMX_CHECK(pBuffer->nInputPortIndex < (nPorts + nStartPortNumber), OMX_ErrorBadPortIndex);

    pPort = pBaseComp->pPorts[pBuffer->nInputPortIndex - nStartPortNumber];
    if( pPort->sPortDef.eDir != OMX_DirInput ) {
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    /* This method is allowed only when the comp is in or a transition
    * to executing or pause state */
    OMX_CHECK((pBaseComp->tCurState == OMX_StateIdle &&
        pBaseComp->tNewState == OMX_StateExecuting) ||
        (pBaseComp->tCurState == OMX_StateExecuting ||
        pBaseComp->tCurState == OMX_StatePause),
        OMX_ErrorIncorrectStateOperation);

    /*Following two checks are based on the 1.1.2 AppNote*/
    /*Supplier ports can accept buffers even if current state is disabled
    if they are transitioning from port enable to disable*/
    if( pPort->sPortDef.bEnabled != OMX_TRUE ) {
        if((!pPort->bIsInTransition) || (!pPort->bIsBufferAllocator)) {
            eError = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }
    }
    /*Non-supplier ports can't accept buffers when transitioning to Idle
    or when port is being transitioned to disabled*/
    if( !pPort->bIsBufferAllocator) {
        if((pBaseComp->tNewState == OMX_StateIdle) || (pPort->bIsInTransition)) {
            eError = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }
    }
    eError = OMXBase_DIO_Queue(hComponent, pBuffer->nInputPortIndex, pBuffer);
    OMX_CHECK(eError == OMX_ErrorNone, eError);
    ((OMXBase_BufHdrPvtData *)(pBuffer->pPlatformPrivate))->bufSt = OWNED_BY_US;

    /*If another buffer comes after eos then reset the variable that causes
    watermark to become meaningless on this port*/
    if( pPort->bEosRecd == OMX_TRUE ) {
        pPort->bEosRecd = OMX_FALSE;
    }
    /*If EOS buffer or CodecConfig buffer then force notify to derived component*/
    if( pBuffer->nFlags & OMX_BUFFERFLAG_EOS ) {
        pPort->bEosRecd = OMX_TRUE;
    }
    eError = pBaseCompPvt->fpInvokeProcessFunction(hComponent, DATAEVENT);

EXIT:
    return (eError);
}

/*
* OMX Base FillThisBuffer
*/
OMX_ERRORTYPE OMXBase_FillThisBuffer(OMX_HANDLETYPE hComponent,
                                      OMX_BUFFERHEADERTYPE *pBuffer)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE   *pComp = NULL;
    OMXBaseComp         *pBaseComp = NULL;
    OMXBaseComp_Pvt     *pBaseCompPvt = NULL;
    OMXBase_Port        *pPort = NULL;
    OMX_U32             nPorts, nStartPortNumber;

    OMX_CHECK((hComponent != NULL) && (pBuffer != NULL), OMX_ErrorBadParameter);
    OMX_CHECK(pBuffer->pBuffer != NULL, OMX_ErrorBadParameter);

    pComp = (OMX_COMPONENTTYPE *)hComponent;
    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;
    nPorts = pBaseComp->nNumPorts;
    nStartPortNumber = pBaseComp->nMinStartPortIndex;

    /*Can't be invoked when the comp is in invalid state*/
    OMX_CHECK(OMX_StateInvalid != pBaseComp->tCurState,
                OMX_ErrorInvalidState);

    /* check for valid port index */
    OMX_CHECK(pBuffer->nOutputPortIndex < (nPorts +
                     nStartPortNumber), OMX_ErrorBadPortIndex);

    pPort = pBaseComp->pPorts[pBuffer->nOutputPortIndex - nStartPortNumber];
    if( pPort->sPortDef.eDir != OMX_DirOutput ) {
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    /* This method is allowed only when the comp is in or a transition
    * to executing or pause state */
    OMX_CHECK((pBaseComp->tCurState == OMX_StateIdle &&
        pBaseComp->tNewState == OMX_StateExecuting) ||
        (pBaseComp->tCurState == OMX_StateExecuting ||
        pBaseComp->tCurState == OMX_StatePause),
        OMX_ErrorIncorrectStateOperation);
    /*Following two checks are based on the 1.1.2 AppNote*/
    /*Supplier ports can accept buffers even if current state is disabled
    if they are transitioning from port enable to disable*/
    if( pPort->sPortDef.bEnabled != OMX_TRUE ) {
        if((!pPort->bIsInTransition) || (!pPort->bIsBufferAllocator)) {
            eError = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }
    }
    /*Non-supplier ports can't accept buffers when transitioning to Idle
    or when port is being transitioned to disabled*/
    if( !pPort->bIsBufferAllocator) {
        if((pBaseComp->tNewState == OMX_StateIdle) ||
                (pPort->bIsInTransition)) {
            eError = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }
    }
    eError = OMXBase_DIO_Queue(hComponent, pBuffer->nOutputPortIndex, pBuffer);
    OMX_CHECK(eError == OMX_ErrorNone, eError);
    ((OMXBase_BufHdrPvtData *)(pBuffer->pPlatformPrivate))->bufSt = OWNED_BY_US;
    eError = pBaseCompPvt->fpInvokeProcessFunction(hComponent, DATAEVENT);

EXIT:
    return (eError);
}

/*
* OMX Base SetConfig
*/
OMX_ERRORTYPE OMXBase_SetConfig(OMX_HANDLETYPE hComponent,
                                 OMX_INDEXTYPE nIndex,
                                 OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OSAL_ERROR          tStatus = OSAL_ErrNone;
    OMX_COMPONENTTYPE   *pComp = NULL;
    OMXBaseComp         *pBaseComp = NULL;
    OMXBaseComp_Pvt     *pBaseCompPvt = NULL;
    OMXBase_Port        *pPort = NULL;
    OMX_U32             nStartPortNumber, nPortIndex, nPorts;

    OMX_CHECK((hComponent != NULL) &&
        (pComponentConfigStructure != NULL), OMX_ErrorBadParameter);

    pComp = (OMX_COMPONENTTYPE *)hComponent;
    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;
    nStartPortNumber = pBaseComp->nMinStartPortIndex;
    nPorts = pBaseComp->nNumPorts;

    /*Can't be invoked when the comp is in invalid state*/
    OMX_CHECK(OMX_StateInvalid != pBaseComp->tCurState,
            OMX_ErrorInvalidState);

    switch( nIndex ) {
        default :
            eError = OMX_ErrorUnsupportedIndex;
        break;
    }

EXIT:
    return (eError);
}

/*
* OMX Base GetConfig
*/
OMX_ERRORTYPE OMXBase_GetConfig(OMX_HANDLETYPE hComponent,
                            OMX_INDEXTYPE nIndex,
                            OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE       eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE   *pComp = NULL;
    OMXBaseComp         *pBaseComp = NULL;
    OMXBaseComp_Pvt     *pBaseCompPvt = NULL;
    OMXBase_Port        *pPort = NULL;
    OMX_U32             nStartPortNumber, nPortIndex, nPorts;

    OMX_CHECK((hComponent != NULL) &&
            (pComponentConfigStructure != NULL), OMX_ErrorBadParameter);

    pComp = (OMX_COMPONENTTYPE *)hComponent;
    pBaseComp = (OMXBaseComp *)pComp->pComponentPrivate;
    pBaseCompPvt = (OMXBaseComp_Pvt *)pBaseComp->pPvtData;
    nStartPortNumber = pBaseComp->nMinStartPortIndex;
    nPorts = pBaseComp->nNumPorts;

    /*Can't be invoked when the comp is in invalid state*/
    OMX_CHECK(OMX_StateInvalid != pBaseComp->tCurState, OMX_ErrorInvalidState);

    switch( nIndex ) {
        default :
            eError = OMX_ErrorUnsupportedIndex;
        break;
    }

EXIT:
    return (eError);
}

/*
* OMX Base UseEGLImage
*/
OMX_ERRORTYPE OMXBase_UseEGLImage(OMX_HANDLETYPE hComponent,
                                   OMX_BUFFERHEADERTYPE * *ppBufferHdr,
                                   OMX_U32 nPortIndex,
                                   OMX_PTR pAppPrivate,
                                   void *eglImage)
{
    return (OMX_ErrorNotImplemented);
}

/*
* OMX Base GetExtentionIndex
*/
OMX_ERRORTYPE OMXBase_GetExtensionIndex(OMX_HANDLETYPE hComponent,
                                         OMX_STRING cParameterName,
                                         OMX_INDEXTYPE *pIndexType)
{
    return (OMX_ErrorNotImplemented);
}

/*
* OMX Base ComponentRoleEnum
*/
OMX_ERRORTYPE OMXBase_ComponentRoleEnum(OMX_HANDLETYPE hComponent,
                                         OMX_U8 *cRole,
                                         OMX_U32 nIndex)
{
    return (OMX_ErrorNotImplemented);
}

