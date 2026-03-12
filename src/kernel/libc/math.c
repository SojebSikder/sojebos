#include "math.h"

double sqrt(double x) {
    if (x <= 0)
        return 0;

    double guess = x;

    for (int i = 0; i < 20; i++) {
        guess = 0.5 * (guess + x / guess);
    }

    return guess;
}
