#pragma once

struct realtime_data {
	static realtime_data& Default() {
		static realtime_data rd{};
		return rd;
	}

	// have to be set for integration
	float low_side_v{150};
	float high_side_v{430};
	float low_side_a{0};
	float high_side_a{0};
	float goal_amp{};
	float err{};

	// are set by the controller
	float prev_err{};
	float error_integral{};
	float duty_cycle{1. - 150. / 450.};
};

