// Simple player (based on SC's Buffer help file)

(
SynthDef(\player, {
	arg output=0, audio_buffer, play=0, restart=0, loop=0, gate=1;
	Out.ar( output, PlayBuf.ar(1, audio_buffer, BufRateScale.kr(audio_buffer)*play, restart, 0.0, loop) * gate);
}).add;

s.sync;
)

/*
To load a file into a buffer:

    p = Platform.resourceDir +/+ "sounds/a11wlk01.wav";
    b = Buffer.read(s, p);

To start the synth:

    ~player = Synth.new(\player, [
        audio_buffer: b
    ]);

To play again if finished, set restart to 1. Don't forget to set to 0 again if you want to restart multiple times.
*/



