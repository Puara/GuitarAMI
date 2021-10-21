/*
Fold Synth version 0.85 (2018-2021) -- Edu Meneses (IDMIL, CIRMMT, McGill University)

This code is based on SuperCollider Tutorial: 12. Nintendo Wiimote

Eli Fieldsteel
Published on Sep 1, 2014
https://youtu.be/JRIUq-46V6M
*/

(

/*
%%%%%%                                       %%%%%%
%%%%%%%%%%%                             %%%%%%%%%%%
%%%%%%%%%%%%%%%%%   SYNTH DEFs    %%%%%%%%%%%%%%%%%
%%%%%%%%%%%                             %%%%%%%%%%%
%%%%%%                                       %%%%%%
*/

/* ................ FOLD SYNTH ....................
Parameters:
\freq (between 2 and 8)
\shape (0 = sawtooth, 1 = sine, 2 = triangular)
\maxrq (between 0.2 and 0.5)
\detune (between 1 and 80)
\boost (between 1.5 and 3)
*/

SynthDef.new(\foldsynth, {
	arg shape=0, freq=4, detune=1, atk=0.01, dec=0.3, rel=0.75, c1=1, c2=(-1), gate=1, mincf=40, maxcf=12000, minrq=0.002, maxrq=0.2, boost=1.25, amp=1, output=0;
	var sig, env, noiseramp;
	env = EnvGen.kr(Env.adsr(atk, dec, 0.5, rel),gate,doneAction:2);
	noiseramp = LFNoise1.kr(LFNoise1.kr(0.5!8).range(0.1,1)).bipolar(detune);
	sig = Select.ar(shape, [
		Saw.ar(freq + noiseramp),
		SinOsc.ar(freq + noiseramp),
		LFTri.ar(freq + noiseramp)
	]);
	5.do{sig = (sig*boost).fold(-1.0,1.0)};
	sig = BPF.ar(
		sig,
		LFNoise1.kr({ExpRand(0.005,0.05)}!8).exprange(mincf,maxcf),
		LFNoise1.kr({ExpRand(0.1,0.75)}!8).range(minrq,maxrq)
	);
	sig = Splay.ar(sig) * env * amp * 2;
	Out.ar(output,sig);
}).add;

)

