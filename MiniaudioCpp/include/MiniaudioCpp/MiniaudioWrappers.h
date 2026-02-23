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

#include "Core.h"
#include "ErrorReporting.h"

#ifndef MA_DEFAULT_NODE_CACHE_CAP_IN_FRAMES_PER_BUS
#define MA_DEFAULT_NODE_CACHE_CAP_IN_FRAMES_PER_BUS 480
#endif
#include "NodeTraits.h"

#include "choc/containers/choc_SmallVector.h"
#include "choc/audio/choc_SampleBuffers.h"

#include <span>

//==============================================================================
/*
	Wrappers around miniaudio C interface provide us with type-safety and 
	a single place to do fix-ups in case of changes to miniaudio's API.
*/

namespace MA = JPL;

namespace JPL
{
	//==========================================================================
	/// Forward declaration
	template<class TNode>
	struct TBaseNode;
	struct Engine;

	//==========================================================================
	/// These must be set by the client
	using GetMiniaudioEngineFunction = MA::Engine& (*)(void* context);
	JPL_EXPORT extern GetMiniaudioEngineFunction GetMiniaudioEngine;
	JPL_EXPORT extern ma_allocation_callbacks* gEngineAllocationCallbacks;


	//==========================================================================
	class ProcessCallbackData
	{
	private:
		ProcessCallbackData() = delete;

		template<class TNode>
		friend struct MA::TBaseNode;
		// ..add more friends if need to construct ProcessCallbackData

		ProcessCallbackData(const uint32_t& InputBusCount,
							const uint32_t& OutputBusCount,
							ma_node_base* nodeBase,
							const float** ppFramesIn,
							ma_uint32* pFrameCountIn,
							float** ppFramesOut,
							ma_uint32* pFrameCountOut)
			: InputBusCount(InputBusCount)
			, OutputBusCount(OutputBusCount)
			, nodeBase(nodeBase)
			, ppFramesIn(ppFramesIn)
			, pFrameCountIn(pFrameCountIn)
			, ppFramesOut(ppFramesOut)
			, pFrameCountOut(pFrameCountOut)
		{
		}

	public:
		using InputBuffer = choc::buffer::InterleavedView<const float>;
		using OutputBuffer = choc::buffer::InterleavedView<float>;

		InputBuffer GetInputBuffer(uint32_t busIndex)
		{
			// Not checking for NULL input, making it caller's responsibility to allow it with node FLAGS
			const uint32_t numChannels = MA::InputBusIndex(busIndex).Of(nodeBase).GetNumChannels();
			return choc::buffer::createInterleavedView(ppFramesIn[busIndex], numChannels, *pFrameCountIn);
		}

		OutputBuffer GetOutputBuffer(uint32_t busIndex)
		{
			const uint32_t numChannels = MA::OutputBusIndex(busIndex).Of(nodeBase).GetNumChannels();
			return choc::buffer::createInterleavedView(ppFramesOut[busIndex], numChannels, *pFrameCountIn);
		}

		const uint32_t InputBusCount;
		const uint32_t OutputBusCount;

		JPL_INLINE bool IsNullInput() const { return ppFramesIn == nullptr; }

		JPL_INLINE uint32_t GetInputFrameCount() const { return *pFrameCountIn; }
		JPL_INLINE uint32_t GetOutputFrameCount() const { return *pFrameCountOut; }

		JPL_INLINE void FillOutputBusWithSilence(uint32_t outputBusIndex)
		{
			const auto outputBuffer = GetOutputBuffer(outputBusIndex);
			ma_silence_pcm_frames(outputBuffer.data.data, outputBuffer.getNumFrames(), ma_format_f32, outputBuffer.getNumChannels());
		}

		void FillOutputWithSilence()
		{
			for (uint32_t i = 0; i < OutputBusCount; ++i)
				FillOutputBusWithSilence(i);
		}

		void CopyInputsToOutputs()
		{
			for (uint32_t i = 0; i < std::min(InputBusCount, OutputBusCount); ++i)
			{
				const auto inputBuffer = GetInputBuffer(i);
				const auto outputBuffer = GetOutputBuffer(i);

				//! Input buffer and output buffer must have the same number of channels
				//! however we still take min, just in case
				const uint32_t numFramesToCopy = std::min(inputBuffer.getNumFrames(), outputBuffer.getNumFrames());
				const uint32_t numChannels = std::min(inputBuffer.getNumChannels(), outputBuffer.getNumChannels());

				memcpy(outputBuffer.data.data, inputBuffer.data.data, sizeof(InputBuffer::Sample) * numFramesToCopy * numChannels);
			}
		}

	private:
		ma_node_base* nodeBase;
		const float** ppFramesIn;
		ma_uint32* pFrameCountIn;
		float** ppFramesOut;
		ma_uint32* pFrameCountOut;
	};

	//==========================================================================
	using OnProcessCb = void(*)(ma_node* pNode, const float** ppFramesIn, ma_uint32* pFrameCountIn, float** ppFramesOut, ma_uint32* pFrameCountOut);
	using OnGetRequiredInputFrameCountCb = ma_result(*)(ma_node* pNode, ma_uint32 outputFrameCount, ma_uint32* pInputFrameCount);


	//==========================================================================
	/// Simple factory utility to pass around bus IO configuration
	struct BusConfig
	{
		choc::SmallVector<uint32_t, 2> Inputs;
		choc::SmallVector<uint32_t, 2> Outputs;

		BusConfig& WithInputs(std::initializer_list<uint32_t> inputs) { Inputs = std::span{ inputs.begin(), inputs.end() }; return *this; }
		BusConfig& WithOutputs(std::initializer_list<uint32_t> outputs) { Outputs = std::span(outputs.begin(), outputs.end()); return *this; }

		inline BusConfig& WithInputs(uint32_t inputChannels) { return WithInputs({ inputChannels }); }
		inline BusConfig& WithOutputs(uint32_t outputChannels) { return WithOutputs({ outputChannels }); }
	};

	//==========================================================================
	/// Helper structure used to construct/initialize nodes
	class NodeLayout
	{
	public:
		NodeLayout() = default;

		NodeLayout& WithInputs(std::initializer_list<uint32_t> inputs) { BusConfig.Inputs = std::span{ inputs.begin(), inputs.end() }; return *this; }
		NodeLayout& WithOutputs(std::initializer_list<uint32_t> outputs) { BusConfig.Outputs = std::span(outputs.begin(), outputs.end()); return *this; }

		inline NodeLayout& WithInputs(uint32_t inputChannels) { return WithInputs({ inputChannels }); }
		inline NodeLayout& WithOutputs(uint32_t outputChannels) { return WithOutputs({ outputChannels }); }

		NodeLayout& WithBusConfig(const BusConfig& busConfig) { BusConfig = busConfig; return *this; }
		
		BusConfig BusConfig;
	};

	//==========================================================================
	namespace impl
	{
		template<class T> concept CHasInit = requires(T ds) { { ds.Init() } -> std::same_as<void>; };
		template<class T> concept CHasCursor = requires(T ds) { { ds.GetCursor() } -> std::same_as<uint64>; };
		template<class T> concept CHasLength = requires(T ds) { { ds.GetLength() } -> std::same_as<uint64>; };
		template<class T> concept CCanLoop = requires(T ds) { { ds.SetLooping(true) } -> std::same_as<void>; };
		template<class T> concept CHasChannelMap = requires(T ds) { { ds.GetChannelMap(std::declval<std::span<ma_channel>>()) } -> std::same_as<void>; };

		template<class T>
		concept CDataSource = requires(T ds)
		{
			// TODO: (can we remove ma_data_source_base from user's Data Source?)
			{ ds.base } -> std::same_as<ma_data_source_base&>;

			{
				ds.Read(static_cast<T::SampleType*>(nullptr), uint64(0), std::declval<uint64&>())
			} -> std::same_as<bool>;

			{
				ds.GetDataFormat(std::declval<ma_format&>(), std::declval<uint32&>(), std::declval<uint32&>())
			} -> std::same_as<void>;
		};
	}

	//==========================================================================
	// Generic Data Source CRTP interface that can be plugged into miniaudio's
	// data_source API.
	template<impl::CDataSource DataSourceType>
	struct DataSource : Internal::DataSource
	{
		bool Init();

	private:
		static ma_result SourceRead(ma_data_source* pDataSource, void* pFramesOut, ma_uint64 frameCount, ma_uint64* pFramesRead);
		static ma_result SourceSeek(ma_data_source* pDataSource, ma_uint64 frameIndex);
		static ma_result SourceGetDataFormat(ma_data_source* pDataSource, ma_format* pFormat, ma_uint32* pChannels, ma_uint32* pSampleRate, ma_channel* pChannelMap, size_t channelMapCap);
		static ma_result SourceGetCursor(ma_data_source* pDataSource, ma_uint64* pCursor) requires (impl::CHasCursor<DataSourceType>);
		static ma_result SourceGetLength(ma_data_source* pDataSource, ma_uint64* pLength) requires (impl::CHasLength<DataSourceType>);
		static ma_result SourceSetLooping(ma_data_source* pDataSource, ma_bool32 isLooping) requires (impl::CCanLoop<DataSourceType>);

	private:
		DataSourceType mInternalSource;

		static inline ma_data_source_vtable sDataSourceVable = 
		{
			SourceRead,
			SourceSeek,
			SourceGetDataFormat,
			impl::CHasCursor<DataSourceType> ? SourceGetCursor : nullptr,	// These can be NULL if not valid for DataSourceType
			impl::CHasLength<DataSourceType> ? SourceGetLength : nullptr,	// These can be NULL if not valid for DataSourceType
			impl::CCanLoop<DataSourceType> ? SourceSetLooping : nullptr,	// These can be NULL if not valid for DataSourceType
			0, // 'flags' (MA_DATA_SOURCE_SELF_MANAGED_RANGE_AND_LOOP_POINT = 1)
		};
	};
	
	// This improves readablility for the functions inherited from the traits
#define TRAIT_DEFS(InternalBaseNode)										\
	using Topology = Traits::NodeDefaultTraits<InternalBaseNode>::Topology;	\
	using Routing = Traits::NodeDefaultTraits<InternalBaseNode>::Routing


	//==========================================================================
	struct Engine : Traits::EngineNodeTraits<Internal::Engine>
	{
		bool Init(uint32_t numChannels, ma_vfs* vfs);
		uint32_t GetSampleRate() const;
		inline double GetSampleRateDouble() const { return static_cast<double>(GetSampleRate()); }
		uint32_t GetProcessingSizeInFrames() const;
		InputBus GetEndpointBus();
	};

	//==========================================================================
	// Initialization utility for custom nodes, used by TBaseNode
	struct BaseNode
	{
		BaseNode() = delete;

	private:
		template<class TNode>
		friend struct TBaseNode;
		//friend bool TBaseNode<TNode>::Init(const NodeLayout& nodeLayout, bool initStarted);
		static ma_node_config InitConfig(const NodeLayout& nodeLayout, bool initStarted = true);
	};

	//==========================================================================
	template<class TNode>
	struct TBaseNode : Traits::NodeDefaultTraits<Internal::TNodeBase<TNode>>
	{
		TRAIT_DEFS(Internal::TNodeBase<TNode>);

		static constexpr bool IS_PASSTHROUGH = (TNode::FLAGS & MA_NODE_FLAG_PASSTHROUGH) != 0;

		bool Init(const NodeLayout& nodeLayout, bool initStarted = true)
		{
			if constexpr (IS_PASSTHROUGH)
			{
				if (!JPL_ENSURE(nodeLayout.BusConfig.Inputs.size() == 1 && nodeLayout.BusConfig.Outputs.size() == 1))
				{
					// Invalid configuration. You must not specify a conflicting bus count between
					// the node's config and the vtable.
					// Passthrough node can only have single Input and Output bus configuration.
					return false;
				}
			}

			static constexpr ma_node_vtable vtable
			{
				.onProcess = sProcess,
				.inputBusCount = IS_PASSTHROUGH ? 1 : MA_NODE_BUS_COUNT_UNKNOWN,
				.outputBusCount = IS_PASSTHROUGH ? 1 : MA_NODE_BUS_COUNT_UNKNOWN,
				.flags = TNode::FLAGS
			};
			
			ma_node_config config = BaseNode::InitConfig(nodeLayout, initStarted);
			config.vtable = &vtable;

			ma_engine* engine = GetMiniaudioEngine(nullptr);
			const ma_result result = this->emplace(&engine->nodeGraph, &config, gEngineAllocationCallbacks);

			if (!JPL_ENSURE(!result))
			{
				TNode* node = this->release();
				delete node;
				return false;
			}

			return true;
		}

		bool StartNode() { return ma_node_set_state(this->get(), ma_node_state_started) == MA_SUCCESS; }
		bool StopNode() { return ma_node_set_state(this->get(), ma_node_state_stopped) == MA_SUCCESS; }

	private:
		static void sProcess(ma_node* pNode, const float** ppFramesIn, ma_uint32* pFrameCountIn, float** ppFramesOut, ma_uint32* pFrameCountOut)
		{
			const uint32_t numInBusses = ma_node_get_input_bus_count(pNode);
			const uint32_t numOutBusses = ma_node_get_output_bus_count(pNode);
			
			JPL::ProcessCallbackData callbackData(
				numInBusses,
				numOutBusses,
				static_cast<ma_node_base*>(pNode),
				ppFramesIn,
				pFrameCountIn,
				ppFramesOut,
				pFrameCountOut
			);

			static_cast<TNode*>(pNode)->Process(std::ref(callbackData));
		}
	};

	//==========================================================================
	struct SplitterNode : Traits::NodeDefaultTraits<Internal::SplitterNode>
						//, WeakPtrBase<SplitterNode>
	{
		TRAIT_DEFS(Internal::SplitterNode);

		bool Init(uint32_t numChannels, uint32_t numOutputBusses = 2);
	};

	//==========================================================================
	struct EngineNode : Traits::NodeDefaultTraits<Internal::EngineNode>
	{
		TRAIT_DEFS(Internal::EngineNode);

		struct GroupNodeSettings
		{
			uint32_t NumInChannels = 0;
			uint32_t NumOutChannels = 0;
			uint32_t VolumeFadeFrameCount = 256;
			bool PitchDisabled = false;
		};
		bool InitGroup(const GroupNodeSettings& settings);

		void SetPitch(float pitch);
		float GetPitch() const;
	};

	//==========================================================================
	struct Sound	: Traits::NodeDefaultTraits<Internal::Sound>
	{
		TRAIT_DEFS(Internal::Sound);

		bool Init(const char* filePathOrId, uint32_t flags, bool bUseSourceChannelCount = false);
		bool InitFromDataSource(Internal::DataSource& dataSource, uint32_t flags);

		void SetVolume(float volume);
		float GetVolume() const;
		void SetPitch(float pitch);
		float GetPitch() const;

		bool Start();
		bool Stop();
		bool IsPlaying() const;
		bool IsAtEnd() const;

		void SetFade(float volumeBeg, float volumeEnd, uint64_t fadeLengthInMilliseconds);
		void SetFadeInFrames(float volumeBeg, float volumeEnd, uint64_t fadeLengthInFrames);
		float GetCurrentFadeVolume() const;
		void SetLooping(bool shouldLoop);
		bool IsLooping() const;

		bool SeekToFrame(uint64_t pcmFrameToSeekTo);
		uint64_t GetLengthInFrames();
		uint64_t GetCursorInFrames();
		float GetLengthInSeconds();
		float GetCursorInSeconds();

		/*	TODO: the rest of the interface(not needed for now)
			void ma_sound_set_start_time_in_pcm_frames(ma_sound* pSound, ma_uint64 absoluteGlobalTimeInFrames);
			void ma_sound_set_start_time_in_milliseconds(ma_sound* pSound, ma_uint64 absoluteGlobalTimeInMilliseconds);
			void ma_sound_set_stop_time_in_pcm_frames(ma_sound* pSound, ma_uint64 absoluteGlobalTimeInFrames);
			void ma_sound_set_stop_time_in_milliseconds(ma_sound* pSound, ma_uint64 absoluteGlobalTimeInMilliseconds);
			ma_uint64 ma_sound_get_time_in_pcm_frames(const ma_sound* pSound);
			ma_uint64 ma_sound_get_time_in_milliseconds(const ma_sound* pSound);
			ma_result ma_sound_set_end_callback(ma_sound* pSound, ma_sound_end_proc callback, void* pUserData);
		*/
	};

	//==========================================================================
	struct LPFNode : Traits::NodeDefaultTraits<Internal::LPFNode>
	{
		TRAIT_DEFS(Internal::LPFNode);

		bool Init(uint32_t numChannels, double cutoffFrequency, uint32_t order, uint32_t sampleRate = 0);

		void SetCutoffFrequency(double newCutoffFrequency);
		double GetCutoffFrequency() const { return mCutoffFrequency; }

		uint32 GetOrder() const { return mOrder; }

	private:
		uint32_t mOrder = 0;
		double mCutoffFrequency = 0.0;
	};

	//==========================================================================
	struct HPFNode : Traits::NodeDefaultTraits<Internal::HPFNode>
	{
		TRAIT_DEFS(Internal::LPFNode);

		bool Init(uint32_t numChannels, double cutoffFrequency, uint32_t order, uint32_t sampleRate = 0);

		void SetCutoffFrequency(double newCutoffFrequency);
		double GetCutoffFrequency() const { return mCutoffFrequency; }
		uint32 GetOrder() const { return mOrder; }

	private:
		uint32_t mOrder = 0;
		double mCutoffFrequency = 0.0;
	};

#undef TRAIT_DEFS

//==============================================================================
//
//   Code beyond this point is implementation detail...
//
//==============================================================================

	template<impl::CDataSource DataSourceType>
	inline bool DataSource<DataSourceType>::Init()
	{
		ma_result result;
		ma_data_source_config baseConfig;

		baseConfig = ma_data_source_config_init();
		baseConfig.vtable = &sDataSourceVable;

		result = emplace(&baseConfig);
		if (result != MA_SUCCESS)
			return false;

		if constexpr (impl::CHasInit<DataSourceType>)
			mInternalSource.Init();

		return result == MA_SUCCESS;
	}

	template<impl::CDataSource DataSourceType>
	inline ma_result DataSource<DataSourceType>::SourceRead(ma_data_source* pDataSource, void* pFramesOut, ma_uint64 frameCount, ma_uint64* pFramesRead)
	{
		uint64 framesRead = 0;
		if (static_cast<DataSourceType*>(pDataSource)->Read(static_cast<DataSourceType::SampleType*>(pFramesOut), frameCount, framesRead))
		{
			if (pFramesRead)
				*pFramesRead = framesRead;
			return MA_SUCCESS;
		}

		return MA_ERROR;
	}
	
	template<impl::CDataSource DataSourceType>
	inline ma_result DataSource<DataSourceType>::SourceSeek(ma_data_source* pDataSource, ma_uint64 frameIndex)
	{
		if constexpr (requires(DataSourceType ds) { { ds.Seek(uint64(0)) } -> std::same_as<bool>; })
		{
			if (static_cast<DataSourceType*>(pDataSource)->Seek(frameIndex))
				return MA_SUCCESS;
			else
				return MA_ERROR;
		}
		else
		{
			return MA_NOT_IMPLEMENTED;
		}
	}

	template<impl::CDataSource DataSourceType>
	inline ma_result DataSource<DataSourceType>::SourceGetDataFormat(ma_data_source* pDataSource, ma_format* pFormat, ma_uint32* pChannels, ma_uint32* pSampleRate, ma_channel* pChannelMap, size_t channelMapCap)
	{
		ma_format format = ma_format_unknown;
		uint32 numChannels = 0;
		uint32 sampleRate = 0;

		auto& source = *static_cast<DataSourceType*>(pDataSource);
		source.GetDataFormat(format, numChannels, sampleRate);

		if (pFormat)
			*pFormat = format;

		if (pChannels)
			*pChannels = numChannels;

		if (pSampleRate)
			*pSampleRate = sampleRate;

		if constexpr (impl::CHasChannelMap<DataSourceType>)
		{
			if (pChannelMap && channelMapCap)
				source.GetChannelMap(std::span<ma_channel>(pChannelMap, channelMapCap));

		}
		else
		{
			if (pChannelMap && channelMapCap)
				ma_channel_map_init_standard(ma_standard_channel_map_microsoft, pChannelMap, channelMapCap, numChannels);
		}

		return MA_SUCCESS;
	}
	
	template<impl::CDataSource DataSourceType>
	inline ma_result DataSource<DataSourceType>::SourceGetCursor(ma_data_source* pDataSource, ma_uint64* pCursor) requires (impl::CHasCursor<DataSourceType>)
	{
		*pCursor = static_cast<DataSourceType*>(pDataSource)->GetCursor();
		return MA_SUCCESS;
	}
	
	template<impl::CDataSource DataSourceType>
	inline ma_result DataSource<DataSourceType>::SourceGetLength(ma_data_source* pDataSource, ma_uint64* pLength) requires (impl::CHasLength<DataSourceType>)
	{
		*pLength = static_cast<DataSourceType*>(pDataSource)->GetLength();
		return MA_SUCCESS;
	}

	template<impl::CDataSource DataSourceType>
	inline ma_result JPL::DataSource<DataSourceType>::SourceSetLooping(ma_data_source* pDataSource, ma_bool32 isLooping) requires (impl::CCanLoop<DataSourceType>)
	{
		static_cast<DataSourceType*>(pDataSource)->SetLooping(isLooping);
		return MA_SUCCESS;
	}

} // namespace JPL
