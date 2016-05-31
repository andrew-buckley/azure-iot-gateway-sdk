// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef JAVA_MODULE_HOST_COMMON_H
#define JAVA_MODULE_HOST_COMMON_H

#ifdef _WIN32
    #define DEFAULT_CLASS_PATH "C:\\Users\\andbuc\\.m2\\repository\\com\\microsoft\\azure\\gateway\\gateway-java-binding\\1.0-SNAPSHOT\\gateway-java-binding-1.0-SNAPSHOT.jar"
#elif __linux__
    #define DEFAULT_CLASS_PATH ""
#endif

#define MESSAGE_BUS_CLASS_NAME "com/microsoft/azure/gateway/core/MessageBus"
#define CONSTRUCTOR_METHOD_NAME "<init>"
#define DEBUG_PORT_DEFAULT 9876

#endif /*JAVA_MODULE_HOST_COMMON_H*/
