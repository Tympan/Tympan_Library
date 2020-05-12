
#include "BTNRH_iir_filter.h"

void filterz(
    float *b, int nb,  //IIR "b" coefficients, and number of coeff
    float *a, int na,  //IIR "a" coefficients, and number of coeff
    float *x, float *y, int n, //signal input (x) and output (y) and number of points
    float *z) //filter states (history), which is (max(nb,na)-1) in length
{
    int     i;

    // normalize coefficients
    //if ((na > 0) && (a[0] != 1)) { //original
    if ((na > 0) && (fabs(a[0]-1.0) > 1e-20)) {
        for (i = 1; i < na; i++)
            a[i] /= a[0];
        for (i = 0; i < nb; i++)
            b[i] /= a[0];
        a[0] = 1.0;
    }
	
	filterz_nocheck(b,nb,a,na,x,y,n,z);
}
	
void filterz_nocheck(
    float *b, int nb,  //IIR "b" coefficients, and number of coeff
    float *a, int na,  //IIR "a" coefficients, and number of coeff
    float *x, float *y, int n, //signal input (x) and output (y) and number of points
    float *z) //filter states (history), which is (max(nb,na)-1) in length
{
    float   yyyy;
	int k;
	int nz = ((na > nb) ? na : nb) - 1;
  
    for (int i = 0; i < n; i++) {
        yyyy = b[0] * x[i] + z[0];
        for (int j = 0; j < nz; j++) {
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
        y[i] = yyyy;
    }
}
