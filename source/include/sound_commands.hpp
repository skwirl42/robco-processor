#pragma once

#include <stdint.h>
#include <string>

// First byte in a command has its 4 MSbs set to the command type,
// the 4 LSbs specify an ID, if appropriate for the command
// set voice volume - LSbs: voice ID - 1 byte following (volume)
// set voice envelope - LSbs: voice ID - 4 bytes following (ADSR), 1 byte start amplitude, 1 byte max life
// set voice oscillator - LSbs: voice ID ->
//   - 1 byte MSbs: oscillator ID, LSbs: oscillator type
//   - 2 bytes following (oscillator octave, oscillator amplitude)
//   - 1 byte (MSbs LFO frequency, LSbs LFO amplitude)
// play note - LSbs: voice ID - one shot play, 1 byte following (MSbs: octave, LSbs: note)
// hold note - LSbs: voice ID - held until released, 1 byte following (MSbs: octave, LSbs: note)
// release note - LSbs: voice ID - releases the note being played on a voice

const int oscillators_per_voice = 4;
const float time_multiplier = 1000000;
const float lfo_amplitude_multiplier = 0.001f;
const float octave_fraction = 0.33333333f;
const float octave_bias = -128;

enum class command_value : uint8_t
{
	none = 0,
	set_voice_volume,
	set_voice_envelope,
	set_voice_oscillator,
	play_note,
	hold_note,
	release_note,
};

enum class oscillator_type : uint8_t
{
	silence,
	sine,
	square,
	triangle,
	saw,
	noise,
};

// English chromatic note scale
enum class note : uint8_t
{
	A,
	AS,
	B,
	C,
	CS,
	D,
	DS,
	E,
	F,
	FS,
	G,
	GS,
	none = 0xf,
};

struct oscillator_settings
{
	// Which waveform is being used?
	oscillator_type type;

	// Which harmonic of the base note to oscillate at, either positive or negative, or zero
	float octave;

	// How much is this octave contributing?
	float amplitude;

	// Are we modulating the frequency of the main signal?
	float lfo_frequency;
	float lfo_amplitude;
};

struct voice
{
	oscillator_settings oscillators[oscillators_per_voice];
	float volume;

	// ADSR params
	float attack_time;
	float decay_time;
	float sustain_amplitude;
	float release_time;
	float start_amplitude;
	float max_life;

	// Playback variables
	float time_started;
	float time_ended;
	// Middle A would be current_note = note::A, octave = 5.333.. repeating
	note current_note;
	float octave;

	std::string to_string()
	{
		std::string output = "";

		output += "Volume=" + std::to_string(volume) + "\n";
		output += "Time: started=" + std::to_string(time_started) + " ended=" + std::to_string(time_ended) + "\n";
		output += "Envelope: a=" + std::to_string(attack_time) + " d=" + std::to_string(decay_time)
			+ " s=" + std::to_string(sustain_amplitude) + " r=" + std::to_string(release_time) + "\n";

		return output;
	}
};

inline uint8_t make_command_byte(command_value command, uint8_t ID)
{
	return (uint8_t)command << 4 + ID;
}

inline command_value get_command(uint8_t byte)
{
	return (command_value)(byte >> 4);
}

inline int get_voice(uint8_t byte)
{
	return byte & 0xF;
}

inline float byte_to_float_fraction(uint8_t byte)
{
	return (float)byte / 255;
}

inline float byte_to_float_time(uint8_t byte)
{
	return byte_to_float_fraction(byte);
}

inline float byte_to_float_time_micros(uint8_t byte)
{
	return byte_to_float_fraction(byte) * time_multiplier;
}

inline float get_shifted_octave(uint8_t byte, uint8_t shift = 0)
{
	return (float)(byte >> shift) + octave_fraction;
}

inline float get_biased_octave(uint8_t byte)
{
	auto biased = (int)byte + octave_bias;
	return (float)biased / 16;
}

inline note get_note(uint8_t byte)
{
	return (note)(byte & 0xF);
}

inline int get_oscillator_id(uint8_t byte)
{
	return byte >> 4;
}

inline oscillator_type get_oscillator_type(uint8_t byte)
{
	return (oscillator_type)(byte & 0xF);
}

inline float get_lfo_frequency(uint8_t byte)
{
	return (float)(byte >> 4);
}

inline float get_lfo_amplitude(uint8_t byte)
{
	return (float)(byte & 0xF) / 15.0f * lfo_amplitude_multiplier;
}