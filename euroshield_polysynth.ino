/*
 * polyphonic synth for Euroshield
 *
 * written by Nick Shaver
 * nick.shaver@gmail.com
 *
 */

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <MIDI.h>

// turn features on and off
// #define ENABLE_REVERB						// reverb is very expensive, CPU and memory wise
#define ENABLE_DETUNED_OSC			// uses an additional detuned oscillator for each oscillator

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

// button
#include <Bounce2.h>
Bounce debouncer = Bounce();
#define BUTTON_PIN 2

// LEDs
#define LEDPINCOUNT 4
int ledPins[LEDPINCOUNT] = { 3, 4, 5, 6 };

// pitch bend variables
int bendRange=12;
float bendFactor=1;

// detune variable
float detuneamt=0.998;

// potentiometer variables
int this_potvalue;
int upperPotInput=20;
int lowerPotInput=21;
int lastupperPot=0;
int lastlowerPot=0;

// arpeggiator variables
int ARPstate=0;
const char ARPOFF=0;
const char ARPRECORDING=1;
const char ARPPLAYING=2;
const char ARPSTARTPLAYING=3;
int ARPmode=0;
const char ARPSTACCATO=0;
const char ARPLEGATO=1;
const char ARPFOLLOW=2;

int ARPcurrentnote=0;
int ARPnotecount=0;
byte ARPnotes[64]={};
byte ARPvelocities[64]={};

// menu variables
int menus=4;
int menunumber=1;
int menublinkcount=0;
unsigned long menublinktime;
byte menuLEDstate=0;

// MIDI variables
int note;
int channel;
int velocity;
int lowestnote=999;																	// used for note stealing priority
int highestnote=0;																	// used for note stealing priority
float MULT127=1.000/127.000;												// used to help convert things to/from 7bit MIDI values
byte MIDILEDstate=0;
unsigned long MIDILEDofftime;

// waveform variables
int lastnote=0;
int lastvelocity=0;
int playing1,playing2,playing3,playing4;						// keeps up with the note that each oscillator is playing
int frequency1,frequency2,frequency3,frequency4;		// keeps up with the frequency that each osc is playing, for pitch bend
int lastwaveform=0;

// other misc variables
int lastclick=0;
float maxoutputvolume=0.8;
float maxamplitude=0.8;

// an attempt to get MIDI clock sent to behringer neutron
/*
int tempo=60;
unsigned long lastclicktime=0;
int clickeveryms=0;
unsigned long nextmidiclick=0;
*/

// uncomment the next line to turn on serial debugging
//#define DEBUG
#ifdef DEBUG
	#define DEBUG_PRINT(x) Serial.println(x)
#else
	#define DEBUG_PRINT(x)
#endif

// GUItool: begin automatically generated code
AudioInputI2S            i2s1;           //xy=195,187
AudioOutputI2S           i2s2;           //xy=795,174

AudioAnalyzePeak         peak1;          //xy=367,171

#ifdef ENABLE_REVERB
AudioEffectFreeverb      freeverb1;      //xy=378,232
AudioConnection          patchCord1(i2s1, 1, freeverb1, 0);
AudioConnection          patchCord2(freeverb1, 0, i2s2, 1);
#endif

AudioSynthWaveformBL       waveform1;      //xy=557,164
AudioSynthWaveformBL       waveform2;      //xy=557,164
AudioSynthWaveformBL       waveform3;      //xy=557,164
AudioSynthWaveformBL       waveform4;      //xy=557,164

#ifdef ENABLE_DETUNED_OSC
AudioSynthWaveformBL       waveform5;      //xy=557,164
AudioSynthWaveformBL       waveform6;      //xy=557,164
AudioSynthWaveformBL       waveform7;      //xy=557,164
AudioSynthWaveformBL       waveform8;      //xy=557,164
#endif

AudioMixer4							 mixer1;
AudioMixer4							 mixer2;
AudioMixer4							 mixer3;
AudioConnection          patchCord11(i2s1, 0, peak1, 0);
AudioConnection          patchCord21(waveform1, 0, mixer1, 0);
AudioConnection          patchCord22(waveform2, 0, mixer1, 1);
AudioConnection          patchCord23(waveform3, 0, mixer1, 2);
AudioConnection          patchCord24(waveform4, 0, mixer1, 3);
AudioConnection          patchCord41(mixer1, 0, mixer3, 0);

#ifdef ENABLE_DETUNED_OSC
AudioConnection          patchCord31(waveform5, 0, mixer2, 0);
AudioConnection          patchCord32(waveform6, 0, mixer2, 1);
AudioConnection          patchCord33(waveform7, 0, mixer2, 2);
AudioConnection          patchCord34(waveform8, 0, mixer2, 3);
AudioConnection          patchCord42(mixer2, 0, mixer3, 1);
#endif

AudioConnection					 patchCord43(mixer3, 0, i2s2, 0);

AudioControlSGTL5000     sgtl5000_1;     //xy=246,330
// GUItool: end automatically generated code

void setup() {
	debouncer.attach(BUTTON_PIN, INPUT_PULLUP);
	debouncer.interval(25);

  for (int i = 0; i < LEDPINCOUNT; i++) pinMode(ledPins[i], OUTPUT);

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(20);

  // Enable the audio shield, select input, and enable output
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
  sgtl5000_1.volume(maxoutputvolume);
  sgtl5000_1.adcHighPassFilterDisable();
  sgtl5000_1.lineInLevel(0,0);
  sgtl5000_1.unmuteHeadphone();
	sgtl5000_1.dacVolume(0.5, 0.9);
	/*
  sgtl5000_1.audioPostProcessorEnable();
  sgtl5000_1.autoVolumeDisable();
  sgtl5000_1.surroundSoundDisable();
  sgtl5000_1.enhanceBassEnable();
	*/

  waveform1.amplitude(0.0);
  waveform2.amplitude(0.0);
  waveform3.amplitude(0.0);
  waveform4.amplitude(0.0);
#ifdef ENABLE_DETUNED_OSC
  waveform5.amplitude(0.0);
  waveform6.amplitude(0.0);
  waveform7.amplitude(0.0);
  waveform8.amplitude(0.0);
#endif

	waveform1.begin(WAVEFORM_SAWTOOTH);
	waveform2.begin(WAVEFORM_SAWTOOTH);
	waveform3.begin(WAVEFORM_SAWTOOTH);
	waveform4.begin(WAVEFORM_SAWTOOTH);
#ifdef ENABLE_DETUNED_OSC
	waveform5.begin(WAVEFORM_SAWTOOTH);
	waveform6.begin(WAVEFORM_SAWTOOTH);
	waveform7.begin(WAVEFORM_SAWTOOTH);
	waveform8.begin(WAVEFORM_SAWTOOTH);
#endif

	mixer1.gain(0, 1.0);
	mixer1.gain(1, 1.0);
	mixer1.gain(2, 1.0);
	mixer1.gain(3, 1.0);
	mixer3.gain(0, 1.0);

#ifdef ENABLE_DETUNED_OSC
	mixer2.gain(0, 1.0);
	mixer2.gain(1, 1.0);
	mixer2.gain(2, 1.0);
	mixer2.gain(3, 1.0);
	mixer3.gain(1, 1.0);
#endif

  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleNoteOn(do_note_on);
  MIDI.setHandleNoteOff(do_note_off);
  MIDI.setHandlePitchBend(pitch_bend_handler);
	//using namespace midi;
	//MIDI.sendRealTime(Start);

	// start clock
#ifdef ENABLE_REVERB
  freeverb1.roomsize(0.3);
  freeverb1.damping(0.6);
#endif
}

void loop(){
	get_midi();
	detect_click();
	get_pot(0);
	get_pot(1);
	detect_button();
	blink_menu_LED();

	// an attempt to get MIDI clock sent to behringer neutron
	/*
	if (clickeveryms>0 && millis()>=nextmidiclick){
		// send it out over MIDI
		using namespace midi;
		MIDI.sendRealTime(Clock);
		nextmidiclick=millis()+clickeveryms;
	}
	*/
}

void get_midi(){
	if (MIDI.read()){
		//delay(20);
		if (MIDILEDstate==1 && millis()>=MIDILEDofftime){
			// time to turn the LED off
			digitalWrite(ledPins[2], LOW);
			MIDILEDstate=0;
		}
	}
}

void do_for_all_notes(){
	// things that need done anytime a new note is played or an old note is released
	if (playing1>0 && playing1<lowestnote) lowestnote=playing1;
	if (playing2>0 && playing2<lowestnote) lowestnote=playing2;
	if (playing3>0 && playing3<lowestnote) lowestnote=playing3;
	if (playing4>0 && playing4<lowestnote) lowestnote=playing4;
	if (playing1>0 && playing1>highestnote) highestnote=playing1;
	if (playing2>0 && playing2>highestnote) highestnote=playing2;
	if (playing3>0 && playing3>highestnote) highestnote=playing3;
	if (playing4>0 && playing4>highestnote) highestnote=playing4;
	if (playing1+playing2+playing3+playing4==0){
		lowestnote=999;
		highestnote=0;
	}

	if (playing1+playing2+playing3+playing4 == 0){
		// unset lastnote
		lastnote=0;
	}
}

void do_note_on(byte channel, byte note, byte velocity){
	int thischannel=int(channel);
	int thisnote=int(note);
	int thisvelocity=int(velocity);

	// keep up with last note for arpeggiator
	lastnote=note;
	lastvelocity=velocity;

	// figure out which oscillator to use
	// always use an inactive osc if available
	int osc=0;
	if (playing1==0) {
		osc=1;
	} else if (playing2==0){
		osc=2;
	} else if (playing3==0){
		osc=3;
	} else if (playing4==0){
		osc=4;
	}

	if (osc==0){
		// there were no free oscillators, don't use the lowest, don't use the highest
		if (playing1>lowestnote && playing1<highestnote){
			osc=1;
		} else if (playing2>lowestnote && playing2<highestnote){
			osc=2;
		} else if (playing3>lowestnote && playing3<highestnote){
			osc=3;
		} else {
			// oh good grief, just use osc4
			osc=4;
		}
	}

	float thisfreq = 2* 440.0 * pow(2.0, (thisnote - 69)/12.0);
	if (osc==1){
		frequency1=thisfreq;
		waveform1.frequency(thisfreq * bendFactor);
		waveform1.amplitude(maxamplitude * velocity * MULT127);
#ifdef ENABLE_DETUNED_OSC
		waveform5.frequency(thisfreq * bendFactor * detuneamt);
		waveform5.amplitude(maxamplitude * velocity * MULT127);
#endif
		playing1=thisnote;
	} else if (osc==2){
		frequency2=thisfreq;
		waveform2.frequency(thisfreq * bendFactor);
		waveform2.amplitude(maxamplitude * velocity * MULT127);
#ifdef ENABLE_DETUNED_OSC
		waveform6.frequency(thisfreq * bendFactor * detuneamt);
		waveform6.amplitude(maxamplitude * velocity * MULT127);
#endif
		playing2=thisnote;
	} else if (osc==3){
		frequency3=thisfreq;
		waveform3.frequency(thisfreq * bendFactor);
		waveform3.amplitude(maxamplitude * velocity * MULT127);
#ifdef ENABLE_DETUNED_OSC
		waveform7.frequency(thisfreq * bendFactor * detuneamt);
		waveform7.amplitude(maxamplitude * velocity * MULT127);
#endif
		playing3=thisnote;
	} else {
		frequency4=thisfreq;
		waveform4.frequency(thisfreq * bendFactor);
		waveform4.amplitude(maxamplitude * velocity * MULT127);
#ifdef ENABLE_DETUNED_OSC
		waveform8.frequency(thisfreq * bendFactor * detuneamt);
		waveform8.amplitude(maxamplitude * velocity * MULT127);
#endif
		playing4=thisnote;
	}
	DEBUG_PRINT(String("Note ") + osc + " On:  ch=" + thischannel + ", note=" + thisnote + ", velocity=" + velocity + ", high=" + highestnote + ", low=" + lowestnote);

	do_for_all_notes();

	if (MIDILEDstate==0){
		// turn on MIDI LED for a bit
  	digitalWrite(ledPins[2], HIGH);
	}

	// set time for it to turn off
	MIDILEDofftime=millis()+100;
	MIDILEDstate=1;

	if (ARPstate==ARPRECORDING && ARPnotecount<64){
		// add this note to the list of notes to store in the arpeggiator
		ARPnotes[ARPnotecount]=note;
		ARPvelocities[ARPnotecount]=velocity;
		ARPnotecount++;
	}
}

void set_oscillators(){
	if (frequency1>0){
		waveform1.frequency(frequency1 * bendFactor);
#ifdef ENABLE_DETUNED_OSC
		waveform5.frequency(frequency1 * bendFactor * detuneamt);
#endif
	}
	if (frequency2>0){
		waveform2.frequency(frequency2 * bendFactor);
#ifdef ENABLE_DETUNED_OSC
		waveform6.frequency(frequency2 * bendFactor * detuneamt);
#endif
	}
	if (frequency3>0){
		waveform3.frequency(frequency3 * bendFactor);
#ifdef ENABLE_DETUNED_OSC
		waveform7.frequency(frequency3 * bendFactor * detuneamt);
#endif
	}
	if (frequency4>0){
		waveform4.frequency(frequency4 * bendFactor);
#ifdef ENABLE_DETUNED_OSC
		waveform8.frequency(frequency4 * bendFactor * detuneamt);
#endif
	}
}

void pitch_bend_handler(byte channel, int bend){
  float bendF = bend;
  bendF = bendF / 8192;
  bendF = bendF * bendRange;
  bendF = bendF / 12;
  bendFactor = pow(2, bendF);

	set_oscillators();
}

void do_note_off(byte channel, byte note, byte velocity){
	/* uncomment if you need channel or velocity
	int thischannel=int(channel);
	int thisvelocity=int(velocity);
	*/
	int thisnote=int(note);

	if (playing1==thisnote){
		DEBUG_PRINT(String("Note 1 Off: note=") + thisnote);
		waveform1.amplitude(0.0);
#ifdef ENABLE_DETUNED_OSC
		waveform5.amplitude(0.0);
#endif
		playing1=0;
		frequency1=0;
	}
	if (playing2==thisnote){
		DEBUG_PRINT(String("Note 2 Off: note=") + thisnote);
		waveform2.amplitude(0.0);
#ifdef ENABLE_DETUNED_OSC
		waveform6.amplitude(0.0);
#endif
		playing2=0;
		frequency2=0;
	}
	if (playing3==thisnote){
		DEBUG_PRINT(String("Note 3 Off: note=") + thisnote);
		waveform3.amplitude(0.0);
#ifdef ENABLE_DETUNED_OSC
		waveform7.amplitude(0.0);
#endif
		playing3=0;
		frequency3=0;
	}
	if (playing4==thisnote){
		DEBUG_PRINT(String("Note 4 Off: note=") + thisnote);
		waveform4.amplitude(0.0);
#ifdef ENABLE_DETUNED_OSC
		waveform8.amplitude(0.0);
#endif
		playing4=0;
		frequency4=0;
	}
	do_for_all_notes();
}

void set_volume(int potvalue){
	// volume control
	//sgtl5000_1.volume(float(potvalue)/1024*maxoutputvolume);
	sgtl5000_1.dacVolume(float(potvalue)/1024, 0.9);
}

void set_waveform(int potvalue){
	// select waveform
	int thiswaveform=int((64+potvalue)/128);
	if (thiswaveform != lastwaveform){
		DEBUG_PRINT(String("setting waveform: ") + thiswaveform + " (raw value=" + potvalue + ")");
		AudioNoInterrupts();
		if (thiswaveform==0){
			waveform1.begin(WAVEFORM_SINE);
			waveform2.begin(WAVEFORM_SINE);
			waveform3.begin(WAVEFORM_SINE);
			waveform4.begin(WAVEFORM_SINE);
#ifdef ENABLE_DETUNED_OSC
			waveform5.begin(WAVEFORM_SINE);
			waveform6.begin(WAVEFORM_SINE);
			waveform7.begin(WAVEFORM_SINE);
			waveform8.begin(WAVEFORM_SINE);
#endif
		} else if (thiswaveform==1){
			waveform1.begin(WAVEFORM_SAWTOOTH);
			waveform2.begin(WAVEFORM_SAWTOOTH);
			waveform3.begin(WAVEFORM_SAWTOOTH);
			waveform4.begin(WAVEFORM_SAWTOOTH);
#ifdef ENABLE_DETUNED_OSC
			waveform5.begin(WAVEFORM_SAWTOOTH);
			waveform6.begin(WAVEFORM_SAWTOOTH);
			waveform7.begin(WAVEFORM_SAWTOOTH);
			waveform8.begin(WAVEFORM_SAWTOOTH);
#endif
		} else if (thiswaveform==2){
			waveform1.begin(WAVEFORM_SAWTOOTH_REVERSE);
			waveform2.begin(WAVEFORM_SAWTOOTH_REVERSE);
			waveform3.begin(WAVEFORM_SAWTOOTH_REVERSE);
			waveform4.begin(WAVEFORM_SAWTOOTH_REVERSE);
#ifdef ENABLE_DETUNED_OSC
			waveform5.begin(WAVEFORM_SAWTOOTH_REVERSE);
			waveform6.begin(WAVEFORM_SAWTOOTH_REVERSE);
			waveform7.begin(WAVEFORM_SAWTOOTH_REVERSE);
			waveform8.begin(WAVEFORM_SAWTOOTH_REVERSE);
#endif
		} else if (thiswaveform==3){
			waveform1.begin(WAVEFORM_SQUARE);
			waveform2.begin(WAVEFORM_SQUARE);
			waveform3.begin(WAVEFORM_SQUARE);
			waveform4.begin(WAVEFORM_SQUARE);
#ifdef ENABLE_DETUNED_OSC
			waveform5.begin(WAVEFORM_SQUARE);
			waveform6.begin(WAVEFORM_SQUARE);
			waveform7.begin(WAVEFORM_SQUARE);
			waveform8.begin(WAVEFORM_SQUARE);
#endif

			waveform1.pulseWidth(0.5);
			waveform2.pulseWidth(0.5);
			waveform3.pulseWidth(0.5);
			waveform4.pulseWidth(0.5);
#ifdef ENABLE_DETUNED_OSC
			waveform5.pulseWidth(0.5);
			waveform6.pulseWidth(0.5);
			waveform7.pulseWidth(0.5);
			waveform8.pulseWidth(0.5);
#endif
		} else if (thiswaveform==4){
			waveform1.begin(WAVEFORM_TRIANGLE);
			waveform2.begin(WAVEFORM_TRIANGLE);
			waveform3.begin(WAVEFORM_TRIANGLE);
			waveform4.begin(WAVEFORM_TRIANGLE);
#ifdef ENABLE_DETUNED_OSC
			waveform5.begin(WAVEFORM_TRIANGLE);
			waveform6.begin(WAVEFORM_TRIANGLE);
			waveform7.begin(WAVEFORM_TRIANGLE);
			waveform8.begin(WAVEFORM_TRIANGLE);
#endif
		} else if (thiswaveform==5){
			waveform1.begin(WAVEFORM_TRIANGLE_VARIABLE);
			waveform2.begin(WAVEFORM_TRIANGLE_VARIABLE);
			waveform3.begin(WAVEFORM_TRIANGLE_VARIABLE);
			waveform4.begin(WAVEFORM_TRIANGLE_VARIABLE);
#ifdef ENABLE_DETUNED_OSC
			waveform5.begin(WAVEFORM_TRIANGLE_VARIABLE);
			waveform6.begin(WAVEFORM_TRIANGLE_VARIABLE);
			waveform7.begin(WAVEFORM_TRIANGLE_VARIABLE);
			waveform8.begin(WAVEFORM_TRIANGLE_VARIABLE);
#endif
		} else if (thiswaveform==6){
			waveform1.begin(WAVEFORM_ARBITRARY);
			waveform2.begin(WAVEFORM_ARBITRARY);
			waveform3.begin(WAVEFORM_ARBITRARY);
			waveform4.begin(WAVEFORM_ARBITRARY);
#ifdef ENABLE_DETUNED_OSC
			waveform5.begin(WAVEFORM_ARBITRARY);
			waveform6.begin(WAVEFORM_ARBITRARY);
			waveform7.begin(WAVEFORM_ARBITRARY);
			waveform8.begin(WAVEFORM_ARBITRARY);
#endif
		} else if (thiswaveform==7){
			waveform1.begin(WAVEFORM_PULSE);
			waveform2.begin(WAVEFORM_PULSE);
			waveform3.begin(WAVEFORM_PULSE);
			waveform4.begin(WAVEFORM_PULSE);
#ifdef ENABLE_DETUNED_OSC
			waveform5.begin(WAVEFORM_PULSE);
			waveform6.begin(WAVEFORM_PULSE);
			waveform7.begin(WAVEFORM_PULSE);
			waveform8.begin(WAVEFORM_PULSE);
#endif
		} else {
			waveform1.begin(WAVEFORM_SAMPLE_HOLD);
			waveform2.begin(WAVEFORM_SAMPLE_HOLD);
			waveform3.begin(WAVEFORM_SAMPLE_HOLD);
			waveform4.begin(WAVEFORM_SAMPLE_HOLD);
#ifdef ENABLE_DETUNED_OSC
			waveform5.begin(WAVEFORM_SAMPLE_HOLD);
			waveform6.begin(WAVEFORM_SAMPLE_HOLD);
			waveform7.begin(WAVEFORM_SAMPLE_HOLD);
			waveform8.begin(WAVEFORM_SAMPLE_HOLD);
#endif
		}
		AudioInterrupts();
		lastwaveform=thiswaveform;
	}
}

void set_detune(int potvalue){
	// adjust detune
	detuneamt=1.0 - (float(potvalue)/1024);
	set_oscillators();
}

void set_pulsewidth(int potvalue){
	// adjust pulsewidth
	float thispw=1.00 * (potvalue+1)/1024;
	waveform1.pulseWidth(thispw);
	waveform2.pulseWidth(thispw);
	waveform3.pulseWidth(thispw);
	waveform4.pulseWidth(thispw);
#ifdef ENABLE_DETUNED_OSC
	waveform5.pulseWidth(thispw);
	waveform6.pulseWidth(thispw);
	waveform7.pulseWidth(thispw);
	waveform8.pulseWidth(thispw);
#endif
}

void blink_menu_LED(){
	// blink menu LED
	if (millis()>menublinktime){
		// time to do something with the menu LED
		if (menuLEDstate==1){
			// currently on, need to turn off
  		digitalWrite(ledPins[3], LOW);
			menuLEDstate=0;

			if (menublinkcount>=menunumber){
				// just finished blinking menunumber times, wait a sec
				menublinktime=millis()+1000;
				menublinkcount=1;
			} else {
				// working up to menunumber times, only turn off for an instant
				menublinktime=millis()+200;
				menublinkcount++;
			}
		} else {
			// currently off, need to turn on
  		digitalWrite(ledPins[3], HIGH);
			menublinktime=millis()+200;
			menuLEDstate=1;
		}
	}

	// set time for it to turn off
	MIDILEDofftime=millis()+100;
	MIDILEDstate=1;
}

void detect_click(){
  if (peak1.available()) {
    float peakValue1 = peak1.read();
		if (peakValue1>0.5){
			// click is high
			if (lastclick==0){
				// just went high
				//DEBUG_PRINT(String("click tick on, input1 freq: ") + peakValue1);
				// play arp?
				int ARPnote_to_play=0;
				int ARPvelocity_to_play=0;
				int ARPnote_to_stop=0;
				if (ARPstate==ARPPLAYING || ARPstate==ARPSTARTPLAYING){
					if (ARPcurrentnote >= ARPnotecount){
						ARPcurrentnote=0;
					}

					// play note
					DEBUG_PRINT(String(" - playing arp note ") + ARPcurrentnote);
					if (ARPmode==ARPFOLLOW){
						if (lastnote>0){
							int ARPoffset=lastnote - ARPnotes[0];
							ARPnote_to_play=ARPnotes[ARPcurrentnote] + ARPoffset;
							ARPvelocity_to_play=lastvelocity;
						}
					} else {
						ARPnote_to_play=ARPnotes[ARPcurrentnote];
						ARPvelocity_to_play=ARPvelocities[ARPcurrentnote];
					}
					MIDI.sendNoteOn(ARPnote_to_play, ARPvelocity_to_play, 1);
				}

				if (ARPstate==ARPPLAYING || ARPstate==ARPSTACCATO || ARPstate==ARPFOLLOW){
					if (ARPstate==ARPPLAYING && ARPmode==ARPFOLLOW){
						ARPnote_to_stop=ARPnote_to_play;
					} else if (ARPmode==ARPSTACCATO){
						// stop note immediately after starting
						ARPnote_to_stop=ARPnotes[ARPcurrentnote];
					} else {
						// stop previous note, but don't do this on first note of ARP
						if (ARPcurrentnote<=0){
							ARPnote_to_stop=ARPnotes[ARPnotecount-1];
						} else {
							ARPnote_to_stop=ARPnotes[ARPcurrentnote-1];
						}
					}
					MIDI.sendNoteOff(ARPnote_to_stop, 0, 1);
					// move to next note
					ARPcurrentnote++;
				}

				if (ARPstate==ARPSTARTPLAYING){
					ARPstate=ARPPLAYING;
				}

				// an attempt to get MIDI clock sent to behringer neutron
				/*
				unsigned long tempoelapsedmillis=millis()-lastclicktime;
				clickeveryms=int(tempoelapsedmillis/12);
				if (clickeveryms>100 && clickeveryms<3000){
					// ok
				} else{
					clickeveryms=0;
				}
				*/

				// send MIDI CC1 high (127)
				//MIDI.sendControlChange(1, 127, 1);

				// send it out over MIDI
				//using namespace midi;
				//MIDI.sendRealTime(Clock);

				lastclick=1;

				// blink LED
  			digitalWrite(ledPins[0], HIGH);
			}
		} else if (lastclick==1){
			// click just went low
			//DEBUG_PRINT(String("click tick off, input1 freq: ") + peakValue1);

			// stop arp?
			/*
			if (ARPstate==ARPPLAYING){
				// stop note and move to next
				MIDI.sendNoteOff(ARPnotes[ARPcurrentnote], ARPvelocities[ARPcurrentnote], 1);
				// move to next note
				ARPcurrentnote++;
				if (ARPcurrentnote >= ARPnotecount){
					ARPcurrentnote=0;
				}
			}
			*/

			lastclick=0;

			// send MIDI CC1 low (0)
			//MIDI.sendControlChange(1, 0, 1);

			// unblink LED
			digitalWrite(ledPins[0], LOW);

			// arp note off
		}
  }
}

void stop_all_notes(){
	// stop over midi
	if (playing1>0) MIDI.sendNoteOff(playing1, 0, 1);
	if (playing2>0) MIDI.sendNoteOff(playing2, 0, 1);
	if (playing3>0) MIDI.sendNoteOff(playing3, 0, 1);
	if (playing4>0) MIDI.sendNoteOff(playing4, 0, 1);

	// stop on teensy oscillators
	playing1=0;
	playing2=0;
	playing3=0;
	playing4=0;
	frequency1=0;
	frequency2=0;
	frequency3=0;
	frequency4=0;
	waveform1.amplitude(0.0);
	waveform2.amplitude(0.0);
	waveform3.amplitude(0.0);
	waveform4.amplitude(0.0);
#ifdef ENABLE_DETUNED_OSC
	waveform5.amplitude(0.0);
	waveform6.amplitude(0.0);
	waveform7.amplitude(0.0);
	waveform8.amplitude(0.0);
#endif
}

void set_arp_mode(int potvalue){
	if (potvalue>=800 && ARPmode!=ARPLEGATO) {
		ARPmode=ARPLEGATO;
		stop_all_notes();
	} else if (potvalue>=400 && potvalue<800 && ARPmode!=ARPSTACCATO) {
		ARPmode=ARPSTACCATO;
	} else if (potvalue<400 && ARPmode!=ARPFOLLOW) {
		ARPmode=ARPFOLLOW;

		// be nice if I could turn thru off, but this doesn't seem to cut it on Euroshield DIN MIDI
		//MIDI.setThruFilterMode(1);
		// nor does this
		//MIDI.turnThruOff();
	} else {
	}
}

void set_arp_state(int potvalue){
	if (potvalue>=800 && ARPstate==ARPOFF){
		// just went into record state
		ARPnotecount=0;
		ARPstate=ARPRECORDING;
		// turn on ARP LED
		digitalWrite(ledPins[2], HIGH);
		DEBUG_PRINT(String("entered ARPRECORDING state"));
	} else if (potvalue>400 && potvalue<800 && ARPstate==ARPRECORDING){
		// just left record state
		if (ARPnotecount>0){
			DEBUG_PRINT(String("entered ARPPLAYING state"));
			ARPstate=ARPPLAYING;
			ARPcurrentnote=0;
			// turn off ARP LED
			digitalWrite(ledPins[2], LOW);
		}
	} else if (potvalue<=400 && ARPstate!=ARPOFF){
		DEBUG_PRINT(String("entered ARPOFF state"));
		ARPstate=ARPOFF;
		stop_all_notes();
	}
}

void get_pot(int whichpot){
	switch (whichpot){
		case 0:
			// make adjustments from upper pot
			this_potvalue = analogRead(upperPotInput);
			if (this_potvalue != lastupperPot){
				switch (menunumber){
					case 1:
						set_volume(this_potvalue);
						break;
					case 2:
						set_waveform(this_potvalue);
						break;
					case 3:
						break;
					case 4:
						set_arp_state(this_potvalue);
						break;
				}

				lastupperPot=this_potvalue;
			}
			break;
		case 1:
			// make adjustments from lower pot
			this_potvalue = analogRead(lowerPotInput);
			if (this_potvalue != lastlowerPot){
				switch (menunumber){
					case 1:
						set_detune(this_potvalue);
						break;
					case 2:
						set_pulsewidth(this_potvalue);
						break;
					case 3:
						break;
					case 4:
						set_arp_mode(this_potvalue);
						break;
				}
				lastlowerPot=this_potvalue;
			}
			break;
	}
}

void detect_button(){
	// detect button pushes
	debouncer.update();
	if (debouncer.fell()){
		// advance menunumber. the menunumber specifies what the upper and lower pots do.
		menunumber++;
		if (menunumber>menus) menunumber=1;
	}
}
