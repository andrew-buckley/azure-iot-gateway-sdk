// Copyright (c) Microsoft. All rights reserved.
// Licensed under MIT license. See LICENSE file in the project root for full license information.

#ifndef JAVA_MODULE_HOST_MANAGER_H
#define JAVA_MODULE_HOST_MANAGER_H

#ifdef __cplusplus
extern "C"
{
#endif

#define JAVA_MODULE_HOST_MANAGER_RESULT_VALUES \
	MANAGER_OK, \
	MANAGER_ERROR \


DEFINE_ENUM(JAVA_MODULE_HOST_MANAGER_RESULT, JAVA_MODULE_HOST_MANAGER_RESULT_VALUES);

typedef struct JAVA_MODULE_HOST_MANAGER_DATA_TAG* JAVA_MODULE_HOST_MANAGER_HANDLE;

extern JAVA_MODULE_HOST_MANAGER_HANDLE JavaModuleHostManager_Create();

extern void JavaModuleHostManager_Destroy(JAVA_MODULE_HOST_MANAGER_HANDLE);

extern JAVA_MODULE_HOST_MANAGER_RESULT JavaModuleHostManager_Add(JAVA_MODULE_HOST_MANAGER_HANDLE);

extern JAVA_MODULE_HOST_MANAGER_RESULT JavaModuleHostManager_Remove(JAVA_MODULE_HOST_MANAGER_HANDLE);

extern size_t JavaModuleHostManager_Size(JAVA_MODULE_HOST_MANAGER_HANDLE);

#ifdef __cplusplus
}
#endif

#endif /*JAVA_MODULE_HOST_MANAGER_H*/