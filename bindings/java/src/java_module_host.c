// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef __CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "message_bus_proxy.h"
#include "java_module_host.h"
#include "parson.h"

static MODULE_HANDLE JavaModuleHost_Create(MESSAGE_BUS_HANDLE bus, const void* configuration)
{
	MODULE_HANDLE result;
	if(bus != NULL && configuration != NULL)
	{
		JAVA_MODULE_HOST_CONFIG* config = (JAVA_MODULE_HOST_CONFIG*)configuration;
		
		/*Codes_SRS_JAVA_MODULE_HOST_14_003: [This function shall return NULL if configuration->configuration_json is not valid JSON.]*/
		/*Codes_SRS_JAVA_MODULE_HOST_14_004: [This function shall return NULL if class_name is NULL.]*/
		/*Codes_SRS_JAVA_MODULE_HOST_14_005: [This function shall return NULL upon any underlying API call failure.]*/
		/*Codes_SRS_JAVA_MODULE_HOST_14_006: [This function shall return a non-NULL MODULE_HANDLE when successful.]*/
		/*Codes_SRS_JAVA_MODULE_HOST_14_007: [This function shall allocate memory for an instance of a JAVA_MODULE_HANDLE structure to be used as the backing structure for this module.]*/
		/*Codes_SRS_JAVA_MODULE_HOST_14_008: [This function shall initize a** JavaVMInitArgs structure using the JVM_OPTIONS **structure configuration->options.]*/
		/*Codes_SRS_JAVA_MODULE_HOST_14_009: [If configuration->options is NULL, JavaVMInitArgs shall be initialized using default values.]*/
		/*Codes_SRS_JAVA_MODULE_HOST_14_010: [This function shall allocate memory for an array of JavaVMOption structures and initialize each with each option provided.]*/
		/*Codes_SRS_JAVA_MODULE_HOST_14_011: [If this is the first Java module to load, this function shall create the JVM using the JavaVMInitArgs through a call to JNI_CreateJavaVM and save the JavaVM and JNIEnv pointers in the JAVA_MODULE_HANDLE.]*/
		/*Codes_SRS_JAVA_MODULE_HOST_14_012: [If the JVM was previously created, the function shall get a pointer to that JavaVM pointer and JNIEnv environment pointer.]*/
		/*Codes_SRS_JAVA_MODULE_HOST_14_013: [This function shall increment the JAVA_MODULE_COUNT global variable.]*/
		/*Codes_SRS_JAVA_MODULE_HOST_14_014: [This function shall return NULL if a JVM could not be created or found.]*/
		/*Codes_SRS_JAVA_MODULE_HOST_14_015: [This function shall find the MessageBus Java class, get the constructor, and create a MessageBus Java object.]*/
		/*Codes_SRS_JAVA_MODULE_HOST_14_016: [This function shall find the user - defined Java module class using configuration->class_name, get the constructor, and create an instance of this module object.]*/
		/*Codes_SRS_JAVA_MODULE_HOST_14_017: [This function shall return NULL if any returned jclass, jmethodID, or jobject is NULL.]*/
		/*Codes_SRS_JAVA_MODULE_HOST_14_018: [This function shall return NULL if any JNI function fails.]*/
		/*Codes_SRS_JAVA_MODULE_HOST_14_019: [The function shall save a new global reference to the Java module object in JAVA_MODULE_HANDLE_DATA->module.]*/
	}
	else
	{
		/*Codes_SRS_JAVA_MODULE_HOST_14_001: [This function shall return NULL if bus is NULL.]*/
		/*Codes_SRS_JAVA_MODULE_HOST_14_002: [This function shall return NULL if configuration is NULL.]*/
		result = NULL;
	}
	return result;
}

static void JavaModuleHost_Destroy(MODULE_HANDLE module)
{

}

static void JavaModuleHost_Receive(MODULE_HANDLE module, MESSAGE_HANDLE message)
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