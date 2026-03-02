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

#ifdef JPL_TEST

#include "MiniaudioCpp/Core.h"
#include "MiniaudioCpp/ErrorReporting.h"
#include "MiniaudioCpp/NodeTraits.h"
#include "MiniaudioCpp/Core.h"

#include "miniaudio/miniaudio.h"

#include <gtest/gtest.h>

namespace JPL
{
	class NodeTraitsTest : public testing::Test
	{
	protected:
		NodeTraitsTest() = default;

		void SetUp() override
		{
			ASSERT_TRUE(engine_node == nullptr);
			ASSERT_TRUE(valid_target_node == nullptr);
			ASSERT_TRUE(engine == nullptr);

			engine = new ma_engine();
			engine_node = new ma_engine_node();
			valid_target_node = new ma_engine_node();

			ma_engine_config engine_config = ma_engine_config_init();
			ASSERT_EQ(ma_engine_init(&engine_config, engine), MA_SUCCESS);

			ma_engine_node_config engine_node_config = ma_engine_node_config_init(engine, ma_engine_node_type_group, 0);
			engine_node_config.channelsIn = inputChannelCount;
			engine_node_config.channelsOut = outputChannelCount;

			ASSERT_EQ(ma_engine_node_init(&engine_node_config, nullptr, engine_node), MA_SUCCESS);
			ASSERT_EQ(ma_engine_node_init(&engine_node_config, nullptr, valid_target_node), MA_SUCCESS);
			ASSERT_EQ(ma_node_set_state(engine_node, ma_node_state_stopped), MA_SUCCESS);
			ASSERT_EQ(ma_node_set_state(valid_target_node, ma_node_state_stopped), MA_SUCCESS);
		}

		void TearDown() override
		{
			ASSERT_TRUE(engine_node != nullptr);
			ASSERT_TRUE(valid_target_node != nullptr);
			ASSERT_TRUE(engine != nullptr);

			ma_engine_node_uninit(engine_node, nullptr);
			ma_engine_node_uninit(valid_target_node, nullptr);
			ma_engine_uninit(engine);

			delete engine_node;
			delete valid_target_node;
			delete engine;
		}

	protected:
		struct UnderlyingMock : Traits::NodeTopology<UnderlyingMock>
		{
			ma_engine_node* node;
			ma_engine_node* get() const { return node; }
			operator bool() const noexcept { return get() != nullptr; }
		};

		static ma_result engine_node_init_mock(const ma_engine_node_config* pConfig, const ma_allocation_callbacks* pAllocationCallbacks, ma_engine_node* pEngineNode) {}
		static void engine_node_uninit_mock(ma_engine_node* pEngineNode, const ma_allocation_callbacks* pAllocationCallbacks) {}
		using c_resource_mock = JPL::Internal::CResource<ma_engine_node, engine_node_init_mock, Internal::impl::uninit<engine_node_uninit_mock>>;

		struct RoutingMock : c_resource_mock, Traits::NodeRouting<RoutingMock>
		{
			ma_engine_node* node;
			ma_engine_node* get() const { return node; }
			operator bool() const noexcept { return get() != nullptr; }
		};

	protected:
		ma_engine* engine = nullptr;
		ma_engine_node* engine_node = nullptr;
		ma_engine_node* valid_target_node = nullptr;

		const uint32 inputBusCount = 1;
		const uint32 outputBusCount = 1;
		const uint32 inputChannelCount = 2;
		const uint32 outputChannelCount = 2;

		const uint32 validOutputBusIndex = 0;
		const uint32 invalidOutputBusIndex = outputBusCount;
		const uint32 validInputBusIndex = 0;
		const uint32 invalidInputBusIndex = inputBusCount;

		static constexpr auto* NULL_NODE = (ma_engine_node*)nullptr;
	};

	TEST_F(NodeTraitsTest, BusIndex)
	{
		EXPECT_FALSE(InputBusIndex(validInputBusIndex).Of(NULL_NODE).IsValid());
		EXPECT_TRUE(InputBusIndex(validInputBusIndex).Of(engine_node).IsValid());
		EXPECT_FALSE(InputBusIndex(invalidInputBusIndex).Of(engine_node).IsValid());
		EXPECT_EQ(InputBusIndex(validInputBusIndex).Of(engine_node).GetIndex(), validInputBusIndex);

		EXPECT_FALSE(OutputBusIndex(validOutputBusIndex).Of(NULL_NODE).IsValid());
		EXPECT_TRUE(OutputBusIndex(validOutputBusIndex).Of(engine_node).IsValid());
		EXPECT_FALSE(OutputBusIndex(invalidOutputBusIndex).Of(engine_node).IsValid());
		EXPECT_EQ(OutputBusIndex(invalidOutputBusIndex).Of(engine_node).GetIndex(), outputBusCount);
	}

	TEST_F(NodeTraitsTest, InputOutputBus_Constructors)
	{
		EXPECT_FALSE(InputBus().IsValid());
		EXPECT_TRUE(InputBus().GetIndex() == 0);

		EXPECT_FALSE(InputBus(Traits::impl::BusImpl(validInputBusIndex, NULL_NODE)).IsValid());
		EXPECT_TRUE(InputBus(Traits::impl::BusImpl(validInputBusIndex, engine_node)).IsValid());
		EXPECT_FALSE(InputBus(Traits::impl::BusImpl(inputBusCount, engine_node)).IsValid());

		EXPECT_FALSE(OutputBus().IsValid());
		EXPECT_TRUE(OutputBus().GetIndex() == 0);

		EXPECT_FALSE(OutputBus(Traits::impl::BusImpl(validOutputBusIndex, NULL_NODE)).IsValid());
		EXPECT_TRUE(OutputBus(Traits::impl::BusImpl(validOutputBusIndex, engine_node)).IsValid());
		EXPECT_FALSE(OutputBus(Traits::impl::BusImpl(outputBusCount, engine_node)).IsValid());
	}

	TEST_F(NodeTraitsTest, InputOutputBus_GetNumChannels)
	{
		EXPECT_EQ(InputBusIndex(validInputBusIndex).Of(NULL_NODE).GetNumChannels(), validInputBusIndex);
		EXPECT_EQ(OutputBusIndex(validOutputBusIndex).Of(NULL_NODE).GetNumChannels(), validOutputBusIndex);

		EXPECT_EQ(InputBusIndex(validInputBusIndex).Of(engine_node).GetNumChannels(), inputChannelCount);
		EXPECT_EQ(OutputBusIndex(validOutputBusIndex).Of(engine_node).GetNumChannels(), outputChannelCount);

		EXPECT_EQ(InputBusIndex(invalidInputBusIndex).Of(engine_node).GetNumChannels(), 0);
		EXPECT_EQ(OutputBusIndex(invalidOutputBusIndex).Of(engine_node).GetNumChannels(), 0);
	}

	TEST_F(NodeTraitsTest, OutputBus_GetVolume)
	{
		const float volume = ma_node_get_output_bus_volume(engine_node, validOutputBusIndex);

		EXPECT_EQ(OutputBusIndex(validOutputBusIndex).Of(engine_node).GetVolume(), volume);
		EXPECT_EQ(OutputBusIndex(invalidOutputBusIndex).Of(engine_node).GetVolume(), 0.0f);
		EXPECT_EQ(OutputBusIndex(validOutputBusIndex).Of(NULL_NODE).GetVolume(), 0.0f);
	}

	TEST_F(NodeTraitsTest, OutputBus_SetVolume)
	{
		EXPECT_TRUE(OutputBusIndex(validOutputBusIndex).Of(engine_node).SetVolume(2.0f));
		EXPECT_EQ(ma_node_get_output_bus_volume(engine_node, validOutputBusIndex), 2.0f);

		EXPECT_FALSE(OutputBusIndex(invalidOutputBusIndex).Of(engine_node).SetVolume(3.0f));
		EXPECT_EQ(ma_node_get_output_bus_volume(engine_node, validOutputBusIndex), 2.0f);

		EXPECT_FALSE(OutputBusIndex(validOutputBusIndex).Of(NULL_NODE).SetVolume(4.0f));
		EXPECT_EQ(ma_node_get_output_bus_volume(engine_node, validOutputBusIndex), 2.0f);
	}

	TEST_F(NodeTraitsTest, OutputBus_CanAttachTo)
	{
		const auto validOutpubBus = OutputBusIndex(validOutputBusIndex).Of(engine_node);
		const auto invalidOutpubBus = OutputBusIndex(invalidOutputBusIndex).Of(engine_node);
		const auto outputBusOfNullNode = OutputBusIndex(validOutputBusIndex).Of(NULL_NODE);

		const auto validInputBusSameNode = InputBusIndex(validInputBusIndex).Of(engine_node);
		const auto invalidInputBusSameNode = InputBusIndex(invalidInputBusIndex).Of(engine_node);
		const auto inputBusOfNullNode = InputBusIndex(validInputBusIndex).Of(NULL_NODE);

		const auto validInputBus = InputBusIndex(validInputBusIndex).Of(valid_target_node);
		const auto invalidInputBus = InputBusIndex(invalidInputBusIndex).Of(valid_target_node);

		EXPECT_FALSE(validOutpubBus.CanAttachTo(validInputBusSameNode));
		EXPECT_FALSE(validOutpubBus.CanAttachTo(invalidInputBusSameNode));
		EXPECT_FALSE(validOutpubBus.CanAttachTo(inputBusOfNullNode));

		EXPECT_FALSE(invalidOutpubBus.CanAttachTo(validInputBusSameNode));
		EXPECT_FALSE(invalidOutpubBus.CanAttachTo(invalidInputBusSameNode));
		EXPECT_FALSE(invalidOutpubBus.CanAttachTo(inputBusOfNullNode));

		EXPECT_FALSE(outputBusOfNullNode.CanAttachTo(validInputBusSameNode));
		EXPECT_FALSE(outputBusOfNullNode.CanAttachTo(invalidInputBusSameNode));
		EXPECT_FALSE(outputBusOfNullNode.CanAttachTo(inputBusOfNullNode));

		EXPECT_TRUE(validOutpubBus.CanAttachTo(validInputBus));
		EXPECT_FALSE(validOutpubBus.CanAttachTo(invalidInputBus));

		EXPECT_FALSE(invalidOutpubBus.CanAttachTo(validInputBus));
		EXPECT_FALSE(invalidOutpubBus.CanAttachTo(invalidInputBus));
	}

	TEST_F(NodeTraitsTest, OutputBus_AttachTo)
	{
		auto validOutpubBus = OutputBusIndex(validOutputBusIndex).Of(engine_node);
		auto invalidOutpubBus = OutputBusIndex(invalidOutputBusIndex).Of(engine_node);
		auto outputBusOfNullNode = OutputBusIndex(validOutputBusIndex).Of(NULL_NODE);

		auto validInputBusSameNode = InputBusIndex(validInputBusIndex).Of(engine_node);
		auto invalidInputBusSameNode = InputBusIndex(invalidInputBusIndex).Of(engine_node);
		auto inputBusOfNullNode = InputBusIndex(validInputBusIndex).Of(NULL_NODE);

		auto validInputBus = InputBusIndex(validInputBusIndex).Of(valid_target_node);
		auto invalidInputBus = InputBusIndex(invalidInputBusIndex).Of(valid_target_node);

		EXPECT_FALSE(validOutpubBus.AttachTo(validInputBusSameNode));
		EXPECT_FALSE(validOutpubBus.AttachTo(invalidInputBusSameNode));
		EXPECT_FALSE(validOutpubBus.AttachTo(inputBusOfNullNode));

		EXPECT_FALSE(invalidOutpubBus.AttachTo(validInputBusSameNode));
		EXPECT_FALSE(invalidOutpubBus.AttachTo(invalidInputBusSameNode));
		EXPECT_FALSE(invalidOutpubBus.AttachTo(inputBusOfNullNode));

		EXPECT_FALSE(outputBusOfNullNode.AttachTo(validInputBusSameNode));
		EXPECT_FALSE(outputBusOfNullNode.AttachTo(invalidInputBusSameNode));
		EXPECT_FALSE(outputBusOfNullNode.AttachTo(inputBusOfNullNode));

		EXPECT_TRUE(validOutpubBus.AttachTo(validInputBus));
		EXPECT_FALSE(validOutpubBus.AttachTo(invalidInputBus));

		EXPECT_FALSE(invalidOutpubBus.AttachTo(validInputBus));
		EXPECT_FALSE(invalidOutpubBus.AttachTo(invalidInputBus));
	}

	TEST_F(NodeTraitsTest, NodeTopology)
	{
		UnderlyingMock validTopology{ .node = engine_node };
		UnderlyingMock nullTopology{ .node = NULL_NODE };

		EXPECT_EQ(validTopology.GetNumInputBusses(), invalidInputBusIndex);
		EXPECT_EQ(validTopology.GetNumOutputBusses(), invalidOutputBusIndex);
		EXPECT_EQ(validTopology.GetNumInputChannels(0), inputChannelCount);
		EXPECT_EQ(validTopology.GetNumInputChannels(invalidInputBusIndex), 0);
		EXPECT_EQ(validTopology.GetNumOutputChannels(0), outputChannelCount);
		EXPECT_EQ(validTopology.GetNumOutputChannels(invalidOutputBusIndex), 0);
		
		EXPECT_EQ(nullTopology.GetNumInputBusses(), 0);
		EXPECT_EQ(nullTopology.GetNumOutputBusses(), 0);
		EXPECT_EQ(nullTopology.GetNumInputChannels(0), 0);
		EXPECT_EQ(nullTopology.GetNumInputChannels(invalidInputBusIndex), 0);
		EXPECT_EQ(nullTopology.GetNumOutputChannels(0), 0);
		EXPECT_EQ(nullTopology.GetNumOutputChannels(invalidOutputBusIndex), 0);
	}

	TEST_F(NodeTraitsTest, NodeRouting_CanAttachTo)
	{
		RoutingMock validRouting{ .node = engine_node };
		RoutingMock validTargetRouting{ .node = valid_target_node };
		RoutingMock nullRouting{ .node = NULL_NODE };

		EXPECT_TRUE(validRouting.CanAttachTo(validOutputBusIndex, valid_target_node, validInputBusIndex));
		EXPECT_FALSE(validRouting.CanAttachTo(invalidOutputBusIndex, valid_target_node, validInputBusIndex));
		EXPECT_FALSE(validRouting.CanAttachTo(validOutputBusIndex, valid_target_node, invalidInputBusIndex));
		EXPECT_FALSE(validRouting.CanAttachTo(invalidOutputBusIndex, valid_target_node, invalidInputBusIndex));

		EXPECT_TRUE(validRouting.CanAttachTo(validOutputBusIndex, validTargetRouting, validInputBusIndex));
		EXPECT_FALSE(validRouting.CanAttachTo(invalidOutputBusIndex, validTargetRouting, validInputBusIndex));
		EXPECT_FALSE(validRouting.CanAttachTo(validOutputBusIndex, validTargetRouting, invalidInputBusIndex));
		EXPECT_FALSE(validRouting.CanAttachTo(invalidOutputBusIndex, validTargetRouting, invalidInputBusIndex));

		EXPECT_FALSE(validRouting.CanAttachTo(validOutputBusIndex, nullRouting, validInputBusIndex));
		EXPECT_FALSE(validRouting.CanAttachTo(invalidOutputBusIndex, nullRouting, validInputBusIndex));
		EXPECT_FALSE(validRouting.CanAttachTo(validOutputBusIndex, nullRouting, invalidInputBusIndex));
		EXPECT_FALSE(validRouting.CanAttachTo(invalidOutputBusIndex, nullRouting, invalidInputBusIndex));
	}

	TEST_F(NodeTraitsTest, NodeRouting_AttachTo)
	{
		RoutingMock validRouting{ .node = engine_node };
		RoutingMock validTargetRouting{ .node = valid_target_node };
		RoutingMock nullRouting{ .node = NULL_NODE };

		EXPECT_TRUE(validRouting.AttachTo(validOutputBusIndex, valid_target_node, validInputBusIndex));
		EXPECT_FALSE(validRouting.AttachTo(invalidOutputBusIndex, valid_target_node, validInputBusIndex));
		EXPECT_FALSE(validRouting.AttachTo(validOutputBusIndex, valid_target_node, invalidInputBusIndex));
		EXPECT_FALSE(validRouting.AttachTo(invalidOutputBusIndex, valid_target_node, invalidInputBusIndex));

		EXPECT_TRUE(validRouting.AttachTo(validOutputBusIndex, validTargetRouting, validInputBusIndex));
		EXPECT_FALSE(validRouting.AttachTo(invalidOutputBusIndex, validTargetRouting, validInputBusIndex));
		EXPECT_FALSE(validRouting.AttachTo(validOutputBusIndex, validTargetRouting, invalidInputBusIndex));
		EXPECT_FALSE(validRouting.AttachTo(invalidOutputBusIndex, validTargetRouting, invalidInputBusIndex));

		EXPECT_FALSE(validRouting.AttachTo(validOutputBusIndex, nullRouting, validInputBusIndex));
		EXPECT_FALSE(validRouting.AttachTo(invalidOutputBusIndex, nullRouting, validInputBusIndex));
		EXPECT_FALSE(validRouting.AttachTo(validOutputBusIndex, nullRouting, invalidInputBusIndex));
		EXPECT_FALSE(validRouting.AttachTo(invalidOutputBusIndex, nullRouting, invalidInputBusIndex));
	}

	TEST_F(NodeTraitsTest, NodeRouting_OutputBus_InputBus)
	{
		RoutingMock validRouting{ .node = engine_node };
		RoutingMock nullRouting{ .node = NULL_NODE };

		EXPECT_TRUE(validRouting.OutputBus(validOutputBusIndex).IsValid());
		EXPECT_FALSE(validRouting.OutputBus(invalidOutputBusIndex).IsValid());
		EXPECT_TRUE(const_cast<const RoutingMock&>(validRouting).OutputBus(validOutputBusIndex).IsValid());
		EXPECT_FALSE(const_cast<const RoutingMock&>(validRouting).OutputBus(invalidOutputBusIndex).IsValid());

		EXPECT_FALSE(nullRouting.OutputBus(validOutputBusIndex).IsValid());
		EXPECT_FALSE(nullRouting.OutputBus(invalidOutputBusIndex).IsValid());
		EXPECT_FALSE(const_cast<const RoutingMock&>(nullRouting).OutputBus(validOutputBusIndex).IsValid());
		EXPECT_FALSE(const_cast<const RoutingMock&>(nullRouting).OutputBus(invalidOutputBusIndex).IsValid());
	}

	TEST_F(NodeTraitsTest, NodeDefaultTraits_IsNodeStarted)
	{
		using DefaultTraitsMock = Traits::NodeDefaultTraits<RoutingMock>;

		DefaultTraitsMock validDefaultTraits;
		validDefaultTraits.node = engine_node;
		
		DefaultTraitsMock invalidDefaultTraits;
		invalidDefaultTraits.node = NULL_NODE;

		EXPECT_FALSE(validDefaultTraits.IsNodeStarted());
		EXPECT_EQ(validDefaultTraits.IsNodeStarted(), ma_node_get_state(engine_node) == ma_node_state_started);

		EXPECT_FALSE(invalidDefaultTraits.IsNodeStarted());
		EXPECT_EQ(invalidDefaultTraits.IsNodeStarted(), ma_node_get_state(NULL_NODE) == ma_node_state_started);

		ASSERT_EQ(ma_node_set_state(engine_node, ma_node_state_started), MA_SUCCESS);
		EXPECT_TRUE(validDefaultTraits.IsNodeStarted());
		EXPECT_EQ(validDefaultTraits.IsNodeStarted(), ma_node_get_state(engine_node) == ma_node_state_started);

		ASSERT_FALSE(ma_node_set_state(NULL_NODE, ma_node_state_started) == MA_SUCCESS);
		EXPECT_FALSE(invalidDefaultTraits.IsNodeStarted());
		EXPECT_EQ(invalidDefaultTraits.IsNodeStarted(), ma_node_get_state(NULL_NODE) == ma_node_state_started);
	}

} // namespace JPL

#endif // JPL_TEST