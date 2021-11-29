
#ifndef _AudioAnalysisCepstrum_FD_F32_h
#define _AudioAnalysisCepstrum_FD_F32_h


#include <AudioFreqDomainBase_FD_F32.h> //from Tympan_Library: inherit all the good stuff from this!
#include <arm_math.h>  //fast math library for our processor


//Create an audio processing class to do the lowpass filtering in the frequency domain.
// Let's inherit from  the Tympan_Library class "AudioFreqDomainBase_FD_F32" to do all of the 
// audio buffering and FFT operations.  That allows us to just focus on manipulating the 
// FFT bins and not all of the detailed, tricky operations of going into and out of the frequency
// domain.
class AudioAnalysisCepstrum_FD_F32 : public AudioFreqDomainBase_FD_F32   //AudioFreqDomainBase_FD_F32 is in Tympan_Library
{
  public:
    //constructor
    AudioAnalysisCepstrum_FD_F32(const AudioSettings_F32 &settings) : AudioFreqDomainBase_FD_F32(settings) {};

    //override setup
    virtual int setup(const AudioSettings_F32 &settings, const int _N_FFT);
        
    // define get methods for accessing the results
    float getSpectrumValue_mag2(int index) { index = max(0,min(getNFFT()-1,index)); return spectrum_mag2[index]; };
    float getCepstrumValue_mag2(int index) { index = max(0,min(getNFFT()-1,index)); return cepstrum_mag2[index]; };

    float getSpectrumFreq_Hz(int index) { return (float)index /(float)getNFFT() * getSampleRate_Hz(); }
    float getCepstrumQuef_Hz(int index) { return getSampleRate_Hz() / (float)index; }
    
    void getSpectrumArray_mag2(float *output_mag2, int NFFT); //copies the most recent result to your given array pointer
    void getCepstrumArray_mag2(float *output_mag2, int NFFT); //copies the most recent result to your given array pointer

    //define get methods for accessing some of the FFT parameters
    virtual int getNFFT(void) { return AudioFreqDomainBase_FD_F32::getNFFT();}
    virtual float getSampleRate_Hz(void) { return AudioFreqDomainBase_FD_F32::getSampleRate_Hz(); }
    
    //this is the method from AudioFreqDomainBase that we are overriding where we will
    //put our own code for manipulating the frequency data.  This is called by update()
    //from the AudioFreqDomainBase_FD_F32.  The update() method is itself called by the
    //Tympan (Teensy) audio system, as with every other Audio processing class.
    virtual void processAudioFD(float32_t *complex_2N_buffer, const int NFFT); 
    virtual void calcMag2(float *complex_2N_buffer, float *output_mag2, int NFFT) ;
    virtual void calcCepstrum(float *spectrum_mag2, float *cepstrum_mag2, int NFFT);

  private:
    //create some data members specific to our processing
    float32_t *ceps_complex_2N_buffer;
    float32_t *spectrum_mag2;
    float32_t *cepstrum_mag2;
    FFT_F32 cepsFFT;
};

//override the setup()
int AudioAnalysisCepstrum_FD_F32::setup(const AudioSettings_F32 &settings, const int _N_FFT) {

  //call the parent setup()
  int allocated_NFFT = AudioFreqDomainBase_FD_F32::setup(settings,_N_FFT); //call the parent version of setup()
  if (allocated_NFFT != _N_FFT) {
    Serial.println("AudioAnalysisCepstrum_FD_F32: *** ERROR *** could not allocated the requested FFT size.");
    Serial.println("    : Requested N = " + String(_N_FFT) + ", Actual N = " + String(allocated_NFFT));
  }
  (myFFT.getFFTObject())->useHanningWindow(); //myFFT is part of the parent class, which has a data member myFFT, which actually does the windowing
  
  //For cepstral processing, the first FFT is setup by the parent class AudioFreqDomainBase_FD_F32
  
  //For the cepstral processing, the second FFT is setup here.
  int ceps_allocated_NFFT = cepsFFT.setup(allocated_NFFT); //hopefully, we got the same N_FFT that we asked for
  if (ceps_allocated_NFFT != allocated_NFFT) {
    Serial.println("AudioAnalysisCepstrum_FD_F32: *** ERROR *** could not allocated the requested cepstrum FFT size.");
    Serial.println("    : Requested N = " + String(allocated_NFFT) + ", Actual N = " + String(ceps_allocated_NFFT));
  }
  cepsFFT.useRectangularWindow(); //defaulted to Hanning, so now we force back to rectangular

  //allocate memory to hold our data
  ceps_complex_2N_buffer = new float32_t[2 * _N_FFT];
  spectrum_mag2 = new float32_t[_N_FFT];
  cepstrum_mag2 = new float32_t[_N_FFT];

  return ceps_allocated_NFFT;
}  

//Here is the method we are overriding with our own cepstral analysis algorithm.
//We get our data from complex_2N_buffer and we put our results back into complex_2N_buffer
void AudioAnalysisCepstrum_FD_F32::processAudioFD(float32_t *complex_2N_buffer, const int NFFT) {

  //compute mag^2 spectrum
  calcMag2(complex_2N_buffer, spectrum_mag2, NFFT);

  //compute the mag^2 cepstrum
  calcCepstrum(spectrum_mag2, cepstrum_mag2, NFFT);

  //if you want to acccess the results, do it in the loop() method via the "get" methods defined earlier  
}

void AudioAnalysisCepstrum_FD_F32::calcMag2(float *complex_2N_buffer, float *output_mag2, int NFFT) {
  for (int i=0; i<NFFT; i++) {
    float real = complex_2N_buffer[2*i];
    float imag = complex_2N_buffer[2*i+1];
    output_mag2[i] = real*real + imag*imag;  //this is the magnitude squared 
  }
}

void AudioAnalysisCepstrum_FD_F32::calcCepstrum(float *spectrum_mag2, float *cepstrum_mag2, int NFFT) {
  int NFFT_2 = NFFT/2+1+1;
  
  //copy into complex buffer and find the minimum value
  float min_val = 1000.0f; //some non-zero value
  for (int i=0; i<NFFT_2; i++) {  //only need to do up through Nyquist for now (will fill in the rest later)
    float val = spectrum_mag2[i];
    ceps_complex_2N_buffer[2*i] = val;      //put this as the real component
    ceps_complex_2N_buffer[2*i+1] = 0.0f;   //there is no imaginary component (yet)
    
    if ( (val > 0.000001f) && (val < min_val) ) min_val = val;
  }

  //limit how small the min value can be
  float min_allowed_min_val = logf(powf(1.0/((float)NFFT),2.0)) ;
  min_val = max(min_val, min_allowed_min_val);
  
  //Go back through the data and enforce the new minimum value.  Then compute the log
  for (int i=0; i<NFFT_2; i++) {  //only need to do up through Nyquist  (will fill in the rest later)

    //enforce a new minimum value (to avoid logf(0.0)
    if (ceps_complex_2N_buffer[2*i] < min_val) ceps_complex_2N_buffer[2*i] = min_val; //move zero values to the new minimum
   
    //compute the log
    ceps_complex_2N_buffer[2*i] = logf(ceps_complex_2N_buffer[2*i]);  //compute the log...this is expensive
  }

  //fill in the rest of the frequency space (ie, all the bins above Nyquist), which is a mirror image of the below-Nyquist space
  int targ_ind = 0;
  for (int source_ind = 1; source_ind < (NFFT/2-1); source_ind++) {
    targ_ind = NFFT - source_ind;
    ceps_complex_2N_buffer[2*targ_ind] = ceps_complex_2N_buffer[2*source_ind]; //ony need to do the real part
    ceps_complex_2N_buffer[2*targ_ind+1] = 0.0;  //imaginary part
  }

  //compute the 2nd FFT to compute the spectrum
  cepsFFT.execute(ceps_complex_2N_buffer); //results are in the complex buffer, output is interleaved [real, imaginary]

  //compute the magnitude
  for (int i=0; i<NFFT; i++) {
    float real = ceps_complex_2N_buffer[2*i], imag = ceps_complex_2N_buffer[2*i+1];
    cepstrum_mag2[i] = real*real + imag*imag;  //this is the magnitude squared
  }
}

void AudioAnalysisCepstrum_FD_F32::getSpectrumArray_mag2(float *output_mag2, int NFFT) { 
  if (NFFT < 1) return;
  NFFT = min(getNFFT(),NFFT); 
  for (int i=0; i<NFFT; i++) output_mag2[i]=spectrum_mag2[i];
}
void AudioAnalysisCepstrum_FD_F32::getCepstrumArray_mag2(float *output_mag2, int NFFT) { 
  if (NFFT < 1) return;
  NFFT = min(getNFFT(),NFFT); 
  for (int i=0; i<NFFT; i++) output_mag2[i]=cepstrum_mag2[i];
}


#endif
