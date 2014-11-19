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

#ifndef _OSAL_EVENTS_H_
#define _OSAL_EVENTS_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*
* OSAL EVENT Operations list
*/
typedef enum osal_event_op
{
    OSAL_EVENT_AND,
    OSAL_EVENT_AND_CONSUME,
    OSAL_EVENT_OR,
    OSAL_EVENT_OR_CONSUME
} OSAL_EventOp;

OSAL_ERROR OSAL_CreateEvent(void **pEvents);

OSAL_ERROR OSAL_DeleteEvent(void *pEvents);

OSAL_ERROR OSAL_SetEvent(void *pEvents, uint32_t uEventFlag, OSAL_EventOp eOperation);

OSAL_ERROR OSAL_RetrieveEvent(void *pEvents, uint32_t uRequestedEvents, OSAL_EventOp eOperation,
                                              uint32_t *pRetrievedEvents, uint32_t uTimeOut);

#ifdef __cplusplus
}
#endif

#endif  /* _OSAL_EVENTS_H_ */
