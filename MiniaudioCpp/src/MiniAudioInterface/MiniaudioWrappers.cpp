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

#include "MiniaudioWrappers.h"

#include "VFS.h"

#include "ErrorReporting.h"

// We have to import this stuff because miniaudio
// doesn't expose some of the functionality we need
#include "c89atomic.h"

#include <string>
#include <format>


namespace JPL
{
	//static std::vector<std::unique_ptr<ma_node_vtable>> s_NodeVTables;
	
	//==========================================================================
	/// This function should be overriden
	static MA::Engine& DummyGetMiniaudioEngine(void* /*context*/)
	{
		JPL_ASSERT(false, "GetMiniaudioEngine global function is not provided by the client");
		static MA::Engine sEngine;
		return sEngine;
	}

	GetMiniaudioEngineFunction GetMiniaudioEngine = DummyGetMiniaudioEngine;
	ma_allocation_callbacks gEngineAllocationCallbacks = ma_allocation_callbacks{};

	namespace // log
	{
		void MALogCallback(void* pUserData, ma_uint32 level, const char* pMessage)
		{
			std::string message = std::format("{0}: {1}", std::string(ma_log_level_to_string(level)), pMessage);
			message.erase(std::remove(message.begin(), message.end(), '\n'), message.end());
			message.erase(std::remove(message.begin(), message.end(), '{'), message.end());
			message.erase(std::remove(message.begin(), message.end(), '}'), message.end());

			switch (level)
			{
				case MA_LOG_LEVEL_INFO:
					JPL_TRACE_TAG("miniaudio", message);
					break;
				case MA_LOG_LEVEL_WARNING:
					JPL_INFO_TAG("miniaudio", message);
					break;
				case MA_LOG_LEVEL_ERROR:
					JPL_ERROR_TAG("miniaudio", message);
					break;
				default:
					JPL_TRACE_TAG("miniaudio", message);
			}
		}

		ma_log g_maLog;
	}

	//==========================================================================
	bool Engine::Init(uint32_t numChannels, ma_vfs* vfs)
	{
		ma_result result = MA_SUCCESS;

		ma_engine_config engineConfig = ma_engine_config_init();
		engineConfig.allocationCallbacks = gEngineAllocationCallbacks;

		// Channel count is only honored if using custom device or MA_NO_DEVICE_IO
		engineConfig.channels = numChannels;

		// TODO: for now splitter node and custom node don't work toghether if custom periodSizeInFrames is set
		//engineConfig.periodSizeInFrames = PCM_FRAME_CHUNK_SIZE;

		// Hook up logging
		{
			result = ma_log_init(&gEngineAllocationCallbacks, &g_maLog);
			JPL_ASSERT(result == MA_SUCCESS, "Failed to initialize miniaudio logger.");
			
			ma_log_callback logCallback = ma_log_callback_init(&MALogCallback, nullptr);
			result = ma_log_register_callback(&g_maLog, logCallback);
			JPL_ASSERT(result == MA_SUCCESS, "Failed to register miniaudio log callback.");
		}

		engineConfig.pLog = &g_maLog;
		engineConfig.noDevice = false;
		engineConfig.pResourceManagerVFS = vfs;

		result = emplace(&engineConfig);
		
		if (!JPL_ENSURE(!result))
		{
			ma_engine* node = release();
			delete node;
		}

		return result == MA_SUCCESS;
	}

	uint32_t Engine::GetSampleRate() const
	{
		return ma_engine_get_sample_rate(get());
	}

	InputBus Engine::GetEndpointBus()
	{
		return InputBusIndex(0).Of(&get()->nodeGraph.endpoint);
	}

	//==========================================================================
	ma_node_config BaseNode::InitConfig(const NodeLayout& nodeLayout, bool initStarted)
	{
		ma_node_config config = ma_node_config_init();
		//config.vtable = &nodeLayout.VTable;
		config.pInputChannels = nodeLayout.BusConfig.Inputs.data();
		config.pOutputChannels = nodeLayout.BusConfig.Outputs.data();
		config.inputBusCount = static_cast<uint32_t>(nodeLayout.BusConfig.Inputs.size());
		config.outputBusCount = static_cast<uint32_t>(nodeLayout.BusConfig.Outputs.size());
		config.initialState = initStarted ? ma_node_state_started : ma_node_state_stopped;

		return config;
	}

	//==========================================================================
	bool SplitterNode::Init(uint32_t numChannels, uint32_t numOutputBusses /*= 2*/)
	{
		if (numChannels == 0 || numOutputBusses == 0)
			return false;

		if (Engine& engine = GetMiniaudioEngine(nullptr))
		{
			ma_splitter_node_config splitterConfig = ma_splitter_node_config_init(numChannels);
			splitterConfig.outputBusCount = std::max(1u, numOutputBusses);

			const ma_result result = emplace(&engine->nodeGraph, &splitterConfig, &gEngineAllocationCallbacks);
			
			if (!JPL_ENSURE(!result))
			{
				ma_splitter_node* node = release();
				delete node;
				return false;
			}

			return result == MA_SUCCESS;
		}
		else
		{
			return false;
		}
	}

	//==========================================================================
	bool EngineNode::InitGroup(const GroupNodeSettings& settings)
	{
		if (settings.NumInChannels == 0 || settings.NumOutChannels == 0)
			return false;

		if (Engine& engine = GetMiniaudioEngine(nullptr))
		{
			ma_engine_node_config nodeConfig = ma_engine_node_config_init(engine, ma_engine_node_type_group, MA_SOUND_FLAG_NO_SPATIALIZATION);

			nodeConfig.channelsIn = settings.NumInChannels;
			nodeConfig.channelsOut = settings.NumOutChannels; // must match endpoint input channel count, or use channel converter
			nodeConfig.volumeSmoothTimeInPCMFrames = settings.VolumeFadeFrameCount;
			nodeConfig.isPitchDisabled = settings.PitchDisabled;

			ma_result result = emplace(&nodeConfig, &gEngineAllocationCallbacks);

			if (!JPL_ENSURE(!result))
			{
				ma_engine_node* node = release();
				delete node;
				return false;
			}

			return result == MA_SUCCESS;
		}
		else
		{
			return false;
		}

	}

	void EngineNode::SetPitch(float pitch)
	{
		if ([[maybe_unused]]const ma_engine_node* node = get())
		{
			if (pitch <= 0)
				return;

			/*	miniaudio doesn't have a function to change pitch of an engine node, and it needs to be
				modified atomically, neither miniaudio's C atomics are exposed.
				Therefore we have to import the same c89 atomics form c89atomic.h.
			*/
			c89atomic_exchange_explicit_f32(&get()->pitch, pitch, c89atomic_memory_order_release);
		}
	}

	float EngineNode::GetPitch() const
	{
		if (const ma_engine_node* node = get())
		{
			// See the comment above, in SetPitch
			return c89atomic_load_explicit_f32(&node->pitch, c89atomic_memory_order_seq_cst);
		}
		else
		{
			return 0.0f;
		}
	}

	//==========================================================================
	bool Sound::Init(const char* filePathOrId, uint32_t flags)
	{
		if (Engine& engine = GetMiniaudioEngine(nullptr))
		{
			// Force disable miniaudio's spatialization, we use our own spatializer
			flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;

			ma_result result = emplace(engine, filePathOrId, flags, static_cast<ma_sound_group*>(nullptr), static_cast<ma_fence*>(nullptr));
			
			if (!JPL_ENSURE(!result))
			{
				ma_sound* node = release();
				delete node;
				return false;
			}

			return result == MA_SUCCESS;
		}
		else
		{
			return false;
		}
	}

	bool Sound::InitFromDataSource(Internal::DataSource& dataSource, uint32_t flags)
	{
		if (!dataSource)
			return false;

		if (Engine& engine = GetMiniaudioEngine(nullptr))
		{
			// Force disable miniaudio's spatialization, we use our own spatializer
			flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;

			reset(new ma_sound());

			ma_result result = ma_sound_init_from_data_source(engine, dataSource.get(), flags, static_cast<ma_sound_group*>(nullptr), get());
			if (!JPL_ENSURE(!result))
			{
				ma_sound* node = release();
				delete node;
				return false;
			}

			return result == MA_SUCCESS;
		}
		else
		{
			return false;
		}
	}

	void Sound::SetVolume(float volume)
	{
		ma_sound_set_volume(get(), volume);
	}

	float Sound::GetVolume() const
	{
		return ma_sound_get_volume(get());
	}

	void Sound::SetPitch(float pitch)
	{
		ma_sound_set_pitch(get(), pitch);
	}

	float Sound::GetPitch() const
	{
		return ma_sound_get_pitch(get());
	}

	bool Sound::Start()
	{
		return ma_sound_start(get()) == MA_SUCCESS;
	}

	bool Sound::Stop()
	{
		return ma_sound_stop(get()) == MA_SUCCESS;
	}

	bool Sound::IsPlaying() const
	{
		return ma_sound_is_playing(get());
	}

	bool Sound::IsAtEnd() const
	{
		return ma_sound_at_end(get());
	}

	void Sound::SetFade(float volumeBeg, float volumeEnd, uint64_t fadeLengthInMilliseconds)
	{
		ma_sound_set_fade_in_milliseconds(get(), volumeBeg, volumeEnd, fadeLengthInMilliseconds);
	}

	void Sound::SetFadeInFrames(float volumeBeg, float volumeEnd, uint64_t fadeLengthInFrames)
	{
		ma_sound_set_fade_in_pcm_frames(get(), volumeBeg, volumeEnd, fadeLengthInFrames);
	}

	float Sound::GetCurrentFadeVolume() const
	{
		return ma_sound_get_current_fade_volume(get());
	}

	void Sound::SetLooping(bool shouldLoop)
	{
		ma_sound_set_looping(get(), shouldLoop);
	}

	bool Sound::IsLooping() const
	{
		return ma_sound_is_looping(get());
	}

	bool Sound::SeekToFrame(uint64_t pcmFrameToSeekTo)
	{
		return ma_sound_seek_to_pcm_frame(get(), pcmFrameToSeekTo) == MA_SUCCESS;
	}

	uint64_t Sound::GetLengthInFrames()
	{
		ma_uint64 lengthPCFrames = 0;
		ma_sound_get_length_in_pcm_frames(get(), &lengthPCFrames);
		return (uint64_t) lengthPCFrames;
	}

	uint64_t Sound::GetCursorInFrames()
	{
		ma_uint64 cursorPCMFrame = 0;
		ma_sound_get_cursor_in_pcm_frames(get(), &cursorPCMFrame);
		return (uint64_t) cursorPCMFrame;
	}

	float Sound::GetLengthInSeconds()
	{
		float lengthSeconds = 0.0f;
		ma_sound_get_length_in_seconds(get(), &lengthSeconds);
		return lengthSeconds;
	}

	float Sound::GetCursorInSeconds()
	{
		float cursorSeconds = 0.0f;
		ma_sound_get_cursor_in_seconds(get(), &cursorSeconds);
		return cursorSeconds;
	}

	//==========================================================================
	bool LPFNode::Init(uint32_t numChannels, double cutoffFrequency, uint32_t order, uint32_t sampleRate /*= 0*/)
	{
		if (numChannels == 0)
			return false;

		if (Engine& engine = GetMiniaudioEngine(nullptr))
		{
			if (!sampleRate)
				sampleRate = engine.GetSampleRate();

			mOrder = std::clamp(order, 1u, static_cast<uint32>(MA_MAX_FILTER_ORDER));
			mCutoffFrequency = cutoffFrequency;

			ma_lpf_node_config config = ma_lpf_node_config_init(numChannels,
																sampleRate,
																mCutoffFrequency,
																mOrder);

			const ma_result result = emplace(&engine->nodeGraph, &config, &gEngineAllocationCallbacks);

			if (!JPL_ENSURE(!result))
			{
				ma_lpf_node* node = release();
				delete node;
				return false;
			}

			return result == MA_SUCCESS;
		}
		else
		{
			return false;
		}
	}

	void LPFNode::SetCutoffFrequency(double newCutoffFrequency)
	{
		if (auto* node = get())
		{
			mCutoffFrequency = newCutoffFrequency;

			const ma_lpf_config config = ma_lpf_config_init(node->lpf.format,
															node->lpf.channels,
															node->lpf.sampleRate,
															mCutoffFrequency,
															mOrder);

			[[maybe_unused]]ma_result result = ma_lpf_node_reinit(&config, node);
			JPL_ASSERT(result == MA_SUCCESS);
		}
	}

	//==========================================================================
	bool HPFNode::Init(uint32_t numChannels, double cutoffFrequency, uint32_t order, uint32_t sampleRate)
	{
		if (Engine& engine = GetMiniaudioEngine(nullptr))
		{
			if (!sampleRate)
				sampleRate = engine.GetSampleRate();

			mCutoffFrequency = cutoffFrequency;
			mOrder = std::clamp(order, 1u, static_cast<uint32>(MA_MAX_FILTER_ORDER));

			ma_hpf_node_config config = ma_hpf_node_config_init(numChannels,
																sampleRate,
																mCutoffFrequency,
																mOrder);

			const ma_result result = emplace(&engine->nodeGraph, &config, &gEngineAllocationCallbacks);

			if (!JPL_ENSURE(!result))
			{
				ma_hpf_node* node = release();
				delete node;
				return false;
			}

			return result == MA_SUCCESS;
		}
		else
		{
			return false;
		}
	}

	void HPFNode::SetCutoffFrequency(double newCutoffFrequency)
	{
		if (auto* node = get())
		{
			mCutoffFrequency = newCutoffFrequency;

			const ma_hpf_config config = ma_hpf_config_init(node->hpf.format,
															node->hpf.channels,
															node->hpf.sampleRate,
															mCutoffFrequency,
															mOrder);

			[[maybe_unused]] ma_result result = ma_hpf_node_reinit(&config, node);
			JPL_ASSERT(result == MA_SUCCESS);
		}
	}
} // namespace JPL
