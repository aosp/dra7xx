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

#ifndef _OSAL_TASK_H_
#define _OSAL_TASK_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef void *(*OSAL_TaskProc) (void *arg);

OSAL_ERROR OSAL_CreateTask(void **pTask, OSAL_TaskProc pFunc, uint32_t uArgc,
                            void *pArgv, uint32_t uStackSize, uint32_t uPriority, int8_t * pName);

OSAL_ERROR OSAL_DeleteTask(void *pTask);

OSAL_ERROR OSAL_SleepTask(uint32_t mSec);

#ifdef __cplusplus
}
#endif

#endif  /* _OSAL_TASK_H_ */
