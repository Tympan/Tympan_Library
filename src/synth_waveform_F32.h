/*
 * AudioSynthWaveform_F32
 * 
 * Created: Patrick Radius, January 2017
 * Purpose: Generate waveforms at a given frequency and amplitude. Allows for pitch-modulation and portamento.
 *          
 * Default modulation is that input shifts the frequency per octave.
 * Alternative mode is to switch it so that it so that the input shifts the frequency per Hz (added Chip Audette, OpenAudio, Sept 2021)
 *
 * This processes a single stream fo audio data (ie, it is mono)       
 *          
 * MIT License.  use at your own risk.
*/
#ifndef SYNTHWAVEFORMF32_H
#define SYNTHWAVEFORMF32_H

#include <arm_math.h>
#include "AudioStream_F32.h"
#include "AudioConvert_F32.h" //for convert_i16_to_f32

class AudioSynthWaveform_F32 : public AudioStream_F32
{
  //GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node
  public:
    enum OscillatorMode {
        OSCILLATOR_MODE_SINE = 0,
        OSCILLATOR_MODE_SAW,
        OSCILLATOR_MODE_SQUARE,
        OSCILLATOR_MODE_TRIANGLE
    };
	enum ModMode {
		MOD_MODE_PER_OCT=0,
		MOD_MODE_PER_HZ
	};

	AudioSynthWaveform_F32(const AudioSettings_F32 &settings) : AudioStream_F32(1, inputQueueArray_f32), 
			_PI(2*acos(0.0f)),
			twoPI(2 * _PI),
			sample_rate_Hz(AUDIO_SAMPLE_RATE_EXACT),
			audio_block_samples(AUDIO_BLOCK_SAMPLES),
			_OscillatorMode(OSCILLATOR_MODE_SINE),
			_ModMode(MOD_MODE_PER_OCT),
			_Frequency(440.0f),
			_Phase(0.0f),
			_PhaseIncrement(0.0f),
			_PitchModAmt(0.0f),
			_PortamentoIncrement(0.0f),
			_PortamentoSamples(0),
			_CurrentPortamentoSample(0),
			_NotesPlaying(0)
	{		
		setSampleRate(settings.sample_rate_Hz);
		setAudioBlockSamples(settings.audio_block_samples);
	}
		
                
	
    AudioSynthWaveform_F32(void) : AudioStream_F32(1, inputQueueArray_f32),  //uses default AUDIO_SAMPLE_RATE and AUDIO_BLOCK_SAMPLES from AudioStream.h
                _PI(2*acos(0.0f)),
                twoPI(2 * _PI),
				sample_rate_Hz(AUDIO_SAMPLE_RATE_EXACT),
				audio_block_samples(AUDIO_BLOCK_SAMPLES),
                _OscillatorMode(OSCILLATOR_MODE_SINE),
                _Frequency(440.0f),
                _Phase(0.0f),
                _PhaseIncrement(0.0f),
                _PitchModAmt(0.0f),
                _PortamentoIncrement(0.0f),
                _PortamentoSamples(0),
                _CurrentPortamentoSample(0),
                _NotesPlaying(0) {};

    void frequency(float32_t freq) {
        float32_t nyquist = sample_rate_Hz/2.f;

        if (freq < 0.0) {
			freq = 0.0f;
        } else if (freq > nyquist) {
			freq = nyquist;
		}
		
        if (_PortamentoSamples > 0 && _NotesPlaying > 0) {
          _PortamentoIncrement = (freq - _Frequency) / (float32_t)_PortamentoSamples;
          _CurrentPortamentoSample = 0;
        } else {
          _Frequency = freq;
        }

        _PhaseIncrement = _Frequency * twoPI / sample_rate_Hz;
    }

    void amplitude(float32_t n) {
        if (n < 0.0f) n = 0.0f;
        _magnitude = n;
    }

    void begin(short t_type) {
        _Phase = 0.0f;
        oscillatorMode(t_type);
    }

    void begin(float32_t t_amp, float32_t t_freq, short t_type) {
        amplitude(t_amp);
        frequency(t_freq);
        begin(t_type);
    }

    void pitchModAmount(float32_t amount) {
      _PitchModAmt = amount;
    }

    void oscillatorMode(int mode) {
      _OscillatorMode = (OscillatorMode)mode;
    }
	void modMode(int mode) {
	  _ModMode = (ModMode)mode;
	  if (_ModMode == MOD_MODE_PER_HZ) _PitchModAmt = 1.0;
	}

    void portamentoTime(float32_t slidetime) {
      _PortamentoTime = slidetime;
      _PortamentoSamples = floorf(slidetime * sample_rate_Hz);
    }

    //is this needed?
    void onNoteOn() {
      _NotesPlaying++;
    }

	//is this needed?
    void onNoteOff() {
      if (_NotesPlaying > 0) {
        _NotesPlaying--;
      }
    }

    void update(void);
	void setSampleRate(const float32_t fs_Hz)
	{
		_PhaseIncrement = _PhaseIncrement*sample_rate_Hz / fs_Hz;
		_PortamentoSamples = floorf( ((float)_PortamentoSamples) * fs_Hz / sample_rate_Hz );
		sample_rate_Hz = fs_Hz;
	}
	void setAudioBlockSamples(const int _audio_block_samples) {
		audio_block_samples = _audio_block_samples;
	}
	float getFrequency_Hz(void) { return _Frequency; }
	float setFrequency_Hz(float _freq_Hz) { frequency(_freq_Hz); return getFrequency_Hz(); }
	float getAmplitude(void) { return _magnitude; }
	float setAmplitude(float amp) { amplitude(amp); return getAmplitude(); }
	
	
  private:
    inline float32_t applyMod(uint32_t sample, audio_block_f32_t *lfo);
    const float32_t _PI;
    float32_t twoPI;
	float32_t sample_rate_Hz;
	int audio_block_samples=AUDIO_BLOCK_SAMPLES;
    
    OscillatorMode _OscillatorMode;
	ModMode _ModMode;
    float32_t _Frequency;
    float32_t _Phase;
    float32_t _PhaseIncrement;
    float32_t _magnitude;
    float32_t _PitchModAmt;
    float32_t _PortamentoTime;
    float32_t _PortamentoIncrement;

    uint64_t _PortamentoSamples;
    uint64_t _CurrentPortamentoSample;
    uint8_t _NotesPlaying;

    audio_block_f32_t *inputQueueArray_f32[1];
	unsigned int block_counter=0;
};

#endif