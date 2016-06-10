// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef __CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#ifdef UNDER_TEST /*write here a comment*/

#define _JAVASOFT_JNI_MD_H_
#define JNIEXPORT
#define JNIIMPORT
#define JNICALL

typedef long jint;
typedef __int64 jlong;
typedef signed char jbyte;

#include <jni.h>

#endif

#include "message_bus_proxy.h"
#include "java_module_host_common.h"
#include "java_module_host.h"
#include "azure_c_shared_utility\iot_logging.h"
#include "azure_c_shared_utility\gballoc.h"
//#include <vld.h>

#define JNIFunc(jptr, call, ...) (*(jptr))->call(jptr, __VA_ARGS__)

typedef struct JAVA_MODULE_HANDLE_DATA_TAG
{
	JavaVM* jvm;
	JNIEnv* env;
	jobject module;
	char* moduleName;
	JAVA_MODULE_HOST_MANAGER_HANDLE manager;
}JAVA_MODULE_HANDLE_DATA;

static int JVM_Create(JavaVM**, JNIEnv**, JVM_OPTIONS*);
static void JVM_Destroy(JavaVM**, JNIEnv*);
static void destroy_module_internal(JAVA_MODULE_HANDLE_DATA*);
static int init_vm_options(JavaVMInitArgs*, VECTOR_HANDLE*, JVM_OPTIONS*);
static void deinit_vm_options(JavaVMInitArgs*, VECTOR_HANDLE);

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
			/*Codes_SRS_JAVA_MODULE_HOST_14_003: [This function shall return NULL if class_name is NULL.]*/
			LogError("Invalid input. configuration->class_name cannot be NULL.");
			result = NULL;
		}
		else
		{
			/*Codes_SRS_JAVA_MODULE_HOST_14_006: [This function shall allocate memory for an instance of a JAVA_MODULE_HANDLE_DATA structure to be used as the backing structure for this module.]*/
			result = (JAVA_MODULE_HANDLE_DATA*)malloc(sizeof(JAVA_MODULE_HANDLE_DATA));
			if (result == NULL)
			{
				/*Codes_SRS_JAVA_MODULE_HOST_14_004: [This function shall return NULL upon any underlying API call failure.]*/
				LogError("Malloc failure.");
			}
			else
			{
				//TODO: Requirements for this
				result->env = NULL;
				result->jvm = NULL;

				result->moduleName = (char*)config->class_name;
				result->manager = JavaModuleHostManager_Create();
				if (result->manager == NULL)
				{
					LogError("Failed to create a JAVA_MODULE_HOST_MANAGER_HANDLE.");
					destroy_module_internal(result);
					result = NULL;
				}
				else
				{
					if (JVM_Create(&(result->jvm), &(result->env), config->options) != JNI_OK)
					{
						/*Codes_SRS_JAVA_MODULE_HOST_14_013: [This function shall return NULL if a JVM could not be created or found.]*/
						LogError("Failed to successfully create JVM.");
						destroy_module_internal(result);
						result = NULL;
					}
					else
					{
						/*Codes_SRS_JAVA_MODULE_HOST_14_012: [This function shall increment the JAVA_MODULE_COUNT global variable.]*/
						if (JavaModuleHostManager_Add(result->manager) == MANAGER_ERROR)
						{
							LogError("JavaModuleHostManager_Add failed.");
							destroy_module_internal(result);
							result = NULL;
						}
						else
						{
							/*Codes_SRS_JAVA_MODULE_HOST_14_014: [This function shall find the MessageBus Java class, get the constructor, and create a MessageBus Java object.]*/
							jclass jMessageBus_class = JNIFunc(result->env, FindClass, MESSAGE_BUS_CLASS_NAME);
							jthrowable exception = JNIFunc(result->env, ExceptionOccurred);
							if (jMessageBus_class == NULL || exception)
							{
								/*Codes_SRS_JAVA_MODULE_HOST_14_016: [This function shall return NULL if any returned jclass, jmethodID, or jobject is NULL.]*/
								/*Codes_SRS_JAVA_MODULE_HOST_14_017: [This function shall return NULL if any JNI function fails.]*/
								LogError("Could not find class (%s).", MESSAGE_BUS_CLASS_NAME);
								JNIFunc(result->env, ExceptionClear);
								destroy_module_internal(result);
								result = NULL;
							}
							else
							{
								jmethodID jMessageBus_constructor = JNIFunc(result->env, GetMethodID, jMessageBus_class, CONSTRUCTOR_METHOD_NAME, MESSAGE_BUS_CONSTRUCTOR_DESCRIPTOR);
								exception = JNIFunc(result->env, ExceptionOccurred);
								if (jMessageBus_constructor == NULL || exception)
								{
									/*Codes_SRS_JAVA_MODULE_HOST_14_016: [This function shall return NULL if any returned jclass, jmethodID, or jobject is NULL.]*/
									/*Codes_SRS_JAVA_MODULE_HOST_14_017: [This function shall return NULL if any JNI function fails.]*/
									LogError("Failed to find the %s constructor.", MESSAGE_BUS_CLASS_NAME);
									JNIFunc(result->env, ExceptionClear);
									destroy_module_internal(result);
									result = NULL;
								}
								else
								{
									jobject jMessageBus_object = JNIFunc(result->env, NewObject, jMessageBus_class, jMessageBus_constructor, (jlong)bus);
									exception = JNIFunc(result->env, ExceptionOccurred);
									if (jMessageBus_object == NULL || exception)
									{
										/*Codes_SRS_JAVA_MODULE_HOST_14_016: [This function shall return NULL if any returned jclass, jmethodID, or jobject is NULL.]*/
										/*Codes_SRS_JAVA_MODULE_HOST_14_017: [This function shall return NULL if any JNI function fails.]*/
										LogError("Failed to create the %s object.", MESSAGE_BUS_CLASS_NAME);
										JNIFunc(result->env, ExceptionClear);
										destroy_module_internal(result);
										result = NULL;
									}
									else
									{
										/*Codes_SRS_JAVA_MODULE_HOST_14_015: [This function shall find the user-defined Java module class using configuration->class_name, get the constructor, and create an instance of this module object.]*/
										jclass jModule_class = JNIFunc(result->env, FindClass, result->moduleName);
										exception = JNIFunc(result->env, ExceptionOccurred);
										if (jModule_class == NULL || exception)
										{
											/*Codes_SRS_JAVA_MODULE_HOST_14_016: [This function shall return NULL if any returned jclass, jmethodID, or jobject is NULL.]*/
											/*Codes_SRS_JAVA_MODULE_HOST_14_017: [This function shall return NULL if any JNI function fails.]*/
											LogError("Could not find class (%s).", result->moduleName);
											JNIFunc(result->env, ExceptionClear);
											destroy_module_internal(result);
											result = NULL;
										}
										else
										{
											jmethodID jModule_constructor = JNIFunc(result->env, GetMethodID, jModule_class, CONSTRUCTOR_METHOD_NAME, MODULE_CONSTRUCTOR_DESCRIPTOR);
											exception = JNIFunc(result->env, ExceptionOccurred);
											if (jModule_constructor == NULL || exception)
											{
												/*Codes_SRS_JAVA_MODULE_HOST_14_016: [This function shall return NULL if any returned jclass, jmethodID, or jobject is NULL.]*/
												/*Codes_SRS_JAVA_MODULE_HOST_14_017: [This function shall return NULL if any JNI function fails.]*/
												LogError("Failed to find the %s constructor.", result->moduleName);
												JNIFunc(result->env, ExceptionClear);
												destroy_module_internal(result);
												result = NULL;
											}
											else
											{
												jstring jModule_configuration = JNIFunc(result->env, NewStringUTF, config->configuration_json);
												exception = JNIFunc(result->env, ExceptionOccurred);
												if (jModule_configuration == NULL || exception)
												{
													/*Codes_SRS_JAVA_MODULE_HOST_14_016: [This function shall return NULL if any returned jclass, jmethodID, or jobject is NULL.]*/
													/*Codes_SRS_JAVA_MODULE_HOST_14_017: [This function shall return NULL if any JNI function fails.]*/
													LogError("Failed to create a new Java String.");
													JNIFunc(result->env, ExceptionClear);
													destroy_module_internal(result);
													result = NULL;
												}
												else
												{
													jobject jModule_object = JNIFunc(result->env, NewObject, jModule_class, jModule_constructor, (jlong)result, jMessageBus_object, jModule_configuration);
													exception = JNIFunc(result->env, ExceptionOccurred);
													if (jModule_object == NULL || exception)
													{
														/*Codes_SRS_JAVA_MODULE_HOST_14_016: [This function shall return NULL if any returned jclass, jmethodID, or jobject is NULL.]*/
														/*Codes_SRS_JAVA_MODULE_HOST_14_017: [This function shall return NULL if any JNI function fails.]*/
														LogError("Failed to create the %s object.", result->moduleName);
														JNIFunc(result->env, ExceptionDescribe);
														JNIFunc(result->env, ExceptionClear);
														destroy_module_internal(result);
														result = NULL;
													}
													else
													{
														/*Codes_SRS_JAVA_MODULE_HOST_14_018: [The function shall save a new global reference to the Java module object in JAVA_MODULE_HANDLE_DATA->module.]*/
														result->module = JNIFunc(result->env, NewGlobalRef, jModule_object);
														if (result->module == NULL)
														{
															LogError("Failed to get a global reference to the module Java object (%s). System ran out of memory.", result->moduleName);
															destroy_module_internal(result);
															result = NULL;
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	/*Codes_SRS_JAVA_MODULE_HOST_14_005: [This function shall return a non-NULL MODULE_HANDLE when successful.]*/
	return result;
}

static void JavaModuleHost_Destroy(MODULE_HANDLE module)
{
	/*Codes_SRS_JAVA_MODULE_HOST_14_019: [This function shall do nothing if module is NULL.]*/
	if (module != NULL)
	{
		JAVA_MODULE_HANDLE_DATA* moduleHandle = (JAVA_MODULE_HANDLE_DATA *)module;

		/*Codes_SRS_JAVA_MODULE_HOST_14_020: [This function shall call the void destroy() method of the Java module object and delete the global reference to this object.]*/
		jint jni_result = JNIFunc(moduleHandle->jvm, AttachCurrentThread, (void**)(&(moduleHandle->env)), NULL);
		if (jni_result == JNI_OK)
		{
			jclass jModule_class = JNIFunc(moduleHandle->env, GetObjectClass, moduleHandle->module);
			if (jModule_class == NULL)
			{
				LogError("Could not find class (%s) for the module Java object. destroy() will not be called on this object.", moduleHandle->moduleName);
			}
			else
			{
				jmethodID jModule_destroy = JNIFunc(moduleHandle->env, GetMethodID, jModule_class, MODULE_DESTROY_METHOD_NAME, MODULE_DESTROY_DESCRIPTOR);
				jthrowable exception = JNIFunc(moduleHandle->env, ExceptionOccurred);
				if (jModule_destroy == NULL || exception)
				{
					LogError("Failed to find the %s destroy() method. destroy() will not be called on this object.", moduleHandle->moduleName);
					JNIFunc(moduleHandle->env, ExceptionClear);
				}
				else
				{
					JNIFunc(moduleHandle->env, CallVoidMethod, moduleHandle->module, jModule_destroy);
					exception = JNIFunc(moduleHandle->env, ExceptionOccurred);
					if (exception)
					{
						LogError("Exception occurred in destroy() of %s.", moduleHandle->moduleName);
						JNIFunc(moduleHandle->env, ExceptionDescribe);
						JNIFunc(moduleHandle->env, ExceptionClear);
					}

					JNIFunc(moduleHandle->env, DeleteGlobalRef, moduleHandle->module);
					jni_result = JNIFunc(moduleHandle->jvm, DetachCurrentThread);
					if (jni_result != JNI_OK)
					{
						LogError("Could not detach the current thread from the JVM. (Result: %i)", jni_result);
					}

					/*Codes_SRS_JAVA_MODULE_HOST_14_029: [This function shall destroy the JVM if it the last module to be disconnected from the gateway.]*/
					/*Codes_SRS_JAVA_MODULE_HOST_14_021: [This function shall free all resources associated with this module.]*/
					destroy_module_internal(moduleHandle);
				}
			}
		}
		else
		{
			LogError("Could not attach the current thread to the JVM. (Result: %i)", jni_result);
		}
	}
}

static void JavaModuleHost_Receive(MODULE_HANDLE module, MESSAGE_HANDLE message)
{
	/*Codes_SRS_JAVA_MODULE_HOST_14_022: [This function shall do nothing if module or message is NULL.]*/
	if (module != NULL && message != NULL)
	{
		JAVA_MODULE_HANDLE_DATA* moduleHandle = (JAVA_MODULE_HANDLE_DATA*)module;

		/*Codes_SRS_JAVA_MODULE_HOST_14_023: [This function shall serialize message.]*/
		int32_t size;
		unsigned char* serialized_message = (unsigned char*)Message_ToByteArray(message, &size);

		/*Codes_SRS_JAVA_MODULE_HOST_14_024: [This function shall call the void receive(byte[] source) method of the Java module object passing the serialized message.]*/
		jint jni_result = JNIFunc(moduleHandle->jvm, AttachCurrentThread, (void**)(&(moduleHandle->env)), NULL);

		if (jni_result == JNI_OK)
		{
			//TODO: error check
			jbyteArray arr = JNIFunc(moduleHandle->env, NewByteArray, size);
			if (arr == NULL)
			{
				LogError("New jbyteArray could not be constructed.");
			}
			else
			{
				JNIFunc(moduleHandle->env, SetByteArrayRegion, arr, 0, size, serialized_message);
				jthrowable exception = JNIFunc(moduleHandle->env, ExceptionOccurred);
				if (exception)
				{
					LogError("Exception occurred in SetByteArrayRegion.");
					JNIFunc(moduleHandle->env, ExceptionClear);
				}
				else
				{
					jclass jModule_class = JNIFunc(moduleHandle->env, GetObjectClass, moduleHandle->module);
					if (jModule_class == NULL)
					{
						LogError("Could not find class (%s) for the module Java object. destroy() will not be called on this object.", moduleHandle->moduleName);
					}
					else
					{
						jmethodID jModule_receive = JNIFunc(moduleHandle->env, GetMethodID, jModule_class, MODULE_RECEIVE_METHOD_NAME, MODULE_RECEIVE_DESCRIPTOR);
						exception = JNIFunc(moduleHandle->env, ExceptionOccurred);
						if (jModule_receive == NULL || exception)
						{
							LogError("Failed to find the %s destroy() method. destroy() will not be called on this object.", moduleHandle->moduleName);
							JNIFunc(moduleHandle->env, ExceptionClear);
						}
						else
						{
							JNIFunc(moduleHandle->env, CallVoidMethod, moduleHandle->module, jModule_receive, arr);
							exception = JNIFunc(moduleHandle->env, ExceptionOccurred);
							if (exception)
							{
								LogError("Exception occurred in receive() of %s.", moduleHandle->moduleName);
								JNIFunc(moduleHandle->env, ExceptionDescribe);
								JNIFunc(moduleHandle->env, ExceptionClear);
							}
						}
					}
					JNIFunc(moduleHandle->env, DeleteLocalRef, arr);
				}
			}
			JNIFunc(moduleHandle->jvm, DetachCurrentThread);
		}

		free(serialized_message);
	}

}

JNIEXPORT jint JNICALL Java_com_microsoft_azure_gateway_core_MessageBus_publishMessage(JNIEnv* env, jobject jMessageBus, jlong module_address, jlong bus_address, jbyteArray serialized_message)
{
	MESSAGE_BUS_RESULT result = MESSAGE_BUS_ERROR;

	MESSAGE_BUS_HANDLE bus = (MESSAGE_BUS_HANDLE)bus_address;
	JAVA_MODULE_HANDLE_DATA* moduleHandle = (JAVA_MODULE_HANDLE_DATA*)module_address;

	JNIFunc(moduleHandle->jvm, AttachCurrentThread, (void**)(&(moduleHandle->env)), NULL);

	/*Codes_SRS_JAVA_MODULE_HOST_14_025: [This function shall use convert the jbyteArray message into an unsigned char array.]*/
	size_t length = JNIFunc(moduleHandle->env, GetArrayLength, serialized_message);
	unsigned char* arr = (unsigned char*)malloc(length);
	JNIFunc(moduleHandle->env, GetByteArrayRegion, serialized_message, 0, length, arr);
	jthrowable exception = JNIFunc(moduleHandle->env, ExceptionOccurred);
	if (exception)
	{
		LogError("Exception occured in GetByteArrayRegion.");
		JNIFunc(moduleHandle->env, ExceptionClear);
	}
	JNIFunc(moduleHandle->jvm, DetachCurrentThread);

	/*Codes_SRS_JAVA_MODULE_HOST_14_026: [This function shall use the serialized message in a call to Message_Create.]*/
	MESSAGE_HANDLE message = Message_CreateFromByteArray(arr, length);

	/*Codes_SRS_JAVA_MODULE_HOST_14_027: [This function shall publish the message to the MESSAGE_BUS_HANDLE addressed by addr and return the value of this function call.]*/
	result = MessageBus_Publish(bus, message);

	//Cleanup
	free(arr);
	Message_Destroy(message);

	return result;
	
}

//Internal functions
static int JVM_Create(JavaVM** jvm, JNIEnv** env, JVM_OPTIONS* options)
{
	/*Codes_SRS_JAVA_MODULE_HOST_14_007: [This function shall initialize a JavaVMInitArgs structure using the JVM_OPTIONS structure configuration->options.]*/
	JavaVMInitArgs jvm_args;
	VECTOR_HANDLE options_strings = NULL;
	int result = init_vm_options(&jvm_args, &options_strings, options);

	if (result == JNI_OK)
	{
		/*Codes_SRS_JAVA_MODULE_HOST_14_010: [If this is the first Java module to load, this function shall create the JVM using the JavaVMInitArgs through a call to JNI_CreateJavaVM and save the JavaVM and JNIEnv pointers in the JAVA_MODULE_HANDLE_DATA.]*/
		result = JNI_CreateJavaVM(jvm, (void**)env, &jvm_args);

		/*Codes_SRS_JAVA_MODULE_HOST_14_011: [If the JVM was previously created, the function shall get a pointer to that JavaVM pointer and JNIEnv environment pointer.]*/
		if (result == JNI_EEXIST)
		{
			jsize vmCount;
			result = JNI_GetCreatedJavaVMs(jvm, 1, &vmCount);
			if (result == JNI_OK)
			{
				JNIFunc(*jvm, GetEnv, (void**)env, jvm_args.version);
			}
		}

		if (result < 0 || !(*env))
		{
			/*Codes_SRS_JAVA_MODULE_HOST_14_013: [This function shall return NULL if a JVM could not be created or found.]*/
			/*Codes_SRS_JAVA_MODULE_HOST_14_017: [This function shall return NULL if any JNI function fails.]*/
			LogError("Failed to launch JVM. JNI_CreateJavaVM returned: %d.", result);
		}

		//Free up any memory used when initializing the JavaVMInitArgs
		deinit_vm_options(&jvm_args, options_strings);
	}
	return result;
}

static void JVM_Destroy(JavaVM** jvm, JNIEnv* env)
{
	if ((jvm && env) && (*jvm && &env))
	{
		//TODO: Attach unnecessary
		JNIFunc(*jvm, AttachCurrentThread, (void**)&env, NULL);
		//TODO: Catch return
		JNIFunc(*jvm, DestroyJavaVM);
	}
}

static int init_vm_options(JavaVMInitArgs* jvm_args, VECTOR_HANDLE* options_strings, JVM_OPTIONS* jvm_options)
{
	int result = 0;
	if (jvm_options == NULL)
	{
		/*Codes_SRS_JAVA_MODULE_HOST_14_008: [If configuration->options is NULL, JavaVMInitArgs shall be initialized using default values.]*/
		//TODO: use version from jvm options and have this function return a value
		jvm_args->version = JNI_VERSION_1_8;
		jvm_args->options = NULL;
		result = JNI_GetDefaultJavaVMInitArgs(jvm_args);
	}
	else
	{
		/*Codes_SRS_JAVA_MODULE_HOST_14_031:[The function shall create a new VECTOR_HANDLE to store the option strings.]*/
		*options_strings = VECTOR_create(sizeof(STRING_HANDLE));

		if (*options_strings == NULL)
		{
			LogError("Failed to create VECTOR_HANDLE for options strings.");
			result = __LINE__;
		}
		else
		{
			int options_count = 0;

			options_count += (jvm_options->class_path != NULL ? 1 : 0);
			options_count += (jvm_options->library_path != NULL ? 1 : 0);
			options_count += (jvm_options->debug == true ? 3 : 0);
			options_count += (jvm_options->verbose == true ? 1 : 0);
			options_count += VECTOR_size(jvm_options->additional_options);

			//Create JavaVMOption structure and set elements.
			/*Codes_SRS_JAVA_MODULE_HOST_14_009: [This function shall allocate memory for an array of JavaVMOption structures and initialize each with each option provided.]*/
			JavaVMOption* options = options_count == 0 ? NULL : (JavaVMOption*)malloc(sizeof(JavaVMOption)*options_count);
			if (options == NULL && options_count != 0)
			{
				LogError("Failed to allocate memory for JavaVMOption.");
				result = __LINE__;
			}
			else
			{
				/*Codes_SRS_JAVA_MODULE_HOST_14_030:[ The function shall set the JavaVMInitArgs structures nOptions, version and JavaVMOption* options member variables]*/
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
				(*jvm_args).nOptions = options_count;
				(*jvm_args).options = options;
				(*jvm_args).ignoreUnrecognized = 0;

				//Set all options
				if (jvm_options->class_path != NULL && result == 0)
				{
					/*Codes_SRS_JAVA_MODULE_HOST_14_032:[ The function shall construct a new STRING_HANDLE for each option. ]*/
					STRING_HANDLE class_path = STRING_construct("-Djava.class.path=");
					if (class_path == NULL)
					{
						LogError("String_construct failed.");
						result = __LINE__;
					}
					else
					{
						/*Codes_SRS_JAVA_MODULE_HOST_14_033:[The function shall concatenate the user supplied options to the option key names.]*/
						if (STRING_concat(class_path, jvm_options->class_path) != 0)
						{
							/*Codes_SRS_JAVA_MODULE_HOST_14_035:[ If any operation fails, the function shall delete the STRING_HANDLE structures, VECTOR_HANDLE and JavaVMOption array. ]*/
							LogError("String_concat failed.");
							STRING_delete(class_path);
							result = __LINE__;
						}
						else
						{
							/*Codes_SRS_JAVA_MODULE_HOST_14_034:[ The function shall push the new STRING_HANDLE onto the newly created vector. ]*/
							if (VECTOR_push_back(*options_strings, &class_path, 1) != 0)
							{
								/*Codes_SRS_JAVA_MODULE_HOST_14_035:[ If any operation fails, the function shall delete the STRING_HANDLE structures, VECTOR_HANDLE and JavaVMOption array. ]*/
								LogError("Failed to push class path onto vector.");
								STRING_delete(class_path);
								result = __LINE__;
							}
							else
							{
								options[--options_count].optionString = (char*)STRING_c_str(class_path);
							}
						}
					}
				}
				if (jvm_options->library_path != NULL && result == 0)
				{
					/*Codes_SRS_JAVA_MODULE_HOST_14_032:[ The function shall construct a new STRING_HANDLE for each option. ]*/
					STRING_HANDLE library_path = STRING_construct("-Djava.library.path=");
					if (library_path == NULL)
					{
						LogError("String_construct failed.");
						result = __LINE__;
					}
					else
					{
						/*Codes_SRS_JAVA_MODULE_HOST_14_033:[The function shall concatenate the user supplied options to the option key names.]*/
						if (STRING_concat(library_path, jvm_options->library_path) != 0)
						{
							/*Codes_SRS_JAVA_MODULE_HOST_14_035:[ If any operation fails, the function shall delete the STRING_HANDLE structures, VECTOR_HANDLE and JavaVMOption array. ]*/
							LogError("String_concat failed.");
							STRING_delete(library_path);
							result = __LINE__;
						}
						else
						{
							/*Codes_SRS_JAVA_MODULE_HOST_14_034:[ The function shall push the new STRING_HANDLE onto the newly created vector. ]*/
							if (VECTOR_push_back(*options_strings, &library_path, 1) != 0)
							{
								/*Codes_SRS_JAVA_MODULE_HOST_14_035:[ If any operation fails, the function shall delete the STRING_HANDLE structures, VECTOR_HANDLE and JavaVMOption array. ]*/
								LogError("Failed to push library path onto vector.");
								STRING_delete(library_path);
								result = __LINE__;
							}
							else
							{
								options[--options_count].optionString = (char*)STRING_c_str(library_path);
							}
						}
					}
				}
				if (jvm_options->debug == 1 && result == 0) {
					char debug_str[64];
					/*Codes_SRS_JAVA_MODULE_HOST_14_033:[The function shall concatenate the user supplied options to the option key names.]*/
					if (sprintf(debug_str, "-Xrunjdwp:transport=dt_socket,address=%i,server=y,suspend=y", jvm_options->debug_port != 0 ? jvm_options->debug_port : DEBUG_PORT_DEFAULT) < 0)
					{
						LogError("sprintf failed.");
						result = __LINE__;
					}
					else
					{
						/*Codes_SRS_JAVA_MODULE_HOST_14_032:[ The function shall construct a new STRING_HANDLE for each option. ]*/
						STRING_HANDLE debug_1 = STRING_construct("-Xrs");
						STRING_HANDLE debug_2 = STRING_construct("-Xdebug"); 
						STRING_HANDLE debug_3 = STRING_construct(debug_str);
						if (debug_1 == NULL || debug_2 == NULL || debug_3 == NULL)
						{
							/*Codes_SRS_JAVA_MODULE_HOST_14_035:[ If any operation fails, the function shall delete the STRING_HANDLE structures, VECTOR_HANDLE and JavaVMOption array. ]*/
							LogError("String_construct failed.");
							STRING_delete(debug_1);
							STRING_delete(debug_2);
							STRING_delete(debug_3);
							result = __LINE__;
						}
						else
						{
							/*Codes_SRS_JAVA_MODULE_HOST_14_034:[ The function shall push the new STRING_HANDLE onto the newly created vector. ]*/
							if (VECTOR_push_back(*options_strings, &debug_1, 1) != 0 || 
								VECTOR_push_back(*options_strings, &debug_2, 1) != 0 || 
								VECTOR_push_back(*options_strings, &debug_3, 1) != 0)
							{
								/*Codes_SRS_JAVA_MODULE_HOST_14_035:[ If any operation fails, the function shall delete the STRING_HANDLE structures, VECTOR_HANDLE and JavaVMOption array. ]*/
								LogError("Failed to push debug options onto vector.");
								STRING_delete(debug_1);
								STRING_delete(debug_2);
								STRING_delete(debug_3);
								result = __LINE__;
							}
							else
							{
								options[--options_count].optionString = (char*)STRING_c_str(debug_1);
								options[--options_count].optionString = (char*)STRING_c_str(debug_2);
								options[--options_count].optionString = (char*)STRING_c_str(debug_3);
							}
						}
					}
				}
				if (jvm_options->verbose == 1 && result == 0) {
					/*Codes_SRS_JAVA_MODULE_HOST_14_032:[ The function shall construct a new STRING_HANDLE for each option. ]*/
					STRING_HANDLE verbose_str = STRING_construct("-verbose:class");
					if (verbose_str == NULL)
					{
						LogError("String_construct failed.");
						result = __LINE__;
					}
					else
					{
						/*Codes_SRS_JAVA_MODULE_HOST_14_034:[ The function shall push the new STRING_HANDLE onto the newly created vector. ]*/
						if (VECTOR_push_back(*options_strings, &verbose_str, 1) != 0)
						{
							/*Codes_SRS_JAVA_MODULE_HOST_14_035:[ If any operation fails, the function shall delete the STRING_HANDLE structures, VECTOR_HANDLE and JavaVMOption array. ]*/
							LogError("Failed to push debug options onto vector.");
							STRING_delete(verbose_str);
							result = __LINE__;
						}
						else
						{
							options[--options_count].optionString = (char*)STRING_c_str(verbose_str);
						}
					}
				}
				if (jvm_options->additional_options != NULL && result == 0) {
					for (size_t opt_index = 0; opt_index < VECTOR_size(jvm_options->additional_options) && result == 0; ++opt_index) {
						STRING_HANDLE* s = (STRING_HANDLE*)VECTOR_element(jvm_options->additional_options, opt_index);
						if (s == NULL)
						{
							LogError("Vector_element failed.");
							result = __LINE__;
						}
						else
						{
							/*Codes_SRS_JAVA_MODULE_HOST_14_036:[ The function shall copy any additional user options into a STRING_HANDLE. ]*/
							STRING_HANDLE str = STRING_clone(*s);
							if (str == NULL)
							{
								LogError("STRING_clone failed.");
								result = __LINE__;
							}
							else
							{
								/*Codes_SRS_JAVA_MODULE_HOST_14_034:[ The function shall push the new STRING_HANDLE onto the newly created vector. ]*/
								if (VECTOR_push_back(*options_strings, &str, 1) != 0)
								{
									/*Codes_SRS_JAVA_MODULE_HOST_14_035:[ If any operation fails, the function shall delete the STRING_HANDLE structures, VECTOR_HANDLE and JavaVMOption array. ]*/
									LogError("Failed to push additional option (%s) onto vector.", STRING_c_str(str));
									STRING_delete(str);
									result = __LINE__;
								}
								else
								{
									options[--options_count].optionString = (char*)STRING_c_str(str);
								}
							}
						}
						
					}
				}
			}

			/*Codes_SRS_JAVA_MODULE_HOST_14_035:[ If any operation fails, the function shall delete the STRING_HANDLE structures, VECTOR_HANDLE and JavaVMOption array. ]*/
			if (result != 0)
			{
				deinit_vm_options(jvm_args, *options_strings);
			}
		}
	}
	return result;
}

static void deinit_vm_options(JavaVMInitArgs* jvm_args, VECTOR_HANDLE options_strings)
{
	if (options_strings != NULL)
	{
		for (size_t options_count = 0; options_count < VECTOR_size(options_strings); options_count++)
		{
			STRING_HANDLE* element = (STRING_HANDLE*)VECTOR_element(options_strings, options_count);
			if (element != NULL)
			{
				STRING_delete(*element);
			}
		}
		VECTOR_destroy(options_strings);
	}
	if ((*jvm_args).options != NULL)
	{
		free((*jvm_args).options);
	}
}

static void destroy_module_internal(JAVA_MODULE_HANDLE_DATA* module)
{
	if (JavaModuleHostManager_Remove(module->manager) == MANAGER_ERROR)
	{
		LogError("JavaModuleHostManager_Remove failed.");
	}

	LogInfo("Module Count: %i.", JavaModuleHostManager_Size(module->manager));
	if (JavaModuleHostManager_Size(module->manager) == 0)
	{
		LogInfo("Destroying JVM");
		JVM_Destroy(&(module->jvm), module->env);
	}
	JavaModuleHostManager_Destroy(module->manager);
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
	/*Codes_SRS_JAVA_MODULE_HOST_14_028: [ This function shall return a non-NULL pointer to a structure of type MODULE_APIS that has all fields non-NULL. ]*/
	return &JavaModuleHost_APIS;
}