/*
GuitarAMI-Probatio OSC patch SC version 1.1 -- Edu Meneses (IDMIL, CIRMMT, McGill University, 2021)
GuitarAMI SPU (Sound Processing Unit) v.5.0  - Rpi 4B

OSC data receiving

*/

// IMPORTANT: define the folowing global variables on the parent SC file:

/*
~probatio_id="XXX";
~osc_path_probatio = "/probatio_m5_" ++ ~id;
~osc_path_spu = "/guitarami_spu";
*/

/*
Probatio OSC message:
\probatio_m5_XXX 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0

- replace XXX by Probatio ID
- device sends single OSC message with 59 integers (floats in the emulator):

BLOCK_DUAL_BUTTONS_1 = 4--5
BLOCK_OBSF_24 = 7
BLOCK_OBSF_30  = 9
BLOCK_CHERRIES_1 = 11--14
BLOCK_DUAL_POTS_1 = 16--17
BLOCK_TAP_1 = 19
BLOCK_TAP_2 = 21
BLOCK_DUAL_PISTONS_1 = 23--24
BLOCK_JOYSTICK_1 = 26--27
BLOCK_DOF4_JOYSTICK_1 40--43
BLOCK_CRANK_1 = 50--51
BLOCK_DIAL_1 = 53--54

*/

// Buses for mapping
~probatio_blocks = Dictionary.newFrom(List[
	\dual_buttons_1a, Bus.control(s),
	\dual_buttons_1b, Bus.control(s),
	\obsf_24, Bus.control(s),
	\obsf_30, Bus.control(s),
	\cherries_1a, Bus.control(s),
	\cherries_1b, Bus.control(s),
	\cherries_1c, Bus.control(s),
	\cherries_1d, Bus.control(s),
	\dual_pots_1a, Bus.control(s),
	\dual_pots_1b, Bus.control(s),
	\tap_1, Bus.control(s),
	\tap_2, Bus.control(s),
	\dual_pistons_1a, Bus.control(s),
	\dual_pistons_1b, Bus.control(s),
	\joystick_1a, Bus.control(s),
	\joystick_1b, Bus.control(s),
	\dof4_joystick_1a, Bus.control(s),
	\dof4_joystick_1b, Bus.control(s),
	\dof4_joystick_1c, Bus.control(s),
	\dof4_joystick_1d, Bus.control(s),
	\crank_1a, Bus.control(s),
	\crank_1b, Bus.control(s),
	\dial_1a, Bus.control(s),
	\dial_1b, Bus.control(s)
]);

// Client-side variables for feedback and some data cooking
~probatio_sensors = Dictionary.newFrom(List[
	\dual_buttons_1a, 0,
	\dual_buttons_1b, 0,
	\obsf_24, 0,
	\obsf_30, 0,
	\cherries_1a, 0,
	\cherries_1b, 0,
	\cherries_1c, 0,
	\cherries_1d, 0,
	\dual_pots_1a, 0,
	\dual_pots_1b, 0,
	\tap_1, 0,
	\tap_2, 0,
	\dual_pistons_1a, 0,
	\dual_pistons_1b, 0,
	\joystick_1a, 0,
	\joystick_1b, 0,
	\dof4_joystick_1a, 0,
	\dof4_joystick_1b, 0,
	\dof4_joystick_1c, 0,
	\dof4_joystick_1d, 0,
	\crank_1a, 0,
	\crank_1b, 0,
	\dial_1a, 0,
	\dial_1b, 0
]);

s.sync;

OSCdef(\probatio_osc, {
	arg msg;
	// data
	~probatio_sensors[\dual_buttons_1a] = msg[4];
	~probatio_blocks[\dual_buttons_1a].set(msg[4]);
	~probatio_sensors[\dual_buttons_1b] = msg[5];
	~probatio_blocks[\dual_buttons_1b].set(msg[5]);
	~probatio_sensors[\obsf_24] = msg[7];
	~probatio_blocks[\obsf_24].set(msg[7]);
	~probatio_sensors[\obsf_30] = msg[9];
	~probatio_blocks[\obsf_30].set(msg[9]);
	~probatio_sensors[\cherries_1a] = msg[11];
	~probatio_blocks[\cherries_1a].set(msg[11]);
	~probatio_sensors[\cherries_1b] = msg[12];
	~probatio_blocks[\cherries_1b].set(msg[12]);
	~probatio_sensors[\cherries_1c] = msg[13];
	~probatio_blocks[\cherries_1c].set(msg[13]);
	~probatio_sensors[\cherries_1d] = msg[14];
	~probatio_blocks[\cherries_1d].set(msg[14]);
	~probatio_sensors[\dual_pots_1a] = msg[16];
	~probatio_blocks[\dual_pots_1a].set(msg[16]);
	~probatio_sensors[\dual_pots_1b] = msg[17];
	~probatio_blocks[\dual_pots_1b].set(msg[17]);
	~probatio_sensors[\tap_1] = msg[19];
	~probatio_blocks[\tap_1].set(msg[19]);
	~probatio_sensors[\tap_2] = msg[21];
	~probatio_blocks[\tap_2].set(msg[21]);
	~probatio_sensors[\dual_pistons_1a] = msg[23];
	~probatio_blocks[\dual_pistons_1a].set(msg[23]);
	~probatio_sensors[\dual_pistons_1b] = msg[24];
	~probatio_blocks[\dual_pistons_1b].set(msg[24]);
	~probatio_sensors[\joystick_1a] = msg[26];
	~probatio_blocks[\joystick_1a].set(msg[26]);
	~probatio_sensors[\joystick_1b] = msg[27];
	~probatio_blocks[\joystick_1b].set(msg[27]);
	~probatio_sensors[\dof4_joystick_1a] = msg[40];
	~probatio_blocks[\dof4_joystick_1a].set(msg[40]);
	~probatio_sensors[\dof4_joystick_1b] = msg[41];
	~probatio_blocks[\dof4_joystick_1b].set(msg[41]);
	~probatio_sensors[\dof4_joystick_1c] = msg[42];
	~probatio_blocks[\dof4_joystick_1c].set(msg[42]);
	~probatio_sensors[\dof4_joystick_1d] = msg[43];
	~probatio_blocks[\dof4_joystick_1d].set(msg[43]);
	~probatio_sensors[\crank_1a] = msg[50];
	~probatio_blocks[\crank_1a].set(msg[50]);
	~probatio_sensors[\crank_1b] = msg[51];
	~probatio_blocks[\crank_1b].set(msg[51]);
	~probatio_sensors[\dial_1a] = msg[53];
	~probatio_blocks[\dial_1a].set(msg[53]);
	~probatio_sensors[\dial_1b] = msg[54];
	~probatio_blocks[\dial_1b].set(msg[54]);
}, (~osc_path_probatio).asSymbol;
);

