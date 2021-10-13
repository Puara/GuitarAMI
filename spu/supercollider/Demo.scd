/*
IDMIL demo patch version 0.5
GuitarAMI | TStick | Probatio
Edu Meneses (IDMIL, CIRMMT, McGill University) - 2021
GuitarAMI SPU (Sound Processing Unit) v.5.0a  - Rpi 4
*/

// s = Server.local;
// ServerOptions.devices; // if needed to check the available interfaces

(

//Server.default.options.device_("AudioBox USB");
/*Server.local.options.blockSize = 256;
Server.local.options.numOutputBusChannels = 2;    // change number of input and$
Server.local.options.numInputBusChannels = 2;
Server.internal.options.numOutputBusChannels = 2;
Server.internal.options.numInputBusChannels = 2;*/

s.waitForBoot{

	//OSCdef.freeAll;

	//0.5.wait;

	// ........................... Global defs ............................

	~id="999";
	~probatio_id="999";
	~tstick_id="999";

	~midi = False;
	~oscForward = False;
	~oscForwardIP= "174.172.24.121"; // replace X by last IP address digits
	~oscForwardPort = 8000; // has to be an int
	~guitarami_input = 0;
	~guitarami_output = 0;
	~osc_path_module = "/GuitarAMI_module_" ++ ~id;
	~osc_path_probatio = "/probatio_m5_" ++ ~id;
	~osc_path_tstick = "/TStick_" ++ ~id;
	~osc_path_spu = "/guitarami_spu";

	~lcd = NetAddr.new("127.0.0.1", 20000); // Setting LCD - Loopback NetAddr and Python code listening port
	~client = NetAddr.new("172.24.1.121", 8000); // Setting LCD - Loopback NetAddr and Python code listening port
	thisProcess.openUDPPort(8000);



	// ......................... Global variables .........................

	~lcd.sendMsg('/lcd', "GuitarAMI:          ", 1, 1);
	~lcd.sendMsg('/lcd', "Loading IDMIL demos ", 2, 1);
	~lcd.sendMsg('/lcd', "[                  ]", 3, 1);

	(thisProcess.nowExecutingPath.dirname +/+ "common/GuitarAMI_OSC.scd").load;

	(thisProcess.nowExecutingPath.dirname +/+ "common/Probatio_OSC.scd").load;

	(thisProcess.nowExecutingPath.dirname +/+ "common/T-Stick_OSC.scd").load;

	~lcd.sendMsg('/lcd', "===", 3, 2);

	// .............................. SYNTHS ..............................

	(thisProcess.nowExecutingPath.dirname +/+ "common/TimeMachine_TM.scd").load;
	// TIme-Machine starts its synths automatically to ensure sync between synths

	~lcd.sendMsg('/lcd', "====", 3, 5);

	(thisProcess.nowExecutingPath.dirname +/+ "common/InfiniteReverb.scd").load;

	~infinitereverb = Synth.new(\infinitereverb, [
		input: ~guitarami_input,
		output: ~guitarami_output
	]);

	(thisProcess.nowExecutingPath.dirname +/+ "common/FoldSynth.scd").load;

	~foldsynth_group = Group.new;
	~foldsynth_array = [];

	// https://github.com/everythingwillbetakenaway/DX7-Supercollider
	~dx7 = ("./common/DX7.scd").loadRelative.wrapAt(-1);

	// ~dx7.value(60,127,16383); // note, vel, preset

	~lcd.sendMsg('/lcd', "====", 3, 9);

	// ........................ GuitarAMI MAPPINGS ........................

	~timemachine.set(
		\speed, ~sensors[\ult].asMap,
		\freeze, ~sensors[\tap].asMap,
		//\pitch, ,
		\reset, ~sensors[\button1].asMap,
		//\gatebp, ,
		//\gatefreeze, ,
		//\gatefft,
	);
	~infinitereverb.set(
		\gate, ~sensors[\acclY].asMap,
		\feedback, ~sensors[\acclY].asMap
	);

	// ......................... T-Stick MAPPINGS .........................

	SynthDef.new(\scale_control, {
		arg min_in, max_in, min_out, max_out, in_bus, out_bus;
		Out.kr(out_bus,LinExp.kr(in_bus, min_in, max_in, min_out, max_out));
	}).add;

	s.sync;

	~accly_scale = Bus.control(s);
	~acclz_scale = Bus.control(s);

	~accly_scale_synth = Synth.new(\scale_control, [
		\min_in, 0,
		\max_in, 1,
		\min_out, 1,
		\max_out, 80,
		\in_bus, ~sensors_tstick[\accly_norm].asMap,
		\out_bus, ~accly_scale
	]);

	~acclz_scale_synth = Synth.new(\scale_control, [
		\min_in, 0,
		\max_in, 1,
		\min_out, 0.2,
		\max_out, 0.5,
		\in_bus, ~sensors_tstick[\acclz_norm].asMap,
		\out_bus, ~acclz_scale
	]);

	~foldsynth_group.set(
		//\freq, ~sensors_tstick[\fsr].asMap, //  (between 2 and 8)
		\maxrq, ~acclz_scale.asMap, // roll (between 0.2 and 0.5)
		\detune, ~accly_scale.asMap, // pitch (between 1 and 80)
		\boost, ~sensors_tstick[\shakex].asMap // shake x (between 0 and 3)
	);

	~foldsynth_counter = 0;

	AppClock.sched(0.0,{
		var cook_touch;
		cook_touch = ~sensors_client_tstick[\touch_all].linlin(0,1,0,8).asInteger;
		if (
			cook_touch > ~foldsynth_counter,
			{
				(cook_touch - ~foldsynth_counter).do(
					{
						~foldsynth_array = ~foldsynth_array.add(
							Synth.new(\foldsynth,
								[
									\shape, linrand(0,2),
									\freq, exprand(1,8),
									\atk, exprand(1,8),
									\rel, exprand(2,8)
								],
								~foldsynth_group
							)
						)
					}
				);
			},
			{
				(~foldsynth_counter - cook_touch).do(
					{
						if(~foldsynth_array.size > 0,
							{
								~foldsynth_array[0].set(\gate, 0);
								~foldsynth_array.removeAt(0);
							}
						)
					}
				)
			}
		);
		~foldsynth_counter = cook_touch;
		0.5; // delay between synth creation/destruction
	});


	~lcd.sendMsg('/lcd', "====", 3, 13);

	// .............................. CHIME ...............................

	s.sync;

	(dur:1, strum: 0.1, degree: [0,4,7]).play;

	// ........................... LCD MESSAGE ............................

	// done LCD message
	~lcd.sendMsg('/lcd', "===", 3, 17);
	~lcd.sendMsg('/lcd', "Loading complete    ", 2, 1);
	(1.0).wait;

/*	AppClock.sched(0.0,{
		~lcd.sendMsg('/lcd', "        Time-Machine", 1, 1);
		~lcd.sendMsg('/lcd', "Speed:        Frz:  ", 2, 1);
		~lcd.sendMsg('/lcd', "Fdbk:         RST:  ", 3, 1);
		10; // delay between LCD updates
	});

	AppClock.sched(0.0,{
		~lcd.sendMsg('/lcd', ~sensors_client[\ult].round(0.001).asString.padRight(5," "), 2, 8);
		~lcd.sendMsg('/lcd', ~sensors_client[\acclY].round(0.001).asString.padRight(7," "), 3, 7);
		~lcd.sendMsg('/lcd', ~sensors_client[\tap].asCompileString, 2, 20);
		~lcd.sendMsg('/lcd', ~sensors_client[\button1].asCompileString, 3, 20);
		~lcd.sendMsg('/lcd', ~sensors_client[\battery].asInteger.asString.padRight(3," "), 1, 1);
		~lcd.sendMsg('/lcd', "%", 1, 4);
		0.04; // delay between LCD updates
	});*/

}; // end of "waitForBoot"
);

// ............................ SENSITIVITY ............................


~infinitereverb.set(\gate, 0.5);
