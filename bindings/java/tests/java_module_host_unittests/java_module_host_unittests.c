// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umocktypes_charptr.h"

// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <stddef.h>

#include "testrunnerswitcher.h"
#include "java_module_host.h"

#define ENABLE_MOCKS

#include "umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif
	MOCKABLE_FUNCTION(, void*, gballoc_malloc, size_t, size);
	MOCKABLE_FUNCTION(, void, gballoc_free, void*, ptr);
#ifdef __cplusplus
}
#endif

#include "umock_c.h"

#define GBALLOC_H

void* real_gballoc_malloc(size_t size);
void* real_gballoc_calloc(size_t nmemb, size_t size);
void* real_gballoc_realloc(void* ptr, size_t size);
void real_gballoc_free(void* ptr);

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
			result = real_gballoc_malloc(size);
		}

		return result;
	}

	void my_gballoc_free(void* ptr)
	{
		real_gballoc_free(ptr);
	}

#ifdef __cplusplus
}
#endif

void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
	ASSERT_FAIL("umock_c reported error");
}

BEGIN_TEST_SUITE(JavaModuleHost_UnitTests)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
	TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
	g_testByTest = TEST_MUTEX_CREATE();
	ASSERT_IS_NOT_NULL(g_testByTest);

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

END_TEST_SUITE(JavaModuleHost_UnitTests);

/*if malloc is defined as gballoc_malloc at this moment, there'd be serious trouble*/
#define Lock(x) (LOCK_OK + gballocState - gballocState) /*compiler warning about constant in if condition*/
#define Unlock(x) (LOCK_OK + gballocState - gballocState)
#define Lock_Init() (LOCK_HANDLE)0x42
#define Lock_Deinit(x) (LOCK_OK + gballocState - gballocState)
#define gballoc_malloc real_gballoc_malloc
#define gballoc_realloc real_gballoc_realloc
#define gballoc_calloc real_gballoc_calloc
#define gballoc_free real_gballoc_free
#include "gballoc.c"
#undef Lock
#undef Unlock
#undef Lock_Init
#undef Lock_Deinit
