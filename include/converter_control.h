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
constexpr float control_step(float delta_ms, 
			     realtime_data &rd = realtime_data::Default(),
			     const settings &settings = settings::Default()) {
	if (get_error().load()) {
		pwm::pwm_disable();
		return 1.f - 1.f / settings.high_to_low_ratio;
	}

	// updating duty cycle values
	float goal_amp = 10.f * (rd.high_side_v - rd.ratio_hl * rd.low_side_v);
	goal_amp = std::clamp(goal_amp, -settings.max_amps, settings.max_amps);
	float err = goal_amp - rd.low_side_a;
	float der = (err - rd.prev_err) / std::max(delta_ms, 1e-3f);
	rd.duty_cycle = settings.k_p * err +
			settings.k_i * rd.error_integral +
			settings.k_d * der;
	rd.duty_cycle = std::clamp(rd.duty_cycle, 0.f, 1.f);
	rd.prev_err = err;

	return rd.duty_cycle;
}

