# euroshield_polysynth
Polyphonic synthesizer for 1010 Euroshield + Teensy

Requires a 1010 Euroshield and a Teensy>=3.2. I have tested it with 3.2 and 3.5. 
If running on a Teensy3.2 then I'd definitely recommend disabling the reverb by commenting out this line (put two slashes in front of it):

#define ENABLE_REVERB

Also, on Teensy3.2, you might want to deactivate the additional detuned oscillators by commenting out this line:

#define ENABLE_DETUNED_OSC

The Euroshield button cycles through four modes of operation. The top LED blinks corresponding to the current mode.

Mode 1 = upper pot = output volume, lower pot = OSC detune amount
Mode 2 = upper pot = waveform select, lower pot = pulsewidth (when using saw wave)
Mode 3 = (not currently doing anything)
Mode 4 = upper pot = arpeggiator state. left=stopped, middle=playing, right=recording
         lower pot = arpeggiator mode. left=follow current not, middle=staccato, right=legato

There are only two waveforms implemented. The stock Teensy waveforms alias pretty bad above the nyquist frequency so I'm using a new waveform class - AudioSynthWaveformBL (BL = band limited). I coded this class to mimic the stock Teensy class AUdioSynthWaveform, it should be a drop-in replacement except it ignores the selection of waveforms other than sawtooth and square. I haven't rounded up code for band limited versions of the other waveforms (triangle, etc...).

Credit goes here for the band-limited sawtooth: https://github.com/silveirago/teensyATK
I took their work and modified it a little to be more like the stock Teensy waveform class.
I attempted to create a square wave that does a little sloping at the edges in an attempt to reduce its aliasing. I don't really know if I did any good at that or not. Nevertheless, it works.
