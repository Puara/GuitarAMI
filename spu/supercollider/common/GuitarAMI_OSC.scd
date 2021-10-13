/*
GuitarAMI OSC patch SC version 1.1 -- Edu Meneses
GuitarAMI SPU (Sound Processing Unit) v.5.0  - Rpi 4B

OSC data receiving

*/

// IMPORTANT: define the folowing global variables on the parent SC file:

/*
    ~id="XXX";

	~midi = True;
    ~oscForward = True;
	~oscForwardIP= "174.172.24.1.X"; // replace X by last IP address digits
	~oscForwardPort = 8000; // has to be an int
	~guitarami_input = 0;
	~guitarami_output = 0;
	~osc_path_module = "/GuitarAMI_module_" ++ ~id;
	~osc_path_spu = "/guitarami_spu";
*/

/*
## OSC namespaces

- continuous:
  - /GuitarAMI_module_XXX/ult [int] (in mm)
  - /GuitarAMI_module_XXX/accl [float] [float] [float] (in m/s^2)
  - /GuitarAMI_module_XXX/gyro [float] [float] [float] (in radians per second)
  - /GuitarAMI_module_XXX/magn [float] [float] [float] (in uTesla)
  - /GuitarAMI_module_XXX/quat [float] [float] [float]
- discrete:
  - /GuitarAMI_module_XXX/tap [0 or 1]
  - /GuitarAMI_module_XXX/dtap [0 or 1]
  - /GuitarAMI_module_XXX/ttap [0 or 1]
  - /GuitarAMI_module_XXX/battery [int] (percentage)
- Instrument:
  - /GuitarAMI_module_XXX/ypr [float] [float] [float] (in degrees)
  - /GuitarAMI_module_XXX/jab [float] [float] [float]
  - /GuitarAMI_module_XXX/shake [float] [float] [float]
  - /GuitarAMI_module_XXX/count [int]
  - /GuitarAMI_module_XXX/ultTrig [int] [0 or 1]
- SPU specific:
  - /SPUXXX/onsets [float]
  - /SPUXXX/frequency [float]
  - /SPUXXX/button1 [0 or 1]
  - /SPUXXX/button2 [0 or 1]
  - /SPUXXX/button3 [0 or 1]
  - /SPUXXX/button1trig [0 or 1]
  - /SPUXXX/button2trig [0 or 1]
  - /SPUXXX/button3trig [0 or 1]
  - /SPUXXX/taptrig [0 or 1]
  - /SPUXXX/dtaptrig [0 or 1]
  - /SPUXXX/ttaptrig [0 or 1]

## MIDI messages

- continuous:
  - CC 20: ultrasonic sensor scaled from 0 to 300 mm
  - CC 21: absolute X-axis acceleration, scaled from 0 to 50 m/s^2 (~5.1 gs)
  - CC 22: absolute Y-axis acceleration, scaled from 0 to 50 m/s^2 (~5.1 gs)
  - CC 23: absolute Z-axis acceleration, scaled from 0 to 50 m/s^2 (~5.1 gs)
  - CC 24: absolute X-axis angular velocity (gyroscope), scaled from 0 to 25 rad/s
  - CC 25: absolute Y-axis angular velocity (gyroscope), scaled from 0 to 25 rad/s
  - CC 26: absolute Z-axis angular velocity (gyroscope), scaled from 0 to 25 rad/s
  - CC 27: yaw, scaled from -180 to 180 degrees
  - CC 28: pitch, scaled from -180 to 180 degrees
  - CC 29: roll, scaled from -180 to 180 degrees
  - CC 85: shake X
  - CC 86: shake Y
  - CC 87: shake Z
  - CC 102: jab X
  - CC 103: jab Y
  - CC 104: jab Z
- discrete:
  - CC 30: tap [0 or 127]
  - CC 31: double tap [0 or 127]
  - CC 14: triple tap [0 or 127]
  - CC 15: touch count starting from 0
  - CC 89: Ultrasonic trigger [0 or 127]
- SPU specific:
  - CC 105: Onsets [0 or 127]
  - CC 106: Frequency
  - CC 107: Button 1 [0 or 127]
  - CC 108: Button 2 [0 or 127]
  - CC 109: Button 3 [0 or 127]
  - CC 110: Button 1 Trigger mode [0 or 127]
  - CC 111: Button 2 Trigger mode [0 or 127]
  - CC 112: Button 3 Trigger mode [0 or 127]
  - CC 113: tap Trigger mode [0 or 127]
  - CC 114: double tap Trigger mode [0 or 127]
  - CC 115: triple tap Trigger mode [0 or 127]
*/

~osc_forward_patch = "/SPU" ++ ~id;

if (~midi==True,{
	~midiChannel = 0;
	MIDIClient.init;
	//MIDIClient.destinations;

	//m = MIDIOut(0);  // use virtual source port "out0" - Windows/macOS
	m = MIDIOut(0, MIDIClient.destinations[0].uid); // GuitarAMI (Linux)
});

if (~oscForward==True,{
	~client = NetAddr.new(~oscForwardIP, ~oscForwardPort); // Setting LCD - Loopback NetAddr and Python code listening port
});

// OSC addresses (GuitarAMI module)
	~osc_address = Dictionary.newFrom(List[
		\ult      , "ult",     // (in mm)
	    \ultTrig  , "ultTrig", // [0 or 1]
		\accl     , "accl",    // (in m/s^2)
		\gyro     , "gyro",    // (in radians per second)
		\magn     , "magn",    // (in uTesla)
		\quat     , "quat",
		\ypr      , "ypr",     // (in degrees)
		\jab      , "jab",
		\shake    , "shake",
		\tap      , "tap",     // [0 or 1]
	    \taptrig  , "taptrig",
		\dtap     , "dtap",    // [0 or 1]
	    \dtaptrig , "dtaptrig",
		\ttap     , "ttap",    // [0 or 1]
    	\ttaptrig , "ttaptrig",
		\count    , "count",
	    \touch    , "touch",
		\battery  , "battery", // (percentage)
		\button1  , "button1",
		\button2  , "button2",
		\button3  , "button3",
	    \button1trig  , "button1trig", // [0 or 1]
	    \button2trig  , "button2trig", // [0 or 1]
	    \button3trig  , "button3trig", // [0 or 1]
	    \taptrig      , "taptrig"    , // [0 or 1]
	    \dtaptrig     , "dtaptrig"   , // [0 or 1]
	    \ttaptrig     , "ttaptrig"   , // [0 or 1]
	    \onsets       , "onsets",
		\freq         , "freq"
	]);

// Buses for mapping
	~sensors = Dictionary.newFrom(List[
		\ult            , Bus.control(s), // (in mm)
	    \ultRaw         , Bus.control(s), // (in mm)
	    \ultTrig        , Bus.control(s), // [0 or 1]
		\acclX          , Bus.control(s), // (in m/s^2)
		\acclY          , Bus.control(s), // (in m/s^2)
		\acclZ          , Bus.control(s), // (in m/s^2)
		\gyroX          , Bus.control(s), // (in radians per second)
		\gyroY          , Bus.control(s), // (in radians per second)
		\gyroZ          , Bus.control(s), // (in radians per second)
		\magnX          , Bus.control(s), // (in uTesla)
		\magnY          , Bus.control(s), // (in uTesla)
		\magnZ          , Bus.control(s), // (in uTesla)
		\quatI          , Bus.control(s),
		\quatJ          , Bus.control(s),
		\quatK          , Bus.control(s),
		\quatReal       , Bus.control(s),
		\yaw            , Bus.control(s), // (in degrees)
		\pitch          , Bus.control(s), // (in degrees)
		\roll           , Bus.control(s), // (in degrees)
		\jabX           , Bus.control(s),
		\jabY           , Bus.control(s),
		\jabZ           , Bus.control(s),
		\shakeX         , Bus.control(s),
		\shakeY         , Bus.control(s),
		\shakeZ         , Bus.control(s),
		\tap            , Bus.control(s), // [0 or 1]
		\dtap           , Bus.control(s), // [0 or 1]
		\ttap           , Bus.control(s), // [0 or 1]
		\count          , Bus.control(s),
	    \touch          , Bus.control(s),
		\battery        , Bus.control(s), // (percentage)
		\button1        , Bus.control(s),
		\button2        , Bus.control(s),
		\button3        , Bus.control(s),
	    \button1trig    , Bus.control(s),
	    \button2trig    , Bus.control(s),
	    \button3trig    , Bus.control(s),
	    \taptrig        , Bus.control(s),
	    \dtaptrig       , Bus.control(s),
	    \ttaptrig       , Bus.control(s),
		\onsets         , Bus.control(s),
		\freq           , Bus.control(s)
	]);

// Client-side variables for feedback and some data cooking
	~sensors_client = Dictionary.newFrom(List[
		\ult            , 0,
	    \ultRaw         , 0,
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
	    \taptrig        , 0,
		\dtap           , 0,
	    \dtaptrig       , 0,
		\ttap           , 0,
	    \ttaptrig       , 0,
		\count          , 0,
	    \touch          , 0,
		\battery        , 0,
		\button1        , 0,
		\button2        , 0,
		\button3        , 0,
	    \button1trig    , 0,
	    \button2trig    , 0,
	    \button3trig    , 0,
	    \taptrig        , 0,
	    \dtaptrig       , 0,
	    \ttaptrig       , 0,
		\onsets         , 0,
		\freq           , 0
	]);

	s.sync;

// ............................ DATA RECEIVING ............................

    SynthDef(\audio_data, {
	    arg input, windowsize=512, onsets_treshold=0.7;
	    var audio, freq, hasFreq, chain, onsets;
	    audio = SoundIn.ar(input);
	    # freq, hasFreq = Pitch.kr(audio, ampThreshold: 0.02, median: 7);
	    chain = FFT(LocalBuf(windowsize), audio);
    	onsets = Onsets.kr(chain, onsets_treshold, \rcomplex);
	    Out.kr(~sensors[\onsets], onsets);
    	Out.kr(~sensors[\freq], freq);
	    SendReply.kr(Impulse.kr(500),'/audio_data', [onsets, freq]);
    }).add;

	s.sync;

	OSCdef(\ult, {
		arg msg;
	    // raw
		    ~sensors_client[\ultRaw] = msg[1];
		    ~sensors[\ultRaw].set(~sensors_client[\ultRaw]);
		// cooked
		    if (msg[1]<=100,
		        { ~sensors_client[\ult] = msg[1].linlin(0, 100, -4, 4) }, // from 0mm to 100mm, mapped from -4 to 4
		        { ~sensors_client[\ult] = 1 }
    	    );
	        if (~sensors_client[\ult] == -4,{~sensors_client[\ult] = 1});
	        ~sensors[\ult].set(~sensors_client[\ult]
		    );
		// OSC forward
		    if (~oscForward==True,{~client.sendMsg(~osc_forward_patch+/+~osc_address[\ult], ~sensors_client[\ult])});
		// midi
    		if (~midi==True,{m.control(chan: ~midiChannel, ctlNum: 20, val: ~sensors_client[\ult].linlin(-4, 4, 0, 127))});
        }, (~osc_path_module+/+~osc_address[\ult]).asSymbol;
    );

    OSCdef(\ultTrig, {
		arg msg;
		// data
		    ~sensors_client[\ultTrig] = msg[1];
		    ~sensors[\ultTrig].set(msg[1]);
		// OSC forward
		    if (~oscForward==True,{~client.sendMsg(~osc_forward_patch+/+~osc_address[\ultTrig], ~sensors_client[\ultTrig])});
		// midi
		    if (~midi==True,{m.control(chan: ~midiChannel, ctlNum: 89, val: ~sensors_client[\ultTrig].linlin(0, 1, 0, 127))});
	    }, (~osc_path_module+/+~osc_address[\ultTrig]).asSymbol;
    );

	OSCdef(\accl, {
		arg msg;
	    // data
	        ~sensors_client[\acclX] = msg[1];
    	    ~sensors_client[\acclY] = msg[2].linlin(1.5, 4.5, 0, 1); // from 1.5 m/s^2 to 2.5 m/s^2, mapped from 0 to 1
	        ~sensors_client[\acclZ] = msg[3];                        // Default: 1.5 to 2.5
		    ~sensors[\acclX].set(msg[1]);                            // Steve: 1.5 to 4.5
    	    ~sensors[\acclY].set(~sensors_client[\acclY]);
	        ~sensors[\acclZ].set(msg[3]);
		// OSC forward
		    if (~oscForward==True,{~client.sendMsg(~osc_forward_patch+/+~osc_address[\accl],
		        ~sensors_client[\acclX], ~sensors_client[\acclY], ~sensors_client[\acclZ])
	        });
		// midi
		    if (~midi==True, {
				m.control(chan: ~midiChannel, ctlNum: 21, val: ~sensors_client[\acclX].linlin(0, 50, 0, 127));
				m.control(chan: ~midiChannel, ctlNum: 22, val: ~sensors_client[\acclY].linlin(0, 50, 0, 127));
				m.control(chan: ~midiChannel, ctlNum: 23, val: ~sensors_client[\acclZ].linlin(0, 50, 0, 127))
	        });
	    }, (~osc_path_module+/+~osc_address[\accl]).asSymbol;
    );

	OSCdef(\gyro, {
		arg msg;
	    // data
	        ~sensors_client[\gyroX] = msg[1]; ~sensors_client[\gyroY] = msg[2]; ~sensors_client[\gyroZ] = msg[3];
    		~sensors[\gyroX].set(msg[1]); ~sensors[\gyroY].set(msg[2]); ~sensors[\gyroZ].set(msg[3]);
	    // OSC forward
		    if (~oscForward==True,{~client.sendMsg(~osc_forward_patch+/+~osc_address[\gyro],
		        ~sensors_client[\gyroX], ~sensors_client[\gyroY], ~sensors_client[\gyroZ])
	        });
		// midi
		    if (~midi==True, {
				    m.control(chan: ~midiChannel, ctlNum: 24, val: ~sensors_client[\gyroX].linlin(0, 25, 0, 127));
    				m.control(chan: ~midiChannel, ctlNum: 25, val: ~sensors_client[\gyroY].linlin(0, 25, 0, 127));
	    			m.control(chan: ~midiChannel, ctlNum: 26, val: ~sensors_client[\gyroZ].linlin(0, 25, 0, 127))
		    });
        }, (~osc_path_module+/+~osc_address[\gyro]).asSymbol;
    );

	OSCdef(\magn, {
    	arg msg;
	    // data
	    	~sensors_client[\magnX] = msg[1]; ~sensors_client[\magnY] = msg[2]; ~sensors_client[\magnZ] = msg[3];
		    ~sensors[\magnX].set(msg[1]);~sensors[\magnY].set(msg[2]); ~sensors[\magnZ].set(msg[3]);
	    // OSC forward
		    if (~oscForward==True,{~client.sendMsg(~osc_forward_patch+/+~osc_address[\magn],
		        ~sensors_client[\magnX], ~sensors_client[\magnY], ~sensors_client[\magnZ])
	        });
	    }, (~osc_path_module+/+~osc_address[\magn]).asSymbol;
    );

	OSCdef(\quat, {
		arg msg;
    	// data
	        ~sensors_client[\quatI] = msg[1]; ~sensors_client[\quatJ] = msg[2];
	        ~sensors_client[\quatK] = msg[3]; ~sensors_client[\quatReal] = msg[4];
    		~sensors[\quatI].set(msg[1]); ~sensors[\quatJ].set(msg[2]);
	        ~sensors[\quatK].set(msg[3]); ~sensors[\quatReal].set(msg[4]);
	    // OSC forward
		    if (~oscForward==True,{~client.sendMsg(~osc_forward_patch+/+~osc_address[\quat],
		        ~sensors_client[\quatI], ~sensors_client[\quatJ], ~sensors_client[\quatK], ~sensors_client[\quatReal])
	        });
	    }, (~osc_path_module+/+~osc_address[\quat]).asSymbol;
    );

	OSCdef(\ypr, {
		arg msg;
	    // data
    		~sensors_client[\yaw] = msg[1]; ~sensors_client[\pitch] = msg[2]; ~sensors_client[\roll] = msg[3];
	    	~sensors[\yaw].set(msg[1]); ~sensors[\pitch].set(msg[2]); ~sensors[\roll].set(msg[3]);
	    // OSC forward
		    if (~oscForward==True,{~client.sendMsg(~osc_forward_patch+/+~osc_address[\ypr],
		        ~sensors_client[\yaw], ~sensors_client[\pitch], ~sensors_client[\roll])
	        });
   		// midi
		    if (~midi==True, {
				m.control(chan: ~midiChannel, ctlNum: 27, val: ~sensors_client[\yaw].linlin(-180, 180, 0, 127));
				m.control(chan: ~midiChannel, ctlNum: 28, val: ~sensors_client[\pitch].linlin(-180, 180, 0, 127));
				m.control(chan: ~midiChannel, ctlNum: 29, val: ~sensors_client[\roll].linlin(-180, 180, 0, 127))
		    });
        }, (~osc_path_module+/+~osc_address[\ypr]).asSymbol;
    );

	OSCdef(\jab, {
		arg msg;
	    // data
    		~sensors_client[\jabX] = msg[1]; ~sensors_client[\jabY] = msg[2]; ~sensors_client[\jabZ] = msg[3];
	    	~sensors[\jabX].set(msg[1]); ~sensors[\jabY].set(msg[2]); ~sensors[\jabZ].set(msg[3]);
	    // OSC forward
		    if (~oscForward==True,{~client.sendMsg(~osc_forward_patch+/+~osc_address[\jab],
		        ~sensors_client[\jabX], ~sensors_client[\jabY], ~sensors_client[\jabZ])
	        });
	    // midi
	        if (~midi==True, {
		        m.control(chan: ~midiChannel, ctlNum: 102, val: ~sensors_client[\jabX].linlin(0, 0.1, 0, 127));
		   	    m.control(chan: ~midiChannel, ctlNum: 103, val: ~sensors_client[\jabY].linlin(0, 0.1, 0, 127));
			    m.control(chan: ~midiChannel, ctlNum: 104, val: ~sensors_client[\jabZ].linlin(0, 0.1, 0, 127))
		    });
	    }, (~osc_path_module+/+~osc_address[\jab]).asSymbol;
    );

	OSCdef(\shake, {
		arg msg;
	    // data
    		~sensors_client[\shakeX] = msg[1]; ~sensors_client[\shakeY] = msg[2]; ~sensors_client[\shakeZ] = msg[3];
	    	~sensors[\shakeX].set(msg[1]); ~sensors[\shakeY].set(msg[2]); ~sensors[\shakeZ].set(msg[3]);
	    // OSC forward
		    if (~oscForward==True,{~client.sendMsg(~osc_forward_patch+/+~osc_address[\shake],
		        ~sensors_client[\shakeX], ~sensors_client[\shakeY], ~sensors_client[\shakeZ])
	        });
	    // midi
		    if (~midi==True, {
				m.control(chan: ~midiChannel, ctlNum: 85, val: ~sensors_client[\shakeX].linlin(0, 0.1, 0, 127));
				m.control(chan: ~midiChannel, ctlNum: 86, val: ~sensors_client[\shakeY].linlin(0, 0.1, 0, 127));
				m.control(chan: ~midiChannel, ctlNum: 87, val: ~sensors_client[\shakeZ].linlin(0, 0.1, 0, 127));
		    });
    	}, (~osc_path_module+/+~osc_address[\shake]).asSymbol;
    );

	OSCdef(\tap, {
		arg msg;
	    // tap:
			~sensors_client[\tap] = msg[1];
            ~sensors[\tap].set(msg[1]);
	    // troggle:
	        if (msg[1] == 1, {
				~sensors_client[\taptrig] = (~sensors_client[\taptrig]+1)%2;
				~sensors[\taptrig].set(~sensors_client[\taptrig])
		    });
	    // OSC forward
		    if (~oscForward==True,{
		        ~client.sendMsg(~osc_forward_patch+/+~osc_address[\tap], ~sensors_client[\tap]);
		        ~client.sendMsg(~osc_forward_patch+/+~osc_address[\taptrig], ~sensors_client[\taptrig])
	        });
	    // midi
		    if (~midi==True,{
		        m.control(chan: ~midiChannel, ctlNum: 30, val: ~sensors_client[\tap].linlin(0, 1, 0, 127));
		        m.control(chan: ~midiChannel, ctlNum: 113, val: ~sensors_client[\taptrig].linlin(0, 1, 0, 127))
		    });
    	}, (~osc_path_module+/+~osc_address[\tap]).asSymbol;
    );

    OSCdef(\dtap, {
		arg msg;
	    // tap:
			~sensors_client[\dtap] = msg[1];
            ~sensors[\dtap].set(msg[1]);
	    // troggle:
	        if (msg[1] == 1, {
				~sensors_client[\dtaptrig] = (~sensors_client[\dtaptrig]+1)%2;
				~sensors[\dtaptrig].set(~sensors_client[\dtaptrig])
		    });
	    // OSC forward
		    if (~oscForward==True,{
		        ~client.sendMsg(~osc_forward_patch+/+~osc_address[\dtap], ~sensors_client[\dtap]);
		        ~client.sendMsg(~osc_forward_patch+/+~osc_address[\dtaptrig], ~sensors_client[\dtaptrig])
	        });
	    // midi
		    if (~midi==True,{
		        m.control(chan: ~midiChannel, ctlNum: 31, val: ~sensors_client[\dtap].linlin(0, 1, 0, 127));
		        m.control(chan: ~midiChannel, ctlNum: 114, val: ~sensors_client[\dtaptrig].linlin(0, 1, 0, 127))
	        });
    	}, (~osc_path_module+/+~osc_address[\dtap]).asSymbol;
    );

    OSCdef(\ttap, {
		arg msg;
	    // tap:
			~sensors_client[\ttap] = msg[1];
            ~sensors[\ttap].set(msg[1]);
	    // troggle:
	        if (msg[1] == 1, {
				~sensors_client[\ttaptrig] = (~sensors_client[\ttaptrig]+1)%2;
				~sensors[\ttaptrig].set(~sensors_client[\ttaptrig])
		    });
	    // OSC forward
		    if (~oscForward==True,{
		        ~client.sendMsg(~osc_forward_patch+/+~osc_address[\ttap], ~sensors_client[\ttap]);
		        ~client.sendMsg(~osc_forward_patch+/+~osc_address[\ttaptrig], ~sensors_client[\ttaptrig])
		    });
	    // midi
		    if (~midi==True,{
		        m.control(chan: ~midiChannel, ctlNum: 14, val: ~sensors_client[\ttap].linlin(0, 1, 0, 127));
		        m.control(chan: ~midiChannel, ctlNum: 115, val: ~sensors_client[\ttaptrig].linlin(0, 1, 0, 127))
		    });
    	}, (~osc_path_module+/+~osc_address[\ttap]).asSymbol;
    );

	OSCdef(\count, {
		arg msg;
	    // data
    		~sensors_client[\count] = msg[1];
	    	~sensors[\count].set(msg[1]);
        // OSC forward
		    if (~oscForward==True,{~client.sendMsg(~osc_forward_patch+/+~osc_address[\count], ~sensors_client[\count])});
        // midi
		    if (~midi==True,{m.control(chan: ~midiChannel, ctlNum: 15, val: ~sensors_client[\count].linlin(0, 127, 0, 127))});
    	}, (~osc_path_module+/+~osc_address[\count]).asSymbol;
    );

    OSCdef(\touch, {
		arg msg;
	    // data
    		~sensors_client[\touch] = msg[1];
	    	~sensors[\touch].set(msg[1]);
	    // OSC forward
		    if (~oscForward==True,{~client.sendMsg(~osc_forward_patch+/+~osc_address[\touch], ~sensors_client[\touch])});
    	}, (~osc_path_module+/+~osc_address[\touch]).asSymbol;
    );

	OSCdef(\battery, {
		arg msg;
	    // data
    		~sensors_client[\battery] = msg[1];
	    	~sensors[\battery].set(msg[1]);
	    // OSC forward
		    if (~oscForward==True,{~client.sendMsg(~osc_forward_patch+/+~osc_address[\battery], ~sensors_client[\battery])});
    	}, (~osc_path_module+/+~osc_address[\battery]).asSymbol;
    );

    OSCdef(\button1, {
		arg msg;
	    // tap:
			~sensors_client[\button1] = msg[1];
            ~sensors[\button1].set(msg[1]);
	    // troggle:
	        if (msg[1] == 1, {
				~sensors_client[\button1trig] = (~sensors_client[\button1trig]+1)%2;
				~sensors[\button1trig].set(~sensors_client[\button1trig])
		    });
	    // OSC forward
		    if (~oscForward==True,{
		        ~client.sendMsg(~osc_forward_patch+/+~osc_address[\button1], ~sensors_client[\button1]);
		        ~client.sendMsg(~osc_forward_patch+/+~osc_address[\button1trig], ~sensors_client[\button1trig])
		    });
	    // midi
		    if (~midi==True,{
		        m.control(chan: ~midiChannel, ctlNum: 107, val: ~sensors_client[\button1].linlin(0, 1, 0, 127));
		        m.control(chan: ~midiChannel, ctlNum: 110, val: ~sensors_client[\button1trig].linlin(0, 1, 0, 127))
		    });
    	}, (~osc_path_spu+/+~osc_address[\button1]).asSymbol;
    );

    OSCdef(\button2, {
		arg msg;
	    // tap:
			~sensors_client[\button2] = msg[1];
            ~sensors[\button2].set(msg[1]);
	    // troggle:
	        if (msg[1] == 1, {
				~sensors_client[\button2trig] = (~sensors_client[\button2trig]+1)%2;
				~sensors[\button2trig].set(~sensors_client[\button2trig])
		    });
	    // OSC forward
		    if (~oscForward==True,{
		        ~client.sendMsg(~osc_forward_patch+/+~osc_address[\button2], ~sensors_client[\button2]);
		        ~client.sendMsg(~osc_forward_patch+/+~osc_address[\button2trig], ~sensors_client[\button2trig])
		    });
	    // midi
		    if (~midi==True,{
		        m.control(chan: ~midiChannel, ctlNum: 108, val: ~sensors_client[\button2].linlin(0, 1, 0, 127));
		        m.control(chan: ~midiChannel, ctlNum: 111, val: ~sensors_client[\button2trig].linlin(0, 1, 0, 127))
	        });
    	}, (~osc_path_spu+/+~osc_address[\button2]).asSymbol;
    );

    OSCdef(\button3, {
		arg msg;
	    // tap:
			~sensors_client[\button3] = msg[1];
            ~sensors[\button3].set(msg[1]);
	    // troggle:
	        if (msg[1] == 1, {
				~sensors_client[\button3trig] = (~sensors_client[\button3trig]+1)%2;
				~sensors[\button3trig].set(~sensors_client[\button3trig])
		    });
	    // OSC forward
		    if (~oscForward==True,{
		        ~client.sendMsg(~osc_forward_patch+/+~osc_address[\button3], ~sensors_client[\button3]);
		        ~client.sendMsg(~osc_forward_patch+/+~osc_address[\button3trig], ~sensors_client[\button3trig])
	        });
	    // midi
		    if (~midi==True,{
		        m.control(chan: ~midiChannel, ctlNum: 109, val: ~sensors_client[\button3].linlin(0, 1, 0, 127));
		        m.control(chan: ~midiChannel, ctlNum: 112, val: ~sensors_client[\button3trig].linlin(0, 1, 0, 127))
	        });
    	}, (~osc_path_spu+/+~osc_address[\button3]).asSymbol;
    );

    OSCdef(\audio_data_OSC, {
		arg msg;
	    // data
		    ~sensors_client[\onsets] = msg[1];
    	    ~sensors_client[\freq] = msg[2];
	    // OSC forward
		    if (~oscForward==True,{
		        ~client.sendMsg(~osc_forward_patch+/+~osc_address[\onsets], ~sensors_client[\onsets]);
		        ~client.sendMsg(~osc_forward_patch+/+~osc_address[\freq], ~sensors_client[\freq])
	        });
	    // midi
		    if (~midi==True, {
				m.control(chan: ~midiChannel, ctlNum: 105, val: ~sensors_client[\onsets].linlin(0, 0.1, 0, 127));
				m.control(chan: ~midiChannel, ctlNum: 106, val: ~sensors_client[\freq].linlin(0, 4000, 0, 127))
    		});
	},
	'/audio_data');

// ............................ SPU DATA RETRIEVING ............................

    ~audio_data = Synth.new(\audio_data,[input: ~guitarami_input]);
