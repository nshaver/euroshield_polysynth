/*
 * synth_waveformbl.h
 *
 * Created by Nick Shaver - 2019-08-04
 * 
*/

#ifndef synth_waveformbl_h_
#define synth_waveformbl_h_

#include "AudioStream.h"

#define WAVEFORM_SINE              0
#define WAVEFORM_SAWTOOTH          1
#define WAVEFORM_SQUARE            2
#define WAVEFORM_TRIANGLE          3
#define WAVEFORM_ARBITRARY         4
#define WAVEFORM_PULSE             5
#define WAVEFORM_SAWTOOTH_REVERSE  6
#define WAVEFORM_SAMPLE_HOLD       7
#define WAVEFORM_TRIANGLE_VARIABLE 8

class AudioSynthWaveformBL : public AudioStream {
    
public:
	AudioSynthWaveformBL() : AudioStream(0, NULL){}

	void begin(short t_type) {
		freq=200;
		amp=0;
		pw=0;
		tone_type = t_type;
	}

	void begin(float _freq, float _amp, float _pw);

	virtual void update(void);
	float getSample();
	float getFreq();
	void frequency(float _freq);
	void amplitude (float _amp);
	void pulsewidth (float _pw);
    
private:
	float sample;
	int 	sampleRate;
	float phase;
	float phaseInc;
	float freq;
	float currentSample;
	float amp;
	float pw;
	short tone_type;
};


#endif /* AudioSynthWaveformBL */
