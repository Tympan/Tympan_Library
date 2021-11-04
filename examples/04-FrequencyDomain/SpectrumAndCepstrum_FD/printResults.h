
// Here are several functions that I created to print the spectral and cepstral results.
// There are several functions because there are several different ways that I might want
// to see the results.
//
// Method 1: Prints the top three peaks in the spectrum and cepstrum.  Watch via SerialMonitor
//
// Method 2: Prints all the spectral and cepstral values.  Watch via SerialPlotter.
//


// Here is Method 1, the function that periodically prints the top 3 peaks in the spectrum 
// and the top 3 peaks in the cepstrum.  Sing or whistle and look at the values.  You'd look
// at the values via via the Arduino SerialMonitor.
void printSpectralAndCepstralPeaks(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;
  static float prev_val = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    printSpectralAndCepstralPeaks();  //thisi is the function that actually prints the spectral/cepstral stuff
    
    lastUpdate_millis = curTime_millis;
  } // end if
}

void printSpectralAndCepstralPeaks(void) {
  int NFFT = audioAnalysisCepstrum.getNFFT();

  //get the spectrum and cepstrum
  float spectrum_mag2[NFFT], cepstrum_mag2[NFFT];
  audioAnalysisCepstrum.getSpectrumArray_mag2(spectrum_mag2, NFFT);
  audioAnalysisCepstrum.getCepstrumArray_mag2(cepstrum_mag2, NFFT);

  //how many peaks to find?  and in what freuqency range?
  int N_PEAKS = 3;
  float allowed_min_freq_Hz = 50.0f, allowed_max_freq_Hz = 2000.0f;  
  int start_freq_ind = (int)(allowed_min_freq_Hz/sample_rate_Hz * (float)NFFT + 0.5);
  int end_freq_ind = (int)(allowed_max_freq_Hz/sample_rate_Hz * (float)NFFT + 0.5);
  int start_ceps_ind = (int)(sample_rate_Hz / (float)allowed_max_freq_Hz + 0.5);
  int end_ceps_ind = (int)(sample_rate_Hz / (float)allowed_min_freq_Hz + 0.5);
  

  //find peaks
  float peak_spectrum_mag2[N_PEAKS], peak_cepstrum_mag2[N_PEAKS];
  int peak_spectrum_ind[N_PEAKS], peak_cepstrum_ind[N_PEAKS];
  findPeaks(spectrum_mag2, start_freq_ind, end_freq_ind, peak_spectrum_mag2, peak_spectrum_ind, N_PEAKS);
  findPeaks(cepstrum_mag2, start_ceps_ind, end_ceps_ind, peak_cepstrum_mag2, peak_cepstrum_ind, N_PEAKS);

  //convert to frequency and quefrency
  float peak_spectrum_Hz[N_PEAKS], peak_cepstrum_Hz[N_PEAKS];
  for (int i=0; i<N_PEAKS; i++) {
    peak_spectrum_Hz[i] = audioAnalysisCepstrum.getSpectrumFreq_Hz(peak_spectrum_ind[i]);
    peak_cepstrum_Hz[i] = audioAnalysisCepstrum.getCepstrumQuef_Hz(peak_cepstrum_ind[i]);
  }

  //print peak spectrum
  Serial.print("Peak Spectrum [freq_Hz, 10log10(mag2)]:");
  for (int i=0; i<N_PEAKS; i++) {
    if (i>0) Serial.print(",");
    Serial.print(" [" + String(peak_spectrum_Hz[i]) + ", " + String(10.0*log10f(peak_spectrum_mag2[i])) + "]");
  }
  Serial.println();

  //print peak cepstrum
  Serial.print("Peak Cepstrum [quef_Hz, 10log10(mag2)]:");
  for (int i=0; i<N_PEAKS; i++) {
    if (i>0) Serial.print(",");
    Serial.print(" [" + String(peak_cepstrum_Hz[i]) + ", " + String(10.0*log10f(peak_cepstrum_mag2[i])) + "]");
  }
  Serial.println(); 
 
}

//find maximum value(s).  Max values must be seperated by at least 1 bin
void findPeaks(float *values_mag2, int start_ind, int end_ind, float *peaks_mag2, int *peaks_ind, int N_PEAKS) {
  for (int Ipeak = 0; Ipeak < N_PEAKS; Ipeak++) {

    //search for the max value (other than those already found
    float cur_max = 0.0f;
    int cur_ind = -1;
    for (int i=start_ind; i<end_ind; i++) {
      float val = values_mag2[i];
      if (val > cur_max) {
        
        //check to ensure that we haven't already gotten this one
        int candidate_ind = i;
        bool is_ok = true;
        if (Ipeak > 0) {
          for (int Iprev=0; Iprev<Ipeak; Iprev++) {
            if ( abs(candidate_ind-peaks_ind[Iprev]) <= 1 ) //reject previous peaks and the neighboring bin on either side!
            is_ok = false;
          }
        }
        if (is_ok) {
          cur_max = val;
          cur_ind = i;
        }
      }
    }

    //save the values to the output
    peaks_mag2[Ipeak] = cur_max;
    peaks_ind[Ipeak] = cur_ind; 
  }
}



// Here is Method 1, which prints all the spectrum and cepstrum values within
// a desired frequency range.   Sing or whistle and look at the values.  You'd look
// at the values via via the Arduino SerialPlotter.
//
// Note that this function needs to put the cepstrum on the same frequency basis as
// the spectrum, which is NOT the native format of the cepstrum.  Down at the fundamental
// frequency of the voice, there are many more cepstral bins than spectral bins, so
// I search over the range of cepstral bins that fit within the spectral bin and then 
// I print out the maximum cepstral value in that range.
void printFullSpectrumAndCepstrum(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;
  static float prev_val = 0;
  static bool firstTime = true;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    if (firstTime) {
      Serial.println("BinFreq_kHz, Spectrum_dB, Cepstrum_dB");
      firstTime=false;
    }

    int NFFT = audioAnalysisCepstrum.getNFFT();

    //int max_ind = NNFT/2;  //plot to Nyquist
    int min_ind = (int)(10.0 / sample_rate_Hz * NFFT + 0.5); min_ind = max(1, min_ind);
    int max_ind = (int)(4000.0 / sample_rate_Hz * NFFT + 0.5); max_ind = min(max_ind, NFFT/2);
    for (int i=min_ind; i<max_ind;i++) { //do only up to nyquist
      float freq_Hz = audioAnalysisCepstrum.getSpectrumFreq_Hz(i);
      float spec_dB = 10.0*log10f(audioAnalysisCepstrum.getSpectrumValue_mag2(i));

      //we now need to find the range of cepstral bins that correspond to this
      //single spectral bin.
      float prev_Hz = audioAnalysisCepstrum.getSpectrumFreq_Hz(i-1);
      float next_Hz = audioAnalysisCepstrum.getSpectrumFreq_Hz(i+1);
      float ceps_low_Hz = 0.5*(prev_Hz+freq_Hz); //half way between FFT bins
      float ceps_high_Hz = 0.5*(freq_Hz+next_Hz);//half way between FFT bins
      int ceps_ind_low = (int)(sample_rate_Hz / ceps_high_Hz + 0.5);
      int ceps_ind_high = (int)(sample_rate_Hz / ceps_low_Hz + 0.5);

      //now we decide what cepstral value to report for this range of cepstral bins
      float ceps_mag2=0.0f;
      if (0) {
        //sum (or average) across all cepstral bins that are within the spectral bin
        for (int Iceps = ceps_ind_low; Iceps <= ceps_ind_high; Iceps++) ceps_mag2 += audioAnalysisCepstrum.getCepstrumValue_mag2(Iceps);
        //ceps_mag2 = ceps_mag2 / (ceps_ind_high-ceps_ind_low+1); //compute the average, rather than just the sum
      } else {
        //get the max across all the cepstral bins that are within the spectral bin 
        for (int Iceps = ceps_ind_low; Iceps <= ceps_ind_high; Iceps++) ceps_mag2 = max(ceps_mag2, audioAnalysisCepstrum.getCepstrumValue_mag2(Iceps));
      }
      float ceps_dB = 10.0*log10f(ceps_mag2);
 
      //print the values!
      Serial.println(String(freq_Hz/1000) + ", " + String(spec_dB+70) + ", " + String(ceps_dB));
    }
    
    lastUpdate_millis = curTime_millis;
  } // end if
}
