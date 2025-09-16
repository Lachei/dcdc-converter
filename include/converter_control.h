#pragma once

#include <atomic>
#include <string_view>

#include "pwm.h"
#include "settings.h"
#include "realtime_data.h"

constexpr std::atomic<const char*>& get_error() {
	static std::atomic<const char*> error{};
	return error;
}

// make a single control parameter update
// delta_t shall be seddons
constexpr float control_step(float delta_seconds) {
	if (get_error().load()) {
		pwm::pwm_disable();
		return -1;
	}
	if (delta_seconds == 0)
		return -1;

	realtime_data &rd = realtime_data::Default();
	settings &settings = settings::Default();

	// updating duty cycle values
	float goal_amp = 10.f * (rd.high_side_v - rd.ratio_hl * rd.low_side_v);
	goal_amp = std::clamp(goal_amp, -settings.max_amps, settings.max_amps);
	float err = goal_amp - rd.low_side_a;
	rd.error_integral += err * delta_seconds;
	float der = (err - rd.prev_err) / delta_seconds;
	rd.duty_cycle = settings.k_p * err +
			settings.k_i * rd.error_integral +
			settings.k_d * der;
	rd.duty_cycle = rd.duty_cycle / (1.f + std::abs(rd.duty_cycle)) * .5f + .5f;
	rd.prev_err = err;

	return rd.duty_cycle;
}

