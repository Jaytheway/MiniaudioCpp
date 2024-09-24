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

#include <utility>
#include <type_traits>
#include <concepts>

namespace JPL::Internal
{
	//==========================================================================
	/// Utility to handle lifetime of some kind of C resoruce
	template<typename ResourceType, auto InitFunction, auto UninitFunction>
	class CResource
	{
	public:
		using element_type = ResourceType;
		using pointer = element_type*;
		using const_pointer = const element_type*;

		static constexpr bool is_c_resource = true;

	private:
		using Constructor = decltype(InitFunction);
		using Destructor = decltype(UninitFunction);

		static_assert(std::is_function_v<std::remove_pointer_t<Constructor>>, "CResource requires a C initialization function.");
		static_assert(std::is_invocable_v<Destructor, pointer>, "CResoruce requires uninitialization function");

	public:
		[[nodiscard]] constexpr CResource() noexcept = default;
		[[nodiscard]] constexpr explicit CResource(pointer resource) noexcept : resource_(resource) {}

		CResource(const CResource&) = delete;
		CResource& operator=(const CResource&) = delete;

		[[nodiscard]] constexpr CResource(CResource&& other) noexcept : resource_(std::exchange(other.resource_, nullptr)) {}

		constexpr CResource& operator=(CResource&& other) noexcept
		{
			if (this != &other)
			{
				reset();
				resource_ = std::exchange(other.resource_, nullptr);
			}

			return *this;
		}

		~CResource() noexcept { destruct(); }

		constexpr void clear() noexcept
		{
			destruct();
			resource_ = nullptr;
		}

		template<typename... Args> requires std::invocable<Constructor, Args..., pointer>
		[[nodiscard]] constexpr auto emplace(Args&&... args)
		{
			destruct();
			resource_ = new element_type();
			return InitFunction(std::forward<Args>(args)..., resource_);
		}

		constexpr void reset(pointer ptr = nullptr) noexcept
		{
			destruct();
			resource_ = ptr;
		}

		constexpr pointer release() noexcept { return std::exchange(resource_, nullptr); }
		constexpr void swap(CResource& other) noexcept { std::swap(resource_, other.resource_); }

		constexpr CResource& operator=(std::nullptr_t) noexcept
		{
			clear();
			return *this;
		}

		[[nodiscard]] constexpr operator pointer() noexcept { return resource_; }
		[[nodiscard]] constexpr operator const_pointer() const noexcept { return resource_; }

		[[nodiscard]] constexpr element_type& operator*() noexcept { return *resource_; }
		[[nodiscard]] constexpr const element_type& operator*() const noexcept { return *resource_; }
		[[nodiscard]] constexpr pointer operator->() noexcept { return resource_; }
		[[nodiscard]] constexpr const_pointer operator->() const noexcept { return resource_; }
		[[nodiscard]] constexpr pointer get() noexcept { return *this; }
		[[nodiscard]] constexpr const_pointer get() const noexcept { return *this; }

		[[nodiscard]] constexpr explicit operator bool() const noexcept { return resource_ != nullptr; }
		[[nodiscard]] constexpr bool empty() const noexcept { return resource_ == nullptr; }
		auto operator<=>(const CResource&) = delete;
		[[nodiscard]] bool operator==(const CResource& rhs) const noexcept { return 0 == std::memcmp(resource_, rhs.resource_, sizeof(ResourceType)); }

	private:
		constexpr void destruct() noexcept requires std::is_invocable_v<Destructor, ResourceType*>
		{
			if (resource_)
			{
				UninitFunction(resource_);
				delete resource_;
			}
		}

	private:
		pointer resource_ = nullptr;
	};

	template<typename ResourceType, auto InitFunction, auto UninitFunction>
	constexpr void swap(
		CResource<ResourceType, InitFunction, UninitFunction>& lhs,
		CResource<ResourceType, InitFunction, UninitFunction>& rhs) noexcept
	{
		lhs.swap(rhs);
	}

} // namespace JPL