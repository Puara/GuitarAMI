/*
GuitarAMI - Simple internal MIDI 2 OSC

GuitarAMI SPU (Sound Processing Unit) v.5.0  - Rpi 4B + PiSound (or Fe-Pi)
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

	~midi = True;
    ~oscForward = True;
	~oscForwardIP= "174.172.24.121"; // replace X by last IP address digits
	~oscForwardPort = 8000; // has to be an int
	~guitarami_input = 0;
	~guitarami_output = 0;
	~osc_patch_module = "/GuitarAMI_module_" ++ ~id;
	~osc_patch_spu = "/guitarami_spu";

	~lcd = NetAddr.new("127.0.0.1", 20000); // Setting LCD - Loopback NetAddr and Python code listening port
	thisProcess.openUDPPort(8000);



// ......................... Global variables .........................

	~lcd.sendMsg('/lcd', "GuitarAMI:          ", 1, 1);
	~lcd.sendMsg('/lcd', "Loading: Simple MIDI", 2, 1);
	~lcd.sendMsg('/lcd', "[                  ]", 3, 1);
	/*
	~lcd.sendMsg('/lcd', "===", 3, 2);
	~lcd.sendMsg('/lcd', "===", 3, 5);
	~lcd.sendMsg('/lcd', "===", 3, 8);
	~lcd.sendMsg('/lcd', "===", 3, 11);
	~lcd.sendMsg('/lcd', "===", 3, 14);
	~lcd.sendMsg('/lcd', "===", 3, 17);
	*/

// ...................... GETTING OSC MESSAGES ........................

	(thisProcess.nowExecutingPath.dirname +/+ "common/GuitarAMI_OSC.scd").load;

	~lcd.sendMsg('/lcd', "===", 3, 2);
	~lcd.sendMsg('/lcd', "===", 3, 5);

// ........................... LCD MESSAGE ............................

	// done LCD message
	~lcd.sendMsg('/lcd', "===", 3, 8);
	~lcd.sendMsg('/lcd', "===", 3, 11);
	~lcd.sendMsg('/lcd', "===", 3, 14);
	~lcd.sendMsg('/lcd', "===", 3, 17);
	~lcd.sendMsg('/lcd', "Loading complete    ", 2, 1);
	(1.0).wait;

	AppClock.sched(0.0,{
		~lcd.sendMsg('/lcd', "         Simple MIDI", 1, 1);
		~lcd.sendMsg('/lcd', "                    ", 2, 1);
		~lcd.sendMsg('/lcd', "                    ", 3, 1);
		10; // delay between LCD updates
	});

	AppClock.sched(0.0,{
		~lcd.sendMsg('/lcd', ~sensors_client[\battery].asInteger.asString.padRight(3," "), 1, 1);
		~lcd.sendMsg('/lcd', "%", 1, 4);
		0.04; // delay between LCD updates
	});

}; // end of "waitForBoot"
);
