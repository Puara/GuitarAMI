

// Infinite Reverb original code: Eli Fieldsteel ("https://www.youtube.com/watch?v=_2N7lG5uzJI")
// (volume control added)

SynthDef(\infinitereverb, {
	arg feedback=0.95, input=0, output=0, volume=1, gate=0;
	var local, sig, soundout;
	sig = SoundIn.ar(input);
	local = LocalIn.ar(1)+sig;
	15.do{local = AllpassN.ar(local, 0.06, Rand(0.001,0.06), 3)}; // apply reverb
	LocalOut.ar(local*feedback); // complete feedback loop -- send audio back to LocalIn
	soundout = (sig+local) * volume;
	Out.ar(output, Pan2.ar(soundout * EnvGen.kr(Env.asr(0.5,1,1), gate)));
}).add;

s.sync;

/*

// ............................ COMMANDS TO MAP............................

~infinitereverb.set(
	\gate, ,
	\feedback, ,
);

*/