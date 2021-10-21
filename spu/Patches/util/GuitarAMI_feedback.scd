/*
GuitarAMI - Visual Feedback for sensors -- Edu Meneses
Run on GuitarAMI Processing Unit (prototype5), made with Rpi 4B
*/


//s = Server.local;
s.waitForBoot{
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

	~lcd.sendMsg('/lcd', "GuitarAMI:          ", 1, 1);
	~lcd.sendMsg('/lcd', "Visual Feedback     ", 2, 1);
	~lcd.sendMsg('/lcd', "[===               ]", 3, 1);
	~lcd.sendMsg('/lcd', "          ", 4, 1);

	(thisProcess.nowExecutingPath.dirname +/+ "common/GuitarAMI_OSC.scd").load;

	~lcd.sendMsg('/lcd', "===", 4, 2);

w = Window.new("GuitarAMI Visual Feedback", Rect(1147.0, 222.0, 450.0, 693.0)).front.alwaysOnTop_(true);
//w.view.decorator_(FlowLayout(w.bounds, 5@5, 5@5));

~feedback = Dictionary.newFrom(List[
	\ult, Slider(w, Rect(20, 45, 211, 30)),    // (in mm)
	\tap, Button(w, Rect(20, 110, 30, 30)).states_([
		["",Color.gray(0.8),Color.gray(0.8)],
		["",Color.gray(0.8),Color.blue]
	]),
	\dtap, Button(w, Rect(60, 110, 30, 30)).states_([
		["",Color.gray(0.8),Color.gray(0.8)],
		["",Color.gray(0.8),Color.blue]
	]),
	\ttap, Button(w, Rect(100, 110, 30, 30)).states_([
		["",Color.gray(0.8),Color.gray(0.8)],
		["",Color.gray(0.8),Color.blue]
	]),
	\onsets, Button(w, Rect(140, 110, 30, 30)).states_([
		["",Color.gray(0.8),Color.gray(0.8)],
		["",Color.gray(0.8),Color.blue]
	]),
	\count, NumberBox(w, Rect(180, 110, 50, 30)),
	\quatI, NumberBox(w, Rect(249, 63, 90, 30)),
	\quatJ, NumberBox(w, Rect(340, 63, 90, 30)),
	\quatK, NumberBox(w, Rect(249, 115, 90, 30)),
	\quatReal, NumberBox(w, Rect(340, 115, 90, 30)),
	\button1, Button(w, Rect(20, 175, 30, 30)).states_([
		["",Color.gray(0.8),Color.gray(0.8)],
		["",Color.gray(0.8),Color.blue]
	]),
	\button2, Button(w, Rect(60, 175, 30, 30)).states_([
		["",Color.gray(0.8),Color.gray(0.8)],
		["",Color.gray(0.8),Color.blue]
	]),
	\button3, Button(w, Rect(100, 175, 30, 30)).states_([
		["",Color.gray(0.8),Color.gray(0.8)],
		["",Color.gray(0.8),Color.blue]
	]),
	\freq, NumberBox(w, Rect(140, 175, 90, 30)),
	\touch, NumberBox(w, Rect(249, 175, 50, 30)),
	\battery, NumberBox(w, Rect(310, 175, 50, 30)), // (percentage)
	\accl, Slider2D(w, Rect(20, 240, 200, 200)).knobColor_(Color.gray(0.9)),  // (in m/s^2)
	\acclX, NumberBox(w, Rect(23, 407, 50, 30)).background_(Color.gray(0.76)).decimals_(4).normalColor_(Color.gray(0.6)),
	\acclY, NumberBox(w, Rect(95, 407, 50, 30)).background_(Color.gray(0.76)).decimals_(4).normalColor_(Color.gray(0.6)),
	\acclZ, NumberBox(w, Rect(167, 407, 50, 30)).background_(Color.gray(0.76)).decimals_(4).normalColor_(Color.gray(0.6)),
	\gyro, Slider2D(w, Rect(230, 240, 200, 200)).knobColor_(Color.gray(0.9)), // (in radians per second)
	\gyroX, NumberBox(w, Rect(210+23, 407, 50, 30)).background_(Color.gray(0.76)).decimals_(4).normalColor_(Color.gray(0.6)),
	\gyroY, NumberBox(w, Rect(210+95, 407, 50, 30)).background_(Color.gray(0.76)).decimals_(4).normalColor_(Color.gray(0.6)),
	\gyroZ, NumberBox(w, Rect(210+167, 407, 50, 30)).background_(Color.gray(0.76)).decimals_(4).normalColor_(Color.gray(0.6)),
	\ypr, MultiSliderView(w, Rect(20, 475, 99, 200)).size_(3).isFilled_(true).indexThumbSize_(30.0),    // (in degrees)
	\jab, MultiSliderView(w, Rect(124, 475, 99, 200)).size_(3).isFilled_(true).indexThumbSize_(30.0),
	\shake, MultiSliderView(w, Rect(228, 475, 99, 200)).size_(3).isFilled_(true).indexThumbSize_(30.0),
	\magn, MultiSliderView(w, Rect(332, 475, 99, 200)).size_(3).isFilled_(true).indexThumbSize_(30.0) // (in uTesla)
]);

~lcd.sendMsg('/lcd', "====", 3, 5);

~label = Dictionary.newFrom(List[
	\ult, StaticText(w, Rect(20,20,200,30)).string_("Distance (ultrasonic)").font_("Monaco",14),
	\tap, StaticText(w, Rect(20,85,30,30)).string_("Tap").font_("Monaco",14).align_(\center),
	\dtap, StaticText(w, Rect(60,85,30,30)).string_("DTap").font_("Monaco",14).align_(\center),
	\ttap, StaticText(w, Rect(100,85,30,30)).string_("TTap").font_("Monaco",14).align_(\center),
	\onsets, StaticText(w, Rect(140,85,30,30)).string_("Att").font_("Monaco",14).align_(\center),
	\count, StaticText(w, Rect(180,85,50,30)).string_("Count").font_("Monaco",14),
	\touch, StaticText(w, Rect(249,150,50,30)).string_("Touch").font_("Monaco",14),
	\quat, StaticText(w, Rect(249,20,90,30)).string_("Quaternions").font_("Monaco",14),
	\quatI, StaticText(w, Rect(249,38,50,30)).string_("I").font_("Monaco",14),
	\quatJ, StaticText(w, Rect(340,38,50,30)).string_("J").font_("Monaco",14),
	\quatK, StaticText(w, Rect(249,90,50,30)).string_("K").font_("Monaco",14),
	\quatR, StaticText(w, Rect(340,90,50,30)).string_("Real").font_("Monaco",14),
	\b1, StaticText(w, Rect(20,150,30,30)).string_("B1").font_("Monaco",14).align_(\center),
	\b2, StaticText(w, Rect(60,150,30,30)).string_("B2").font_("Monaco",14).align_(\center),
	\b3, StaticText(w, Rect(100,150,30,30)).string_("B3").font_("Monaco",14).align_(\center),
	\freq, StaticText(w, Rect(140,150,90,30)).string_("Frequency").font_("Monaco",14),
	\battery, StaticText(w, Rect(310,150,90,30)).string_("Battery").font_("Monaco",14),
	\perc, StaticText(w, Rect(365,175,90,30)).string_("%").font_("Monaco",14),
	\accl, StaticText(w, Rect(20,215,200,30)).string_("Accelerometer").font_("Monaco",14),
	\gyro, StaticText(w, Rect(230,215,200,30)).string_("Gyroscope").font_("Monaco",14),
	\ypr, StaticText(w, Rect(22,450,200,30)).string_("YPR").font_("Monaco",14),
	\jab, StaticText(w, Rect(126,450,200,30)).string_("Jab").font_("Monaco",14),
	\shake, StaticText(w, Rect(230,450,200,30)).string_("Shake").font_("Monaco",14),
	\magn, StaticText(w, Rect(334,450,99,30)).string_("Magnetometer").font_("Monaco",14)
]);

~lcd.sendMsg('/lcd', "===", 3, 9);

AppClock.sched(0.0,{
	~feedback[\ult].value = ~sensors_client[\ult].linlin(-4,4,0,1);
	~feedback[\tap].value = ~sensors_client[\tap];
	~feedback[\dtap].value = ~sensors_client[\dtap];
	~feedback[\ttap].value = ~sensors_client[\ttap];
	~feedback[\onsets].value = ~sensors_client[\onsets];
	~feedback[\count].value = ~sensors_client[\count];
	~feedback[\touch].value = ~sensors_client[\touch];
	~feedback[\quatI].value = ~sensors_client[\quatI].linlin(-1,1,0,1);
	~feedback[\quatJ].value = ~sensors_client[\quatJ].linlin(-1,1,0,1);
	~feedback[\quatK].value = ~sensors_client[\quatK].linlin(-1,1,0,1);
	~feedback[\quatReal].value = ~sensors_client[\quatReal].linlin(-1,1,0,1);
	~feedback[\button1].value = ~sensors_client[\button1];
	~feedback[\button2].value = ~sensors_client[\button2];
	~feedback[\button3].value = ~sensors_client[\button3];
	~feedback[\freq].value = ~sensors_client[\freq];
	~feedback[\battery].value = ~sensors_client[\battery];
	~feedback[\accl].x_(~sensors_client[\acclX].linlin(-20,20,0,1));
	~feedback[\acclX].value = ~sensors_client[\acclX];
	~feedback[\acclY].value = ~sensors_client[\acclY];
	~feedback[\acclZ].value = ~sensors_client[\acclZ];
	~feedback[\accl].y_(~sensors_client[\acclY].linlin(-20,20,0,1));
	~feedback[\accl].knobColor_(Color.gray(~sensors_client[\acclZ].linlin(-20,20,0,1)));
	~feedback[\gyro].x_(~sensors_client[\gyroX].linlin(-5,5,0,1));
	~feedback[\gyro].y_(~sensors_client[\gyroY].linlin(-5,5,0,1));
	~feedback[\gyro].knobColor_(Color.gray(~sensors_client[\gyroZ].linlin(-5,5,0,1)));
	~feedback[\gyroX].value = ~sensors_client[\gyroX];
	~feedback[\gyroY].value = ~sensors_client[\gyroY];
	~feedback[\gyroZ].value = ~sensors_client[\gyroZ];
	~feedback[\ypr].value = [~sensors_client[\yaw].linlin(-180,180,0,1),~sensors_client[\pitch].linlin(-180,180,0,1),~sensors_client[\roll].linlin(-180,180,0,1)];
	~feedback[\jab].value = [~sensors_client[\jabX].linlin(0,10,0,1),~sensors_client[\jabY].linlin(0,10,0,1),~sensors_client[\jabZ].linlin(0,10,0,1)];
	~feedback[\shake].value = [~sensors_client[\shakeX].linlin(0,4,0,1),~sensors_client[\shakeY].linlin(0,4,0,1),~sensors_client[\shakeZ].linlin(0,4,0,1)];
	~feedback[\magn].value = [~sensors_client[\magnX].linlin(0,0.1,0,1),~sensors_client[\magnY].linlin(0,0.1,0,1),~sensors_client[\magnZ].linlin(0,0.1,0,1)];
	0.15;
	});

// ............................ SEND INFO TO LCD ............................

	// done LCD message
	~lcd.sendMsg('/lcd', "=========", 3, 12);
	~lcd.sendMsg('/lcd', "Loading complete", 4, 1);
	(1.0).wait;
	~lcd.sendMsg('/lcd', "     Visual feedback", 1, 1);
	~lcd.sendMsg('/lcd', "                    ", 2, 1);
	~lcd.sendMsg('/lcd', "Please connect      ", 3, 1);
	~lcd.sendMsg('/lcd', "using VNC        ", 4, 1);

)
}

//AppClock.clear;
