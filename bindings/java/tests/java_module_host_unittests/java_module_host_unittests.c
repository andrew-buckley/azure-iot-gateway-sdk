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

#ifdef __cplusplus
#include <cstdint>
#include <cstddef>
extern "C" {
#else
#include <stdint.h>
#include <stddef.h>
#endif
//#include <jni.h>

	#ifndef _JAVASOFT_JNI_H_
	#define _JAVASOFT_JNI_H_

	#define JNIEXPORT __declspec(dllexport)
	#define JNIIMPORT __declspec(dllimport)
	#define JNICALL __stdcall

	typedef long jint;
	typedef __int64 jlong;
	typedef signed char jbyte;

	typedef unsigned char   jboolean;
	typedef unsigned short  jchar;
	typedef short           jshort;
	typedef float           jfloat;
	typedef double          jdouble;

	typedef jint            jsize;

	struct _jobject;

	typedef struct _jobject *jobject;
	typedef jobject jclass;
	typedef jobject jthrowable;
	typedef jobject jstring;
	typedef jobject jarray;
	typedef jarray jbooleanArray;
	typedef jarray jbyteArray;
	typedef jarray jcharArray;
	typedef jarray jshortArray;
	typedef jarray jintArray;
	typedef jarray jlongArray;
	typedef jarray jfloatArray;
	typedef jarray jdoubleArray;
	typedef jarray jobjectArray;

	typedef jobject jweak;

	typedef union jvalue {
		jboolean z;
		jbyte    b;
		jchar    c;
		jshort   s;
		jint     i;
		jlong    j;
		jfloat   f;
		jdouble  d;
		jobject  l;
	} jvalue;

	struct _jfieldID;
	typedef struct _jfieldID *jfieldID;

	struct _jmethodID;
	typedef struct _jmethodID *jmethodID;

	/* Return values from jobjectRefType */
	typedef enum _jobjectType {
		JNIInvalidRefType = 0,
		JNILocalRefType = 1,
		JNIGlobalRefType = 2,
		JNIWeakGlobalRefType = 3
	} jobjectRefType;

	/*
	* jboolean constants
	*/

	#define JNI_FALSE 0
	#define JNI_TRUE 1

	/*
	* possible return values for JNI functions.
	*/

	#define JNI_OK           0                 /* success */
	#define JNI_ERR          (-1)              /* unknown error */
	#define JNI_EDETACHED    (-2)              /* thread detached from the VM */
	#define JNI_EVERSION     (-3)              /* JNI version error */
	#define JNI_ENOMEM       (-4)              /* not enough memory */
	#define JNI_EEXIST       (-5)              /* VM already created */
	#define JNI_EINVAL       (-6)              /* invalid arguments */

	/*
	* used in ReleaseScalarArrayElements
	*/

	#define JNI_COMMIT 1
	#define JNI_ABORT 2

	/*
	* used in RegisterNatives to describe native method name, signature,
	* and function pointer.
	*/

	typedef struct {
		char *name;
		char *signature;
		void *fnPtr;
	} JNINativeMethod;

	/*
	* JNI Native Method Interface.
	*/

	struct JNINativeInterface_;

	struct JNIEnv_;

	/*
	* JNI Native Method Interface.
	*/

	struct JNINativeInterface_;

	struct JNIEnv_;

	#ifdef __cplusplus
		typedef JNIEnv_ JNIEnv;
	#else
		typedef const struct JNINativeInterface_ *JNIEnv;
	#endif

		/*
		* JNI Invocation Interface.
		*/

		struct JNIInvokeInterface_;

		struct JavaVM_;

	#ifdef __cplusplus
		typedef JavaVM_ JavaVM;
	#else
		typedef const struct JNIInvokeInterface_ *JavaVM;
	#endif

	typedef struct JavaVMOption {
		char *optionString;
		void *extraInfo;
	} JavaVMOption;

	typedef struct JavaVMInitArgs {
		jint version;

		jint nOptions;
		JavaVMOption *options;
		jboolean ignoreUnrecognized;
	} JavaVMInitArgs;

	typedef struct JavaVMAttachArgs {
		jint version;

		char *name;
		jobject group;
	} JavaVMAttachArgs;

	/* These will be VM-specific. */

	#define JDK1_2
	#define JDK1_4

	/* End VM-specific. */

	struct JNIInvokeInterface_ {
		void *reserved0;
		void *reserved1;
		void *reserved2;

		jint(JNICALL *DestroyJavaVM)(JavaVM *vm);

		jint(JNICALL *AttachCurrentThread)(JavaVM *vm, void **penv, void *args);

		jint(JNICALL *DetachCurrentThread)(JavaVM *vm);

		jint(JNICALL *GetEnv)(JavaVM *vm, void **penv, jint version);

		jint(JNICALL *AttachCurrentThreadAsDaemon)(JavaVM *vm, void **penv, void *args);
	};

	struct JavaVM_ {
		const struct JNIInvokeInterface_ *functions;
	};

	#endif

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
	MOCKABLE_FUNCTION(JNICALL, jint, JNI_CreateJavaVM, JavaVM**, pvm, void **, peng, void*, args);
	MOCKABLE_FUNCTION(JNICALL, jint, JNI_GetCreatedJavaVMs, JavaVM **, pvm, jsize, count, jsize*, size);

#ifdef __cplusplus
}
#endif

#undef ENABLE_MOCKS

#include "umock_c.h"

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

static bool malloc_will_fail = false;

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

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
