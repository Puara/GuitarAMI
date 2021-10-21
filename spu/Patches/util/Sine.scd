/*
GuitarAMI - Bypass patch SC -- Edu Meneses
Run on GuitarAMI Processing Unit (prototype5), made with Rpi 4B
*/

//s = Server.local;

s.boot;
s.doWhenBooted {
//s.waitForBoot{

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

	~lcd.sendMsg('/lcd', "GuitarAMI:          ", 1, 1);
	~lcd.sendMsg('/lcd', "Loading Sinusoid    ", 2, 1);
	~lcd.sendMsg('/lcd', "[===               ]", 3, 1);

	(thisProcess.nowExecutingPath.dirname +/+ "common/GuitarAMI_OSC.scd").load;

	~lcd.sendMsg('/lcd', "===", 3, 2);

/*
%%%%%%                                  %%%%%%
%%%%%%%%%%%                        %%%%%%%%%%%
%%%%%%%%%%%%%%%%%   SINOSC   %%%%%%%%%%%%%%%%%
%%%%%%%%%%%                        %%%%%%%%%%%
%%%%%%                                  %%%%%%
*/

	SynthDef(\Sine, {
		arg volume=0.01, input=0, output=0, gate=0, freq=10;
		var sound;
		sound = SinOsc.ar(LinLin.kr(freq, -20, 20, 220, 440), 0.0, EnvGen.kr(Env.asr(0.5,1,1), gate) * volume);
		Out.ar(output, sound);
	}).add;

	~lcd.sendMsg('/lcd', "====", 3, 5);


// ............................ STARTING SYNTH ............................

	s.sync;

	~bypass = Synth.new(\Sine,
		[
			input: ~guitarami_input,
			output: ~guitarami_output,
			gate: ~sensors[\tap].asMap,
			freq: ~sensors[\acclY].asMap
	]);

	~lcd.sendMsg('/lcd', "===", 3, 9);

// ............................ SEND INFO TO LCD ............................

	// done LCD message
	~lcd.sendMsg('/lcd', "========", 3, 12);
	~lcd.sendMsg('/lcd', "Loading complete    ", 2, 1);
	(1.0).wait;

	AppClock.sched(0.0,{
		~lcd.sendMsg('/lcd', "                Sine", 1, 1);
		~lcd.sendMsg('/lcd', "mode:               ", 2, 1);
		~lcd.sendMsg('/lcd', "freq:               ", 3, 1);
		10; // delay between LCD updates
	});

	AppClock.sched(0.0,{
		~lcd.sendMsg('/lcd', ~sensors_client[\tap].asCompileString, 2, 7);
		~lcd.sendMsg('/lcd', ~sensors_client[\acclY].round(0.001).asString.padRight(10," "), 3, 7);
		~lcd.sendMsg('/lcd', ~sensors_client[\battery].asInteger.asString.padRight(3," "), 1, 1);
		~lcd.sendMsg('/lcd', "%", 1, 4);
		0.04; // delay between LCD updates
	});

// ............................ SYNTH COMMANDS ............................

// BYPASS Commands:
     // open (1) and close (0) gate
//          ~bypass.set(\gate, 0);
     // Set volume
//          ~bypass.set(\volume, 1);

}; // end of "waitForBoot"
