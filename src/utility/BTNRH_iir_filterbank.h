// iirfb_design.c - IIR-filterbank design
//include <stdio.h>
//include <stdlib.h>
//include <string.h>
//include <math.h>
//include "chapro.h"

#include "BTNRH_iir_design.h"
#include "BTNRH_iir_filter.h"
#include "BTNRH_rfft.h"

//include <Tympan_Library.h>

// transform poles and zeros to IIR coefficients for a bank of filters...float version
void zp2ba_fb(float *z, //zeros
           float *p, //poles
           int nz,   //number of zeros (filter order)
           int nb,   //number of filter bands,
           float *b, //filter "b" coefficients (output)
           float *a) //filter "a" coefficients (output)
{
    float *bk, *ak;
    float *zk, *pk;
    int k;

    if (nb > 0){
        for (k = 0; k < nb; k++) {
            zk = z + k * nz * 2;
            pk = p + k * nz * 2;
            bk = b + k * (nz + 1);
            ak = a + k * (nz + 1);
            //root2poly(zk, bk, nz);
            //root2poly(pk, ak, nz);
            zp2ba(zk,pk,nz,bk,ak); //this is in iir_design.h
        }
    }
}


// transform filterbank poles and zeros to IIR coefficients...double version
void zp2ba_fb(float *z, float *p, int nz, int nb, double *b, double *a)
{
    double *bk, *ak;
    float  *zk, *pk;
    int k;

    if ((nz > 0) && (nb > 0)) {
        for (k = 0; k < nb; k++) {
            zk = z + k * nz * 2;
            pk = p + k * nz * 2;
            bk = b + k * (nz + 1);
            ak = a + k * (nz + 1);
            //root2poly(zk, bk, nz);
            //root2poly(pk, ak, nz);
            zp2ba(zk,pk,nz,bk,ak); //this is in iir_design.h
        }
    }
}

extern "C" char* sbrk(int incr);
int FreeRam() {
  char top; //this new variable is, in effect, the mem location of the edge of the heap
  return &top - reinterpret_cast<char*>(sbrk(0));
}


// apply bank of filters (zero-pole representation) to an audio block
void filterbank_zp(float *y, //output: a block of filtered signal per filter band 
                float *x, //input: a block of signal
                int n,    //input: number of points in the block of signal
                float *z, //filter zeros for all filters
                float *p, //filter poles for all filters
                float *g, //filter gain for each filter
                int nb,   //number of filters in the filterbank
                int nz)   //number of zeros in each filter (same for each filter)
{
    //float *b, *a, 
	float *bb, *aa;
    float *yy;
    int i, j, m, nc;

    //Serial.println("BTNRH_iir_filterbank: filterbank_zp (float): starting...");

    m = (nz + 1) * nb * 2;
    //b = (double *) calloc(m, sizeof(double));
    //a = (double *) calloc(m, sizeof(double));
    float b[m],a[m];
	//b = (float *) calloc(m, sizeof(float));
    //a = (float *) calloc(m, sizeof(float));
    nc = nz + 1;
    //zz = (double *) calloc(nc, sizeof(double));
    //zz = (float *) calloc(nc, sizeof(float));
    //zz = (float *) calloc(nc*nb, sizeof(float));
	float zz[nc*nb];
	
    //Serial.print("BTNRH_iir_filterbank: filterbank_zp: allocated memory...FreeRAM(B) = ");
    //Serial.println(FreeRam());

    // transform poles & zeros to IIR coeficients
    zp2ba_fb(z, p, nz, nb, b, a);
    
    // loop over frequency bands
    for (j = 0; j < nb; j++) {
        bb = b + j * nc; // band IIR numerator
        aa = a + j * nc; // band IIR denominator
        yy = y + j * n;  // band response pointer

        //dzero(zz, nc);   // clear filter history
        fzero(zz, nc);   // clear filter history
        
        filterz(bb, nc, aa, nc, x, yy, n, zz); 
        for (i = 0; i < n; i ++) {
            yy[i] *= g[j];
        }
    }
	
	//Serial.println("BTNRH_iir_filterbank: filterbank_zp: finished.");

    // release the memory
    //free(zz);
    //free(a);
    //free(b);
}


// perform peak alignment and apply gain (ie, actually shift the data in time)
void peak_align_fb(float *y, //input and output: impulse response of each filter band  (float[nb][nt])
	int *d,  // input: desired delay for each filter (int[nb])
	int nb,  // input: number of filters (aka. number of bands)
	int nt)  // input: number of time samples in array "y"
{
    float *yy;
    int j, i;

    // loop over frequency bands
    for (j = 0; j < nb; j++) {  
        yy = y + j * nt;  // band response pointer
        // delay shift
        i = d[j];  //yes, the dealy is in samples
        fmove(yy + i, yy, nt - i);
        fzero(yy, i);
    }
}




// filterbank sum across bands
void combine_fb(float *x, //input: many blocks of input signal (ie, the output of the filterbank_zp function)
                float *y, //output: the output signal block resulting from summing the input blocks
                double *g, //input: gain to apply for each filter band
                int nb,   //input: numer of filter bands
                int nt)   //input: length of a data block (ie, number of samples in time)
{
    double sum;
    float *xx;
    int j, i;

    // loop over time
    for (i = 0; i < nt; i++) {
        sum = 0;
        // loop over frequency bands
        for (j = 0; j < nb; j++) {
            xx = x + j * nt;  // band response pointer
            sum += xx[i] * g[j];
        }
        y[i] = (float) sum;
    }
}

// filterbank sum across bands...float-only version
void combine_fb(float *x, //input: many blocks of input signal (ie, the output of the filterbank_zp function)
                float *y, //output: the output signal block resulting from summing the input blocks
                float *g, //input: gain to apply for each filter band
                int nb,   //input: numer of filter bands
                int nt)   //nt: length of a data block (ie, number of samples in time)
{
    float sum;
    float *xx;
    int j, i;

    // loop over time
    for (i = 0; i < nt; i++) {
        sum = 0;
        // loop over frequency bands
        for (j = 0; j < nb; j++) {
            xx = x + j * nt;  // band response pointer
            sum += xx[i] * g[j];
        }
        y[i] = sum;
    }
}

/* 
// filterbank sum across bands...float-only version, gain of 1.0 for all filter bands
void combine_fb(float *x, //input: many blocks of input signal (ie, the output of the filterbank_zp function)
                float *y, //output: the output signal block resulting from summing the input blocks
                int nb,   //input: numer of filter bands
                int nt)   //nt: length of a data block (ie, number of samples in time)
{
  float sum;
  float *xx;
  int j, i;
  
  // loop over time
  for (i = 0; i < nt; i++) {
    sum = 0;
    // loop over frequency bands
    for (j = 0; j < nb; j++) {
        xx = x + j * nt;  // band response pointer
        sum += xx[i];
    }
    y[i] = sum;
  }
}
*/


//compute the fft of an array of time-domain signals
void fb_fft(float *y,  //Input: array of time-domain signals to be converted.  float[nb][nt]
			float *x,  //Output: complex fft results in real-complex pairs.  float[nb][nt+2]
			int nb,    //Input: number of bands in the filterbank
			int nt)    //Input: number of time-domain samples per band
{
    float *xx, *yy;
    int j;

    for (j = 0; j < nb; j++) {
        xx = x + j * (nt + 2);
        yy = y + j * nt;
        fcopy(xx, yy, nt);
        BTNRH_FFT::cha_fft_rc(xx, nt);     // real-to-complex FFT
    }
}


/***********************************************************/

//  //design filterbank, zeros and poles
//  iirfb_zp(z,   //output: filter zeros.  pointer to array float[64]
//      p,        //output: filter poles.  pointer to array float[64]
//      g,        //output: gain for each filter.  pointer to array float[8]
//      cf,       //input: pointer to cutoff frequencies, float[7]
//      sr,       //input: sample rate (Hz) 
//      nc,       //input: number of channels (8) 
//      nz);      //input: number of zeros for each filter (4)

// compute IIR-filterbank zeros, poles, & gains
void iirfb_zp(float *z, float *p, float *g, float *cf, const float fs, const int nb, const int nz)
{
  //double  fn, wn[2], *sp;
  double  fn, wn[2], sp[nb-1];
  float  *zj, *pj, *gj;
  int     j, no;
  const double c_o_s = 9; // cross-over spread
  
  //sp = (double *) calloc(nb - 1, sizeof(double));  
  no = nz / 2;    // basic filter order
  fn = fs / 2.0;    // Nyquist frequency
  
  // compute cross-over-spread factors
  for (j = 0; j < (nb - 1); j++) {
      sp[j] = 1 + c_o_s / cf[j];
  }
  
  // design low-pass filter
  wn[0] = (cf[0] / sp[0]) / fn;  //compute cutoff relative to nyquist
  butter_zp(z, p, g, nz, wn, 0); // output is in first element of z,p,g
  
  // design band-pass filters
  for (j = 1; j < (nb - 1); j++) {
      zj = z + j * nz * 2; //increment pointer
      pj = p + j * nz * 2; //increment pointer
      gj = g + j; //increment pointer
      wn[0] = cf[j - 1] * sp[j - 1] / fn;  //compute low cutoff relative to nyquist
      wn[1] = cf[j] / sp[j] / fn;          //compute high cutoff relative to nyquist
      butter_zp(zj, pj, gj, no, wn, 2); // BP
  }
  
  // design high-pass filter
  zj = z + (nb - 1) * nz * 2; //increment pointer
  pj = p + (nb - 1) * nz * 2; //increment pointer
  gj = g + (nb - 1);  //increment pointer
  wn[0] = (cf[nb - 2] * sp[nb - 2]) / fn;   //compute cutoff relative to nyquist
  butter_zp(zj, pj, gj, nz, wn, 1); // HP
  
  //free(sp);
}


// compute the best delay for each filter to align the peaks in the filters' impulse responses
void align_peak_fb(float *z,  // Input: filter's zeros, complex values  (float[nb][2*nz])
				   float *p,  // Input: filter's poles, complex values  (float[nb][2*nz])   
				   float *g,  // Input/Output: each filter's gain...this code might flip the sign (float[nb])
				   int *d,    // Output: best delay (samples) 
				   double td, // Input: maximum delay (msec) allowed for each filter
				   double fs, // Input: sample rate (Hz)
				   int nb,    // Input: number of filter bands
				   int nz)    // Input: filter order (number of zeros in each filter)
{
    //float ymn, ymx, *x, *y, *yy;
	float ymn, ymx, *yy;
    int i, j, imx, itd, nt;
	
	//Serial.println("BTNRH_iir_filterbank: align_peak_fb: starting");

    itd = round(td * fs / 1000.0);
    nt = itd + 1;
    //x = (float *) calloc(nt, sizeof(float));
    //y = (float *) calloc(nt * nb, sizeof(float));
	float x[nt], y[nt*nb];
	
	//Serial.println("BTNRH_iir_filterbank: align_peak_fb: allocated first part.  Calling filterbank_zp...");
	
    // compute initial impulse responses
    x[0] = 1.0; // impulse
    filterbank_zp(y, x, nt, z, p, g, nb, nz); //apply the bank of filters to compute the impulse response

	//Serial.println("BTNRH_iir_filterbank: align_peak_fb: flipping bands...");
	
    // flip bands with abs(min) > max
    for (j = 0; j < nb; j++) {
        yy = y + j * nt; // band response pointer
        ymn = ymx = 0;
        for (i = 0; i < nt; i++) {
            if (ymn > yy[i]) {
                ymn = yy[i];
            }
            if (ymx < yy[i]) {
                ymx = yy[i];
            }
        }
        if (fabs(ymn) > ymx) {
            g[j] = -g[j];
        }
    }
	
	//with the adjusted gain values, resompute the impulse response
	//Serial.println("BTNRH_iir_filterbank: align_peak_fb: calling filterbank_zp again...");
    filterbank_zp(y, x, nt, z, p, g, nb, nz); //apply the bank of filters to compute the impulse response
	
    // find delay for each band that shifts peak to target delay
	//Serial.println("BTNRH_iir_filterbank: align_peak_fb: find delay for each band...");
    for (j = 0; j < nb; j++) {
        yy = y + j * nt; // band response pointer
        imx = 0;
        for (i = 1; i < nt; i++) {
            if (yy[imx] < yy[i]) {
                imx = i;
            }
        }
        d[j] = (itd > imx) ? (itd - imx) : 0;
    }
	
	//Serial.println("BTNRH_iir_filterbank: align_peak_fb: done.");
   
	//free(x);
    //free(y);
}

// adjust filterbank gains for combined unity gain...float version
int adjust_gain_fb(float *z,  // Input: filter's zeros, complex values  (float[nb][2*nz])
				    float *p,  // Input: filter's poles, complex values  (float[nb][2*nz])   
				    float *g,  // Input: each filter's gain (float[nb])
				    int *d,    // Output: best delay (samples)  
					float *cf, // Input: Cross-over frequencies between filters (Hz)  (float[nb-1])
					float fs,  // Input: sample rate (Hz)
					int nb,    // Input: number of filter bands)
					int nz)    // Input: filter order (number of zeros in each filter)
{ 
    //float *G, e, f, mag, sum, avg;
	float e, f, mag, sum, avg;
    //float *h, *H, *x, *y;
    float *y, *h;
	int i, j, jj, mr, mi, m, nt, ni = 8, nf = 5;

    //Serial.println("BTNRH_iir_filterbank: adjust_gain_fb: here.");

	//decide resolution of this analysis.  Originally, it targeted 1 Hz resolution (ie, nt was equal
	//to the sample rate).  But, that requires too much RAM.  So, we've dialed it back 
    //nt = 1024;
    //while (nt < fs) nt *= 2; //make NT longer until it is longer than 1 second
    nt = 2048;  //at 24kHz, this leads to a resolution of about 12 Hz.  This is going to have to be good enough
	float H[(nt+2)*nb];
	//H = (float *) calloc((nt + 2) * nb, sizeof(float));
    //h = (float *) calloc((nt + 2) * nb, sizeof(float));
    float G[nb];
	//G = (float *) calloc(nb, sizeof(float));
    float x[nt];
	//x = (float *) calloc(nt, sizeof(float)); 
	y = (float *) calloc(nt * nb, sizeof(float));
    
    x[0] = 1;  //define input as an impulse
    ones(G, nb);

	//check to see if the memory was correctly allocated
    if (y == NULL) {
		Serial.println("BTNRH_iir_filterbank: adjust_gain_fB: ***ERROR *** could not allocate memory for y!!!");
		return -1;
	}

    //Serial.print("BTNRH_iir_filterbank: adjust_gain_fb: allocated a lot of memory.  FreeRAM(B) = ");
    //Serial.println(FreeRam());
    
    //get frequency response
	filterbank_zp(y, x, nt, z, p, g, nb, nz);
	peak_align_fb(y, d, nb, nt);
	fb_fft(y, H, nb, nt);
    free(y);
	
	//Serial.print("BTNRH_iir_filterbank: filterbank_zp: allocated memory...FreeRAM(B) = ");
    //Serial.println(FreeRam());


    // iteration loop
    h = (float *) calloc((nt + 2) * nb, sizeof(float));
    if (h == NULL) Serial.println("adjust_gain_fB: ***ERROR *** could not allocate memory for h!!!");
    //Serial.println("BTNRH_iir_filterbank: adjust_gain_fb: iteration loop");
	for (i = 0; i < ni; i++) { //number of iterations ("ni") is predefined, not adaptive
        combine_fb(H, h, G, nb, nt + 2); // sum across bands
        // loop over frequency bands
        for (j = 0; j < (nb - 2); j++) {
            sum = 0.0;
            for (jj = 0; jj < nf; jj++) {
                e = jj / (nf - 1.0);
                f = powf(cf[j], e) * powf(cf[j + 1], 1.0 - e);
                m = round(f * (nt / fs));
                mr = m * 2;
                mi = mr + 1;
                mag = sqrtf(h[mr] * h[mr] + h[mi] * h[mi]);
                sum += logf(mag);
            }
            avg = expf(sum / nf);
            G[j + 1] /= avg;
        }
    }
	
    //Serial.println("BTNRH_iir_filterbank: adjust_gain_fb: iteration loop complete.");
    
    // loop over frequency bands
    for (j = 0; j < nb - 1; j++) {
        g[j] *=  G[j];
    }
	
    //Serial.println("BTNRH_iir_filterbank: adjust_gain_fb: freeing memory.");

    //free(H);
    //free(h);
    //free(G);
    //free(x);
	free(h);
	
	//Serial.print("BTNRH_iir_filterbank: filterbank_zp: allocated memory...FreeRAM(B) = ");
    //Serial.println(FreeRam());


    //Serial.println("BTNRH_iir_filterbank: adjust_gain_fb: done.");
   
	return 0;
}


//// adjust filterbank gains for combined unity gain...double version
//void adjust_gain_fb(float *z, float *p, float *g, int *d, double *cf, double fs, int nb, int nz)
//{
//    double *G, e, f, mag, sum, avg;
//    float *h, *H, *x, *y;
//    int i, j, jj, mr, mi, m, nt, ni = 8, nf = 5;
//
//    nt = 1024;
//    while (nt < fs) nt *= 2;
//    H = (float *) calloc((nt + 2) * nb, sizeof(float));
//    h = (float *) calloc((nt + 2) * nb, sizeof(float));
//    G = (double *) calloc(nb, sizeof(double));
//    x = (float *) calloc(nt, sizeof(float));
//    y = (float *) calloc(nt * nb, sizeof(float));
//    x[0] = 1;
//    ones(G, nb);
//    // iteration loop
//    filterbank_zp(y, x, nt, z, p, g, nb, nz);
//    peak_align_fb(y, d, nb, nt);
//    fb_fft(y, H, nb, nt);
//    for (i = 0; i < ni; i++) {
//        combine_fb(H, h, G, nb, nt + 2); // sum across bands
//        // loop over frequency bands
//        for (j = 0; j < (nb - 2); j++) {
//            sum = 0;
//            for (jj = 0; jj < nf; jj++) {
//                e = jj / (nf - 1.0);
//                f = pow(cf[j], e) * pow(cf[j + 1], 1 - e);
//                m = round(f * (nt / fs));
//                mr = m * 2;
//                mi = mr + 1;
//                mag = sqrt(h[mr] * h[mr] + h[mi] * h[mi]);
//                sum += log(mag);
//            }
//            avg = exp(sum / nf);
//            G[j + 1] /= avg;
//        }
//    }
//    // loop over frequency bands
//    for (j = 0; j < nb - 1; j++) {
//        g[j] *= (float) G[j];
//    }
//    free(H);
//    free(h);
//    free(G);
//    free(x);
//    free(y);
//}

/***********************************************************/


//cha_iirfb_design(
//    z,       //output: filter zeros.  pointer to array float[64]
//    p,       //output: filter poles.  pointer to array float[64]
//    g,       //output: gains for each filter.  pointer to array float[8]
//    d,       //output: delay for each filter. (samples?).  pointer to array int[8]
//    cf,      //input: pointer to cutoff frequencies. float[7]
//    nc,      //input: number of filber bands (aka. channels) (8)
//    nz,      //input: number of filter states per filter?  (4)
//    sr,      //input: sample rate (Hz)
//    td);     //input: max time delay (samples?) for the filters?

// IIR-filterbank design
int cha_iirfb_design(float *z, float *p, float *g, int *d, 
                 float *cf, int nc, int nz, float sr, float td)
{

  //Serial.println("cha_iirfb_design: calling iirfb_zp...");delay(500);
  
  //design filterbank, zeros and poles and gains
  iirfb_zp(z,   //filter zeros.  pointer to array float[64]
      p, //filter poles.  pointer to array float[64]
      g, //gain for each filter.  pointer to array float[8]
      cf, //pointer to cutoff frequencies, float[7]
      sr, //sample rate (Hz) 
      nc, //number of channels (8) 
      nz);  //filter order (4)

  //Serial.println("cha_iirfb_design: calling align_peak_fb...");delay(500);
  
  //adjust filter to time-align the peaks of the fiter response
  align_peak_fb(z, //filter zeros.  pointer to array float[64]
      p, //filter poles.  pointer to array float[64]
      g, //gain for each filter.  pointer to array float[8]
      d, //delay for each filter (samples?).  pointer to array int[8]
      td,  //max time delay (samples?) for the filters?
      sr, //sample rate (Hz)
      nc,  //number of channels
      nz);  //filter order  (4)

  //Serial.println("cha_iirfb_design: calling adjust_gain_fb...");delay(500);
 
  //adjust gain of each filter (for flat response even during crossover?)...
  //WARNING: this operation might run out of RAM!!!  check ret_val.  If -1, it ran out of RAM and didn't compute
  int ret_val = adjust_gain_fb(z,   //filter zeros.  pointer to array float[64]
      p, //filter poles.  pointer to array float[64]
      g, //gain for each filter.  pointer to array float[8] 
      d, //delay for each filter (samples).  pointer to array int[8]
      cf, //pointer to cutoff frequencies, float[7]
      sr, //sample rate (Hz) 
      nc, //number of channels (8) 
      nz);  //filter order  (4)
	
	return ret_val;

}
