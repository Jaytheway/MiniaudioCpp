//
//      ██╗██████╗     ██╗     ██╗██████╗ ███████╗
//      ██║██╔══██╗    ██║     ██║██╔══██╗██╔════╝		** MiniaudioCpp **
//      ██║██████╔╝    ██║     ██║██████╔╝███████╗
// ██   ██║██╔═══╝     ██║     ██║██╔══██╗╚════██║		https://github.com/Jaytheway/MiniaudioCpp
// ╚█████╔╝██║         ███████╗██║██████╔╝███████║
//  ╚════╝ ╚═╝         ╚══════╝╚═╝╚═════╝ ╚══════╝
//
//   Copyright 2024 Jaroslav Pevno, MiniaudioCpp is offered under the terms of the ISC license:
//
//   Permission to use, copy, modify, and/or distribute this software for any purpose with or
//   without fee is hereby granted, provided that the above copyright notice and this permission
//   notice appear in all copies. THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
//   AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
//   CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
//   WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
//   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#ifdef JPL_TEST

#include "MiniaudioCpp/Core.h"
#include "MiniaudioCpp/ErrorReporting.h"
#include "MiniaudioCpp/MiniaudioWrappers.h"

#include "miniaudio/miniaudio.h"

#undef max
#undef min

#include <iostream>
#include <gtest/gtest.h>

//==========================================================================
using AllocationCallbackData = std::atomic<uint64_t>;

inline static std::atomic<uint64_t> sMemoryUsedByEngine{ 0 };


//==========================================================================
static void JPLTraceCallback(const char* message)
{
	std::cout << message << '\n';
}

//==========================================================================
MA::Engine& GetMiniaudioEngine(void* /*context*/)
{
	static MA::Engine sEngine;
	return sEngine;
}

//==========================================================================
// TODO: JP. wrap this into a proper sufix allocator
void MemFreeCallback(void* p, void* pUserData)
{
	if (p == NULL)
		return;

	constexpr auto offset = std::max(sizeof(int), alignof(max_align_t));

	char* buffer = (char*)p - offset; //get the start of the buffer
	int* sizeBox = (int*)buffer;

	// Memory tracking
	{
		auto& allocatedData = *static_cast<AllocationCallbackData*>(pUserData);
		allocatedData -= *sizeBox;
	}

#ifdef LOG_MEMORY
	JPL_INFO_TAG("Audio", "Freed mem KB : {0}", *sizeBox / 1000.0f);
#endif
	ma_free(buffer, nullptr);
}

void* MemAllocCallback(size_t sz, void* pUserData)
{
	constexpr auto offset = std::max(sizeof(int), alignof(max_align_t));

	char* buffer = (char*)ma_malloc(sz + offset, nullptr); //allocate offset extra bytes 
	if (buffer == NULL)
		return NULL; // no memory! 

	// Memory tracking
	{
		auto& allocatedData = *static_cast<AllocationCallbackData*>(pUserData);
		allocatedData += sz;
	}

	int* sizeBox = (int*)buffer;
	*sizeBox = (int)sz; //store the size in first sizeof(int) bytes!

#ifdef LOG_MEMORY
	JPL_INFO_TAG("Audio", "Allocated mem KB : {0}", *sizeBox / 1000.0f);
#endif
	return buffer + offset; //return buffer after offset bytes!
}

void* MemReallocCallback(void* p, size_t sz, void* pUserData)
{
	if (p == nullptr)
		return MemAllocCallback(sz, pUserData);

	constexpr auto offset = std::max(sizeof(int), alignof(max_align_t));

	auto* buffer = (char*)p - offset; //get the start of the buffer
	int* sizeBox = (int*)buffer;

	// Memory tracking
	{
		auto& allocatedData = *static_cast<AllocationCallbackData*>(pUserData);
		allocatedData += (sz - *sizeBox);
	}

	*sizeBox = (int)sz;

	auto* newBuffer = (char*)ma_realloc(buffer, sz, NULL);
	if (newBuffer == NULL)
		return NULL;

#ifdef LOG_MEMORY
	JPL_INFO_TAG("Audio", "Reallocated mem KB : {0}", sz / 1000.0f);
#endif
	return newBuffer + offset;
}

ma_allocation_callbacks gMAAllocationCallbacks{
	.pUserData = &sMemoryUsedByEngine,
	.onMalloc = MemAllocCallback,
	.onRealloc = MemReallocCallback,
	.onFree = MemFreeCallback
};

//==========================================================================
static void Main(int argc, char* argv[])
{

	JPL::Trace = JPLTraceCallback;

	JPL::GetMiniaudioEngine = GetMiniaudioEngine;
#ifdef JPL_PLATFORM_WINDOWS
	JPL::gEngineAllocationCallbacks = &gMAAllocationCallbacks;
#else
	JPL::gEngineAllocationCallbacks = nullptr;
#endif
}


//==========================================================================
int main(int argc, char* argv[])
{
	testing::InitGoogleTest(&argc, argv);
	
	Main(argc, argv);

	return RUN_ALL_TESTS();
}

#endif // JPL_TEST
