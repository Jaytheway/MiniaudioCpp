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
#include "MiniaudioCpp/MiniaudioWrappers.h"
#include "MiniaudioCpp/VFS.h"

#include "choc/audio/choc_SampleBuffers.h"
#include "choc/audio/choc_AudioFileFormat_WAV.h"
#include "choc/audio/choc_Oscillators.h"

#include <gtest/gtest.h>

#include <fstream>
#include <filesystem>
#include <sstream>

namespace JPL
{
	class MiniaudioWrappersTest : public testing::Test
	{
	protected:
		//==========================================================================
		class WaveformMockReader
		{
		public:
			static constexpr std::string_view Tag = "WaveformMock";

			static constexpr uint32 sourceNumChannels = 2;
			static constexpr uint32 sourceSampleRate = 48'000;
			static constexpr double sourceAmplitude = 1.0f;
			static constexpr double sineFrequency = 440.0f;

			static constexpr uint64 frameSize = sourceNumChannels * sizeof(float);
			static constexpr double durationInSeconds = 2.0;
			static constexpr uint64 durationInFrames = static_cast<uint64>(durationInSeconds * sourceSampleRate);
			static inline uint64 fakeFileSize = durationInFrames * frameSize;

			explicit WaveformMockReader(const char* filepath)
			{
				// Create wav file with sinewave

				choc::audio::WAVAudioFileFormat<true> wavFormat;
				sourceFile = std::make_shared<std::stringstream>();
				auto writer = wavFormat.createWriter(sourceFile, choc::audio::AudioFileProperties
									   {
											.sampleRate = static_cast<double>(sourceSampleRate),
											.numFrames = 0, //? when writing wav info, this value is added to the num frames we append later
											.numChannels = sourceNumChannels,
											.bitDepth = choc::audio::BitDepth::float32
									   });

				JPL_ASSERT(writer);
				auto sourceData = choc::oscillator::createChannelArray<choc::oscillator::Sine<double>, double>(
					choc::buffer::Size{
						.numChannels = sourceNumChannels,
						.numFrames = durationInFrames
					},
					sineFrequency,
					static_cast<double>(sourceSampleRate));

				bool success = writer->appendFrames(sourceData);
				JPL_ASSERT(success);

				success = writer->flush();
				JPL_ENSURE(success);

				fakeFileSize = sourceFile->str().size();
			}

			~WaveformMockReader()
			{
			}

			void ReadData(void* outData, size_t inNumBytes) { sourceFile->read((char*)outData, inNumBytes); }
			JPL_INLINE size_t GetStreamPosition() const { return sourceFile->tellg(); }
			JPL_INLINE void SetStreamPosition(size_t position) { sourceFile->seekg(position); }

		private:
			std::shared_ptr<std::stringstream> sourceFile;
		};

		inline static MA::Engine engine;

		struct VFSCustomTraits { using TStreamReader = WaveformMockReader; };
		inline static JPL::VFS<VFSCustomTraits> engineVfs
		{
			.onCreateReader = [](const char* filepath) { return new WaveformMockReader(filepath); },
			.onGetFileSize = [](const char* filepath) { return WaveformMockReader::fakeFileSize; }
		};

	protected:
		MiniaudioWrappersTest() = default;

		static void SetUpTestSuite()
		{
			if (!MiniaudioWrappersTest::engine)
			{
				engineVfs.init(JPL::gEngineAllocationCallbacks);

				MiniaudioWrappersTest::engine.reset(new ma_engine());
				JPL_ENSURE(MiniaudioWrappersTest::engine.Init(2, &engineVfs));
			}

			JPL::GetMiniaudioEngine = MiniaudioWrappersTest::GetMiniaudioEngine;
		}

		static void TearDownTestSuite()
		{
			if (MiniaudioWrappersTest::engine)
				MiniaudioWrappersTest::engine.reset();
		}

		void SetUp() override
		{


		}

		void TearDown() override
		{
			
		}

		static MA::Engine& GetMiniaudioEngine(void* /*context*/)
		{
			return engine;
		}

		auto CreateEmptyBuffer(uint32 numChannels, uint32 numFrames)
		{
			return choc::buffer::createChannelArrayBuffer(numChannels, numFrames, [](uint32 channel, uint32 frame) { return 0.0f; });
		}

	protected:
		template<int Flags>
		struct node_base_mock
		{
			ma_node_base base;
			static constexpr int FLAGS = Flags;
			std::function<void(JPL::ProcessCallbackData& callback)> onProcess;
			void Process(JPL::ProcessCallbackData& callback)
			{
				if (onProcess)
					onProcess(callback);
			}
		};
	};

	JPL_INLINE static bool operator==(const BusConfig& lhs, const BusConfig& rhs)
	{
		return lhs.Inputs == rhs.Inputs && lhs.Outputs == rhs.Outputs;
	}

	JPL_INLINE static bool operator==(const NodeLayout& lhs, const NodeLayout& rhs)
	{
		return lhs.BusConfig.Inputs == rhs.BusConfig.Inputs && lhs.BusConfig.Outputs == rhs.BusConfig.Outputs;
	}

	TEST_F(MiniaudioWrappersTest, BusConfig)
	{
		EXPECT_TRUE(BusConfig().Inputs.empty());
		EXPECT_TRUE(BusConfig().Outputs.empty());

		EXPECT_EQ(BusConfig().WithInputs({ 2, 3 }).Inputs.size(), 2);
		EXPECT_EQ(BusConfig().WithInputs({ 2, 3 }).Outputs.size(), 0);

		EXPECT_EQ(BusConfig().WithOutputs({ 4, 5, 6 }).Inputs.size(), 0);
		EXPECT_EQ(BusConfig().WithOutputs({ 4, 5, 6 }).Outputs.size(), 3);
		
		EXPECT_EQ(BusConfig().WithInputs({ 2, 3 }).WithOutputs({4, 5, 6}).Inputs.size(), 2);
		EXPECT_EQ(BusConfig().WithInputs({ 2, 3 }).WithOutputs({4, 5, 6}).Outputs.size(), 3);

		EXPECT_EQ(BusConfig().WithInputs(2).Inputs.size(), 1);
		EXPECT_EQ(BusConfig().WithInputs(2).Outputs.size(), 0);

		EXPECT_EQ(BusConfig().WithOutputs(4).Inputs.size(), 0);
		EXPECT_EQ(BusConfig().WithOutputs(4).Outputs.size(), 1);

		EXPECT_EQ(BusConfig().WithInputs(2).Inputs[0], 2);
		EXPECT_EQ(BusConfig().WithOutputs(4).Outputs[0], 4);

		EXPECT_EQ(BusConfig().WithInputs({ 2, 3 }).Inputs[0], 2);
		EXPECT_EQ(BusConfig().WithInputs({ 2, 3 }).Inputs[1], 3);

		EXPECT_EQ(BusConfig().WithOutputs({ 4, 5, 6 }).Outputs[0], 4);
		EXPECT_EQ(BusConfig().WithOutputs({ 4, 5, 6 }).Outputs[1], 5);
		EXPECT_EQ(BusConfig().WithOutputs({ 4, 5, 6 }).Outputs[2], 6);

		EXPECT_EQ(BusConfig().WithInputs({ 2, 3 }).WithOutputs({ 4, 5, 6 }).Inputs[0], 2);
		EXPECT_EQ(BusConfig().WithInputs({ 2, 3 }).WithOutputs({ 4, 5, 6 }).Inputs[1], 3);

		EXPECT_EQ(BusConfig().WithInputs({ 2, 3 }).WithOutputs({ 4, 5, 6 }).Outputs[0], 4);
		EXPECT_EQ(BusConfig().WithInputs({ 2, 3 }).WithOutputs({ 4, 5, 6 }).Outputs[1], 5);
		EXPECT_EQ(BusConfig().WithInputs({ 2, 3 }).WithOutputs({ 4, 5, 6 }).Outputs[2], 6);

		EXPECT_EQ(BusConfig().WithInputs({ 2 }), BusConfig().WithInputs(2));
		EXPECT_EQ(BusConfig().WithOutputs({ 4 }), BusConfig().WithOutputs(4));

		EXPECT_EQ(BusConfig().WithInputs({ 2 }).WithOutputs({ 4 }), BusConfig().WithInputs(2).WithOutputs(4));
		EXPECT_EQ(BusConfig().WithInputs({ 2 }).WithOutputs(4), BusConfig().WithInputs(2).WithOutputs({ 4 }));
		ASSERT_EQ(BusConfig().WithInputs(2).WithOutputs({ 4 }), BusConfig().WithInputs({ 2 }).WithOutputs(4));
	}

	TEST_F(MiniaudioWrappersTest, NodeLayout)
	{
		EXPECT_TRUE(NodeLayout().BusConfig.Inputs.empty());
		EXPECT_TRUE(NodeLayout().BusConfig.Outputs.empty());

		EXPECT_EQ(NodeLayout().WithInputs({ 2, 3 }).BusConfig.Inputs.size(), 2);
		EXPECT_EQ(NodeLayout().WithInputs({ 2, 3 }).BusConfig.Outputs.size(), 0);

		EXPECT_EQ(NodeLayout().WithOutputs({ 4, 5, 6 }).BusConfig.Inputs.size(), 0);
		EXPECT_EQ(NodeLayout().WithOutputs({ 4, 5, 6 }).BusConfig.Outputs.size(), 3);

		EXPECT_EQ(NodeLayout().WithInputs({ 2, 3 }).WithOutputs({ 4, 5, 6 }).BusConfig.Inputs.size(), 2);
		EXPECT_EQ(NodeLayout().WithInputs({ 2, 3 }).WithOutputs({ 4, 5, 6 }).BusConfig.Outputs.size(), 3);

		EXPECT_EQ(NodeLayout().WithInputs(2).BusConfig.Inputs.size(), 1);
		EXPECT_EQ(NodeLayout().WithInputs(2).BusConfig.Outputs.size(), 0);

		EXPECT_EQ(NodeLayout().WithOutputs(4).BusConfig.Inputs.size(), 0);
		EXPECT_EQ(NodeLayout().WithOutputs(4).BusConfig.Outputs.size(), 1);

		EXPECT_EQ(NodeLayout().WithInputs(2).BusConfig.Inputs[0], 2);
		EXPECT_EQ(NodeLayout().WithOutputs(4).BusConfig.Outputs[0], 4);

		EXPECT_EQ(NodeLayout().WithInputs({ 2, 3 }).BusConfig.Inputs[0], 2);
		EXPECT_EQ(NodeLayout().WithInputs({ 2, 3 }).BusConfig.Inputs[1], 3);

		EXPECT_EQ(NodeLayout().WithOutputs({ 4, 5, 6 }).BusConfig.Outputs[0], 4);
		EXPECT_EQ(NodeLayout().WithOutputs({ 4, 5, 6 }).BusConfig.Outputs[1], 5);
		EXPECT_EQ(NodeLayout().WithOutputs({ 4, 5, 6 }).BusConfig.Outputs[2], 6);

		EXPECT_EQ(NodeLayout().WithInputs({ 2, 3 }).WithOutputs({ 4, 5, 6 }).BusConfig.Inputs[0], 2);
		EXPECT_EQ(NodeLayout().WithInputs({ 2, 3 }).WithOutputs({ 4, 5, 6 }).BusConfig.Inputs[1], 3);

		EXPECT_EQ(NodeLayout().WithInputs({ 2, 3 }).WithOutputs({ 4, 5, 6 }).BusConfig.Outputs[0], 4);
		EXPECT_EQ(NodeLayout().WithInputs({ 2, 3 }).WithOutputs({ 4, 5, 6 }).BusConfig.Outputs[1], 5);
		EXPECT_EQ(NodeLayout().WithInputs({ 2, 3 }).WithOutputs({ 4, 5, 6 }).BusConfig.Outputs[2], 6);

		EXPECT_EQ(NodeLayout().WithInputs({ 2 }), NodeLayout().WithInputs(2));
		EXPECT_EQ(NodeLayout().WithOutputs({ 4 }), NodeLayout().WithOutputs(4));

		EXPECT_EQ(NodeLayout().WithInputs({ 2 }).WithOutputs({ 4 }), NodeLayout().WithInputs(2).WithOutputs(4));
		EXPECT_EQ(NodeLayout().WithInputs({ 2 }).WithOutputs(4), NodeLayout().WithInputs(2).WithOutputs({ 4 }));
		EXPECT_EQ(NodeLayout().WithInputs(2).WithOutputs({ 4 }), NodeLayout().WithInputs({ 2 }).WithOutputs(4));

		EXPECT_EQ(NodeLayout().WithBusConfig(BusConfig().WithInputs(2).WithOutputs(4)).BusConfig, BusConfig().WithInputs(2).WithOutputs(4));
	}

	TEST_F(MiniaudioWrappersTest, TBaseNode)
	{
		static constexpr int ZERO_FLAGS = 0;
		TBaseNode<node_base_mock<ZERO_FLAGS>> uninitBaseNode;
		EXPECT_TRUE(uninitBaseNode.get() == nullptr);
		EXPECT_FALSE(uninitBaseNode.StartNode());
		EXPECT_FALSE(uninitBaseNode.IsNodeStarted());
		EXPECT_FALSE(uninitBaseNode.StopNode());

		EXPECT_FALSE(uninitBaseNode.Init(NodeLayout().WithInputs(0).WithOutputs(0)));
		EXPECT_FALSE(uninitBaseNode.Init(NodeLayout().WithInputs(0).WithOutputs(2)));
		EXPECT_FALSE(uninitBaseNode.Init(NodeLayout().WithInputs(2).WithOutputs(0)));

		// Initialize node with 2 input and 2 output buses
		EXPECT_TRUE(uninitBaseNode.Init(NodeLayout().WithInputs({ 2, 2 }).WithOutputs({ 2, 2 })));

		TBaseNode<node_base_mock<ZERO_FLAGS>>& initializedBaseNode = uninitBaseNode;
		EXPECT_FALSE(initializedBaseNode.get() == nullptr);
		EXPECT_TRUE(initializedBaseNode.StartNode() && initializedBaseNode.IsNodeStarted());
		EXPECT_TRUE(initializedBaseNode.StopNode() && !initializedBaseNode.IsNodeStarted());
		EXPECT_FALSE(initializedBaseNode.GetNumInputBusses() == 1);
		EXPECT_FALSE(initializedBaseNode.GetNumOutputBusses() == 1);


		// if PASSTHROUGH, In and Out bus counts should be 1 and 1
		TBaseNode<node_base_mock<MA_NODE_FLAG_PASSTHROUGH>> passthroughNode;
		EXPECT_FALSE(passthroughNode.Init(NodeLayout().WithInputs({ 2, 2 }).WithOutputs({ 2, 2 })));
		EXPECT_TRUE(passthroughNode.Init(NodeLayout().WithInputs(2).WithOutputs(2)));
		EXPECT_EQ(passthroughNode.GetNumInputBusses(), 1);
		EXPECT_EQ(passthroughNode.GetNumOutputBusses(), 1);

		TBaseNode<node_base_mock<ZERO_FLAGS>> nodeInitStarted;
		ASSERT_TRUE(nodeInitStarted.Init(NodeLayout().WithInputs({ 2, 2 }).WithOutputs({ 2, 2 }), true));
		EXPECT_TRUE(nodeInitStarted.IsNodeStarted());

		TBaseNode<node_base_mock<ZERO_FLAGS>> nodeInitStopped;
		ASSERT_TRUE(nodeInitStopped.Init(NodeLayout().WithInputs({ 2, 2 }).WithOutputs({ 2, 2 }), false));
		EXPECT_FALSE(nodeInitStopped.IsNodeStarted());
	}

	TEST_F(MiniaudioWrappersTest, ProcessCallbackData)
	{
		// Initialize mock data
		static constexpr int ZERO_FLAGS = 0;
		TBaseNode<node_base_mock<ZERO_FLAGS>> node;

		const uint32 validNumInputBusses = 2;
		const uint32 validNumOutputBusses = 2;
		uint32 validNumChannels = 2;
		uint32 validNumFrames = 4;

		ASSERT_TRUE(node.Init(NodeLayout().WithInputs({ 2, 2 }).WithOutputs({ 2, 2 }), false));
		ASSERT_TRUE(node.get() != nullptr);

		ma_node_base* pNodeBase = (ma_node_base*)node.get();
		ASSERT_TRUE(pNodeBase->vtable != nullptr);
		ASSERT_TRUE(pNodeBase->vtable->onProcess != nullptr);

		// 1. Valid callback, test buffer sizes and bus counts,
		// general structure of ProcessCallbackData

		 auto validCallback = [&](MA::ProcessCallbackData& callback)
			{
				EXPECT_FALSE(callback.IsNullInput());
				EXPECT_EQ(callback.InputBusCount, validNumInputBusses);
				EXPECT_EQ(callback.OutputBusCount, validNumOutputBusses);

				EXPECT_EQ(callback.GetInputFrameCount(), validNumFrames);
				EXPECT_EQ(callback.GetOutputFrameCount(), validNumFrames);

				ASSERT_TRUE(validNumInputBusses > 0);
				ASSERT_TRUE(validNumOutputBusses > 0);

				auto inputBuffer = callback.GetInputBuffer(validNumInputBusses - 1);
				EXPECT_EQ(inputBuffer.getNumChannels(), validNumChannels);
				EXPECT_EQ(inputBuffer.getNumFrames(), validNumFrames);

				auto outputBuffer = callback.GetOutputBuffer(validNumOutputBusses - 1);
				EXPECT_EQ(outputBuffer.getNumChannels(), validNumChannels);
				EXPECT_EQ(outputBuffer.getNumFrames(), validNumFrames);
			};

		// Create bus arrays of interleaved buffers
		auto bufferIn = CreateEmptyBuffer(validNumChannels, validNumFrames * validNumChannels);
		auto bufferOut = CreateEmptyBuffer(validNumChannels, validNumFrames * validNumChannels);

		node->onProcess = validCallback;
		pNodeBase->vtable->onProcess(pNodeBase, const_cast<const float**>(bufferIn.getView().data.channels), &validNumFrames, const_cast<float**>(bufferIn.getView().data.channels), &validNumFrames);
		
		// 2. null input callback, test if callback reports null input correctly

		auto nullInputCallback = [&](MA::ProcessCallbackData& callback)
			{
				EXPECT_TRUE(callback.IsNullInput());
				EXPECT_EQ(callback.InputBusCount, validNumInputBusses);
				EXPECT_EQ(callback.OutputBusCount, validNumOutputBusses);

				EXPECT_EQ(callback.GetInputFrameCount(), validNumFrames);
				EXPECT_EQ(callback.GetOutputFrameCount(), validNumFrames);

				ASSERT_TRUE(validNumInputBusses > 0);
				ASSERT_TRUE(validNumOutputBusses > 0);

				auto outputBuffer = callback.GetOutputBuffer(validNumOutputBusses - 1);
				ASSERT_EQ(outputBuffer.getNumChannels(), validNumChannels);
				ASSERT_EQ(outputBuffer.getNumFrames(), validNumFrames);
			};

		node->onProcess = nullInputCallback;
		pNodeBase->vtable->onProcess(pNodeBase, nullptr, &validNumFrames, const_cast<float**>(bufferOut.getView().data.channels), &validNumFrames);

		// 3. Fill 1st output buffer with silense, other should be untouched

		const float outputBusFillValue = 0.5f;

		auto fillFirstOutputBusWithSilence = [&](MA::ProcessCallbackData& callback)
			{
				ASSERT_EQ(callback.OutputBusCount, validNumOutputBusses);
				ASSERT_EQ(callback.GetOutputFrameCount(), validNumFrames);
				ASSERT_TRUE(validNumOutputBusses > 0);

				const int testedOutputBusIndex = 0;
				callback.FillOutputBusWithSilence(testedOutputBusIndex);

				auto outputBuffer = callback.GetOutputBuffer(testedOutputBusIndex);
				ASSERT_EQ(outputBuffer.getNumChannels(), validNumChannels);
				ASSERT_EQ(outputBuffer.getNumFrames(), validNumFrames);

				// Check the bus we filled with silence
				for (uint32 s = 0; s < outputBuffer.getNumFrames(); ++s)
					EXPECT_FLOAT_EQ(outputBuffer.getSample(0, 0), 0.0f);

				// Check that the rest of the busses were not touched
				for (uint32 b = testedOutputBusIndex + 1; b < validNumOutputBusses; ++b)
				{
					auto outputBuffer = callback.GetOutputBuffer(b);
					ASSERT_EQ(outputBuffer.getNumChannels(), validNumChannels);
					ASSERT_EQ(outputBuffer.getNumFrames(), validNumFrames);

					for (uint32 s = 0; s < outputBuffer.getNumFrames(); ++s)
						EXPECT_FLOAT_EQ(outputBuffer.getSample(b, s), outputBusFillValue);
				}
			};

		ASSERT_TRUE(validNumOutputBusses >= 2);

		// Fill busses with some value
		auto fillOutBussesWithValue = [&]
			{
				for (uint32 b = 0; b < validNumOutputBusses; ++b)
					for (uint32 s = 0; s < bufferOut.getNumFrames(); ++s)
						bufferOut.getSample(b, s) = outputBusFillValue;
			};

		fillOutBussesWithValue();

		node->onProcess = fillFirstOutputBusWithSilence;
		pNodeBase->vtable->onProcess(pNodeBase, const_cast<const float**>(bufferIn.getView().data.channels), &validNumFrames, const_cast<float**>(bufferOut.getView().data.channels), &validNumFrames);

		// 4. Fill output with silense (all busses in one function)

		auto fillOutputWithSilence = [&](MA::ProcessCallbackData& callback)
			{
				ASSERT_EQ(callback.OutputBusCount, validNumOutputBusses);
				ASSERT_EQ(callback.GetOutputFrameCount(), validNumFrames);
				ASSERT_TRUE(validNumOutputBusses > 0);

				callback.FillOutputWithSilence();

				for (uint32 b = 0; b < validNumOutputBusses; ++b)
				{
					auto outputBuffer = callback.GetOutputBuffer(b);
					ASSERT_EQ(outputBuffer.getNumChannels(), validNumChannels);
					ASSERT_EQ(outputBuffer.getNumFrames(), validNumFrames);

					for (uint32 s = 0; s < outputBuffer.getNumFrames(); ++s)
						EXPECT_FLOAT_EQ(outputBuffer.getSample(b, s), 0.0f);
				}
			};

		fillOutBussesWithValue();

		node->onProcess = fillOutputWithSilence;
		pNodeBase->vtable->onProcess(pNodeBase, const_cast<const float**>(bufferIn.getView().data.channels), &validNumFrames, const_cast<float**>(bufferOut.getView().data.channels), &validNumFrames);

		// 5. Copy inputs to outputs
	
		auto copyInputsToOutputs = [&](MA::ProcessCallbackData& callback)
			{
				ASSERT_EQ(callback.InputBusCount, validNumInputBusses);
				ASSERT_EQ(callback.GetInputFrameCount(), validNumFrames);
				ASSERT_TRUE(validNumInputBusses > 0);

				callback.CopyInputsToOutputs();

				for (uint32 b = 0; b < std::min(validNumInputBusses, validNumOutputBusses); ++b)
				{
					auto inputBuffer = callback.GetInputBuffer(b);
					auto outputBuffer = callback.GetOutputBuffer(b);
					ASSERT_EQ(inputBuffer.getNumChannels(), validNumChannels);
					ASSERT_EQ(inputBuffer.getNumFrames(), validNumFrames);
					ASSERT_EQ(outputBuffer.getNumChannels(), validNumChannels);
					ASSERT_EQ(outputBuffer.getNumFrames(), validNumFrames);

					for (uint32 s = 0; s < validNumFrames; ++s)
						EXPECT_FLOAT_EQ(inputBuffer.getSample(b, s), outputBuffer.getSample(b, s));
				}
			};

		// Fill input busses with some value
		const float inputBusFillValue = 0.2f;
		{
			for (uint32 b = 0; b < validNumOutputBusses; ++b)
				for (uint32 s = 0; s < bufferIn.getNumFrames(); ++s)
					bufferIn.getSample(b, s) = inputBusFillValue;
		};

		fillOutBussesWithValue();

		node->onProcess = copyInputsToOutputs;
		pNodeBase->vtable->onProcess(pNodeBase, const_cast<const float**>(bufferIn.getView().data.channels), &validNumFrames, const_cast<float**>(bufferOut.getView().data.channels), &validNumFrames);
	}

	TEST_F(MiniaudioWrappersTest, Engine)
	{
		static bool vfsWasCleanedUp = false;
		struct VFSCustomTraits { using TStreamReader = WaveformMockReader; };
		struct VFSMock : JPL::VFS<VFSCustomTraits>
		{
		};

		VFSMock vfs;
		vfs.onCreateReader = [](const char* filepath) { return new WaveformMockReader(filepath); };
		vfs.onGetFileSize = [](const char* filepath) { return WaveformMockReader::fakeFileSize; };

		const uint32 validOutputBusIndex = 0;
		const uint32 invalidOutputBusIndex = 1;
		const uint32 validInputBusIndex = 0;
		const uint32 invalidInputBusIndex = 1;

		{
			MA::Engine engineTest;
			engineTest.reset(new ma_engine());
			EXPECT_TRUE(engineTest.Init(2, &vfs));

			// Here we check to make sure Engine itself has no inputs,
			// but the endpoint bus is configured to match the device channel count.
			//
			// To init engine with channel count not matching the default device,
			// use has to init their own device or use MA_NO_DEVICE_IO

			EXPECT_EQ(engineTest.GetNumOutputBusses(), 1);
			EXPECT_EQ(engineTest.GetNumInputBusses(), 0);
			EXPECT_EQ(engineTest.GetNumInputChannels(validInputBusIndex), 0);
			EXPECT_EQ(engineTest.GetNumInputChannels(invalidInputBusIndex), 0);
			EXPECT_EQ(engineTest.GetNumOutputChannels(validOutputBusIndex), 2);
			EXPECT_EQ(engineTest.GetNumOutputChannels(invalidOutputBusIndex), 0);

			EXPECT_TRUE(engineTest.GetSampleRate() > 0);
			EXPECT_EQ(engineTest.GetSampleRate(), engineTest->pDevice->sampleRate);
			EXPECT_EQ(engineTest.GetSampleRateDouble(), static_cast<double>(engineTest->pDevice->sampleRate));

			ASSERT_TRUE(engineTest.get() != nullptr);
			ASSERT_TRUE(engineTest->pDevice != nullptr);
			EXPECT_EQ(engineTest.GetEndpointBus().GetNumChannels(), engineTest->pDevice->playback.channels);
		}
		{
			MA::Engine engineTest;
			engineTest.reset(new ma_engine());
			EXPECT_TRUE(engineTest.Init(0, &vfs));
			EXPECT_EQ(engineTest.GetNumOutputBusses(), 1);
			EXPECT_EQ(engineTest.GetNumInputBusses(), 0);
			EXPECT_EQ(engineTest.GetNumInputChannels(0), 0);
			EXPECT_EQ(engineTest.GetNumInputChannels(1), 0);
			EXPECT_EQ(engineTest.GetNumOutputChannels(validOutputBusIndex), 2);
			EXPECT_EQ(engineTest.GetNumOutputChannels(invalidOutputBusIndex), 0);
			
			EXPECT_TRUE(engineTest.GetSampleRate() > 0);
			EXPECT_EQ(engineTest.GetSampleRate(), engineTest->pDevice->sampleRate);
			EXPECT_EQ(engineTest.GetSampleRateDouble(), static_cast<double>(engineTest->pDevice->sampleRate));

			ASSERT_TRUE(engineTest.get() != nullptr);
			ASSERT_TRUE(engineTest->pDevice != nullptr);
			EXPECT_EQ(engineTest.GetEndpointBus().GetNumChannels(), engineTest->pDevice->playback.channels);
		}
	}

	TEST_F(MiniaudioWrappersTest, SplitterNode)
	{
		MA::SplitterNode uninitSplitterNode;
		EXPECT_TRUE(uninitSplitterNode.get() == nullptr);
		// Splitter node is started by default,
		// there seems to be no need to evern stop it (?)
		EXPECT_FALSE(uninitSplitterNode.IsNodeStarted());

		static constexpr uint32 invalidNumChannels = 0;
		static constexpr uint32 invalidNumOutBusses = 0;

		static constexpr uint32 validNumChannels = 2;
		static constexpr uint32 validNumOutBusses = 2;

		EXPECT_FALSE(uninitSplitterNode.Init(invalidNumChannels, invalidNumOutBusses));
		EXPECT_FALSE(uninitSplitterNode.Init(invalidNumChannels, validNumOutBusses));
		EXPECT_FALSE(uninitSplitterNode.Init(validNumChannels, invalidNumOutBusses));

		EXPECT_FALSE(uninitSplitterNode.IsNodeStarted());


		// Initialize node with 2 channel input bus and 2 output buses
		EXPECT_TRUE(uninitSplitterNode.Init(validNumChannels, validNumOutBusses));

		MA::SplitterNode& initializedSplitterNode = uninitSplitterNode;
		EXPECT_FALSE(initializedSplitterNode.get() == nullptr);
		EXPECT_TRUE(initializedSplitterNode.IsNodeStarted());
		EXPECT_EQ(initializedSplitterNode.GetNumInputBusses(), 1);
		EXPECT_EQ(initializedSplitterNode.GetNumOutputBusses(), validNumOutBusses);
	}

	TEST_F(MiniaudioWrappersTest, EngineNode)
	{
		MA::EngineNode uninitEngineNode;
		EXPECT_TRUE(uninitEngineNode.get() == nullptr);
		EXPECT_FALSE(uninitEngineNode.IsNodeStarted());
		EXPECT_EQ(uninitEngineNode.GetPitch(), 0.0f);

		static constexpr uint32 invalidNumChannels = 0;
		static constexpr uint32 invalidNumOutChannels = 0;

		static constexpr uint32 validNumChannels = 2;
		static constexpr uint32 validNumOutChannels = 2;

		EXPECT_FALSE(uninitEngineNode.InitGroup({ .NumInChannels = invalidNumChannels, .NumOutChannels = invalidNumOutChannels }));
		EXPECT_FALSE(uninitEngineNode.InitGroup({ .NumInChannels = invalidNumChannels, .NumOutChannels = validNumOutChannels }));
		EXPECT_FALSE(uninitEngineNode.InitGroup({ .NumInChannels = validNumChannels, .NumOutChannels = invalidNumOutChannels }));

		EXPECT_FALSE(uninitEngineNode.IsNodeStarted());


		// Initialize node with 2 channel input and outpu buses
		EXPECT_TRUE(uninitEngineNode.InitGroup({ .NumInChannels = validNumChannels, .NumOutChannels = validNumOutChannels }));

		MA::EngineNode& initializedEngineNode = uninitEngineNode;
		EXPECT_FALSE(initializedEngineNode.get() == nullptr);
		EXPECT_TRUE(initializedEngineNode.IsNodeStarted());
		EXPECT_EQ(initializedEngineNode.GetNumInputBusses(), 1);
		EXPECT_EQ(initializedEngineNode.GetNumOutputBusses(), 1);

		static constexpr float pitchInvalid = 0.0f;
		static constexpr float pitchValid = 2.0f;
		static constexpr float pitchNominal = 1.0f;

		EXPECT_EQ(initializedEngineNode.GetPitch(), pitchNominal);

		initializedEngineNode.SetPitch(pitchInvalid);
		EXPECT_EQ(initializedEngineNode.GetPitch(), pitchNominal);

		initializedEngineNode.SetPitch(pitchValid);
		EXPECT_EQ(initializedEngineNode.GetPitch(), pitchValid);
	}

	TEST_F(MiniaudioWrappersTest, Sound)
	{
		MA::Sound uninitSound;

		// Uninitialized node
		{
			EXPECT_TRUE(uninitSound.get() == nullptr);
			// Splitter node is started by default,
			// there seems to be no need to evern stop it (?)
			EXPECT_FALSE(uninitSound.IsNodeStarted());
			EXPECT_EQ(uninitSound.GetPitch(), 0.0f);
		}

		// We use file reader mock and internal in-memory wav file
		// instead of real files
		static constexpr const char* fakeFilepath = "Some filepath";
		static constexpr uint32 flags = 0;

		EXPECT_TRUE(uninitSound.Init(fakeFilepath, flags));

		MA::Sound& initializedSound = uninitSound;

		// Initialized node topology
		{
			EXPECT_FALSE(initializedSound.get() == nullptr);
			// Sounds in miniaudio are stopped by default
			EXPECT_FALSE(initializedSound.IsNodeStarted());
			EXPECT_EQ(initializedSound.GetNumInputBusses(), 0);
			EXPECT_EQ(initializedSound.GetNumOutputBusses(), 1);
			EXPECT_EQ(initializedSound.GetNumOutputChannels(0), 2);
			EXPECT_EQ(initializedSound.GetNumOutputChannels(1), 0);
			EXPECT_EQ(initializedSound.GetNumInputChannels(0), 0);
		}

		// Pitch and volume
		{
			static constexpr float pitchInvalid = 0.0f;
			static constexpr float pitchValid = 2.0f;
			const float pitchNominal = ma_sound_get_pitch(initializedSound);

			EXPECT_EQ(initializedSound.GetPitch(), pitchNominal);

			initializedSound.SetPitch(pitchInvalid);
			EXPECT_EQ(ma_sound_get_pitch(initializedSound), pitchNominal);
			EXPECT_EQ(initializedSound.GetPitch(), ma_sound_get_pitch(initializedSound));

			initializedSound.SetPitch(pitchValid);
			EXPECT_EQ(ma_sound_get_pitch(initializedSound), pitchValid);
			EXPECT_EQ(initializedSound.GetPitch(), ma_sound_get_pitch(initializedSound));

			const float baseVolume = ma_sound_get_volume(initializedSound);
			EXPECT_EQ(baseVolume, 1.0f);

			const float newVolume = 2.0f;
			initializedSound.SetVolume(newVolume);
			EXPECT_EQ(ma_sound_get_volume(initializedSound), newVolume);
			EXPECT_EQ(initializedSound.GetVolume(), ma_sound_get_volume(initializedSound));
		}


		// Playing and looping states
		{
			EXPECT_FALSE(initializedSound.IsAtEnd());

			EXPECT_TRUE(initializedSound.Start());
			EXPECT_TRUE(ma_sound_is_playing(initializedSound));
			EXPECT_TRUE(initializedSound.IsPlaying());
			EXPECT_EQ(initializedSound.IsAtEnd(), ma_sound_at_end(initializedSound));

			EXPECT_TRUE(initializedSound.Stop());
			EXPECT_FALSE(ma_sound_is_playing(initializedSound));
			EXPECT_FALSE(initializedSound.IsPlaying());

			EXPECT_FALSE(initializedSound.IsLooping());
			EXPECT_FALSE(ma_sound_is_looping(initializedSound));

			initializedSound.SetLooping(true);
			EXPECT_TRUE(ma_sound_is_looping(initializedSound));
			EXPECT_TRUE(initializedSound.IsLooping());
		}

		// Cursor and length
		{
			EXPECT_EQ(initializedSound.GetLengthInFrames(), WaveformMockReader::durationInFrames);
			EXPECT_EQ(initializedSound.GetLengthInSeconds(), WaveformMockReader::durationInSeconds);

			EXPECT_EQ(initializedSound.GetCursorInFrames(), 0);
			// This sets 'seekTarget', and it's always succeeds
			EXPECT_TRUE(initializedSound.SeekToFrame(initializedSound.GetLengthInFrames() + 1));
			// This always returns 'seekTarget' if set, even if it is invalid
			EXPECT_EQ(initializedSound.GetCursorInFrames(), initializedSound.GetLengthInFrames() + 1);

			EXPECT_TRUE(initializedSound.SeekToFrame(initializedSound.GetLengthInFrames() - 1));
			EXPECT_EQ(initializedSound.GetCursorInFrames(), initializedSound.GetLengthInFrames() - 1);
			EXPECT_FLOAT_EQ(initializedSound.GetCursorInSeconds(), static_cast<float>(initializedSound.GetLengthInFrames() - 1) / WaveformMockReader::sourceSampleRate);
		}

		// Initialize Sound from Data Source
		{
			struct DataSourceMock
			{
				ma_data_source_base base;
				using SampleType = float;

				bool Read(SampleType* pFramesOut, uint64 frameCount, uint64& pFramesRead) { return true; }
				bool Seek(uint64 frameIndex) { return true; }
				void GetDataFormat(ma_format& format, uint32& numChannels, uint32& sampleRate)
				{
					format = ma_format_f32;
					numChannels = WaveformMockReader::sourceNumChannels;
					sampleRate = WaveformMockReader::sourceSampleRate;
				}
				uint64 GetCursor() { return 0; }
				uint64 GetLength() { return WaveformMockReader::durationInFrames; }
				void SetLooping(bool shouldLoop) {}
			};

			JPL::DataSource<DataSourceMock> dataSource;
			ASSERT_TRUE(dataSource.Init());

			JPL::Sound dataSourceSound;
			EXPECT_TRUE(dataSourceSound.InitFromDataSource(dataSource, flags));
			EXPECT_FALSE(dataSourceSound.IsNodeStarted());
			EXPECT_EQ(dataSourceSound.GetNumInputBusses(), 0);
			EXPECT_EQ(dataSourceSound.GetNumOutputBusses(), 1);
			EXPECT_EQ(dataSourceSound.GetNumOutputChannels(0), 2);
			EXPECT_EQ(dataSourceSound.GetNumOutputChannels(1), 0);
			EXPECT_EQ(dataSourceSound.GetNumInputChannels(0), 0);
		}
	}

	TEST_F(MiniaudioWrappersTest, LPFNode)
	{
		static constexpr uint32 validNumChannels = 2;
		static constexpr uint32 invalidNumChannels = 0;
		static constexpr double validCutoffFrequency = 1'000.0;
		static constexpr double invalidCutoffFrequency = -500.0;
		static constexpr double tooHighCutoffFrequency = 50'000.0;
		static constexpr uint32 zeroOrder = 0;
		static constexpr uint32 validOrder = 1;
		static constexpr uint32 invalidOrder = MA_MAX_FILTER_ORDER + 1;
		static constexpr uint32 defaultSampleRate = 0;

		EXPECT_FALSE(JPL::LPFNode().Init(invalidNumChannels, validCutoffFrequency, validOrder, defaultSampleRate));
		EXPECT_TRUE(JPL::LPFNode().Init(validNumChannels, validCutoffFrequency, invalidOrder, defaultSampleRate));
		EXPECT_TRUE(JPL::LPFNode().Init(validNumChannels, validCutoffFrequency, validOrder, defaultSampleRate));
		EXPECT_TRUE(JPL::LPFNode().Init(validNumChannels, invalidCutoffFrequency, validOrder, defaultSampleRate));
		EXPECT_TRUE(JPL::LPFNode().Init(validNumChannels, tooHighCutoffFrequency, validOrder, defaultSampleRate));
		EXPECT_TRUE(JPL::LPFNode().Init(validNumChannels, validCutoffFrequency, zeroOrder, defaultSampleRate));

		JPL::LPFNode lpf;
		ASSERT_TRUE(lpf.Init(validNumChannels, validCutoffFrequency, validOrder, defaultSampleRate));
		EXPECT_EQ(lpf.GetOrder(), validOrder);

		lpf.SetCutoffFrequency(tooHighCutoffFrequency);
		EXPECT_DOUBLE_EQ(lpf.GetCutoffFrequency(), tooHighCutoffFrequency);
	}

	TEST_F(MiniaudioWrappersTest, HPFNode)
	{
		static constexpr uint32 validNumChannels = 2;
		static constexpr uint32 invalidNumChannels = 0;
		static constexpr double validCutoffFrequency = 1'000.0;
		static constexpr double invalidCutoffFrequency = -500.0;
		static constexpr double tooHighCutoffFrequency = 50'000.0;
		static constexpr uint32 zeroOrder = 0;
		static constexpr uint32 validOrder = 3;
		static constexpr uint32 invalidOrder = MA_MAX_FILTER_ORDER + 1;
		static constexpr uint32 defaultSampleRate = 0;

		EXPECT_FALSE(JPL::HPFNode().Init(invalidNumChannels, validCutoffFrequency, validOrder, defaultSampleRate));
		EXPECT_TRUE(JPL::HPFNode().Init(validNumChannels, validCutoffFrequency, invalidOrder, defaultSampleRate));
		EXPECT_TRUE(JPL::HPFNode().Init(validNumChannels, validCutoffFrequency, validOrder, defaultSampleRate));
		EXPECT_TRUE(JPL::HPFNode().Init(validNumChannels, invalidCutoffFrequency, validOrder, defaultSampleRate));
		EXPECT_TRUE(JPL::HPFNode().Init(validNumChannels, tooHighCutoffFrequency, validOrder, defaultSampleRate));
		EXPECT_TRUE(JPL::HPFNode().Init(validNumChannels, validCutoffFrequency, zeroOrder, defaultSampleRate));

		JPL::HPFNode lpf;
		ASSERT_TRUE(lpf.Init(validNumChannels, validCutoffFrequency, validOrder, defaultSampleRate));
		EXPECT_EQ(lpf.GetOrder(), validOrder);

		lpf.SetCutoffFrequency(tooHighCutoffFrequency);
		EXPECT_DOUBLE_EQ(lpf.GetCutoffFrequency(), tooHighCutoffFrequency);
	}
} // namespace JPL

#endif // JPL_TEST