/*
GuitarAMI-TStick patch SC version 0.96 -- Edu Meneses
Based on Time Machine patch (Pure Data - Edu Meneses, Winter-2017)
Run on GuitarAMI Processing Unit (prototype5), made with PiSound + Rpi 4B
*/

//s = Server.local;
s.waitForBoot{

//OSCdef.freeAll;

1.wait;


// ........................... Global defs ............................

	~id="XXX";

	~midi = False;
    ~oscForward = False;
	~oscForwardIP= "174.172.24.121"; // replace X by last IP address digits
	~oscForwardPort = 8000; // has to be an int
	~guitarami_input = 0;
	~guitarami_output = 0;
	~osc_patch_module = "/GuitarAMI_module_" ++ ~id;
	~osc_patch_spu = "/guitarami_spu";

	~lcd = NetAddr.new("127.0.0.1", 20000); // Setting LCD - Loopback NetAddr and Python code listening port
	~client = NetAddr.new("172.24.1.121", 8000); // Setting LCD - Loopback NetAddr and Python code listening port
	thisProcess.openUDPPort(8000);



// ......................... Global variables .........................

	~ult = 0;
	~acelerometro = 0;
	~windowsize = 1024; // the frame size for the analysis - experiment with other sizes (powers of 2)
	~hopsize = 0.25; // the hop size (overlapping)
	~dur = 60.0; // Buffer duration
	~buffer = Buffer.alloc(s, ~dur.calcPVRecSize(~windowsize, ~hopsize, s.sampleRate), 1); // Allocate a Buffer to store FFT data (60 seconds, 1 channel)
	~buffer_numFrames = ~buffer.numFrames;

	s.sync;

	~lcd.sendMsg('/lcd', "GuitarAMI:      0.96", 1, 1);
	~lcd.sendMsg('/lcd', "Loading Time-Machine", 2, 1);
	~lcd.sendMsg('/lcd', "[===               ]", 3, 1);

	(thisProcess.nowExecutingPath.dirname +/+ "common/GuitarAMI_OSC.scd").load;


// ............................ SYNTH DEFS ............................

/*
%%%%%%                                  %%%%%%
%%%%%%%%%%%                        %%%%%%%%%%%
%%%%%%%%%%%%%%%%%   BYPASS   %%%%%%%%%%%%%%%%%
%%%%%%%%%%%                        %%%%%%%%%%%
%%%%%%                                  %%%%%%
*/

	SynthDef(\bypass, {
		arg volume=1, input=0, output=0, gate=1;
		var sound;
		sound = SoundIn.ar(input, EnvGen.kr(Env.asr(0.5,1,1), gate) * volume);
		Out.ar(output, sound);
	}).add;

	~lcd.sendMsg('/lcd', "=", 3, 5);

/*
%%%%%%                                            %%%%%%
%%%%%%%%%%%                                  %%%%%%%%%%%
%%%%%%%%%%%%%%%%%   TIME MACHINE FFT   %%%%%%%%%%%%%%%%%
%%%%%%%%%%%                                  %%%%%%%%%%%
%%%%%%                                            %%%%%%
*/

	SynthDef(\tm_rec, { // this does the analysis and saves it to buffer
		arg window=1, ws=1024, hs=0.25, input=0, buffer=0, frames=0, rate=1, cookedrate=1, trig=0, bufferdur=1;
		var writestorage, in, chain, writepointer, readpointer, diferenca;
		writestorage = LocalBuf.new(ws);
		in = SoundIn.ar(input);
		// note the window type and overlaps... this is important for resynth parameters
		chain = FFT(writestorage, in, hs, window);
		chain = PV_RecordBuf(chain, buffer, 0, 1, 1, hs, window); // save the analysis to buffer
		writepointer = Phasor.kr(0, ControlDur.ir, 0, bufferdur);
		readpointer = Phasor.kr(0, cookedrate*ControlDur.ir, 0, bufferdur);
		diferenca = Select.kr(writepointer >= readpointer, [ // to compare distance between RT audio and playback
				((bufferdur-readpointer)+writepointer),
				(writepointer-readpointer)
		]);
		SendReply.kr(Impulse.kr(500),'/writepointer', [rate, diferenca]); // make rate and pointer's distance available in sclang
	}).add;

	SynthDef(\tm_play, { // play your analysis back...
		arg window=1, ws=1024, hs=0.25, output=0, buffer=0, cookedrate=1, offset=0, gate=0;
		var readstorage, chain;
		readstorage = LocalBuf.new(ws);
		chain = PV_PlayBuf(readstorage, buffer, cookedrate, offset, loop: 1);
		Out.ar(output, (IFFT(chain, window).dup)*EnvGen.kr(Env.asr(0.5,1,1), gate));
	}).add;

	OSCdef(\writepointer, { // receive rate and pointer's distance in sclang, returns 'cooked' rate to \tm_play
		arg msg; var rate, distance, cookedrate, low_threshold, mid_threshold, high_threshold;
		// using these thresholds one can use rates between -100 and 5
		low_threshold = 0.01; // in seconds
		mid_threshold = low_threshold * 5;
		high_threshold = 2.0;
		rate = msg[3].min(4); // setting max rate to 4 just in case...
		distance = msg[4];
		cookedrate = case
		{ (distance>mid_threshold) && (distance<(~dur-high_threshold)) }   { cookedrate = rate }
		{ (distance>low_threshold) && (distance<mid_threshold) }   { cookedrate = rate.min(1.2) }
		{ distance<low_threshold } { cookedrate = 0.99 }
		{ (distance>(~dur-high_threshold)) } { cookedrate = 1.1 };
		~timemachine.set(\cookedrate, cookedrate);
		//"distance: ".post; msg[4].post; ", rate: ".post; cookedrate.postln;
	}, '/writepointer');

	s.sync;

	~lcd.sendMsg('/lcd', "====", 3, 6);

/*
%%%%%%                                           %%%%%%
%%%%%%%%%%%                                 %%%%%%%%%%%
%%%%%%%%%%%%%%%%%   INFINITE REVERB   %%%%%%%%%%%%%%%%%
%%%%%%%%%%%                                 %%%%%%%%%%%
%%%%%%                                           %%%%%%
*/

// Original code: Eli Fieldsteel ("https://www.youtube.com/watch?v=_2N7lG5uzJI")
// (volume control added)

SynthDef(\infinitereverb, {
	arg feedback=0.95, input=0, output=0, volume=1, gate=0.5;
	var local, sig, soundout;
	sig = SoundIn.ar(input);
	local = LocalIn.ar(1)+sig;
	15.do{local = AllpassN.ar(local, 0.06, Rand(0.001,0.06), 3)}; // apply reverb
	LocalOut.ar(local*feedback); // complete feedback loop -- send audio back to LocalIn
	soundout = (sig+local) * volume;
	Out.ar(output, soundout * EnvGen.kr(Env.asr(0.5,1,1), gate));
}).add;


/*
%%%%%%                                           %%%%%%
%%%%%%%%%%%                                 %%%%%%%%%%%
%%%%%%%%%%%%%%%%%   SPECTRAL FREEZE   %%%%%%%%%%%%%%%%%
%%%%%%%%%%%                                 %%%%%%%%%%%
%%%%%%                                           %%%%%%
*/

SynthDef(\freeze, {
	arg output=0, window=0, ws=1024, hs=0.25, input=0, gate=0, volume=1;
    var tempstorage, in, chain;
    tempstorage = LocalBuf.new(ws);
    in = SoundIn.ar(input);
    // note the window type and overlaps... this is important for resynth parameters
    chain = FFT(tempstorage, in, hs, window, winsize: ws);
	chain = PV_Freeze(chain, gate);
    Out.ar(output, EnvGen.kr(Env.asr(0.5,0.7,1), gate) * IFFT(chain).dup * volume);
}).add;

	~lcd.sendMsg('/lcd', "===", 3, 10);

// ............................ STARTING SYNTHS (and mappings)............................

	s.sync;

	~timemachine = Group.new;

	~tm_rec = Synth.new(\tm_rec,
		[
			hs: ~hopsize,
			ws: ~windowsize,
			input: ~guitarami_input,
			buffer: ~buffer,
			frames: ~buffer_numFrames,
			bufferdur: ~dur,
			rate: ~sensors[\ult].asMap // controls moment in time
		],
		~timemachine);

	s.sync;

	~tm_play = Synth.new(\tm_play,
		[
			hs: ~hopsize,
			ws: ~windowsize,
			output: ~guitarami_output,
			buffer: ~buffer,
			gate: ~sensors[\button2].asMap // button 4 : Time Machine ON/OFF
		],
		~timemachine);

	s.sync;

	//~bypass = Synth.new(\bypass, [volume: 3]);

	~infinitereverb = Synth.new(\infinitereverb,
		[
			input: ~guitarami_input,
			output: ~guitarami_output,
			gate: ~sensors[\acclY].asMap,    // Y axis accelerometer:reverb ON/OFF
			feedback: ~sensors[\acclY].asMap // AND feedback control

	]);

	~freeze = Synth.new(\freeze,
		[
			ws: ~windowsize,
			hs: ~hopsize,
			input: ~guitarami_input,
			output: ~guitarami_output,
			volume: 0.6,
			gate: ~sensors[\button1].asMap // button 1: Freeze ON/OFF
	]);

	s.sync;

	~lcd.sendMsg('/lcd', "====", 3, 13);

// ............................ SYNTH COMMANDS ............................


// BYPASS Commands:
     // open (1) and close (0) gate
//          ~bypass.set(\gate, 0);
     // Set volume
//          ~bypass.set(\volume, 1);

// SPECTRAL FREEZE Commands:
     // open gate and set volume (any value between 0 and 1)
//          ~freeze.set(\gate, 0.5);
     // Set volume
//          ~freeze.set(\volume, 0.5);

// TIME MACHINE Commands:
     // open (1) and close (0) gate
//          ~timemachine.set(\gate, 0);
     // Change reading speed
//          ~timemachine.set(\pbspeed, 1.1);

// INFINITE REVERB Commands:
     // open (1) and close (0) gate
//          ~infinitereverb.set(\gate, 1);
     // Set reverb volume
//          ~infinitereverb.set(\volume, 0.5);
     // Set reverb value
//          ~infinitereverb.set(\feedback, 0.9);



// ............................ CHIME ............................

	s.sync;

	(dur:1, strum: 0.1, degree: [0,4,7]).play;


// ............................ SEND INFO TO LCD ............................

	// done LCD message
	~lcd.sendMsg('/lcd', "===", 3, 17);
	~lcd.sendMsg('/lcd', "Loading complete    ", 2, 1);
	(1.0).wait;

	AppClock.sched(0.0,{
		~lcd.sendMsg('/lcd', "       _Time-Machine", 1, 1);
		~lcd.sendMsg('/lcd', "Speed:        Frz:  ", 2, 1);
		~lcd.sendMsg('/lcd', "Fdbk:          TM:  ", 3, 1);
		10; // delay between LCD updates
	});

	AppClock.sched(0.0,{
	~lcd.sendMsg('/lcd', ~sensors_client[\ult].round(0.001).asString.padRight(5," "), 2, 8);
	~lcd.sendMsg('/lcd', ~sensors_client[\acclY].round(0.001).asString.padRight(7," "), 3, 7);
	~lcd.sendMsg('/lcd', ~sensors_client[\button1].asCompileString, 2, 20);
	~lcd.sendMsg('/lcd', ~sensors_client[\button2].asCompileString, 3, 20);
	~lcd.sendMsg('/lcd', ~sensors_client[\battery].asInteger.asString.padRight(3," "), 1, 1);
	~lcd.sendMsg('/lcd', "%", 1, 4);
	0.04; // delay between LCD updates
	});

}; // end of "waitForBoot"


// DEBUG commands:

// ~debug = NetAddr.new("192.168.5.2", 57120);    // NetAddr Mac Pro
// ~debug.sendMsg('/guitarami/acc', 1);
// s.queryAllNodes;

// ~lcd.sendMsg('/lcd', " ",0,14);
// ~lcd.sendMsg('/lcd', "bypass",0,14);