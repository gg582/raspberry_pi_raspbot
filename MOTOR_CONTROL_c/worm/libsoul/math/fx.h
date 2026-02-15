#ifndef LIBTTAK_MATH_FX_H
#define LIBTTAK_MATH_FX_H

#include <stdint.h>
#include <stdbool.h>

#define TTAK_FX_FRACTIONAL_BITS 16
#define TTAK_FX_ONE (1 << TTAK_FX_FRACTIONAL_BITS)

typedef int32_t ttak_fx_t;

static inline ttak_fx_t ttak_fx_from_float(float value) {
    return (ttak_fx_t)(value * (float)TTAK_FX_ONE);
}

static inline float ttak_fx_to_float(ttak_fx_t value) {
    return (float)value / (float)TTAK_FX_ONE;
}

static inline ttak_fx_t ttak_fx_add(ttak_fx_t a, ttak_fx_t b) {
    return a + b;
}

static inline ttak_fx_t ttak_fx_sub(ttak_fx_t a, ttak_fx_t b) {
    return a - b;
}

static inline ttak_fx_t ttak_fx_mul(ttak_fx_t a, ttak_fx_t b) {
    return (ttak_fx_t)(((int64_t)a * (int64_t)b) >> TTAK_FX_FRACTIONAL_BITS);
}

static inline ttak_fx_t ttak_fx_div(ttak_fx_t a, ttak_fx_t b) {
    return (ttak_fx_t)(((int64_t)a << TTAK_FX_FRACTIONAL_BITS) / b);
}

static inline ttak_fx_t ttak_fx_clamp(ttak_fx_t value, ttak_fx_t min, ttak_fx_t max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

static inline ttak_fx_t ttak_fx_abs(ttak_fx_t value) {
    return value < 0 ? -value : value;
}

static inline ttak_fx_t ttak_fx_mix(ttak_fx_t a, ttak_fx_t b, ttak_fx_t ratio) {
    return ttak_fx_add(ttak_fx_mul(a, ttak_fx_sub(TTAK_FX_ONE, ratio)),
                       ttak_fx_mul(b, ratio));
}

static inline ttak_fx_t ttak_fx_decay(ttak_fx_t value, ttak_fx_t factor) {
    return ttak_fx_mul(value, factor);
}

static inline ttak_fx_t ttak_fx_sigmoid(ttak_fx_t x) {
    ttak_fx_t abs_x = ttak_fx_abs(x);
    ttak_fx_t denom = ttak_fx_add(TTAK_FX_ONE, abs_x);
    ttak_fx_t frac = ttak_fx_div(x, denom);
    ttak_fx_t half = TTAK_FX_ONE >> 1;
    return ttak_fx_add(half, ttak_fx_mul(frac, half));
}

static inline ttak_fx_t ttak_fx_swish(ttak_fx_t x) {
    return ttak_fx_mul(x, ttak_fx_sigmoid(x));
}

#define TTAK_FX_CONST(value) ttak_fx_from_float(value)

#endif // LIBTTAK_MATH_FX_H
