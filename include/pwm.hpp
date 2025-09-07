#pragma once

#include "hardware/pwm.h"

namespace pwm {

constexpr int p_1_pin = 21;
constexpr int p_2_pin = 19;
constexpr int p_3_pin = 17;
// TODO: adopt
constexpr float pwm_freq = 80e3;
constexpr float pico_freq = 125e6;

constexpr void init() {
    gpio_set_function(p_1_pin, GPIO_FUNC_PWM);
    gpio_set_function(p_1_pin - 1, GPIO_FUNC_PWM);
    gpio_set_function(p_2_pin, GPIO_FUNC_PWM);
    gpio_set_function(p_2_pin - 1, GPIO_FUNC_PWM);
    gpio_set_function(p_3_pin, GPIO_FUNC_PWM);
    gpio_set_function(p_3_pin - 1, GPIO_FUNC_PWM);

    uint slice_p1 = pwm_gpio_to_slice_num(p_1_pin);
    uint slice_p2 = pwm_gpio_to_slice_num(p_2_pin);
    uint slice_p3 = pwm_gpio_to_slice_num(p_3_pin);

    pwm_config c = pwm_get_default_config();
    pwm_config_set_output_polarity(&c, false, true);
    constexpr uint16_t ticks = uint16_t(pico_freq / pwm_freq);
    pwm_config_set_wrap(&c, ticks);

    pwm_init(slice_p1, &c, false);
    pwm_init(slice_p2, &c, false);
    pwm_init(slice_p3, &c, false);

    pwm_set_both_levels(slice_p1, ticks / 2, ticks / 2);
    pwm_set_both_levels(slice_p2, ticks / 2, ticks / 2);
    pwm_set_both_levels(slice_p3, ticks / 2, ticks / 2);

    constexpr uint16_t counter_set_delay = 0;
    pwm_set_counter(slice_p1, 0);
    pwm_set_counter(slice_p2, ticks / 3);
    pwm_set_counter(slice_p3, ticks * 2 / 3);

    pwm_set_mask_enabled(1 << slice_p1 | 1 << slice_p2 | 1 << slice_p3);
} 

}

