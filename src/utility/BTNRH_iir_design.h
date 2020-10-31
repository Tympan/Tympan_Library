

#ifndef _BTNRH_IIR_DESIGN_H
#define _BTNRH_IIR_DESIGN_H

#ifndef M_PI
#define M
_PI            3.14159265358979323846
#endif

//#define round(x)        ((int)(floor(x+0.5)))

#define dmove(x,y,n)    memmove(x,y,(n)*sizeof(double))
#define fmove(x,y,n)    memmove(x,y,(n)*sizeof(float))

#define dcopy(x,y,n)    memcpy(x,y,(n)*sizeof(double))
#define fcopy(x,y,n)    memcpy(x,y,(n)*sizeof(float))

#define dzero(x,n)      memset(x,0,(n)*sizeof(double))
#define fzero(x,n)      memset(x,0,(n)*sizeof(float))


/***********************************************************/

// set double-precision array to one
void ones(double *d, int n) { for (int j = 0; j < n; j++) d[j] = 1.0; }
void ones(float *d, int n) { for (int j = 0; j < n; j++) d[j] = 1.0; }

/***********************************************************/

// bilinear_pole - transform analog pole to IIR pole
void bilinear_pole(float *p, //IIR Pole
    double *ap,   //analog ole
    double wp)    //Unused here???
{
    double aa, bb, c1, c2, c3, p1, p2;

    aa = 2.0 * ap[0]; 
    bb = ap[0] * ap[0] + ap[1] * ap[1]; 
    c1 = 1.0 - aa + bb;
    c2 = 2.0 * (bb - 1);
    c3 = 1.0 + aa + bb;
    p1 = -c2 / (2.0 * c1);
    if (ap[1] == 0) {
        p[0] = (float) p1;
        p[1] = 0.0;
    } else {
        p2 = sqrt(c3 / c1 - p1 * p1);
        p[0] = (float) p1;
        p[1] = (float) p2;
        p[2] = (float) p1;
        p[3] = (float) -p2;
    }
}

double gain(float *z,  //zeros
    float *p,   //poles
    int nz,     //filter order (number of zeros or poles)
    double w)   //frequency to evaluate gain
{
    double f[2], x[2], y[2], xr, xi, yr, yi, xm, ym, temp;
    int j, jr, ji;

    f[0] = cos(M_PI * w);
    f[1] = sin(M_PI * w);
    x[0] = y[0] = 1.0;
    x[1] = y[1] = 0.0;
    for (j = 0; j < nz; j++) {
        jr = j * 2;
        ji = jr + 1;
        xr = f[0] - z[jr];
        xi = f[1] - z[ji];
        yr = f[0] - p[jr];
        yi = f[1] - p[ji];
        temp = x[0] * xr - x[1] * xi;
        x[1] = x[0] * xi + x[1] * xr;
        x[0] = temp;
        temp = y[0] * yr - y[1] * yi;
        y[1] = y[0] * yi + y[1] * yr;
        y[0] = temp;
    }
    xm = sqrt(x[0] * x[0] + x[1] * x[1]);
    ym = sqrt(y[0] * y[0] + y[1] * y[1]);
    temp = ym / xm;
    return (temp);
}


// pole2zp - transform analog pole to IIR zeros, poles, and gain
void pole2zp(float *z,  //zeros
    float *p,   //poles
    float *g,   //gain
    double *ap,  //analog prototype
    int np,     //number poles?  number of elements in ap?
    double *wn, //cutoff frequencies (normalized by Nyquist)
    int ft)     //filter type.  0 = LP, 1 =HP, 2 = BP
{
    double bw, u0, u1, wc, wp, Q, M, A, zz[2], pp[2], M1[2], M2[2], N[2];
    float  p1[4], p2[4], z1[4];
    int j, m = 4;

    if ((ft == 0) || (ft == 1)) {  //low-pass or high-pass filters
        u0 = tan(M_PI * wn[0] / 2.0);
        pp[0] = ap[0] * u0;
        pp[1] = ap[1] * u0;
        wp = (ft == 0) ? 1.0 : -1.0;  //sets lowpass or highpass
        bilinear_pole(p, pp, wp);   //but wp doesn't do anything here
        z[0] = (float) -wp;
        z[1] = 0.0;
        if (ap[1] == 0) {
            g[0] = (float) fabs(wp - p[0]) / 2.0; //compute gain
        } else {
            z[2] = z[0];
            z[3] = z[1];
            g[0] = (float) ((wp - p[0]) * (wp - p[0]) + p[1] * p[1]) / 4.0; //compute gain
        }
    } else {  //compute for bandpass
        u0 = tan(M_PI * wn[0] / 2.0);  //low cutoff
        u1 = tan(M_PI * wn[1] / 2.0);  //high cutoff
        bw = u1 - u0;        // bandwidth
        wc = sqrt(u1 * u0);  // center frequency
        if (ft == 2) {
            wp=1;
            z1[0] = 1.0;
            z1[1] = 0.0;
            z1[2] = -1.0;
            z1[3] = 0.0;
        } else {
            wp=-1.0;
            zz[0]=0.0;
            zz[1]=wc;
            bilinear_pole(z1, zz, wp); // doesn't do anything in this function!
        }
        Q = wc / bw;
        M1[0] = (ap[0] / Q) / 2.0;
        M1[1] = (ap[1] / Q) / 2.0;
        N[0] = M1[0] * M1[0] - M1[1] * M1[1] - 1.0;
        N[1] = M1[0] * M1[1] + M1[1] * M1[0];
        M = sqrt(sqrt(N[0] * N[0] + N[1] * N[1]));
        A = atan2(N[1], N[0]) / 2.0;
        M2[0] = M * cos(A);
        M2[1] = M * sin(A);
        u0 = tan(M_PI * wn[0] / 2.0);
        pp[0] = (M1[0] + M2[0]) * wc;
        pp[1] = (M1[1] + M2[1]) * wc;
        bilinear_pole(p1, pp, wp);  //wp doesn't do anything in this function!
        pp[0] = (M1[0] - M2[0]) * wc;
        pp[1] = (M1[1] - M2[1]) * wc;
        bilinear_pole(p2, pp, wp); //wp doesn't do anything in this function!
        if (ap[1] == 0) {
            for (j = 0; j < m; j++) {
                z[j]=z1[j];
                p[j]=p1[j];
            }
        } else {
            for (j = 0; j < m; j++) {
                z[j] = z1[j];
                p[j] = p1[j];
                z[j + m] = z1[j];
                p[j + m] = p2[j];
            }
        }
        wc = (ft == 2) ? sqrt(wn[0] * wn[1]) : 0.0; //frequency to evaluate gain? (re: Nyquist)
        g[0] = (float) gain(z, p, m, wc);  
    }
}



// ap2zp - transform analog prototype to IIR zeros, poles, and gain
void ap2zp(float *z,  //zeros
           float *p,  //poles
           float *g,  //gains
           double *ap, //analog prototypes
           int np,    //number of poles (?)
           double *wn, //cutoff frequency
           int ft)  //filter type
{
    double gg;
    int j, jm, m;

    m = ((ft == 0) || (ft == 1)) ? 1 : 2;
    gg = 1;
    for (j = 0; j < np; j += 2) {
        jm = j * m * 2;  //increment amount for each pointer
        pole2zp(z + jm, p + jm, g, ap + j * 2, np, wn, ft); 
        gg *= g[0];
    }
    *g = (float) gg;
}


/**********************************************************/

// analog prototype Butterworth filter
void butter_ap(double *ap, int np)  //analog progotype, number of elements
{
    double aa;
    int j, jr, ji;

    if (np < 1) {
        return;
    }
    for (j = 0; j < (np - 1); j += 2) {
        jr = 2 * j;
        ji = jr + 1;
        aa = (j + 1) * M_PI / (2 * np);
        ap[jr] = -sin(aa);
        ap[ji] = cos(aa);
        ap[jr + 2] = ap[jr];
        ap[ji + 2] = -ap[ji];
    }
    if (np % 2) {
        jr = 2 * (np - 1);
        ji = jr + 1;
        ap[jr] = -1;
        ap[ji] = 0;
    }
}

//void butter_zp(   // Butterworth filter design
//    float *z,     // zeros
//    float *p,     // poles
//    float *g,     // gain
//    int n,        // filter order
//    double *wn,   // cutoff frequency
//    int ft)        // filter type: 0=LP, 1=HP, 2=BP 
//{
//  double *ap;
//  int m;
//
//  m = ((ft == 0) || (ft == 1)) ? 1 : 2;
//  ap = (double *) calloc(n * m * 2, sizeof(double));
//  butter_ap(ap, n);
//  ap2zp(z, p, g, ap, n, wn, ft);
//  free(ap);
//}

void butter_zp( //butterworth filter design
    float *z,     // zeros
    float *p,     // poles
    float *g,     // gain
    const int n,  // filter order
    double *wn,   // cutoff frequency
    const int ft, // filter type: 0=LP, 1=HP, 2=BP 
    const int m)        //m = 1 for LP or HP, 2 for BP 
{
  double ap[n*m*2];  //analog prototype
  butter_ap(ap, n);  //butterworth analog prototype 
  ap2zp(z, p, g, ap, n, wn, ft); //convert analog prototype to zero-pole representation
}

void butter_zp(   // Butterworth filter design
    float *z,     // zeros
    float *p,     // poles
    float *g,     // gain
    const int n,        // filter order
    double *wn,   // cutoff frequency
    const int ft)        // filter type: 0=LP, 1=HP, 2=BP 
{
  int m=1;  //default to 1 for LP and HP
  if (ft == 2) m = 2;  //if BP set to 2
  butter_zp(z,p,g,n,wn,ft,m);
}



/***********************************************************/

 
// transform polynomial roots to coefficients...float version
void root2poly(float *r, float *p, const int n)
{
    //double *pp, *qq;
    int i, ir, ii, j, jr, ji;

    //pp = (double *) calloc((n + 1) * 2, sizeof(double));
    //qq = (double *) calloc((n + 1) * 2, sizeof(double));
    float pp[(n+1)*2];
    float qq[(n+1)*2];
    
    fzero(pp, (n + 1) * 2);
    fzero(qq, (n + 1) * 2);
    pp[0] = qq[0] = 1;
    for (i = 0; i < n; i++) {
        ir = i * 2;
        ii = i * 2 + 1;
        qq[2] = pp[2] - r[ir];
        qq[3] = pp[3] - r[ii];
        for (j = 0; j < i; j++) {
            jr = j * 2;
            ji = j * 2 + 1;
            qq[jr + 4] = pp[jr + 4] - (pp[jr + 2] * r[ir] - pp[ji + 2] * r[ii]);
            qq[ji + 4] = pp[ji + 4] - (pp[ji + 2] * r[ir] + pp[jr + 2] * r[ii]);
        }
        fcopy(pp, qq, (n + 1) * 2);
    }
    // return real part of product-polynomial coefficients
    for (i = 0; i < (n + 1); i++) {
        p[i] = pp[i * 2];
    }
    //free(pp);
    //free(qq);
}

// transform polynomial roots to coefficients...double version
void root2poly(float *r, double *p, const int n)
{
    //double *pp, *qq;
    int i, ir, ii, j, jr, ji;

    //pp = (double *) calloc((n + 1) * 2, sizeof(double));
    //qq = (double *) calloc((n + 1) * 2, sizeof(double));
    double pp[(n+1)*2];
    double qq[(n+1)*2];
    
    dzero(pp, (n + 1) * 2);
    dzero(qq, (n + 1) * 2);
    pp[0] = qq[0] = 1;
    for (i = 0; i < n; i++) {
        ir = i * 2;
        ii = i * 2 + 1;
        qq[2] = pp[2] - r[ir];
        qq[3] = pp[3] - r[ii];
        for (j = 0; j < i; j++) {
            jr = j * 2;
            ji = j * 2 + 1;
            qq[jr + 4] = pp[jr + 4] - (pp[jr + 2] * r[ir] - pp[ji + 2] * r[ii]);
            qq[ji + 4] = pp[ji + 4] - (pp[ji + 2] * r[ir] + pp[jr + 2] * r[ii]);
        }
        dcopy(pp, qq, (n + 1) * 2);
    }
    // return real part of product-polynomial coefficients
    for (i = 0; i < (n + 1); i++) {
        p[i] = pp[i * 2];
    }
    //free(pp);
    //free(qq);
}


// transform filter poles and zeros to IIR coefficients...float version
void zp2ba(float *z, //zeros
           float *p, //poles
           int nz,   //number of zeros (filter order)
           float *b, //filter "b" coefficients (output)
           float *a) //filter "a" coefficients (output)
{
  if (nz > 0) {
    root2poly(z, b, nz);
    root2poly(p, a, nz);
  }
}



// transform one filter's poles and zeros to IIR coefficients...double version
void zp2ba(float *z, //zeros
           float *p, //poles
           int nz,   //number of zeros (same as number of poles)
           double *b, //IIR filter "b" coefficients (output)
           double *a) //IIR filter "a" coefficients (output)
{
  if (nz > 0) {     
    root2poly(z, b, nz);
    root2poly(p, a, nz);
  }
}

#endif
