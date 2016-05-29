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
#define DEBUG_PORT_DEFAULT 9876

#define JNIFunc(jptr, call, ...) (*(jptr))->call(jptr, __VA_ARGS__)

int JAVA_MODULE_COUNT = 0;

typedef struct JAVA_MODULE_HANDLE_DATA_TAG
{
	JavaVM* jvm;
	JNIEnv* env;
	jobject module;
	char* moduleName;
}JAVA_MODULE_HANDLE_DATA;

static int JVM_Create(JavaVM**, JNIEnv*, JVM_OPTIONS*);
static void JVM_Destroy(JavaVM**, JNIEnv*);
static void destroy_module_internal(JAVA_MODULE_HANDLE_DATA*);
static void init_vm_options(JavaVMInitArgs*, JVM_OPTIONS*);
static void deinit_vm_options(JavaVMInitArgs*);

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
				if (JVM_Create(&(result->jvm), result->env, config->options) != JNI_OK)
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
	if (module != NULL)
	{
		JAVA_MODULE_HANDLE_DATA* moduleHandle = (JAVA_MODULE_HANDLE_DATA *)module;

		/*SRS_JAVA_MODULE_HOST_14_020: [This function shall call the void destroy() method of the Java module object and delete the global reference to this object.]*/
		JNIFunc(moduleHandle->jvm, AttachCurrentThread, (void**)(moduleHandle->env), NULL);
		jclass jModule_class = JNIFunc(moduleHandle->env, GetObjectClass, moduleHandle->module);
		jmethodID jModule_destroy = JNIFunc(moduleHandle->env, GetMethodID, jModule_class, "destroy", "()V");
		JNIFunc(moduleHandle->env, CallVoidMethod, moduleHandle->module, jModule_destroy);

		JNIFunc(moduleHandle->env, DeleteGlobalRef, moduleHandle->module);
		JNIFunc(moduleHandle->jvm, DetachCurrentThread);

		/*SRS_JAVA_MODULE_HOST_14_029: [This function shall destroy the JVM if it the last module to be disconnected from the gateway.]*/
		/*SRS_JAVA_MODULE_HOST_14_021: [This function shall free all resources associated with this module.]*/
		destroy_module_internal(moduleHandle);
	}
}

static void JavaModuleHost_Receive(MODULE_HANDLE module, MESSAGE_HANDLE message)
{
	/*SRS_JAVA_MODULE_HOST_14_022: [This function shall do nothing if module or message is NULL.]*/
	if (module != NULL && message != NULL)
	{
		JAVA_MODULE_HANDLE_DATA* moduleHandle = (JAVA_MODULE_HANDLE_DATA*)module;

		/*SRS_JAVA_MODULE_HOST_14_023: [This function shall serialize message.]*/
		int32_t size;
		const unsigned char* serialized_message = Message_ToByteArray(message, &size);
		//TODO: error check
		jbyteArray arr = JNIFunc(moduleHandle->env, NewByteArray, size);
		JNIFunc(moduleHandle->env, SetByteArrayRegion, arr, 0, size, serialized_message);

		/*SRS_JAVA_MODULE_HOST_14_024: [This function shall call the void receive(byte[] source) method of the Java module object passing the serialized message.]*/
		JNIFunc(moduleHandle->jvm, AttachCurrentThread, (void**)(moduleHandle->env), NULL);
		jclass jModule_class = JNIFunc(moduleHandle->env, GetObjectClass, moduleHandle->module);
		jmethodID jModule_receive = JNIFunc(moduleHandle->env, GetMethodID, jModule_class, "receive", "([B)V");
		JNIFunc(moduleHandle->env, CallVoidMethod, moduleHandle->module, jModule_receive, arr);

		//JNIFunc(moduleHandle->env, ReleaseByteArrayElements, arr, serialized_message, JNI_ABORT);
	}
}

JNIEXPORT jint JNICALL Java_com_microsoft_azure_gateway_core_MessageBus_publishMessage(JNIEnv* env, jobject jMessageBus, jlong bus, jbyteArray serialized_message)
{
	/*SRS_JAVA_MODULE_HOST_14_025: [This function shall use convert the jbyteArray message into an unsigned char array.]*/
	/*SRS_JAVA_MODULE_HOST_14_026: [This function shall use the serialized message in a call to Message_Create.]*/
	/*SRS_JAVA_MODULE_HOST_14_027: [This function shall publish the message to the MESSAGE_BUS_HANDLE addressed by addr and return the value of this function call.]*/
	return 0;
}

//Internal functions
static int JVM_Create(JavaVM** jvm, JNIEnv* env, JVM_OPTIONS* options)
{
	/*Codes_SRS_JAVA_MODULE_HOST_14_009: [This function shall initialize a JavaVMInitArgs structure using the JVM_OPTIONS structure configuration->options.]*/
	JavaVMInitArgs jvm_args;
	init_vm_options(&jvm_args, options);

	/*Codes_SRS_JAVA_MODULE_HOST_14_012: [If this is the first Java module to load, this function shall create the JVM using the JavaVMInitArgs through a call to JNI_CreateJavaVM and save the JavaVM and JNIEnv pointers in the JAVA_MODULE_HANDLE_DATA.]*/
	int result = JNI_CreateJavaVM(jvm, (void**)&env, &jvm_args);

	/*Codes_SRS_JAVA_MODULE_HOST_14_013: [If the JVM was previously created, the function shall get a pointer to that JavaVM pointer and JNIEnv environment pointer.]*/
	if (result == JNI_EEXIST)
	{
		jsize vmCount;
		result = JNI_GetCreatedJavaVMs(jvm, 1, &vmCount);
		if (result == JNI_OK)
		{
			JNIFunc(*jvm, GetEnv, (void**)&env, jvm_args.version);
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
		/*Codes_SRS_JAVA_MODULE_HOST_14_014: [This function shall increment the JAVA_MODULE_COUNT global variable.]*/
		JAVA_MODULE_COUNT++;
	}

	//Free up any memory used when initializing the JavaVMInitArgs
	deinit_vm_options(&jvm_args);

	return result;
}

static void JVM_Destroy(JavaVM** jvm, JNIEnv* env)
{
	JNIFunc(*jvm, AttachCurrentThread, (void**)&env, NULL);
	//TODO: Catch return
	JNIFunc(*jvm, DestroyJavaVM);
	JNIFunc(*jvm, DetachCurrentThread);
}

static void init_vm_options(JavaVMInitArgs* jvm_args, JVM_OPTIONS* jvm_options)
{
	if (jvm_options == NULL)
	{
		/*Codes_SRS_JAVA_MODULE_HOST_14_010: [If configuration->options is NULL, JavaVMInitArgs shall be initialized using default values.]*/
		//TODO: use version from jvm options and have this function return a value
		jvm_args->version = JNI_VERSION_1_8;
		int result = JNI_GetDefaultJavaVMInitArgs(jvm_args);
	}
	else
	{
		/*Codes_SRS_JAVA_MODULE_HOST_14_011: [This function shall allocate memory for an array of JavaVMOption structures and initialize each with each option provided.]*/
		int options_count = 0;

		options_count += (jvm_options->class_path != NULL ? 1 : 0);
		options_count += (jvm_options->library_path != NULL ? 1 : 0);
		options_count += (jvm_options->debug == true ? 3 : 0);
		options_count += (jvm_options->verbose == true ? 1 : 0);
		options_count += VECTOR_size(jvm_options->additional_options);

		switch (jvm_options->version)
		{
		case 1:
			(*jvm_args).version = JNI_VERSION_1_1;
			break;
		case 2:
			(*jvm_args).version = JNI_VERSION_1_2;
			break;
		case 4:
			(*jvm_args).version = JNI_VERSION_1_4;
			break;
		case 6:
			(*jvm_args).version = JNI_VERSION_1_6;
			break;
		case 8:
			(*jvm_args).version = JNI_VERSION_1_8;
			break;
		default:
			(*jvm_args).version = JNI_VERSION_1_8;
		}

		//Create JavaVMOption structure and set elements.
		JavaVMOption* options = (JavaVMOption*)malloc(sizeof(JavaVMOption)*options_count);
		(*jvm_args).nOptions = options_count;
		(*jvm_args).options = options;
		(*jvm_args).ignoreUnrecognized = 0;

		//Set all options
		if (jvm_options->class_path != NULL)
		{
			char* cp = (char*)malloc(strlen("-Djava.class.path") + 1 + strlen(jvm_options->class_path) + 1);
			sprintf(cp, "%s=%s", "-Djava.class.path", jvm_options->class_path);
			options[--options_count].optionString = cp;
		}
		if (jvm_options->library_path != NULL)
		{
			char* lp = (char*)malloc(strlen("-Djava.library.path") + 1 + strlen(jvm_options->library_path) + 1);
			sprintf(lp, "%s=%s", "-Djava.library.path", jvm_options->library_path);
			options[--options_count].optionString = lp;
		}
		if (jvm_options->debug == 1) {
			char *debug_str = (char*)malloc(strlen("-Xrunjdwp:transport=dt_socket,address=0000,server=y,suspend=y") + 2);
			sprintf(debug_str, "-Xrunjdwp:transport=dt_socket,address=%i,server=y,suspend=y", jvm_options->debug_port != 0 ? jvm_options->debug_port : DEBUG_PORT_DEFAULT);
			options[--options_count].optionString = "-Xrs";
			options[--options_count].optionString = "-Xdebug";
			options[--options_count].optionString = debug_str;
		}
		if (jvm_options->verbose == 1) {
			options[--options_count].optionString = "-verbose:jni";
		}
		if (jvm_options->additional_options != NULL) {
			for (size_t opt_index = 0; opt_index < VECTOR_size(jvm_options->additional_options); ++opt_index) {
				options[--options_count].optionString = (char*)STRING_c_str(VECTOR_element(jvm_options->additional_options, opt_index));
			}
		}
	}
}

static void deinit_vm_options(JavaVMInitArgs* jvm_args)
{
	for (jint options_count = 0; options_count < jvm_args->nOptions; options_count++)
	{
		free((*jvm_args).options[options_count].optionString);
	}
	free((*jvm_args).options);
}

static void destroy_module_internal(JAVA_MODULE_HANDLE_DATA* module)
{
	JAVA_MODULE_COUNT--;
	if (JAVA_MODULE_COUNT == 0)
	{
		JVM_Destroy(&(module->jvm), module->env);
	}
	free(module->moduleName);
	free(module);
}

static const MODULE_APIS JavaModuleHost_APIS =
{
	JavaModuleHost_Create,
	JavaModuleHost_Destroy,
	JavaModuleHost_Receive
};

#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(JAVA_MODULE_HOST)(void)
#else
MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void)
#endif
{
	return &JavaModuleHost_APIS;
}