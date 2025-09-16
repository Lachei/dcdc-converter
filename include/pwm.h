#pragma once

#include <cstdint>

namespace pwm {

constexpr int p_1_pin = 21;
constexpr int p_2_pin = 19;
constexpr int p_3_pin = 17;
// TODO: adopt
constexpr float pwm_freq = 80e3;
constexpr float pico_freq = 125e6;

struct slices {
    static slices& Default();
    uint32_t p1{};
    uint32_t p2{};
    uint32_t p3{};
};

void pwm_enable();
void pwm_disable();
void set_duty_cycle(uint16_t dc);
void init();

}

