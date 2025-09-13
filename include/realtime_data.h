#pragma once

struct realtime_data {
	static realtime_data& Default() {
		static realtime_data rd{};
		return rd;
	}

	float r_hl;
	float low_side_v{150};
	float high_side_v{430};
	float low_side_a{0};
	float high_side_a{0};
	float prev_err{};
	float error_integral{};
	float duty_cycle{1. - 150. / 450.};
};

