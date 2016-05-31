// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef __CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <stdio.h>
#include "module.h"
#include "azure_c_shared_utility\iot_logging.h"
#include "java_module_host.h"
#include "java_module_host_hl.h"
#include "parson.h"

static void parse_jvm_options_internal(JAVA_MODULE_HOST_CONFIG*, JSON_Object*);

static MODULE_HANDLE JavaModuleHost_HL_Create(MESSAGE_BUS_HANDLE bus, const void* configuration)
{
	MODULE_HANDLE result;
	if (bus == NULL || configuration == NULL)
	{
		/*Codes_SRS_JAVA_MODULE_HOST_HL_14_002: [This function shall return NULL if bus is NULL or configuration is NULL.]*/
		LogError("Invalid input (bus = %p, configuration = %p).", bus, configuration);
		result = NULL;
	}
	else
	{
		/*Codes_SRS_JAVA_MODULE_HOST_HL_14_003: [This function shall return NULL if configuration is not a valid JSON object.]*/
		JSON_Value* json = json_parse_string((const char*)configuration);
		if (json == NULL)
		{
			LogError("Unable to parse the JSON string input.");
			result = NULL;
		}
		else
		{
			JSON_Object* obj = json_value_get_object(json);
			if (obj == NULL)
			{
				LogError("Unable to get the JSON object.");
				result = NULL;
			}
			else
			{
				/*Codes_SRS_JAVA_MODULE_HOST_HL_14_004: [This function shall return NULL if configuration.args does not contain a field named class_name.]*/
				if (json_object_get_string(obj, "class_name") == NULL)
				{
					LogError("No field 'class_name' in JSON object.");
					result = NULL;
				}
				else
				{
					/*Codes_SRS_JAVA_MODULE_HOST_HL_14_005: [This function shall parse the configuration.args JSON object and initialize a new JAVA_MODULE_HOST_CONFIG setting default values to all missing fields.]*/
					JAVA_MODULE_HOST_CONFIG config;
					JVM_OPTIONS options;
					config.options = &options;
					parse_jvm_options_internal(&config, obj);
					
					/*Codes_SRS_JAVA_MODULE_HOST_HL_14_006: [This function shall pass bus and the newly created JAVA_MODULE_HOST_CONFIG structure to JavaModuleHost_Create.]*/
					result = MODULE_STATIC_GETAPIS(JAVA_MODULE_HOST)()->Module_Create(bus, &config);

					/*Codes_SRS_JAVA_MODULE_HOST_HL_14_007: [This function shall fail or succeed after this function call and return the value from this function call.]*/
					if (result == NULL)
					{
						LogError("Unable to create Java Module.");
					}

					//Cleanup
					for (size_t index = 0; index < VECTOR_size(options.additional_options); index++)
					{
						STRING_delete(*((STRING_HANDLE*)VECTOR_element(options.additional_options, index)));
					}
					free(config.configuration_json);
					VECTOR_destroy(options.additional_options);
				}
			}
			json_value_free(json);
		}
	}
	return result;
}

static void JavaModuleHost_HL_Destroy(MODULE_HANDLE module)
{
	/*Codes_SRS_JAVA_MODULE_HOST_HL_14_008: [ This function shall call the underlying Java Module Host's _Destroy function using this module MODULE_HANDLE. ]*/
	MODULE_STATIC_GETAPIS(JAVA_MODULE_HOST)()->Module_Destroy(module);
}

static void JavaModuleHost_HL_Receive(MODULE_HANDLE module, MESSAGE_HANDLE message)
{
	/*Codes_SRS_JAVA_MODULE_HOST_HL_14_009: [ This function shall call the underlying Java Module Host's _Receive function using this module MODULE_HANDLE and message MESSAGE_HANDLE. ]*/
	MODULE_STATIC_GETAPIS(JAVA_MODULE_HOST)()->Module_Receive(module, message);
}

static void parse_jvm_options_internal(JAVA_MODULE_HOST_CONFIG* config, JSON_Object* obj)
{
	config->class_name = json_object_get_string(obj, "class_name");
	config->configuration_json = json_serialize_to_string(json_object_get_value(obj, "args"));
	config->options->class_path = json_object_get_string(obj, "class_path");
	config->options->library_path = json_object_get_string(obj, "library_path");

	JSON_Object* jvm_options = json_object_get_object(obj, "jvm_options");
	config->options->version = (int)json_object_get_number(jvm_options, "version");
	config->options->debug = json_object_get_boolean(jvm_options, "debug") == 1 ? true : false;
	config->options->debug_port = (int)json_object_get_number(jvm_options, "debug_port");
	config->options->verbose = json_object_get_boolean(jvm_options, "verbose") == 1 ? true : false;

	JSON_Array* arr = json_object_get_array(jvm_options, "additional_options");
	size_t addition_options_count = json_array_get_count(arr);
	if(addition_options_count != 0)
	{
		//TODO: Check if vector created properly
		config->options->additional_options = VECTOR_create(sizeof(STRING_HANDLE));
		for (size_t index = 0; index < addition_options_count; index++) {
			//TODO: Check each string
			STRING_HANDLE str = STRING_construct(json_array_get_string(arr, index));
			VECTOR_push_back(config->options->additional_options, &str, 1);
		}
	}
}

static const MODULE_APIS JavaModuleHost_HL_APIS =
{
	JavaModuleHost_HL_Create,
	JavaModuleHost_HL_Destroy,
	JavaModuleHost_HL_Receive
};

#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(JAVA_MODULE_HOST_HL)(void)
#else
MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void)
#endif
{
	return &JavaModuleHost_HL_APIS;
}
