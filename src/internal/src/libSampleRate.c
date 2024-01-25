
#include <stdbool.h>
#define PACKAGE "ls-lsr"
#define VERSION "1.0"
#define ENABLE_SINC_BEST_CONVERTER
#define ENABLE_SINC_MEDIUM_CONVERTER
#define ENABLE_SINC_FAST_CONVERTER
#include "libsamplerate/src/samplerate.c"
#include "libsamplerate/src/src_linear.c"
#include "libsamplerate/src/src_sinc.c"
#include "libsamplerate/src/src_zoh.c"
