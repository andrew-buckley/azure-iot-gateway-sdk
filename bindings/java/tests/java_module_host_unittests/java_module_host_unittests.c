// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <stddef.h>

#include "testrunnerswitcher.h"

#define ENABLE_MOCKS

#include "umock_c_prod.h"
#include "java_module_host_manager.h"
#include "azure_c_shared_utility\vector.h"
#include "azure_c_shared_utility\strings.h"
#include "java_module_host.h"

#include <stdint.h>
#include <stddef.h>

#define _JAVASOFT_JNI_MD_H_
#define JNIEXPORT
#define JNIIMPORT
#define JNICALL

typedef long jint;
typedef __int64 jlong;
typedef signed char jbyte;

#include <jni.h>

//gballoc mocks
MOCKABLE_FUNCTION(, void*, gballoc_malloc, size_t, size);
MOCKABLE_FUNCTION(, void, gballoc_free, void*, ptr);

//Message mocks
MOCKABLE_FUNCTION(, MESSAGE_HANDLE, Message_CreateFromByteArray, const unsigned char*, source, int32_t, size);
MOCKABLE_FUNCTION(, const unsigned char*, Message_ToByteArray, MESSAGE_HANDLE, messageHandle, int32_t*, size);
MOCKABLE_FUNCTION(, void, Message_Destroy, MESSAGE_HANDLE, message);

//MessageBus mocks
MOCKABLE_FUNCTION(, MESSAGE_BUS_RESULT, MessageBus_Publish, MESSAGE_BUS_HANDLE, bus, MESSAGE_HANDLE, message);

//JNI mocks
MOCKABLE_FUNCTION(JNICALL, jint, JNI_GetDefaultJavaVMInitArgs, void*, args);
MOCKABLE_FUNCTION(JNICALL, jint, JNI_CreateJavaVM, JavaVM**, pvm, void **, penv, void*, args);
MOCKABLE_FUNCTION(JNICALL, jint, JNI_GetCreatedJavaVMs, JavaVM **, pvm, jsize, count, jsize*, size);

//JEnv function mocks
MOCK_FUNCTION_WITH_CODE(JNICALL, jclass, my_FindClass, JNIEnv*, env, const char*, name);
return (jclass)0x42;
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(JNICALL, jmethodID, my_GetMethodID, JNIEnv*, env, jclass, clazz, const char*, name, const char*, sig);
return (jmethodID)0x42;
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(JNICALL, jobject, my_NewObject, JNIEnv*, env, jclass, clazz, jmethodID, methodID);
return (jobject)0x42;
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(JNICALL, jthrowable, my_ExceptionOccurred);
return NULL;
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(JNICALL, void, my_ExceptionClear);
MOCK_FUNCTION_END()

//JVM function mocks
MOCK_FUNCTION_WITH_CODE(JNICALL, jint, my_AttachCurrentThread, JavaVM*, vm, void**, penv, void*, args);
return JNI_OK;
MOCK_FUNCTION_END()

#undef ENABLE_MOCKS

#include "umock_c.h"

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

static bool malloc_will_fail = false;

static bool module_manager_count = 0;
static JAVA_MODULE_HOST_MANAGER_HANDLE global_manager = NULL;


void* my_gballoc_malloc(size_t size)
{
	void* result = NULL;
	if (malloc_will_fail == false)
	{
		result = malloc(size);
	}

	return result;
}

void my_gballoc_free(void* ptr)
{
	free(ptr);
}

JAVA_MODULE_HOST_MANAGER_HANDLE my_JavaModuleHostManager_Create()
{
	if (module_manager_count == 0)
	{
		global_manager = malloc(0x42);
	}
	module_manager_count++;
	return global_manager;
}

void my_JavaModuleHostManager_Destroy(JAVA_MODULE_HOST_MANAGER_HANDLE manager)
{
	if (manager != NULL && manager == global_manager)
	{
		if (module_manager_count == 0)
		{
				free(manager);
				manager = NULL;
				global_manager = NULL;
		}
	}
}

JAVA_MODULE_HOST_MANAGER_RESULT my_JavaModuleHostManager_Add(JAVA_MODULE_HOST_MANAGER_HANDLE manager)
{
	module_manager_count++;
	return MANAGER_OK;
}

JAVA_MODULE_HOST_MANAGER_RESULT my_JavaModuleHostManager_Remove(JAVA_MODULE_HOST_MANAGER_HANDLE manager)
{
	module_manager_count--;
	return MANAGER_OK;
}

size_t my_JavaModuleHostManager_Size(JAVA_MODULE_HOST_MANAGER_HANDLE manager)
{
	return module_manager_count;
}

jint mock_JNI_CreateJavaVM(JavaVM** pvm, void** penv, void* args)
{
	//233
	struct JNINativeInterface_ env = {
		0, 0, 0, 0, 
		
		0, 0, my_FindClass, 0, 0, 0, 0, 0, 0, 0,
		0, my_ExceptionOccurred, 0, my_ExceptionClear, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, my_NewObject, 0, 0, 0, 0, my_GetMethodID,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	struct JNIInvokeInterface_ vm = {
		0,
		0,
		0,

		0,
		my_AttachCurrentThread,
		0,
		0,
		0
	};

	*pvm = (JavaVM*)malloc(sizeof(JavaVM));
	*(*pvm) = (struct JNIInvokeInterface_ *)malloc(sizeof(struct JNIInvokeInterface_));
	*(struct JNIInvokeInterface_ *)**pvm = vm;
	//memcpy((*(*pvm)), &vm, sizeof(vm));


	*penv = (JNIEnv*)malloc(sizeof(JNIEnv));
	*((JNIEnv*)(*penv)) = malloc(sizeof(struct JNINativeInterface_));
	*(struct JNINativeInterface_*)(*((JNIEnv*)(*penv))) = env;
	//memcpy((*((JNIEnv*)(*penv))), &env, sizeof(env));

	return 0;
}

void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
	ASSERT_FAIL("umock_c reported error");
}

static JAVA_MODULE_HOST_CONFIG config =
{
	"TestClass",
	"{hello}",
	NULL
};

/*these are simple cached variables*/
static pfModule_Create  JavaModuleHost_Create = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_Destroy JavaModuleHost_Destroy = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/
static pfModule_Receive JavaModuleHost_Receive = NULL; /*gets assigned in TEST_SUITE_INITIALIZE*/

BEGIN_TEST_SUITE(JavaModuleHost_UnitTests)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
	TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
	g_testByTest = TEST_MUTEX_CREATE();
	ASSERT_IS_NOT_NULL(g_testByTest);

	JavaModuleHost_Create = Module_GetAPIS()->Module_Create;
	JavaModuleHost_Destroy = Module_GetAPIS()->Module_Destroy;
	JavaModuleHost_Receive = Module_GetAPIS()->Module_Receive;

	umock_c_init(on_umock_c_error);

	REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
	REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

	REGISTER_GLOBAL_MOCK_HOOK(JavaModuleHostManager_Create, my_JavaModuleHostManager_Create);

	REGISTER_GLOBAL_MOCK_HOOK(JavaModuleHostManager_Destroy, my_JavaModuleHostManager_Destroy);

	REGISTER_GLOBAL_MOCK_HOOK(JavaModuleHostManager_Add, my_JavaModuleHostManager_Add);

	REGISTER_GLOBAL_MOCK_HOOK(JavaModuleHostManager_Remove, my_JavaModuleHostManager_Remove);

	REGISTER_GLOBAL_MOCK_HOOK(JavaModuleHostManager_Size, my_JavaModuleHostManager_Size);

	REGISTER_GLOBAL_MOCK_HOOK(JNI_CreateJavaVM, mock_JNI_CreateJavaVM);

	REGISTER_UMOCK_ALIAS_TYPE(JAVA_MODULE_HOST_MANAGER_HANDLE, void*);

	REGISTER_UMOCK_ALIAS_TYPE(JavaVM, void*);
	REGISTER_UMOCK_ALIAS_TYPE(jint, long);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
	umock_c_deinit();

	TEST_MUTEX_DESTROY(g_testByTest);
	TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
	if (TEST_MUTEX_ACQUIRE(g_testByTest) != 0)
	{
		ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
	}

	umock_c_reset_all_calls();
	malloc_will_fail = false;
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
	TEST_MUTEX_RELEASE(g_testByTest);
}

/*Tests_SRS_JAVA_MODULE_HOST_14_001: [This function shall return NULL if bus is NULL.]*/
TEST_FUNCTION(JavaModuleHost_Create_with_NULL_bus_fails)
{
	//Arrange

	//Act
	MODULE_HANDLE result = JavaModuleHost_Create(NULL, &config);

	//Assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//Cleanup
}

/*Tests_SRS_JAVA_MODULE_HOST_14_002: [This function shall return NULL if configuration is NULL. ]*/
TEST_FUNCTION(JavaModuleHost_Create_with_NULL_config_fails)
{
	//Arrange

	//Act
	MODULE_HANDLE result = JavaModuleHost_Create((MESSAGE_BUS_HANDLE)0x42, NULL);

	//Assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//Cleanup
}

/*Tests_SRS_JAVA_MODULE_HOST_13_003: [This function shall return NULL if class_name is NULL.]*/
TEST_FUNCTION(JavaModuleHost_Create_with_NULL_class_name_fails)
{
	//Arrange

	JAVA_MODULE_HOST_CONFIG no_class_name_config =
	{
		NULL,
		"{hello}",
		(JVM_OPTIONS*)0x42
	};

	//Act
	MODULE_HANDLE result = JavaModuleHost_Create((MESSAGE_BUS_HANDLE)0x42, &no_class_name_config);

	//Assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_JAVA_MODULE_HOST_14_004: [This function shall return NULL upon any underlying API call failure.]*/
TEST_FUNCTION(JavaModuleHost_Create_malloc_failure_fails)
{
	//Arrange
	malloc_will_fail = true;

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
		.IgnoreArgument(1);

	//Act
	MODULE_HANDLE result = JavaModuleHost_Create((MESSAGE_BUS_HANDLE)0x42, &config);

	//Assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_JAVA_MODULE_HOST_14_006: [This function shall allocate memory for an instance of a JAVA_MODULE_HANDLE_DATA structure to be used as the backing structure for this module. ]*/
TEST_FUNCTION(JavaModuleHost_Create_allocates_structure_success)
{
	//Arrange

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is for the structure*/
		.IgnoreArgument(1);

	//Act
	MODULE_HANDLE result = JavaModuleHost_Create((MESSAGE_BUS_HANDLE)0x42, &config);

	//Assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(JavaModuleHost_UnitTests);
