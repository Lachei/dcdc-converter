#include "../../include/pwm.h"

namespace pwm {

slices& slices::Default() { static slices s{}; return s; }

void pwm_disable() {}

}

