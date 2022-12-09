#include "xlaudio.h"
#include "xlaudio_armdsp.h"

// This is an interpolation filter with 49 taps at 8 taps per symbol
#define NUMCOEF 49
#define RATE    8

float32_t coef[NUMCOEF] = {
                           0.0000,
                           0.0152,
                           0.0294,
                           0.0403,
                           0.0458,
                           0.0445,
                           0.0360,
                           0.0206,
                          -0.0000,
                          -0.0234,
                          -0.0463,
                          -0.0651,
                          -0.0763,
                          -0.0769,
                          -0.0648,
                          -0.0389,
                           0.0000,
                           0.0501,
                           0.1079,
                           0.1692,
                           0.2290,
                           0.2820,
                           0.3238,
                           0.3505,
                           0.3596,
                           0.3505,
                           0.3238,
                           0.2820,
                           0.2290,
                           0.1692,
                           0.1079,
                           0.0501,
                           0.0000,
                          -0.0389,
                          -0.0648,
                          -0.0769,
                          -0.0763,
                          -0.0651,
                          -0.0463,
                          -0.0234,
                          -0.0000,
                           0.0206,
                           0.0360,
                           0.0445,
                           0.0458,
                           0.0403,
                           0.0294,
                           0.0152,
                           0.0000};

uint32_t r = 0xAAAAAAAAA;
float32_t randomSymbol() {
    r = (r >> 1) | ((((r >> 1) ^ r ) & 1) << 22);
    return (r % 2) ? -0.25 : 0.25;
}

static int phase = 0;

// traditional implementation
float32_t taps[NUMCOEF];
uint16_t processSample(uint16_t x) {
    float32_t input = randomSymbol();

    // the input is 'upsampled' to RATE, by inserting N-1 zeroes every N samples
    phase = (phase + 1) % RATE;
    if (phase == 0)
        taps[0] = input;
    else
        taps[0] = 0.0;

    // debug signal
    if (phase == 0)
        xlaudio_debugpinhigh();
    else
        xlaudio_debugpinlow();

    float32_t q = 0.0;
    uint16_t i;
    for (i = 0; i<NUMCOEF; i++)
        q += taps[i] * coef[i];

    for (i = NUMCOEF-1; i>0; i--)
        taps[i] = taps[i-1];

    return xlaudio_f32_to_dac14(0.25 * q);
}

float32_t symboltaps[NUMCOEF/RATE + 1];

uint16_t processSampleMultirate(uint16_t x) {
    float32_t input = randomSymbol();

    phase = (phase + 1) % RATE;

    // debug signal
    if (phase == 0)
        xlaudio_debugpinhigh();
    else
        xlaudio_debugpinlow();

    uint16_t i;
    if (phase == 0) {
        symboltaps[0] = input;
        for (i=NUMCOEF/RATE; i>0; i--)
            symboltaps[i] = symboltaps[i-1];
    }

    float32_t q = 0.0;
    uint16_t limit = (phase == 0) ? NUMCOEF/RATE + 1 : NUMCOEF/RATE;
    for (i=0; i<limit; i++)
        q += symboltaps[i] * coef[i * RATE + phase];

    return xlaudio_f32_to_dac14(0.25 * q);
}


#include <stdio.h>

int main(void) {
    WDT_A_hold(WDT_A_BASE);

    int c = xlaudio_measurePerfSample(processSample);
    printf("Cycles Direct %d\n", c);

    c = xlaudio_measurePerfSample(processSampleMultirate);
    printf("Cycles Multirate %d\n", c);

    xlaudio_init_intr(FS_16000_HZ, XLAUDIO_J1_2_IN, processSampleMultirate);
    xlaudio_run();

    return 1;
}
