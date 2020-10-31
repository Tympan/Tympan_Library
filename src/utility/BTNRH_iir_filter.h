
#ifndef _BTNRH_IIR_FILTER_H
#define _BTNRH_IIR_FILTER_H

#include <math.h>

/*
// apply IIR filter (with filter history to enable successive calls of the filter)...using doubles
double filterz(
    double *b, int nb,  //IIR "b" coefficients, and number of coeff
    double *a, int na,  //IIR "a" coefficients, and number of coeff
    float *x, float *y, int n, //signal input (x) and output (y) and number of points
    double *z) //filter states (history), which is (max(nb,na)-1) in length
{
    double   yyyy;
    int     i, j, k, nz;

    // normalize coefficients
    //if ((na > 0) && (a[0] != 1)) { //original
    if ((na > 0) && (abs(a[0]-1.0) > 1e-20)) {
        for (i = 1; i < na; i++)
            a[i] /= a[0];
        for (i = 0; i < nb; i++)
            b[i] /= a[0];
        a[0] = 1.0;
    }
    
    nz = ((na > nb) ? na : nb) - 1;

    for (i = 0; i < n; i++) {
        yyyy = b[0] * x[i] + z[0];
        for (j = 0; j < nz; j++) {
            k = j + 1;
            if (k < nz)
                z[j] = z[k];
            else
                z[j] = 0.0;
            if (k < nb)
                z[j] += b[k] * x[i];
            if (k < na)
                z[j] -= a[k] * yyyy;
        }
        y[i] = (float) yyyy;
    }

    return (0.0);
}
*/

// apply IIR filter (with filter history to enable successive calls of the filter)...using floats
void filterz(
    float *b, int nb,  //IIR "b" coefficients, and number of coeff
    float *a, int na,  //IIR "a" coefficients, and number of coeff
    float *x, float *y, int n, //signal input (x) and output (y) and number of points
    float *z); //filter states (history), which is (max(nb,na)-1) in length

// apply IIR filter but assume that a[0] is 1.0.  Do not check.
void filterz_nocheck(
    float *b, int nb,  //IIR "b" coefficients, and number of coeff
    float *a, int na,  //IIR "a" coefficients, and number of coeff
    float *x, float *y, int n, //signal input (x) and output (y) and number of points
    float *z); //filter states (history), which is (max(nb,na)-1) in length

#endif
