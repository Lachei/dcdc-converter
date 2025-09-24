#include <pwm.h>

#include <algorithm>

#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/platform_defs.h"

namespace pwm {

constexpr float pwm_freq = 80e3;
constexpr float pico_freq = SYS_CLK_HZ;
constexpr uint16_t ticks = uint16_t(pico_freq / pwm_freq);

slices& slices::Default() {
	static slices s{
		.p1 = pwm_gpio_to_slice_num(p_1_pin),
		.p2 = pwm_gpio_to_slice_num(p_2_pin),
		.p3 = pwm_gpio_to_slice_num(p_3_pin),
	};
	return s;
};

void pwm_enable() {
    slices &s = slices::Default();
    pwm_set_mask_enabled(1 << s.p1 | 1 << s.p2 | 1 << s.p3);
}

void pwm_disable() {
    pwm_set_mask_enabled(0);
}

void set_duty_cycle(uint16_t dc) {
    slices &s = slices::Default();
    pwm_set_both_levels(s.p1, dc, dc);
    pwm_set_both_levels(s.p2, dc, dc);
    pwm_set_both_levels(s.p3, dc, dc);
}

void set_duty_cycle(float dc) {
    const uint16_t dc_int = std::clamp(dc, 0.f, 1.f) * ticks;
    slices &s = slices::Default();
    pwm_set_both_levels(s.p1, dc_int, dc_int);
    pwm_set_both_levels(s.p2, dc_int, dc_int);
    pwm_set_both_levels(s.p3, dc_int, dc_int);
}

void init() {
    gpio_set_function(p_1_pin, GPIO_FUNC_PWM);
    gpio_set_function(p_1_pin - 1, GPIO_FUNC_PWM);
    gpio_set_function(p_2_pin, GPIO_FUNC_PWM);
    gpio_set_function(p_2_pin - 1, GPIO_FUNC_PWM);
    gpio_set_function(p_3_pin, GPIO_FUNC_PWM);
    gpio_set_function(p_3_pin - 1, GPIO_FUNC_PWM);

    slices &s = slices::Default();

    pwm_config c = pwm_get_default_config();
    pwm_config_set_output_polarity(&c, false, true);
    pwm_config_set_wrap(&c, ticks);

    pwm_init(s.p1, &c, false);
    pwm_init(s.p2, &c, false);
    pwm_init(s.p3, &c, false);

    set_duty_cycle(uint16_t(ticks >> 1));

    pwm_set_counter(s.p1, 0);
    pwm_set_counter(s.p2, ticks / 3);
    pwm_set_counter(s.p3, ticks * 2 / 3);

    pwm_enable();
} 

}

