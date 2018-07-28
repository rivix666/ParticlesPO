#pragma once

#define PI 3.141592653589793238462643383279
#define TWO_PI 6.283185307179586476925286766559
#define HALF_PI 1.57079632679
#define DEG_TO_RAD 0.01745329251994329576923690768489

#include<math.h>

class MathHelper
{
public:
    static double dRand(double dMin, double dMax)
    {
        double d = (double)rand() / RAND_MAX;
        return dMin + d * (dMax - dMin);
    }
};

