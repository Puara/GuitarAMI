/*
GuitarAMI OSC to MIDI (client) patch SC version 1.0 -- Edu Meneses
GuitarAMI SPU (Sound Processing Unit) v.5.0  - Rpi 4B

*/

(

~midi = True;

~osc_patch = "/SPU_00X";

if (~midi==True,{
	~midiChannel = 0;
	MIDIClient.init;
	//MIDIClient.destinations;

	m = MIDIOut(0);  // use virtual source port "out0" - Windows/macOS
	//m = MIDIOut(0, MIDIClient.destinations[0].uid); // Linux
});

// OSC addresses (GuitarAMI module)
	~osc_addresses = Dictionary.newFrom(List[
		\ult      , ~osc_patch +/+ "ult",     // (in mm)
	    \ultTrig  , ~osc_patch +/+ "ultTrig", // [0 or 1]
		\accl     , ~osc_patch +/+ "accl",    // (in m/s^2)
		\gyro     , ~osc_patch +/+ "gyro",    // (in radians per second)
		\magn     , ~osc_patch +/+ "magn",    // (in uTesla)
		\quat     , ~osc_patch +/+ "quat",
		\ypr      , ~osc_patch +/+ "ypr",     // (in degrees)
		\jab      , ~osc_patch +/+ "jab",
		\shake    , ~osc_patch +/+ "shake",
		\tap      , ~osc_patch +/+ "tap",     // [0 or 1]
		\dtap     , ~osc_patch +/+ "dtap",    // [0 or 1]
		\ttap     , ~osc_patch +/+ "ttap",    // [0 or 1]
		\count    , ~osc_patch +/+ "count",
	    \touch    , ~osc_patch +/+ "touch",
		\battery  , ~osc_patch +/+ "battery", // (percentage)
		\button1  , ~osc_patch +/+ "button1",
		\button2  , ~osc_patch +/+ "button2",
		\button3  , ~osc_patch +/+ "button3"
	]);

// Client-side variables for feedback and some data cooking
	~sensors_client = Dictionary.newFrom(List[
		\ult            , 0,
	    \ultTrig        , 0,
		\acclX          , 0,
		\acclY          , 0,
		\acclZ          , 0,
		\gyroX          , 0,
		\gyroY          , 0,
		\gyroZ          , 0,
		\magnX          , 0,
		\magnY          , 0,
		\magnZ          , 0,
		\quatI          , 0,
		\quatJ          , 0,
		\quatK          , 0,
		\quatReal       , 0,
		\yaw            , 0,
		\pitch          , 0,
		\roll           , 0,
		\jabX           , 0,
		\jabY           , 0,
		\jabZ           , 0,
		\shakeX         , 0,
		\shakeY         , 0,
		\shakeZ         , 0,
		\tap            , 0,
		\dtap           , 0,
		\ttap           , 0,
		\count          , 0,
	    \touch          , 0,
		\battery        , 0,
		\button1        , 0,
		\button2        , 0,
		\button3        , 0,
		\onsets         , 0,
		\freq           , 0
	]);

	s.sync;

// ............................ DATA RECEIVING ............................

	OSCdef(\ult,
	{
		arg msg;
		~sensors_client[\ult] = msg[1];
	    /*if (msg[1]<=100,
		    { ~sensors_client[\ult] = msg[1].linlin(0, 100, -4, 4) }, // from 0mm to 100mm, mapped from -4 to 4
		    { ~sensors_client[\ult] = 1 }
	    );
	    if (~sensors_client[\ult] == -4,{~sensors_client[\ult] = 1});*/
		if (~midi==True,{m.control(chan: ~midiChannel, ctlNum: 20, val: ~sensors_client[\ult])});
	},
	~osc_addresses[\ult].asSymbol);

    OSCdef(\ultTrig,
	{
		arg msg;
		~sensors_client[\ultTrig] = msg[1];
		if (~midi==True,{m.control(chan: ~midiChannel, ctlNum: 89, val: ~sensors_client[\ultTrig])});
	},
	~osc_addresses[\ultTrig].asSymbol);

	OSCdef(\accl,
	{
		arg msg;
	    ~sensors_client[\acclX] = msg[1];
	    //~sensors_client[\acclY] = msg[2].linlin(1.5, 2.5, 0, 1); // from 1.5 m/s^2 to 2.5 m/s^2, mapped from 0 to 1
		~sensors_client[\acclY] = msg[2];
	    ~sensors_client[\acclZ] = msg[3];
		if (~midi==True,
			{
				m.control(chan: ~midiChannel, ctlNum: 21, val: ~sensors_client[\acclX]);
				m.control(chan: ~midiChannel, ctlNum: 22, val: ~sensors_client[\acclY]);
				m.control(chan: ~midiChannel, ctlNum: 23, val: ~sensors_client[\acclZ])
		});
	},
	~osc_addresses[\accl].asSymbol);

	OSCdef(\gyro,
	{
		arg msg;
	    ~sensors_client[\gyroX] = msg[1]; ~sensors_client[\gyroY] = msg[2]; ~sensors_client[\gyroZ] = msg[3];
		if (~midi==True,
			{
				m.control(chan: ~midiChannel, ctlNum: 24, val: ~sensors_client[\gyroX]);
				m.control(chan: ~midiChannel, ctlNum: 25, val: ~sensors_client[\gyroY]);
				m.control(chan: ~midiChannel, ctlNum: 26, val: ~sensors_client[\gyroZ])
		});
	},
	~osc_addresses[\gyro].asSymbol);

	OSCdef(\magn,
	{
		arg msg;
		~sensors_client[\magnX] = msg[1]; ~sensors_client[\magnY] = msg[2]; ~sensors_client[\magnZ] = msg[3];
	},
	~osc_addresses[\magn].asSymbol);

	OSCdef(\quat,
	{
		arg msg;
	    ~sensors_client[\quatI] = msg[1]; ~sensors_client[\quatJ] = msg[2];
	    ~sensors_client[\quatK] = msg[3]; ~sensors_client[\quatReal] = msg[4];
	},
	~osc_addresses[\quat].asSymbol);

	OSCdef(\ypr,
	{
		arg msg;
		~sensors_client[\yaw] = msg[1]; ~sensors_client[\pitch] = msg[2]; ~sensors_client[\roll] = msg[3];
		if (~midi==True,
			{
				m.control(chan: ~midiChannel, ctlNum: 27, val: ~sensors_client[\yaw]);
				m.control(chan: ~midiChannel, ctlNum: 28, val: ~sensors_client[\pitch]);
				m.control(chan: ~midiChannel, ctlNum: 29, val: ~sensors_client[\roll])
		});
	},
	~osc_addresses[\ypr].asSymbol);

	OSCdef(\jab,
	{
		arg msg;
		~sensors_client[\jabX] = msg[1]; ~sensors_client[\jabY] = msg[2]; ~sensors_client[\jabZ] = msg[3];
		if (~midi==True,
			{
				m.control(chan: ~midiChannel, ctlNum: 102, val: ~sensors_client[\jabX]);
				m.control(chan: ~midiChannel, ctlNum: 103, val: ~sensors_client[\jabY]);
				m.control(chan: ~midiChannel, ctlNum: 104, val: ~sensors_client[\jabZ])
		});
	},
	~osc_addresses[\jab].asSymbol);

	OSCdef(\shake,
	{
		arg msg;
		~sensors_client[\shakeX] = msg[1]; ~sensors_client[\shakeY] = msg[2]; ~sensors_client[\shakeZ] = msg[3];
		if (~midi==True,
			{
				m.control(chan: ~midiChannel, ctlNum: 85, val: ~sensors_client[\shakeX]);
				m.control(chan: ~midiChannel, ctlNum: 86, val: ~sensors_client[\shakeY]);
				m.control(chan: ~midiChannel, ctlNum: 87, val: ~sensors_client[\shakeZ]);
		});
	},
	~osc_addresses[\shake].asSymbol);

	OSCdef(\tap,
	{
		arg msg;// var old;
	    // if only tap:
            ~sensors_client[\tap] = msg[1];
	    // to act as a troggle:
	    /*if (msg[1] == 1,
			{
				~sensors_client[\tap] = (~sensors_client[\tap]+1)%2;
				~sensors[\tap].set(~sensors_client[\tap])
		});*/
		if (~midi==True,{m.control(chan: ~midiChannel, ctlNum: 30, val: ~sensors_client[\tap])});
	},
	~osc_addresses[\tap].asSymbol);

	OSCdef(\dtap,
	{
		arg msg;
		~sensors_client[\dtap] = msg[1];
		if (~midi==True,{m.control(chan: ~midiChannel, ctlNum: 31, val: ~sensors_client[\dtap])});
	},
	~osc_addresses[\dtap].asSymbol);

	OSCdef(\ttap,
	{
		arg msg;
	    ~sensors_client[\ttap] = msg[1];
		if (~midi==True,{m.control(chan: ~midiChannel, ctlNum: 14, val: ~sensors_client[\ttap])});
	},
	~osc_addresses[\ttap].asSymbol);

	OSCdef(\count,
	{
		arg msg;
		~sensors_client[\count] = msg[1];
	},
	~osc_addresses[\count].asSymbol);

    OSCdef(\touch,
	{
		arg msg;
		~sensors_client[\touch] = msg[1];
		if (~midi==True,{m.control(chan: ~midiChannel, ctlNum: 15, val: ~sensors_client[\touch])});
	},
	~osc_addresses[\touch].asSymbol);

	OSCdef(\battery,
	{
		arg msg;
		~sensors_client[\battery] = msg[1];
	},
	~osc_addresses[\battery].asSymbol);

    OSCdef(\button1,
	{
		arg msg;
		~sensors_client[\button1] = msg[1];
		if (~midi==True,{m.control(chan: ~midiChannel, ctlNum: 107, val: ~sensors_client[\button1])});
	},
	~osc_addresses[\button1].asSymbol);

    OSCdef(\button2,
	{
		arg msg;
		~sensors_client[\button2] = msg[1];
		if (~midi==True,{m.control(chan: ~midiChannel, ctlNum: 108, val: ~sensors_client[\button2])});
	},
	~osc_addresses[\button2].asSymbol);

    OSCdef(\button3,
	{
		arg msg;
		~sensors_client[\button3] = msg[1];
		if (~midi==True,{m.control(chan: ~midiChannel, ctlNum: 109, val: ~sensors_client[\button3])});
	},
	~osc_addresses[\button3].asSymbol);

    OSCdef(\audio_data_OSC,
	{
		arg msg;
		~sensors_client[\onsets] = msg[1];
	    ~sensors_client[\freq] = msg[2];
		if (~midi==True,
			{
				m.control(chan: ~midiChannel, ctlNum: 105, val: ~sensors_client[\onsets]);
				m.control(chan: ~midiChannel, ctlNum: 106, val: ~sensors_client[\freq])
		});
	},
	'/audio_data');

)
