/*
GuitarAMI-TStick patch SC version 1.01 -- Edu Meneses
GuitarAMI SPU (Sound Processing Unit) v.5.0  - Rpi 4
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
	~lcd.sendMsg('/lcd', "Loading Time-Machine", 2, 1);
	~lcd.sendMsg('/lcd', "[                  ]", 3, 1);

	(thisProcess.nowExecutingPath.dirname +/+ "common/GuitarAMI_OSC.scd").load;

	~lcd.sendMsg('/lcd', "===", 3, 2);

// .............................. SYNTHS ..............................

	(thisProcess.nowExecutingPath.dirname +/+ "common/TimeMachine_TM.scd").load;
	// TIme-Machine starts its synths automatically to ensure sync

	~lcd.sendMsg('/lcd', "====", 3, 5);

	(thisProcess.nowExecutingPath.dirname +/+ "common/InfiniteReverb.scd").load;

	~infinitereverb = Synth.new(\infinitereverb, [
		input: ~guitarami_input,
		output: ~guitarami_output
	]);

	~lcd.sendMsg('/lcd', "====", 3, 9);

// ............................. MAPPING ..............................

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

	~lcd.sendMsg('/lcd', "====", 3, 13);

// .............................. CHIME ...............................

	s.sync;

	(dur:1, strum: 0.1, degree: [0,4,7]).play;

// ........................... LCD MESSAGE ............................

	// done LCD message
	~lcd.sendMsg('/lcd', "===", 3, 17);
	~lcd.sendMsg('/lcd', "Loading complete    ", 2, 1);
	(1.0).wait;

	AppClock.sched(0.0,{
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
	});

}; // end of "waitForBoot"
);

// ............................ SENSITIVITY ............................


~infinitereverb.set(\gate, 0.5);
