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

#pragma once

#include "CResource.h"
#include "miniaudio/miniaudio.h"

#include <tuple>

namespace JPL
{
	extern ma_allocation_callbacks gEngineAllocationCallbacks;
}

namespace JPL
{
	namespace Internal
	{
		namespace impl
		{
			template<typename Function> struct parameter_count;
			template<typename Result, typename... Args>
			struct parameter_count<Result(*)(Args...)>
			{
				static constexpr auto value = sizeof...(Args);
			};

			template<typename Function, size_t N> struct parameter_type;
			template<typename Result, typename... Args, size_t N>
			struct parameter_type<Result(*)(Args...), N>
			{
				using type = std::tuple_element_t<N, std::tuple<Args...>>;
			};


			// This is just a helper function wrapper to call various ma_uninit functinos with only
			// a single parameter 'node' pointer, and nullptr allocation callbacks
			template<auto* uninit_function>
			void uninit(typename parameter_type<decltype(uninit_function), 0>::type param)
			{
				using FunctionType = decltype(uninit_function);
				constexpr size_t num_params = parameter_count<FunctionType>::value;

				// Available arguments
				auto arg1 = param;
				auto arg2 = &gEngineAllocationCallbacks;

				if constexpr (num_params == 1)
				{
					using ParamType0 = std::remove_cvref_t<typename parameter_type<FunctionType, 0>::type>;

					if constexpr (std::is_convertible_v<ParamType0, decltype(arg1)>)
						uninit_function(arg1);
					else
						static_assert(false, "No matching argument for the function's parameter type.");
				}
				else if constexpr (num_params == 2)
				{
					using ParamType0 = typename parameter_type<FunctionType, 0>::type;
					using ParamType1 = typename parameter_type<FunctionType, 1>::type;

					if constexpr (
						std::is_convertible_v<decltype(arg1), ParamType0> &&
						std::is_convertible_v<decltype(arg2), ParamType1>)
					{
						uninit_function(arg1, arg2);
					}
					else if constexpr (
						std::is_convertible_v<decltype(arg2), ParamType0> &&
						std::is_convertible_v<decltype(arg1), ParamType1>)
					{
						uninit_function(arg2, arg1);
					}
					else
					{
						static_assert(false, "Cannot determine argument order for the function.");
					}
				}
				else
				{
					static_assert(false, "The function must have 1 or 2 parameters.");
				}
			}

		} //namespace impl

		using DataSource = Internal::CResource<ma_data_source_base, ma_data_source_init, impl::uninit<ma_data_source_uninit>>;

		using NodeBase = Internal::CResource<ma_node_base, ma_node_init, impl::uninit<ma_node_uninit>>;

		using SplitterNode = Internal::CResource<ma_splitter_node, ma_splitter_node_init, impl::uninit<ma_splitter_node_uninit>>;

		// base_node_t must be either ma_node_base or a struct with ma_node_base as the first member and not a pointer
		template<typename base_node_t>
		using TNodeBase = Internal::CResource<base_node_t, ma_node_init, impl::uninit<ma_node_uninit>>;

		// Should be used carefuly, initialized only in the main centralized engine class.
		// Output bus access should be prohibited.
		using Engine = Internal::CResource<ma_engine, ma_engine_init, impl::uninit<ma_engine_uninit>>;

		using EngineNode = Internal::CResource<ma_engine_node, ma_engine_node_init, impl::uninit<ma_engine_node_uninit>>;

		// For now we only handle init from file (or hashed file path as string)
		using Sound = Internal::CResource<ma_sound, ma_sound_init_from_file, impl::uninit<ma_sound_uninit>>;

		using LPFNode = Internal::CResource<ma_lpf_node, ma_lpf_node_init, impl::uninit<ma_lpf_node_uninit>>;
		using HPFNode = Internal::CResource<ma_hpf_node, ma_hpf_node_init, impl::uninit<ma_hpf_node_uninit>>;
	} // namespace Internal
} // namespace JPL
