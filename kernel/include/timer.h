#pragma once

#include "types.h"

/*
Because of my testing machine has problem with HPET
(it counts really slow. i set it to 1kHz but only got about 50Hz)
so Mosix uses PIT for accurate counting.

Codes about HPET can be found in Mosix 3a,
it is welcomed to come here anytime.
*/

extern volatile uint64_t jiffies;