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

#ifndef _OMX_BASE_DIO_PLUGIN_H
#define _OMX_BASE_DIO_PLUGIN_H

#ifdef _cplusplus
extern "C" {
#endif

#include <OMX_Core.h>
#include <OMX_Component.h>

#define OMX_BASE_MAXNAMELEN OMX_MAX_STRINGNAME_SIZE

/* The OMX_DIO_OpenMode enumeration is used to
 *  sepcify the open mode type i.e reader or writer
 */
typedef enum OMX_DIO_OpenMode {
    OMX_DIO_READER  =  0x0,
    OMX_DIO_WRITER  =  0x1
}OMX_DIO_OpenMode;

/** OMX_DIO_CreateParams :
 *  This structure contains the parameters required to
 *  create DIO object
 *
 *  @param cChannelName      :  channel name
 *  @param hComponent        :  Handle of Component
 *  @param nPortIndex        :  Index of the port
 *  @param pAppCallbacks     :  Application callbacks
 */
typedef struct OMX_DIO_CreateParams {
    char               cChannelName[OMX_BASE_MAXNAMELEN];
    OMX_HANDLETYPE     hComponent;
    OMX_U8             nPortIndex;
    OMX_CALLBACKTYPE  *pAppCallbacks;
}OMX_DIO_CreateParams;


/** OMX_DIO_OpenParams :
 *  This structure contains the open parameters for DIO object
 *  @param nMode           : open mode reader or writer
 *  @param bCacheFlag      : cache access flag - true if buffer is accessed via
 *                           processor/cache
 *  @param nBufSize        : used by non-tunnel open as allocate buffer call
 *                           can specify a different size
 */
typedef struct OMX_DIO_OpenParams {
    OMX_U32  nMode;
    OMX_BOOL bCacheFlag;
    OMX_U32  nBufSize;
}OMX_DIO_OpenParams;

typedef OMX_ERRORTYPE (*OMX_DIO_Init)(OMX_IN OMX_HANDLETYPE hComponent,
                                               OMX_IN OMX_PTR pCreateParams);

/* OMX_DIO_Register  :
 *  This structure contains the params used to register the DIO object
 *  @param pChannelName  :  Channel Name
 *  @param pInitialize   :  DIO instace initialization function
 */
typedef struct OMX_DIO_Register {
    const char   *cChannelType;
    OMX_DIO_Init  pInitialize;
}OMX_DIO_Register;

OMX_DIO_Register    OMX_DIO_Registered[2];
/*
 * The OMX_DIO_CtrlCmd_TYPE            : This enumeration is used to
 *                                          specify the action in the
 *                                          OMX_DIO_Control Macro.
 *
 * @ param OMX_DIO_CtrlCmd_Flush        : Flush the buffers on this port.
 *                                          This command is used only by
 *                                          omx_base for now.
 * @ param OMX_DIO_CtrlCmd_Start        : Start buffer processing on this
 *                                          port. This command is used only by
 *                                          omx_base for now.
 * @ param OMX_DIO_CtrlCmd_Stop         : Stop buffer processing on this
 *                                          port. This command is used only by
 *                                          omx_base for now.
 * @ param OMX_DIO_CtrlCmd_GetCtrlAttribute : To get the attribute pending
 *                                          on this port. A pointer to structure
 *                                          of type
 *                                          OMXBase_CodecConfig is
 *                                          passed as the parameter to the
 *                                          control command. This should be used
 *                                          if dio_dequeue returns
 *                                          OMX_WarningAttributePending
 *                                          indicating that an attribute buffer
 *                                          is pending.
 * @ param OMX_DIO_CtrlCmd_SetCtrlAttribute : To send some attribute on
 *                                          this port. A pointer to structure of
 *                                          type OMXBase_CodecConfig
 *                                          is passed as the parameter to the
 *                                          control command.
 * @ param OMX_DIO_CtrlCmd_ExtnStart    : If some specific DIO
 *                                          implementation wants to have control
 *                                          commands specific for that DIO then
 *                                          these extended commands can be added
 *                                          after this.
 */
typedef enum OMX_DIO_CtrlCmdType {
    OMX_DIO_CtrlCmd_Flush            = 0x1,
    OMX_DIO_CtrlCmd_Start            = 0x2,
    OMX_DIO_CtrlCmd_Stop             = 0x3,
    OMX_DIO_CtrlCmd_GetCtrlAttribute = 0x4,
    OMX_DIO_CtrlCmd_SetCtrlAttribute = 0x5,
    OMX_DIO_CtrlCmd_ExtnStart        = 0xA
}OMX_DIO_CtrlCmdType;


/** OMX_DIO_Object :
 *  This structure contains the params and interface to access the DIO object
 *
 * @param pContext         :  pointer to the DIO private data area
 * @param (*open)          :  DIO object open Implementation
 * @param (*close)         :  DIO object close Implementation
 * @param (*queue)         :  DIO object queue Implementation
 * @param (*dequeue)       :  DIO object dequeu Implementation
 * @param (*send)          :  DIO object send Implementation
 * @param (*cancel)        :  DIO object cancel Implementation
 * @param (*control)       :  DIO object control Implementation
 * @param (*getcount)      :  DIO object getcount Implementation
 * @param (*deinit)        :  DIO object deinit Implementation
 * @param bOpened          :  Indicates whether DIO has been opened on this port
 */
typedef struct OMX_DIO_Object {

    OMX_PTR pContext;

    OMX_ERRORTYPE (*open)(OMX_HANDLETYPE handle,
                          OMX_DIO_OpenParams *pParams);

    OMX_ERRORTYPE (*close)(OMX_HANDLETYPE handle);

    OMX_ERRORTYPE (*queue)(OMX_HANDLETYPE handle,
                           OMX_PTR pBuffHeader);

    OMX_ERRORTYPE (*dequeue)(OMX_HANDLETYPE handle,
                             OMX_PTR *pBuffHeader);

    OMX_ERRORTYPE (*send)(OMX_HANDLETYPE handle,
                          OMX_PTR pBuffHeader);

    OMX_ERRORTYPE (*cancel)(OMX_HANDLETYPE handle,
                            OMX_PTR pBuffHeader);

    OMX_ERRORTYPE (*control)(OMX_HANDLETYPE handle,
                             OMX_DIO_CtrlCmdType nCmdType,
                             OMX_PTR pParams);

    OMX_ERRORTYPE (*getcount)(OMX_HANDLETYPE handle,
                              OMX_U32 *pCount);

    OMX_ERRORTYPE (*deinit)(OMX_HANDLETYPE handle);

    OMX_BOOL bOpened;

}OMX_DIO_Object;

#ifdef __cplusplus
}
#endif

#endif

