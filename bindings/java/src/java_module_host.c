// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef __CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "message_bus_proxy.h"
#include "java_module_host.h"
#include "azure_c_shared_utility\iot_logging.h"

#define MESSAGE_BUS_CLASS_NAME "MessageBus"
#define CONSTRUCTOR_METHOD_NAME "<init>"
#define JNIFunc(jptr, call, ...) (*(jptr))->call(jptr, __VA_ARGS__)

typedef struct JAVA_MODULE_HANDLE_DATA_TAG
{
	JavaVM* jvm;
	JNIEnv* env;
	jobject module;
	char* moduleName;
}JAVA_MODULE_HANDLE_DATA;

static JNIEnv* JVM_Create(JavaVM**, JVM_OPTIONS*);
static void JVM_Destroy(JavaVM**);
static void destroy_module_internal(JAVA_MODULE_HANDLE_DATA*);
static void set_vm_options(JavaVMInitArgs*, JVM_OPTIONS*);

static MODULE_HANDLE JavaModuleHost_Create(MESSAGE_BUS_HANDLE bus, const void* configuration)
{
	JAVA_MODULE_HANDLE_DATA* result;
	if(bus == NULL || configuration == NULL)
	{
		/*Codes_SRS_JAVA_MODULE_HOST_14_001: [This function shall return NULL if bus is NULL.]*/
		/*Codes_SRS_JAVA_MODULE_HOST_14_002: [This function shall return NULL if configuration is NULL.]*/
		LogError("Invalid input (bus = %p, configuration = %p).", bus, configuration);
		result = NULL;
	}
	else
	{
		JAVA_MODULE_HOST_CONFIG* config = (JAVA_MODULE_HOST_CONFIG*)configuration;

		if (config->class_name == NULL)
		{
			/*Codes_SRS_JAVA_MODULE_HOST_14_005: [This function shall return NULL if class_name is NULL.]*/
			LogError("Invalid input. configuration->class_name cannot be NULL.");
			result = NULL;
		}
		else
		{
			/*Codes_SRS_JAVA_MODULE_HOST_14_008: [This function shall allocate memory for an instance of a JAVA_MODULE_HANDLE_DATA structure to be used as the backing structure for this module.]*/
			result = (JAVA_MODULE_HANDLE_DATA*)malloc(sizeof(JAVA_MODULE_HANDLE_DATA));
			if (result == NULL)
			{
				/*Codes_SRS_JAVA_MODULE_HOST_14_006: [This function shall return NULL upon any underlying API call failure.]*/
				LogError("Malloc failure.");
			}
			else
			{
				/*Codes_SRS_JAVA_MODULE_HOST_14_006: [This function shall return NULL upon any underlying API call failure.]*/
				/*Codes_SRS_JAVA_MODULE_HOST_14_007: [This function shall return a non-NULL MODULE_HANDLE when successful.]*/

				result->env = JVM_Create(&(result->jvm), config->options);
				if (result->env == NULL)
				{
					/*Codes_SRS_JAVA_MODULE_HOST_14_015: [This function shall return NULL if a JVM could not be created or found.]*/
					LogError("Failed to successfully create JVM.");
					destroy_module_internal(result);
					result = NULL;
				}
				else
				{
					/*Codes_SRS_JAVA_MODULE_HOST_14_016: [This function shall find the MessageBus Java class, get the constructor, and create a MessageBus Java object.]*/
					jclass jMessageBus_class = JNIFunc(result->env, FindClass, MESSAGE_BUS_CLASS_NAME);
					if (jMessageBus_class == NULL)
					{
						/*Codes_SRS_JAVA_MODULE_HOST_14_018: [This function shall return NULL if any returned jclass, jmethodID, or jobject is NULL.]*/
						/*Codes_SRS_JAVA_MODULE_HOST_14_019: [This function shall return NULL if any JNI function fails.]*/
						LogError("Could not find class (%s).", MESSAGE_BUS_CLASS_NAME);
						destroy_module_internal(result);
						result = NULL;
					}
					else
					{
						jmethodID jMessageBus_constructor = JNIFunc(result->env, GetMethodID, jMessageBus_class, CONSTRUCTOR_METHOD_NAME, "(J)V");
						jobject jMessageBus_object = JNIFunc(result->env, NewObject, jMessageBus_class, jMessageBus_constructor, (jlong)bus);
						if (jMessageBus_constructor == NULL || jMessageBus_object == NULL)
						{
							/*Codes_SRS_JAVA_MODULE_HOST_14_018: [This function shall return NULL if any returned jclass, jmethodID, or jobject is NULL.]*/
							/*Codes_SRS_JAVA_MODULE_HOST_14_019: [This function shall return NULL if any JNI function fails.]*/
							LogError("Failed to find the %s constructor or create object.", MESSAGE_BUS_CLASS_NAME);
							destroy_module_internal(result);
							result = NULL;
						}
						else
						{
							/*Codes_SRS_JAVA_MODULE_HOST_14_017: [This function shall find the user-defined Java module class using configuration->class_name, get the constructor, and create an instance of this module object.]*/
							jclass jModule_class = JNIFunc(result->env, FindClass, result->moduleName);
							if (jModule_class == NULL)
							{
								/*Codes_SRS_JAVA_MODULE_HOST_14_018: [This function shall return NULL if any returned jclass, jmethodID, or jobject is NULL.]*/
								/*Codes_SRS_JAVA_MODULE_HOST_14_019: [This function shall return NULL if any JNI function fails.]*/
								LogError("Could not find class (%s).", result->moduleName);
								destroy_module_internal(result);
								result = NULL;
							}
							else
							{
								jmethodID jModule_constructor = JNIFunc(result->env, GetMethodID, jModule_class, CONSTRUCTOR_METHOD_NAME, "(JLcom/microsoft/azure/gateway/core/MessageBus;Ljava/lang/String;)V");
								jobject jModule_object = JNIFunc(result->env, NewObject, jModule_class, jModule_constructor, (jlong)result, jMessageBus_object);
								if (jModule_constructor == NULL || jModule_object == NULL)
								{
									/*Codes_SRS_JAVA_MODULE_HOST_14_018: [This function shall return NULL if any returned jclass, jmethodID, or jobject is NULL.]*/
									/*Codes_SRS_JAVA_MODULE_HOST_14_019: [This function shall return NULL if any JNI function fails.]*/
									LogError("Failed to find the %s constructor or create object.", result->moduleName);
									destroy_module_internal(result);
									result = NULL;
								}
								else
								{
									/*Codes_SRS_JAVA_MODULE_HOST_14_020: [The function shall save a new global reference to the Java module object in JAVA_MODULE_HANDLE_DATA->module.]*/
									result->module = JNIFunc(result->env, NewGlobalRef, jModule_object);
								}
							}
						}
					}
				}
			}
		}
	}
	return result;
}

static void JavaModuleHost_Destroy(MODULE_HANDLE module)
{
	/*SRS_JAVA_MODULE_HOST_14_019: [This function shall do nothing if module is NULL.]*/
	/*SRS_JAVA_MODULE_HOST_14_020: [This function shall call the void destroy() method of the Java module object and delete the global reference to this object.]*/
	/*SRS_JAVA_MODULE_HOST_14_021: [This function shall free all resources associated with this module.]*/
	/*SRS_JAVA_MODULE_HOST_14_029: [This function shall destroy the JVM if it the last module to be disconnected from the gateway.]*/
}

static void JavaModuleHost_Receive(MODULE_HANDLE module, MESSAGE_HANDLE message)
{
	/*SRS_JAVA_MODULE_HOST_14_022: [This function shall do nothing if module or message is NULL.]*/
	/*SRS_JAVA_MODULE_HOST_14_023: [This function shall serialize message.]*/
	/*SRS_JAVA_MODULE_HOST_14_024: [This function shall call the void receive(byte[] source) method of the Java module object passing the serialized message.]*/
}

JNIEXPORT jint JNICALL Java_com_microsoft_azure_gateway_core_MessageBus_publishMessage(JNIEnv* env, jobject jMessageBus, jlong bus, jbyteArray serialized_message)
{

}

//Internal functions
static JNIEnv* JVM_Create(JavaVM** jvm, JVM_OPTIONS* options)
{
	JNIEnv* env = NULL;

	/*Codes_SRS_JAVA_MODULE_HOST_14_009: [This function shall initialize a JavaVMInitArgs structure using the JVM_OPTIONS structure configuration->options.]*/
	JavaVMInitArgs vm_args;
	set_vm_options(&vm_args, options);

	/*Codes_SRS_JAVA_MODULE_HOST_14_012: [If this is the first Java module to load, this function shall create the JVM using the JavaVMInitArgs through a call to JNI_CreateJavaVM and save the JavaVM and JNIEnv pointers in the JAVA_MODULE_HANDLE_DATA.]*/
	int result = JNI_CreateJavaVM(jvm, (void**)&env, &vm_args);

	/*Codes_SRS_JAVA_MODULE_HOST_14_013: [If the JVM was previously created, the function shall get a pointer to that JavaVM pointer and JNIEnv environment pointer.]*/
	if (result == JNI_EEXIST)
	{
		jsize vmCount;
		result = JNI_GetCreatedJavaVMs(jvm, (void**)&env, &vmCount);
		if (result == JNI_OK)
		{
			JNIFunc(*jvm, GetEnv, (void**)&env, vm_args.version);
		}
	}

	if (result < 0 || !env)
	{
		/*Codes_SRS_JAVA_MODULE_HOST_14_015: [This function shall return NULL if a JVM could not be created or found.]*/
		/*Codes_SRS_JAVA_MODULE_HOST_14_019: [This function shall return NULL if any JNI function fails.]*/
		LogError("Failed to launch JVM. JNI_CreateJavaVM returned: %d.", result);
	}
	else
	{
		//TODO
		/*Codes_SRS_JAVA_MODULE_HOST_14_014: [This function shall increment the JAVA_MODULE_COUNT global variable.]*/
	}

	return env;
}

static void JVM_Destroy(JavaVM** jvm)
{

}

static void set_vm_options(JavaVMInitArgs* jvm_args, JVM_OPTIONS* options)
{
	if (options == NULL)
	{
		/*Codes_SRS_JAVA_MODULE_HOST_14_010: [If configuration->options is NULL, JavaVMInitArgs shall be initialized using default values.]*/
		jvm_args->version = JNI_VERSION_1_8;
		int result = JNI_GetDefaultJavaVMInitArgs(jvm_args);
	}
	else
	{
		/*Codes_SRS_JAVA_MODULE_HOST_14_011: [This function shall allocate memory for an array of JavaVMOption structures and initialize each with each option provided.]*/
		int options_count = 0;
	}
}

static void destroy_module_internal(JAVA_MODULE_HANDLE_DATA* module)
{

}

static const MODULE_APIS JavaModuleHost_APIS =
{
	JavaModuleHost_Create,
	JavaModuleHost_Destroy,
	JavaModuleHost_Receive
};

#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GET_APIS(JAVA_MODULE_HOST)(void)
#else
MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void)
#endif
{
	return &JavaModuleHost_APIS;
}