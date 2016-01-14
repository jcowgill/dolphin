// Copyright 2009 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <atomic>

#include "AudioCommon/WaveFile.h"
#include "Common/CommonTypes.h"

// 16 bit Stereo
#define MAX_SAMPLES     (1024 * 4) // 128 ms
#define INDEX_MASK      (MAX_SAMPLES * 2 - 1)

#define MAX_FREQ_SHIFT  200  // per 32000 Hz
#define CONTROL_FACTOR  0.2f // in freq_shift per fifo size offset
#define CONTROL_AVG     32

class CMixer final
{
public:
	explicit CMixer(unsigned int BackendSampleRate);
	~CMixer();

	// Called from audio threads
	unsigned int Mix(short* samples, unsigned int numSamples, bool consider_framelimit = true);

	// Called from main thread
	void PushSamples(const short* samples, unsigned int num_samples);
	void PushStreamingSamples(const short* samples, unsigned int num_samples);
	void PushWiimoteSpeakerSamples(const short* samples, unsigned int num_samples, unsigned int sample_rate);
	unsigned int GetSampleRate() const { return m_sampleRate; }

	void SetDMAInputSampleRate(unsigned int rate);
	void SetStreamInputSampleRate(unsigned int rate);
	void SetStreamingVolume(unsigned int lvolume, unsigned int rvolume);
	void SetWiimoteSpeakerVolume(unsigned int lvolume, unsigned int rvolume);

	void StartLogDTKAudio(const std::string& filename);
	void StopLogDTKAudio();

	void StartLogDSPAudio(const std::string& filename);
	void StopLogDSPAudio();

	float GetCurrentSpeed() const { return m_speed.load(); }
	void UpdateSpeed(float val) { m_speed.store(val); }

private:
	class MixerFifo final
	{
	public:
		MixerFifo(CMixer* mixer, unsigned sample_rate)
			: m_mixer(mixer)
			, m_input_sample_rate(sample_rate)
		{
		}
		void PushSamples(const short* samples, unsigned int num_samples);
		unsigned int Mix(short* samples, unsigned int numSamples, bool consider_framelimit = true);
		void SetInputSampleRate(unsigned int rate);
		void SetVolume(unsigned int lvolume, unsigned int rvolume);
	private:
		CMixer* m_mixer;
		unsigned m_input_sample_rate;
		std::array<short, MAX_SAMPLES * 2> m_buffer{};
		std::atomic<u32> m_indexW{0};
		std::atomic<u32> m_indexR{0};
		// Volume ranges from 0-256
		std::atomic<s32> m_LVolume{256};
		std::atomic<s32> m_RVolume{256};
		float m_numLeftI = 0.0f;
		u32 m_frac = 0;
	};
	MixerFifo m_dma_mixer{this, 32000};
	MixerFifo m_streaming_mixer{this, 48000};
	MixerFifo m_wiimote_speaker_mixer{this, 3000};
	unsigned int m_sampleRate;

	WaveFileWriter m_wave_writer_dtk;
	WaveFileWriter m_wave_writer_dsp;

	bool m_log_dtk_audio = false;
	bool m_log_dsp_audio = false;

	// Current rate of emulation (1.0 = 100% speed)
	std::atomic<float> m_speed{0.0f};
};
