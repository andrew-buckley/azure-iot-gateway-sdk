// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef __CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "azure_c_shared_utility\lock.h"
#include "azure_c_shared_utility\iot_logging.h"
#include "java_module_host_manager.h"

JAVA_MODULE_HOST_MANAGER_HANDLE instance = NULL;

typedef struct JAVA_MODULE_HOST_MANAGER_DATA_TAG {
	int module_count;
	LOCK_HANDLE lock;
} JAVA_MODULE_HOST_MANAGER_HANDLE_DATA;

JAVA_MODULE_HOST_MANAGER_HANDLE JavaModuleHostManager_Create()
{
	JAVA_MODULE_HOST_MANAGER_HANDLE_DATA* result;

	if (instance == NULL)
	{
		result = (JAVA_MODULE_HOST_MANAGER_HANDLE_DATA*)malloc(sizeof(JAVA_MODULE_HOST_MANAGER_HANDLE_DATA));
		if (result == NULL)
		{
			LogError("Failed to allocate memory for a JAVA_MODULE_HOST_MANAGER_HANDLE");
		}
		else
		{
			if ((result->lock = Lock_Init()) == NULL)
			{
				LogError("Lock_Init() failed.");
				result = NULL;
			}
			else
			{
				result->module_count = 0;
				instance = result;
			}
		}
	}
	else
	{
		result = instance;
	}
	return result;
}

void JavaModuleHostManager_Destroy(JAVA_MODULE_HOST_MANAGER_HANDLE manager)
{
	if (manager == NULL)
	{
		LogError("JAVA_MODULE_HOST_MANAGER_HANDLE is NULL.");
	}
	else
	{
		if (manager->module_count == 0)
		{
			if (Lock_Deinit(manager->lock) != LOCK_OK)
			{
				LogError("Lock_Deinit() failed.");
			}
			else
			{
				free(manager);
				manager = NULL;
				instance = NULL;
			}
		}
		else
		{
			LogInfo("Manager module count is not 0 (%i) and will not be destroyed.", manager->module_count);
		}
	}
}

JAVA_MODULE_HOST_MANAGER_RESULT JavaModuleHostManager_Add(JAVA_MODULE_HOST_MANAGER_HANDLE manager)
{
	return internal_inc_dec(manager, 1);
}

JAVA_MODULE_HOST_MANAGER_RESULT JavaModuleHostManager_Remove(JAVA_MODULE_HOST_MANAGER_HANDLE manager)
{
	return internal_inc_dec(manager, -1);
}

size_t JavaModuleHostManager_Size(JAVA_MODULE_HOST_MANAGER_HANDLE manager)
{
	return manager->module_count;
}

static JAVA_MODULE_HOST_MANAGER_RESULT internal_inc_dec(JAVA_MODULE_HOST_MANAGER_HANDLE manager, int addend)
{
	JAVA_MODULE_HOST_MANAGER_RESULT result;

	if (manager == NULL)
	{
		LogError("JAVA_MODULE_HOST_MANAGER_HANDLE is NULL.");
		result = MANAGER_ERROR;
	}
	else
	{
		if (Lock(manager->lock) != LOCK_OK)
		{
			LogError("Failed to acquire lock.");
			result = MANAGER_ERROR;
		}
		else
		{
			manager->module_count += addend;
			if (Unlock(manager->lock) != LOCK_OK)
			{
				LogError("Failed to release lock.");
				result = MANAGER_ERROR;
			}
			else
			{
				result = MANAGER_OK;
			}
		}
	}
	return result;
}
