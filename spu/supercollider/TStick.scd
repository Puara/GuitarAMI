/*
TStick SC synth/mapping version 0.8 -- Edu Meneses
Run on IDMIL SPU 002 (Sound Processing Unit) - Rpi 3B

This code is based on SuperCollider Tutorial: 12. Nintendo Wiimote

Eli Fieldsteel
Published on Sep 1, 2014
https://youtu.be/JRIUq-46V6M
*/

//s = Server.local;

(

// Server.local.options.numOutputBusChannels = 8;    // change number of input and$
// Server.local.options.numInputBusChannels = 8;
// Server.internal.options.numOutputBusChannels = 8;
// Server.internal.options.numInputBusChannels = 8;

s.waitForBoot{

/*OSCdef.freeAll;

1.wait;*/


// ............................ Global variables ............................

	~tstick = NetAddr.new("192.168.21.107", 8888);    // NetAddr T-Stick

	~lcd = NetAddr.new("127.0.0.1", 20000); // Setting LCD - Loopback NetAddr and Python code listening port

	~tstick_output = 0;

	~tstick_state = Array.fill(16,0);
	~tstick_laststate = Array.fill(16,0);
	~tstick_synth = Array.fill(16,0);
	//~tstick_reduced = Array.fill(3,0); // reducing cap sensing to controls 4 variables in \addsaw
	//~tstick_lastreduced = Array.fill(3,0);
	~tstick_group = Group.new;

	~lcd.sendMsg('/lcd', "GuitarAMI:          ", 1, 1);
	~lcd.sendMsg('/lcd', "Loading T-Stick demo", 2, 1);
	~lcd.sendMsg('/lcd', "[                  ]", 3, 1);

1.wait;

// ............................ SYNTH DEF ............................

/*
%%%%%%                                           %%%%%%
%%%%%%%%%%%                                 %%%%%%%%%%%
%%%%%%%%%%%%%%%%%   T-STICK FOLDSAW   %%%%%%%%%%%%%%%%%
%%%%%%%%%%%                                 %%%%%%%%%%%
%%%%%%                                           %%%%%%
*/

	SynthDef.new(\foldsaw, {
		arg freq=4, detune=1, atk=0.01, dec=0.3, rel=3, c1=1, c2=(-1), gate=1, mincf=40, maxcf=12000, minrq=0.002, maxrq=0.2, boost=1.25, amp=1, output=0;
		var sig, env;
		env = EnvGen.kr(Env.adsr(atk, dec, 0.5, rel),gate,doneAction:1);
		sig = Saw.ar(
			freq +
			LFNoise1.kr(LFNoise1.kr(0.5!8).range(0.1,1)).bipolar(detune)
		);
		5.do{sig = (sig*boost).fold(-1.0,1.0)};
		sig = BPF.ar(
			sig,
			LFNoise1.kr({ExpRand(0.005,0.05)}!8).exprange(mincf,maxcf),
			LFNoise1.kr({ExpRand(0.1,0.75)}!8).range(minrq,maxrq)
		);
		sig = Splay.ar(sig) * env * amp * 2;
		Out.ar(output,sig);
	}).add;

	~lcd.sendMsg('/lcd', "===", 3, 2);

/*
%%%%%%                                                 %%%%%%
%%%%%%%%%%%                                       %%%%%%%%%%%
%%%%%%%%%%%%%%%%%   T-STICK CHEAP FOLDSAW   %%%%%%%%%%%%%%%%%
%%%%%%%%%%%                                       %%%%%%%%%%%
%%%%%%                                                 %%%%%%
*/

	SynthDef.new(\cheapfoldsaw, {
		arg freq=4, detune=1, atk=0.01, dec=0.3, rel=3, c1=1, c2=(-1), gate=1, mincf=40, maxcf=12000, minrq=0.002, maxrq=0.2, boost=1.25, amp=1, output=0;
		var sig, env;
		env = EnvGen.kr(Env.adsr(atk, dec, 0.5, rel),gate,doneAction:1);
		sig = Saw.ar(
			freq +
			LFNoise1.kr(LFNoise1.kr(0.5!1).range(0.1,1)).bipolar(detune)
		);
		5.do{sig = (sig*boost).fold(-1.0,1.0)};
		sig = BPF.ar(
			sig,
			LFNoise1.kr({ExpRand(0.005,0.05)}!1).exprange(mincf,maxcf),
			LFNoise1.kr({ExpRand(0.1,0.75)}!1).range(minrq,maxrq)
		);
		sig = Splay.ar(sig) * env * amp * 2;
		Out.ar(output,sig);
	}).add;

	~lcd.sendMsg('/lcd', "====", 3, 5);

// ............................ STARTING SYNTHS ............................

0.4.wait;

for (0,15,{ // define number of Synths
		arg i;
		~tstick_synth[i] = Synth.newPaused(\cheapfoldsaw,[\freq, exprand(1,8), \atk, exprand(2,8), \rel, exprand(2,8), \output, ~tstick_output],~tstick_group);
	}
);

~lcd.sendMsg('/lcd', "====", 3, 9);

// ............................ SYNTH COMMANDS ............................


     // start synth
//          ~tstick_synth[i].run;
     // stop synth
//          ~tstick_synth[i].run(false);



// ............................ DATA RECEIVING AND MAPPING ............................


// receiving T-Stick OSC accelerometer data and mapping to \foldsaw
OSCdef.new(\accelerometer, {
	arg msg;
	~tstick_group.set(
		\detune, msg[1].linexp(-32000,32000,0.001,80), // pitch do acelerômetro
		\maxrq, msg[2].linexp(-32000,32000,0.01,0.5), // roll do acelerômetro
	);
    }, 'TStick_202/raw/accl', recvPort:8000
);

// receiving T-Stick OSC gyroscope data and mapping to \foldsaw
OSCdef.new(\gyros, {
	arg msg;
	var temp;
	temp = msg[1].fold(0,41) + msg[2].fold(0,41) + msg[3].fold(0,41);
	~tstick_group.set(
		\boost, temp.linlin(0,41,1,16), // aceleração do acelerômetro
	);
	//msg.postln;
    }, 'TStick_201/raw/gyro', recvPort:8000
);

// running and pausing T-Stick synths
OSCdef.new(\addsaw, {
		arg msg, temp, reduced;
			temp = msg[1].asBinaryDigits ++ msg[2].asBinaryDigits;
			for (0, 15, { arg i;
			    if(~tstick_state[i] != temp[i],
				    {
					~tstick_state[i]=temp[i];
					if(~tstick_state[i]==1,
					    {~tstick_synth[i].run.set(\gate,1);},
					    {~tstick_synth[i].set(\gate,0);});
					}
				);
			});

    }, 'TStick_201/raw/capsense', recvPort:8000
);

~lcd.sendMsg('/lcd', "====", 3, 13);

// ............................ T-STICK HANDSHAKING ............................

OSCdef.new(\tstickreturn, {
	arg msg;
	~tstickstatus = msg[1];
    }, '/information'
);

~tstickroutine = Task(
	{
		{
			~tstick.sendMsg('/status', 115);    // send the OSC message
			1.wait;
			if(~tstickstatus==181,{~lcd.sendMsg('/lcd',"T", 0, 15)});
			if(~tstickstatus==0,{~lcd.sendMsg('/lcd'," ", 0, 15)});
			~tstickstatus = 0;
		}.loop
	}
).start;

~lcd.sendMsg('/lcd', "===", 3, 17);

// ............................ CHIME ............................

	0.4.wait;

	(dur:1, strum: 0.1, degree: [0,4,7]).play;

	~lcd.sendMsg('/lcd', "Loading complete    ", 2, 1);
	(1.0).wait;

	AppClock.sched(0.0,{
		~lcd.sendMsg('/lcd', "        T-Stick demo", 1, 1);
		10; // delay between LCD updates
	});

}; // end of "waitForBoot"
)

// DEBUG commands:

// ~debug = NetAddr.new("192.168.5.2", 57120);    // NetAddr Mac Pro
// s.queryAllNodes;
