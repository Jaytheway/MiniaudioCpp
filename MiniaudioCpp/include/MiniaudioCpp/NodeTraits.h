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

#include "C_ResourceHandling.h"

// A workaround for borked static_assert within `if constexpr` blocks (#undef at the bottom)
#define if_constexpr_static_assert(condition, message) []<bool flag = (condition)>(){ static_assert(flag, message); }()

namespace JPL::Internal
{
	// This is just concept to ensure we're dealing with CResource.
	template<typename T>
	concept c_is_c_resoruce = T::is_c_resource;
}

namespace JPL
{
	/// Forward declarations
	namespace Traits
	{
		struct BusBase;
		template<class TBusType> struct BusIndex;
	}

	template<bool IsInput> struct Bus;
	using InputBus = Bus<true>;
	using OutputBus = Bus<false>;
	struct NodeIO;

	//======================================================================
	/// Handy aliases to avoid typing templates
	using InputBusIndex = typename Traits::BusIndex<InputBus>;
	using OutputBusIndex = typename Traits::BusIndex<OutputBus>;

	//==========================================================================
	/// Type-erased, safe to copy, Bus interface wrapper
	template<bool IsInput>
	struct Bus
	{
		Bus();
		Bus(const Traits::BusBase& bus);

		uint32_t GetIndex() const;
		uint32_t GetNumChannels() const;

		bool AttachTo(InputBus other) requires (!IsInput);
		bool CanAttachTo(InputBus other) const requires (!IsInput);
		bool AttachTo(const NodeIO& other) requires (!IsInput);
		bool CanAttachTo(const NodeIO& other) const requires (!IsInput);

		bool SetVolume(float newVolumeMultiplier) requires (!IsInput);
		float GetVolume() const requires (!IsInput);

		bool IsValid() const;
		operator bool() const { return IsValid(); }

	private:
		friend struct Bus<true>;
		friend struct Bus<false>;
		ma_node_base* GetMaOwner();
		ma_node_base* GetMaOwner() const;

	private:
		// shared_ptr to be able to copy easily
		//std::shared_ptr<Traits::BusBase> Impl;
		ma_node_base* Node;
		uint32_t Index;
	};

	//==========================================================================
	/// Simple pair of Input and Output
	/// can be of the same node, can be of different nodes
	struct NodeIO
	{
		InputBus Input;
		OutputBus Output;

		bool IsValid() const { return Input.IsValid() && Output.IsValid(); };
		operator bool() const { return IsValid(); }

		inline bool AttachTo(InputBus input);
		inline bool AttachTo(NodeIO other);
	};

	//==========================================================================
	/// Internal
	namespace Traits
	{
		//======================================================================
		/// Interface to query topology of a node,
		/// must be inherited by something convertible to any ma_node*
		template<class TUnderlying>
		class NodeTopology
		{
		public:
			NodeTopology() = default;

			JPL_INLINE uint32_t GetNumInputBusses() const { const auto& p = Underlying(); return p ? ma_node_get_input_bus_count(p.get()) : 0; }
			JPL_INLINE uint32_t GetNumOutputBusses() const { const auto& p = Underlying(); return p ? ma_node_get_output_bus_count(p.get()) : 0; }

			JPL_INLINE uint32_t GetNumInputChannels(uint32_t busIndex) const { const auto& p = Underlying(); return p ? ma_node_get_input_channels(p.get(), busIndex) : 0; }
			JPL_INLINE uint32_t GetNumOutputChannels(uint32_t busIndex) const { const auto& p = Underlying(); return p ? ma_node_get_output_channels(p.get(), busIndex) : 0; }

		private:
			JPL_INLINE const TUnderlying& Underlying() const { return static_cast<const TUnderlying&>(*this); };
		};

		//======================================================================
		/// Interface to attach and dettach node Busses,
		/// NodeRouting always has NodeTopology
		template<class TUnderlying>
		class NodeRouting : public NodeTopology<TUnderlying> // TODO: consider deleting this class and use Bus interface
		{
			using TNodeTopology = NodeTopology<TUnderlying>;
		public:
			NodeRouting() = default;

			template<class TOtherInternalNode> requires JPL::Internal::c_is_c_resoruce<TOtherInternalNode>
			bool CanAttachTo(uint32_t outputBus, const NodeTopology<TOtherInternalNode>& target, uint32_t targetInputBus) const;

			template<class ma_node_t> requires (!JPL::Internal::c_is_c_resoruce<ma_node_t>)
				bool CanAttachTo(uint32_t outputBus, ma_node_t& target, uint32_t targetInputBus) const;

			template<class TOtherInternalNode> requires JPL::Internal::c_is_c_resoruce<TOtherInternalNode>
			bool AttachTo(uint32_t outputBus, TOtherInternalNode& target, uint32_t targetInputus);

			template<class ma_node_t> requires (!JPL::Internal::c_is_c_resoruce<ma_node_t>)
				bool AttachTo(uint32_t outputBus, ma_node_t& target, uint32_t targetInputus);

			JPL_INLINE OutputBus OutputBus(uint32_t busIndex);
			JPL_INLINE InputBus InputBus(uint32_t busIndex);
			JPL_INLINE const Bus<false> OutputBus(uint32_t busIndex) const;
		};

		//======================================================================
		/// Default node that has Topology and capable of Routing Busses
		template<class TInternalNode>
		struct NodeDefaultTraits : TInternalNode
			, NodeRouting<NodeDefaultTraits<TInternalNode>>
		{
			using TMaResoruce = TInternalNode;
			using Topology = NodeTopology<TMaResoruce>;
			using Routing = NodeRouting<NodeDefaultTraits<TInternalNode>>;

			// Exposing internal c_resoruce interface, mainly to CRTPs
			using pointer = TInternalNode::pointer;
			using const_pointer = TInternalNode::const_pointer;
			using element_type = TInternalNode::element_type;

			using TInternalNode::operator pointer;
			using TInternalNode::operator const_pointer;
			using TInternalNode::operator->;
			using TInternalNode::get;

			bool IsNodeStarted() const;
		};

		template<class TInternalNode>
		struct EngineNodeTraits : TInternalNode
			, NodeTopology<EngineNodeTraits<TInternalNode>>
		{
			// Changing the routing of the Engine Node is not allowed
			using Topology = NodeTopology<EngineNodeTraits<TInternalNode>>;

			// Exposing internal c_resoruce interface, mainly to CRTPs
			using pointer = TInternalNode::pointer;
			using const_pointer = TInternalNode::const_pointer;
			using element_type = TInternalNode::element_type;

			using TInternalNode::operator pointer;
			using TInternalNode::operator const_pointer;
			using TInternalNode::operator->;
			using TInternalNode::get;
		};


		//======================================================================
		/// Type erasure base.
		/// Currently only BusImpl is used as possible implementation.
		struct BusBase
		{
			virtual ~BusBase() = default;

			virtual uint32_t GetIndex() const = 0;
			virtual ma_node_base* GetMaOwner() = 0;
			virtual const ma_node_base* GetMaOwner() const = 0;
		};

		//======================================================================
		namespace impl
		{
			template<class TOwner>
			struct BusImpl;
		}

		//======================================================================
		/// Utility to get Bus interface by index
		template<class TBusType>
		struct BusIndex
		{
			static_assert(std::is_same_v<TBusType, InputBus> || std::is_same_v<TBusType, OutputBus>);

			BusIndex() = delete;
			BusIndex(uint32_t index) : Index(index) {}

			template<class TOwner>
			TBusType Of(TOwner* owner) { return impl::BusImpl(Index, owner); }

		private:
			uint32_t Index;
		};

	} // namespace Traits

	//==============================================================================
	//
	//   Code beyond this point is implementation detail...
	//
	//==============================================================================

	template<class TInternalNode>
	inline bool Traits::NodeDefaultTraits<TInternalNode>::IsNodeStarted() const
	{
		return ma_node_get_state(get()) == ma_node_state_started;
	}

	namespace Traits::impl
	{
		//======================================================================
		/// Output Bus implementation.
		// It's sole purpose is to retrieve ma_node_base* from the Owner.
		// It should not be used directly, instead construct InputBus or OutputBus
		// using // BusIndex<TBusType>, or InputBusIndex and OutputBusIndex aliases 
		template<class TOwner>
		struct BusImpl : BusBase
		{
			BusImpl() = delete;
			BusImpl(uint32_t index, TOwner* owner) : Index(index), Owner(owner) {}

			ma_node_base* GetMaOwner() final { return const_cast<ma_node_base*>(GetMaOwnerImpl()); }
			const ma_node_base* GetMaOwner() const final { return GetMaOwnerImpl(); }
			uint32_t GetIndex() const final { return Index; }

		private:
			using cond_const_ma_node_base_ptr = std::conditional_t<std::is_const_v<TOwner>, const ma_node_base*, ma_node_base*>;

			cond_const_ma_node_base_ptr GetMaOwnerImpl() const requires JPL::Internal::c_is_c_resoruce<TOwner>;
			cond_const_ma_node_base_ptr GetMaOwnerImpl() const requires (!JPL::Internal::c_is_c_resoruce<TOwner>);

		private:
			uint32_t Index;
			TOwner* Owner;	// Can be our internal c_resoruce handler or any of miniaudio ma_nodes
		};
	} // namespace Traits::impl

		//==========================================================================
	template<bool IsInput>
	inline Bus<IsInput>::Bus() : Node(nullptr), Index(0) {}

	template<bool IsInput>
	inline Bus<IsInput>::Bus(const Traits::BusBase& bus) : Node(const_cast<ma_node_base*>(bus.GetMaOwner())), Index(bus.GetIndex()) {}

	template<bool IsInput> ma_node_base* Bus<IsInput>::GetMaOwner() { return Node; } //  Impl ? Impl->GetMaOwner() : 0; }
	template<bool IsInput> ma_node_base* Bus<IsInput>::GetMaOwner()	const { return Node; } //  Impl ? Impl->GetMaOwner() : 0; }
	template<bool IsInput> uint32_t			Bus<IsInput>::GetIndex() const { return Index; }// Impl ? Impl->GetIndex() : 0; }

	template<bool IsInput>
	inline uint32_t JPL::Bus<IsInput>::GetNumChannels() const
	{
		if constexpr (IsInput)
			return ma_node_get_input_channels(GetMaOwner(), GetIndex());
		else
			return ma_node_get_output_channels(GetMaOwner(), GetIndex());
	}

	template<bool IsInput>
	bool Bus<IsInput>::AttachTo(InputBus target) requires (!IsInput)
	{
		if (CanAttachTo(target))
			return ma_node_attach_output_bus(GetMaOwner(), Index, target.GetMaOwner(), target.GetIndex()) == MA_SUCCESS;
		return false;
	}

	template<bool IsInput>
	bool Bus<IsInput>::CanAttachTo(InputBus target) const requires (!IsInput)
	{
		const uint32_t numChannels = GetNumChannels();
		return numChannels && numChannels == target.GetNumChannels() && GetMaOwner() != target.GetMaOwner();
	}

	template<bool IsInput>
	inline bool Bus<IsInput>::AttachTo(const NodeIO& other) requires (!IsInput)
	{
		return AttachTo(other.Input);
	}

	template<bool IsInput>
	inline bool Bus<IsInput>::CanAttachTo(const NodeIO& other) const requires (!IsInput)
	{
		return CanAttachTo(other.Input);
	}

	template<bool IsInput>
	inline bool Bus<IsInput>::SetVolume(float newVolumeMultiplier) requires (!IsInput)
	{
		return ma_node_set_output_bus_volume(GetMaOwner(), GetIndex(), newVolumeMultiplier) == MA_SUCCESS;
	}

	template<bool IsInput>
	inline float Bus<IsInput>::GetVolume() const requires (!IsInput)
	{
		return ma_node_get_output_bus_volume(GetMaOwner(), GetIndex());
	}

	template<bool IsInput>
	inline bool Bus<IsInput>::IsValid() const
	{
		if constexpr (IsInput)
			return Node != nullptr && GetIndex() < ma_node_get_input_bus_count(GetMaOwner());
		else
			return Node != nullptr && GetIndex() < ma_node_get_output_bus_count(GetMaOwner());
	}

	//==========================================================================
	inline bool NodeIO::AttachTo(InputBus input)
	{
		return Output.AttachTo(input);
	}

	inline bool NodeIO::AttachTo(NodeIO other)
	{
		return Output.AttachTo(other.Input);
	}

	namespace Traits
	{
		//======================================================================
		namespace impl
		{
			template <typename T> concept CIsMaBaseNode = std::is_same_v<T, ma_node_base>;
			template <typename T> concept CHasMemberBase = requires(T node) { { node.base } -> std::same_as<ma_node_base&>; };
			template <typename T> concept CHasMemberBaseNode = requires(T node) { { node.baseNode } -> std::same_as<ma_node_base&>; };
			template <typename T> concept CIsMaCompoundNode = CHasMemberBase<T> || CHasMemberBaseNode<T>;
		} // namespace impl


		template<class TOwner>
		impl::BusImpl<TOwner>::cond_const_ma_node_base_ptr impl::BusImpl<TOwner>::GetMaOwnerImpl() const requires JPL::Internal::c_is_c_resoruce<TOwner>
		{
			auto* ptr = Owner->get();
			if (!ptr)
				return nullptr;

			if constexpr (impl::CIsMaBaseNode<typename TOwner::element_type>)		return ptr;
			else if constexpr (impl::CHasMemberBase<typename TOwner::element_type>)		return &ptr->base;
			else if constexpr (impl::CHasMemberBaseNode<typename TOwner::element_type>)	return &ptr->baseNode;
			else
			{
				//? assuming this is ma_sound
				return &ptr->engineNode.baseNode;
				//if_constexpr_static_assert(false, "Unknown ma_node_ type, doesn't have 'base' or 'baseNode' member.");
				//return nullptr;
			}
		}

		template<class TOwner>
		impl::BusImpl<TOwner>::cond_const_ma_node_base_ptr impl::BusImpl<TOwner>::GetMaOwnerImpl() const requires (!JPL::Internal::c_is_c_resoruce<TOwner>)
		{
			if constexpr (impl::CIsMaBaseNode<TOwner>)			return Owner;
			else if constexpr (impl::CHasMemberBase<TOwner>)		return &Owner->base;
			else if constexpr (impl::CHasMemberBaseNode<TOwner>)	return &Owner->baseNode;
			else
			{
				//? assuming this is ma_sound
				return &Owner->engineNode.baseNode;
				//if_constexpr_static_assert(false, "Unknown ma_node_ type, doesn't have 'base' or 'baseNode' member.");
				//return nullptr;
			}
		}

		//======================================================================
		template<class TUnderlying>
		template<class TOtherInternalNode> requires JPL::Internal::c_is_c_resoruce<TOtherInternalNode>
		bool NodeRouting<TUnderlying>::CanAttachTo(uint32_t outputBus, const NodeTopology<TOtherInternalNode>& target, uint32_t targetInputBus) const
		{
			return OutputBus(outputBus).CanAttachTo(InputBusIndex(targetInputBus).Of(static_cast<const TOtherInternalNode*>(&target)));
		}

		template<class TUnderlying>
		template<class ma_node_t> requires (!JPL::Internal::c_is_c_resoruce<ma_node_t>)
			bool NodeRouting<TUnderlying>::CanAttachTo(uint32_t outputBus, ma_node_t& target, uint32_t targetInputBus) const
		{
			return OutputBus(outputBus).CanAttachTo(InputBusIndex(targetInputBus).Of(target));
		}

		template<class TUnderlying>
		template<class TOtherInternalNode> requires JPL::Internal::c_is_c_resoruce<TOtherInternalNode>
		bool NodeRouting<TUnderlying>::AttachTo(uint32_t outputBus, TOtherInternalNode& target, uint32_t targetInputBus)
		{
			return OutputBus(outputBus).AttachTo(InputBusIndex(targetInputBus).Of(static_cast<const TOtherInternalNode*>(&target)));
		}

		template<class TUnderlying>
		template<class ma_node_t> requires (!JPL::Internal::c_is_c_resoruce<ma_node_t>)
			bool NodeRouting<TUnderlying>::AttachTo(uint32_t outputBus, ma_node_t& target, uint32_t targetInputBus)
		{
			return OutputBus(outputBus).AttachTo(InputBusIndex(targetInputBus).Of(target));
		}

		template<class TUnderlying>
		inline OutputBus NodeRouting<TUnderlying>::OutputBus(uint32_t busIndex)
		{
			return Traits::impl::BusImpl(busIndex, static_cast<TUnderlying*>(this));
		}

		template<class TUnderlying>
		inline InputBus NodeRouting<TUnderlying>::InputBus(uint32_t busIndex)
		{
			return Traits::impl::BusImpl(busIndex, static_cast<TUnderlying*>(this));
		}

		template<class TUnderlying>
		inline const OutputBus NodeRouting<TUnderlying>::OutputBus(uint32_t busIndex) const
		{
			return Traits::impl::BusImpl(busIndex, static_cast<const TUnderlying*>(this));
		}

	} // namespace Traits
} // namespace JPL

#undef if_constexpr_static_assert
