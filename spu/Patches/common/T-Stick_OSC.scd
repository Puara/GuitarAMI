/*
GuitarAMI T-Stick OSC patch SC version 0.5 -- Edu Meneses (IDMIL, CIRMMT, McGill University, 2021)
GuitarAMI SPU (Sound Processing Unit) v.5.0a  - Rpi 4B

OSC data receiving

*/

// IMPORTANT: define the folowing global variables on the parent SC file:

/*
~tstick_id="XXX";
~osc_path_tstick = "/TStick_" ++ ~id;
~osc_path_spu = "/guitarami_spu";
*/

/*
T-Stick firmware 200422 - OSC sensor data

#### (Replace XXX for the T-Stick ID number)

## Raw data

/TStick_XXX/raw/capsense, i..., 0--255, ... (1 int per 8 capacitive stripes -- 8 bits)
/TStick_XXX/raw/button/short, i, 0 or 1
/TStick_XXX/raw/button/long, i, 0 or 1
/TStick_XXX/raw/button/double, i, 0 or 1
/TStick_XXX/raw/fsr, i, 0--4095
/TStick_XXX/raw/piezo, i, 0--1023
/TStick_XXX/raw/accl, iii, +/-32767 (integers)
/TStick_XXX/raw/gyro, fff, +/-34.90659 (floats)
/TStick_XXX/raw/magn, iii, +/-32767 (integers)
/raw (IMU data to be send to callibration app)

## Normalized data

/TStick_XXX/norm/fsr, f, 0--1
/TStick_XXX/norm/piezo, f, 0--1
/TStick_XXX/norm/accl, fff, +/-1, +/-1, +/-1
/TStick_XXX/norm/gyro, fff, +/-1, +/-1, +/-1
/TStick_XXX/norm/magn, fff, +/-1, +/-1, +/-1

## Instrument data

/TStick_XXX/orientation, ffff, ?, ? ,? ,?
/TStick_XXX/instrument/ypr, fff, +/-180, +/-90 ,+/-180 (degrees)
/TStick_XXX/instrument/touch/all, f, 0--1
/TStick_XXX/instrument/touch/top, f, 0--1
/TStick_XXX/instrument/touch/middle, f, 0--1
/TStick_XXX/instrument/touch/bottom, f, 0--1
/TStick_XXX/instrument/brush, f, 0--? (~cm/s)
/TStick_XXX/instrument/multibrush, ffff, 0--? (~cm/s)
/TStick_XXX/instrument/rub, f, 0--? (~cm/s)
/TStick_XXX/instrument/multirub, ffff, 0--? (~cm/s)
/TStick_XXX/instrument/shakexyz, fff, 0--?
/TStick_XXX/instrument/jabxyz, fff, 0--?
/TStick_XXX/battery

*/

// OSC addresses (GuitarAMI module)
~osc_address_tstick = Dictionary.newFrom(List[
    \capsense, "raw/capsense",
    \short, "raw/button/short",
    \long, "raw/button/long",
    \double, "raw/button/double",
    \fsr, "raw/fsr",
    \piezo, "raw/piezo",
    \accl, "raw/accl",
    \gyro, "raw/gyro",
    \magn, "raw/magn",
    \fsr_norm, "norm/fsr",
    \piezo_norm, "norm/piezo",
    \accl_norm, "norm/accl",
    \gyro_norm, "norm/gyro",
    \magn_norm, "norm/magn",
    \orientation, "orientation",
    \ypr, "instrument/ypr",
    \all, "instrument/touch/all",
    \top, "instrument/touch/top",
    \middle, "instrument/touch/middle",
    \bottom, "instrument/touch/bottom",
    \brush, "instrument/brush",
    \multibrush, "instrument/multibrush",
    \rub, "instrument/rub",
    \multirub, "instrument/multirub",
    \shakexyz, "instrument/shakexyz",
    \jabxyz, "instrument/jabxyz",
    \battery, "battery"
]);

// Buses for mapping
~sensors_tstick = Dictionary.newFrom(List[
    \capsense1, Bus.control(s),
    \capsense2, Bus.control(s),
    \capsense3, Bus.control(s),
    \capsense4, Bus.control(s),
    \capsense5, Bus.control(s),
    \capsense6, Bus.control(s),
    \capsense7, Bus.control(s),
    \capsense8, Bus.control(s),
    \btn_short, Bus.control(s),
    \btn_long, Bus.control(s),
    \btn_double, Bus.control(s),
    \fsr, Bus.control(s),
    \piezo, Bus.control(s),
    \acclx, Bus.control(s),
    \accly, Bus.control(s),
    \acclz, Bus.control(s),
    \gyrox, Bus.control(s),
    \gyroy, Bus.control(s),
    \gyroz, Bus.control(s),
    \magnx, Bus.control(s),
    \magny, Bus.control(s),
    \magnz, Bus.control(s),
    \fsr_norm, Bus.control(s),
    \piezo_norm, Bus.control(s),
    \acclx_norm, Bus.control(s),
    \accly_norm, Bus.control(s),
    \acclz_norm, Bus.control(s),
    \gyrox_norm, Bus.control(s),
    \gyroy_norm, Bus.control(s),
    \gyroz_norm, Bus.control(s),
    \magnx_norm, Bus.control(s),
    \magny_norm, Bus.control(s),
    \magnz_norm, Bus.control(s),
    \q1, Bus.control(s),
    \q2, Bus.control(s),
    \q3, Bus.control(s),
    \q4, Bus.control(s),
    \yaw, Bus.control(s),
    \pitch, Bus.control(s),
    \roll, Bus.control(s),
    \touch_all, Bus.control(s),
    \touch_top, Bus.control(s),
    \touch_middle, Bus.control(s),
    \touch_bottom, Bus.control(s),
    \brush, Bus.control(s),
    \multibrush1, Bus.control(s),
    \multibrush2, Bus.control(s),
    \multibrush3, Bus.control(s),
    \multibrush4, Bus.control(s),
    \rub, Bus.control(s),
    \multirub1, Bus.control(s),
    \multirub2, Bus.control(s),
    \multirub3, Bus.control(s),
    \multirub4, Bus.control(s),
    \shakex, Bus.control(s),
    \shakey, Bus.control(s),
    \shakez, Bus.control(s),
    \jabx, Bus.control(s),
    \jaby, Bus.control(s),
    \jabz, Bus.control(s),
    \battery, Bus.control(s)
]);

// Client-side variables for feedback and some data cooking
~sensors_client_tstick = Dictionary.newFrom(List[
    \capsense1, 0,
    \capsense2, 0,
    \capsense3, 0,
    \capsense4, 0,
    \capsense5, 0,
    \capsense6, 0,
    \capsense7, 0,
    \capsense8, 0,
    \btn_short, 0,
    \btn_long, 0,
    \btn_double, 0,
    \fsr, 0,
    \piezo, 0,
    \acclx, 0,
    \accly, 0,
    \acclz, 0,
    \gyrox, 0,
    \gyroy, 0,
    \gyroz, 0,
    \magnx, 0,
    \magny, 0,
    \magnz, 0,
    \fsr_norm, 0,
    \piezo_norm, 0,
    \acclx_norm, 0,
    \accly_norm, 0,
    \acclz_norm, 0,
    \gyrox_norm, 0,
    \gyroy_norm, 0,
    \gyroz_norm, 0,
    \magnx_norm, 0,
    \magny_norm, 0,
    \magnz_norm, 0,
    \q1, 0,
    \q2, 0,
    \q3, 0,
    \q4, 0,
    \yaw, 0,
    \pitch, 0,
    \roll, 0,
    \touch_all, 0,
    \touch_top, 0,
    \touch_middle, 0,
    \touch_bottom, 0,
    \brush, 0,
    \multibrush1, 0,
    \multibrush2, 0,
    \multibrush3, 0,
    \multibrush4, 0,
    \rub, 0,
    \multirub1, 0,
    \multirub2, 0,
    \multirub3, 0,
    \multirub4, 0,
    \shakex, 0,
    \shakey, 0,
    \shakez, 0,
    \jabx, 0,
    \jaby, 0,
    \jabz, 0,
    \battery, 0
]);

s.sync;

OSCdef(\capsense, {
    arg msg; var size;
    size = msg.size;
    if (size > 0, {
        ~sensors_client_tstick[\capsense1] = msg[1];
        ~sensors_tstick[\capsense1].set(msg[1]);});
    if (size > 1, {
        ~sensors_client_tstick[\capsense2] = msg[2];
        ~sensors_tstick[\capsense2].set(msg[2]);});
    if (size > 2, {
        ~sensors_client_tstick[\capsense3] = msg[3];
        ~sensors_tstick[\capsense3].set(msg[3]);});
    if (size > 3, {
        ~sensors_client_tstick[\capsense4] = msg[4];
        ~sensors_tstick[\capsense4].set(msg[4]);});
    if (size > 4, {
        ~sensors_client_tstick[\capsense5] = msg[5];
        ~sensors_tstick[\capsense5].set(msg[5]);});
    if (size > 5, {
        ~sensors_client_tstick[\capsense6] = msg[6];
        ~sensors_tstick[\capsense6].set(msg[6]);});
    if (size > 6, {
        ~sensors_client_tstick[\capsense7] = msg[7];
        ~sensors_tstick[\capsense7].set(msg[7]);});
    if (size > 7, {
        ~sensors_client_tstick[\capsense8] = msg[8];
        ~sensors_tstick[\capsense8].set(msg[8]);});
}, (~osc_path_tstick+/+~osc_address_tstick[\capsense]).asSymbol;
);

OSCdef(\short, {
    arg msg; var size;
    ~sensors_client_tstick[\btn_short] = msg[1];
    ~sensors_tstick[\btn_short].set(msg[1]);
}, (~osc_path_tstick+/+~osc_address_tstick[\short]).asSymbol;
);

OSCdef(\long, {
    arg msg; var size;
    ~sensors_client_tstick[\btn_long] = msg[1];
    ~sensors_tstick[\btn_long].set(msg[1]);
}, (~osc_path_tstick+/+~osc_address_tstick[\long]).asSymbol;
);

OSCdef(\double, {
    arg msg; var size;
    ~sensors_client_tstick[\btn_double] = msg[1];
    ~sensors_tstick[\btn_double].set(msg[1]);
}, (~osc_path_tstick+/+~osc_address_tstick[\double]).asSymbol;
);

OSCdef(\fsr, {
    arg msg; var size;
    ~sensors_client_tstick[\fsr] = msg[1];
    ~sensors_tstick[\fsr].set(msg[1]);
}, (~osc_path_tstick+/+~osc_address_tstick[\fsr]).asSymbol;
);

OSCdef(\piezo, {
    arg msg; var size;
    ~sensors_client_tstick[\piezo] = msg[1];
    ~sensors_tstick[\piezo].set(msg[1]);
}, (~osc_path_tstick+/+~osc_address_tstick[\piezo]).asSymbol;
);

OSCdef(\accl, {
    arg msg; var size;
    ~sensors_client_tstick[\acclx] = msg[1];
    ~sensors_tstick[\acclx].set(msg[1]);
    ~sensors_client_tstick[\accly] = msg[2];
    ~sensors_tstick[\accly].set(msg[2]);
    ~sensors_client_tstick[\acclz] = msg[3];
    ~sensors_tstick[\acclz].set(msg[3]);
}, (~osc_path_tstick+/+~osc_address_tstick[\accl]).asSymbol;
);

OSCdef(\gyro, {
    arg msg; var size;
    ~sensors_client_tstick[\gyrox] = msg[1];
    ~sensors_tstick[\gyrox].set(msg[1]);
    ~sensors_client_tstick[\gyroy] = msg[2];
    ~sensors_tstick[\gyroy].set(msg[2]);
    ~sensors_client_tstick[\gyroz] = msg[3];
    ~sensors_tstick[\gyroz].set(msg[3]);
}, (~osc_path_tstick+/+~osc_address_tstick[\gyro]).asSymbol;
);

OSCdef(\magn, {
    arg msg; var size;
    ~sensors_client_tstick[\magnx] = msg[1];
    ~sensors_tstick[\magnx].set(msg[1]);
    ~sensors_client_tstick[\magny] = msg[2];
    ~sensors_tstick[\magny].set(msg[2]);
    ~sensors_client_tstick[\magnz] = msg[3];
    ~sensors_tstick[\magnz].set(msg[3]);
}, (~osc_path_tstick+/+~osc_address_tstick[\magn]).asSymbol;
);

OSCdef(\fsr_norm, {
    arg msg; var size;
    ~sensors_client_tstick[\fsr_norm] = msg[1];
    ~sensors_tstick[\fsr_norm].set(msg[1]);
}, (~osc_path_tstick+/+~osc_address_tstick[\fsr_norm]).asSymbol;
);

OSCdef(\piezo_norm, {
    arg msg; var size;
    ~sensors_client_tstick[\piezo_norm] = msg[1];
    ~sensors_tstick[\piezo_norm].set(msg[1]);
}, (~osc_path_tstick+/+~osc_address_tstick[\piezo_norm]).asSymbol;
);

OSCdef(\accl_norm, {
    arg msg; var size;
    ~sensors_client_tstick[\acclx_norm] = msg[1];
    ~sensors_tstick[\acclx_norm].set(msg[1]);
    ~sensors_client_tstick[\accly_norm] = msg[2];
    ~sensors_tstick[\accly_norm].set(msg[2]);
    ~sensors_client_tstick[\acclz_norm] = msg[3];
    ~sensors_tstick[\acclz_norm].set(msg[3]);
}, (~osc_path_tstick+/+~osc_address_tstick[\accl_norm]).asSymbol;
);

OSCdef(\gyro_norm, {
    arg msg; var size;
    ~sensors_client_tstick[\gyrox_norm] = msg[1];
    ~sensors_tstick[\gyrox_norm].set(msg[1]);
    ~sensors_client_tstick[\gyroy_norm] = msg[2];
    ~sensors_tstick[\gyroy_norm].set(msg[2]);
    ~sensors_client_tstick[\gyroz_norm] = msg[3];
    ~sensors_tstick[\gyroz_norm].set(msg[3]);
}, (~osc_path_tstick+/+~osc_address_tstick[\gyro_norm]).asSymbol;
);

OSCdef(\magn_norm, {
    arg msg; var size;
    ~sensors_client_tstick[\magnx_norm] = msg[1];
    ~sensors_tstick[\magnx_norm].set(msg[1]);
    ~sensors_client_tstick[\magny_norm] = msg[2];
    ~sensors_tstick[\magny_norm].set(msg[2]);
    ~sensors_client_tstick[\magnz_norm] = msg[3];
    ~sensors_tstick[\magnz_norm].set(msg[3]);
}, (~osc_path_tstick+/+~osc_address_tstick[\magn_norm]).asSymbol;
);

OSCdef(\orientation, {
    arg msg; var size;
    ~sensors_client_tstick[\q1] = msg[1];
    ~sensors_tstick[\q1].set(msg[1]);
    ~sensors_client_tstick[\q2] = msg[2];
    ~sensors_tstick[\q2].set(msg[2]);
    ~sensors_client_tstick[\q3] = msg[3];
    ~sensors_tstick[\q3].set(msg[3]);
    ~sensors_client_tstick[\q4] = msg[4];
    ~sensors_tstick[\q4].set(msg[4]);
}, (~osc_path_tstick+/+~osc_address_tstick[\orientation]).asSymbol;
);

OSCdef(\ypr, {
    arg msg; var size;
    ~sensors_client_tstick[\yaw] = msg[1];
    ~sensors_tstick[\yaw].set(msg[1]);
    ~sensors_client_tstick[\pitch] = msg[2];
    ~sensors_tstick[\pitch].set(msg[2]);
    ~sensors_client_tstick[\roll] = msg[3];
    ~sensors_tstick[\roll].set(msg[3]);
}, (~osc_path_tstick+/+~osc_address_tstick[\ypr]).asSymbol;
);

OSCdef(\piezo_norm, {
    arg msg; var size;
    ~sensors_client_tstick[\piezo_norm] = msg[1];
    ~sensors_tstick[\piezo_norm].set(msg[1]);
}, (~osc_path_tstick+/+~osc_address_tstick[\piezo_norm]).asSymbol;
);

OSCdef(\all, {
    arg msg; var size;
    ~sensors_client_tstick[\touch_all] = msg[1];
    ~sensors_tstick[\touch_all].set(msg[1]);
}, (~osc_path_tstick+/+~osc_address_tstick[\all]).asSymbol;
);

OSCdef(\top, {
    arg msg; var size;
    ~sensors_client_tstick[\touch_top] = msg[1];
    ~sensors_tstick[\touch_top].set(msg[1]);
}, (~osc_path_tstick+/+~osc_address_tstick[\top]).asSymbol;
);

OSCdef(\middle, {
    arg msg; var size;
    ~sensors_client_tstick[\touch_middle] = msg[1];
    ~sensors_tstick[\touch_middle].set(msg[1]);
}, (~osc_path_tstick+/+~osc_address_tstick[\middle]).asSymbol;
);

OSCdef(\bottom, {
    arg msg; var size;
    ~sensors_client_tstick[\touch_bottom] = msg[1];
    ~sensors_tstick[\touch_bottom].set(msg[1]);
}, (~osc_path_tstick+/+~osc_address_tstick[\bottom]).asSymbol;
);

OSCdef(\brush, {
    arg msg; var size;
    ~sensors_client_tstick[\brush] = msg[1];
    ~sensors_tstick[\brush].set(msg[1]);
}, (~osc_path_tstick+/+~osc_address_tstick[\brush]).asSymbol;
);

OSCdef(\multibrush, {
    arg msg; var size;
    ~sensors_client_tstick[\multibrush1] = msg[1];
    ~sensors_tstick[\multibrush1].set(msg[1]);
    ~sensors_client_tstick[\multibrush2] = msg[2];
    ~sensors_tstick[\multibrush2].set(msg[2]);
    ~sensors_client_tstick[\multibrush3] = msg[3];
    ~sensors_tstick[\multibrush3].set(msg[3]);
    ~sensors_client_tstick[\multibrush4] = msg[4];
    ~sensors_tstick[\multibrush4].set(msg[4]);
}, (~osc_path_tstick+/+~osc_address_tstick[\multibrush]).asSymbol;
);

OSCdef(\rub, {
    arg msg; var size;
    ~sensors_client_tstick[\rub] = msg[1];
    ~sensors_tstick[\rub].set(msg[1]);
}, (~osc_path_tstick+/+~osc_address_tstick[\rub]).asSymbol;
);

OSCdef(\multirub, {
    arg msg; var size;
    ~sensors_client_tstick[\multirub1] = msg[1];
    ~sensors_tstick[\multirub1].set(msg[1]);
    ~sensors_client_tstick[\multirub2] = msg[2];
    ~sensors_tstick[\multirub2].set(msg[2]);
    ~sensors_client_tstick[\multirub3] = msg[3];
    ~sensors_tstick[\multirub3].set(msg[3]);
    ~sensors_client_tstick[\multibrush4] = msg[4];
    ~sensors_tstick[\multirub4].set(msg[4]);
}, (~osc_path_tstick+/+~osc_address_tstick[\multirub]).asSymbol;
);

OSCdef(\shake, {
    arg msg; var size;
    ~sensors_client_tstick[\shakex] = msg[1];
    ~sensors_tstick[\shakex].set(msg[1]);
    ~sensors_client_tstick[\shakey] = msg[2];
    ~sensors_tstick[\shakey].set(msg[2]);
    ~sensors_client_tstick[\shakez] = msg[3];
    ~sensors_tstick[\shakez].set(msg[3]);
}, (~osc_path_tstick+/+~osc_address_tstick[\shakexyz]).asSymbol;
);

OSCdef(\jab, {
    arg msg; var size;
    ~sensors_client_tstick[\jabx] = msg[1];
    ~sensors_tstick[\jabx].set(msg[1]);
    ~sensors_client_tstick[\jaby] = msg[2];
    ~sensors_tstick[\jaby].set(msg[2]);
    ~sensors_client_tstick[\jabz] = msg[3];
    ~sensors_tstick[\jabz].set(msg[3]);
}, (~osc_path_tstick+/+~osc_address_tstick[\jabxyz]).asSymbol;
);

OSCdef(\battery, {
    arg msg; var size;
    ~sensors_client_tstick[\battery] = msg[1];
    ~sensors_tstick[\battery].set(msg[1]);
}, (~osc_path_tstick+/+~osc_address_tstick[\battery]).asSymbol;
);
