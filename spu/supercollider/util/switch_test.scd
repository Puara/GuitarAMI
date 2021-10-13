/*
GuitarAMI Foot switch test patch SC version 1.0 -- Edu Meneses
GuitarAMI SPU (Sound Processing Unit) v.5.0  - Rpi 4B

*/

(

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

~lcd.sendMsg('/lcd', "clear", 1, 1);

// OSC addresses (GuitarAMI module)
	~osc_addresses = Dictionary.newFrom(List[
		\button1  , ~osc_patch_spu +/+ "button1",
		\button2  , ~osc_patch_spu +/+ "button2",
		\button3  , ~osc_patch_spu +/+ "button3"
	]);

// Client-side variables for feedback and some data cooking
	~sensors_client = Dictionary.newFrom(List[
		\button1        , 0,
		\button2        , 0,
		\button3        , 0
	]);

// ............................ DATA RECEIVING ............................

    OSCdef(\button1,
	{
		arg msg;
		~sensors_client[\button1] = msg[1];
	},
	~osc_addresses[\button1].asSymbol);

    OSCdef(\button2,
	{
		arg msg;
		~sensors_client[\button2] = msg[1];
	},
	~osc_addresses[\button2].asSymbol);

    OSCdef(\button3,
	{
		arg msg;
		~sensors_client[\button3] = msg[1];
	},
	~osc_addresses[\button3].asSymbol);

    ~lcd.sendMsg('/lcd', "b1:   b2:     b3:   ", 2, 1);

    AppClock.sched(0.0,{
	~lcd.sendMsg('/lcd', ~sensors_client[\button1].asCompileString, 2, 5);
	~lcd.sendMsg('/lcd', ~sensors_client[\button2].asCompileString, 2, 11);
	~lcd.sendMsg('/lcd', ~sensors_client[\button3].asCompileString, 2, 19);
	0.04; // delay between LCD updates
	});

)