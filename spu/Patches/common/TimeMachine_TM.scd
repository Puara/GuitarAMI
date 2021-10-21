/*

%%%%%%                                            %%%%%%
%%%%%%%%%%%                                  %%%%%%%%%%%
%%%%%%%%%%%%%%%%%   TIME MACHINE FFT   %%%%%%%%%%%%%%%%%
%%%%%%%%%%%                                  %%%%%%%%%%%
%%%%%%                                            %%%%%%

Edu Meneses - 2016-2020
IDMIL - McGill University

*/

~audio_output = ~guitarami_output;
~audio_input = ~guitarami_input;

~windowsize = 1024; // the frame size for the analysis - experiment with other sizes (powers of 2)
~hopsize = 0.25; // the hop size (overlapping)

// Allocate a Buffer to store FFT data (60 seconds, 1 channel)
~dur = 60.0; // Buffer duration
~buffer = Buffer.alloc(s, ~dur.calcPVRecSize(~windowsize, ~hopsize, s.sampleRate), 1);
~readpointer = Bus.control(s, 1);
~distance = Bus.control(s, 1);

SynthDef(\tm_rec, { // this does the analysis and saves it to buffer
	arg window=1, ws=1024, hs=0.25, input=0, speed=1, buffer, bufferdur, reset=0;
	var in, chainA, rate, writepointer, readpointer, distance;
	in = SoundIn.ar(input);
	rate = Select.kr( ((~distance.kr < 0.001) * (speed > 1)) + (((~distance.kr > 0.999) * (speed < 1)) * 2),[
		speed, // normal operation (trig=0)
		0.99,  // too close to min distance (trig=1)
		1.01   // too close to max distance (trig=2)
	]);
	chainA = FFT(LocalBuf(ws), in, hs, window);
	chainA = PV_RecordBuf(chainA, buffer, 0, 1, 1, hs, window); // save the analysis to buffer
	writepointer = Phasor.kr(0, ControlDur.ir/bufferdur, 0, 1);
	readpointer = Phasor.kr(reset, rate*ControlDur.ir/bufferdur, 0, 1, writepointer);
	distance = Select.kr(writepointer >= readpointer, [ // to compare distance between RT audio and playback
		((1-readpointer)+writepointer),
		(writepointer-readpointer)
	]);
	Out.kr(~distance, distance);
	Out.kr(~readpointer, readpointer);
}).add;

SynthDef(\tm_play, {
	arg window=1, ws=1024, hs=0.25, buffer, readpointer, pointer, input=0, output=0, gatebp=1, gatefreeze=1, gatefft=1, freeze=0, onsets_treshold=0.7, pitch=1.0;
	var bypass, gatetm, fftout, tmout, freezeout, chainA, chainB, onsets, click;
	var harmonic, percussive, freq, hasFreq;

	bypass = SoundIn.ar(input) * EnvGen.kr(Env.asr(0.5,1,1), gatebp);
	# freq, hasFreq = Pitch.kr(bypass, ampThreshold: 0.02, median: 7);
	chainB = FFT(LocalBuf(ws), bypass, hs, window);
	harmonic = PV_Copy(chainB, LocalBuf(ws));
	percussive = PV_Copy(chainB, LocalBuf(ws));
	#harmonic,percussive = MedianSeparation(chainB,harmonic,percussive,ws,17); // change to 17 (orig. 27) if too CPU intensive
	onsets = Onsets.kr(chainB, onsets_treshold, \rcomplex);
	harmonic = PV_Freeze(harmonic, freeze);
	freezeout = IFFT.ar(harmonic, window) * EnvGen.kr(Env.asr(0.5,1,1), freeze);
	gatetm = Select.kr((~distance.kr > 0.001), [0, 1]);
	pointer = readpointer - (ControlDur.ir * hs); // offset the read so we can FFT in time
	chainA = PV_BufRd(LocalBuf(ws), buffer, pointer);
	tmout = IFFT.ar(chainA, window);
	fftout = PitchShift.ar(tmout, 0.1, pitch, 0, 0.004) * EnvGen.kr(Env.asr(0.5,1,1), gatefft*gatetm);
	click = Pan2.ar(LPF.ar(WhiteNoise.ar * Decay.kr(onsets, 0.05), 1000), 0.0); // also add to Out.ar if wanted
	Out.kr(~sensors[\onsets], onsets);
	Out.kr(~sensors[\freq], freq);
	Out.ar( output, Mix([bypass, fftout, freezeout]) ); // uncomment line and add click if wanted
}).add;

s.sync;

// ............................ STARTING SYNTHS ............................

~timemachine = Group.new;

~tm_rec = Synth.new(\tm_rec,
	[
		ws: ~windowsize,
		hs: ~hopsize,
		input: ~audio_input,
		buffer: ~buffer,
		bufferdur: ~dur
	],
	~timemachine);

s.sync;

~tm_play = Synth.new(\tm_play,
	[
		ws: ~windowsize,
		hs: ~hopsize,
		input: ~audio_input,
		output: ~audio_output,
		buffer: ~buffer,
		readpointer: ~readpointer.asMap
	],
	~timemachine);

s.sync;

/*

// ............................ COMMANDS TO MAP............................

~timemachine.set(
	\speed, ,
	\freeze, ,
	\pitch, ,
	\reset, ,
	\gatebp, ,
	\gatefreeze, ,
	\gatefft,
);

*/

