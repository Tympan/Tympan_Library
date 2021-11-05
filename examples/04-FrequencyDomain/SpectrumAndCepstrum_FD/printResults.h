
// Here are several functions that I created to print the spectral and cepstral results.
// There are several functions because there are several different ways that I might want
// to see the results.
//
// Method 1: Prints the top three peaks in the spectrum and cepstrum.  Watch via SerialMonitor
//
// Method 2: Prints all the spectral and cepstral values.  Watch via SerialPlotter.
//


// First, we have some helper functions

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

// Here is Method 1, the function that periodically prints the top 3 peaks in the spectrum 
// and the top 3 peaks in the cepstrum.  Sing or whistle and look at the values.  You'd look
// at the values via via the Arduino SerialMonitor.
void printSpectralAndCepstralPeaks(void) {
  int NFFT = audioAnalysisCepstrum.getNFFT();

  //get the spectrum and cepstrum
  float spectrum_mag2[NFFT], cepstrum_mag2[NFFT];
  audioAnalysisCepstrum.getSpectrumArray_mag2(spectrum_mag2, NFFT);
  audioAnalysisCepstrum.getCepstrumArray_mag2(cepstrum_mag2, NFFT);

  //how many peaks to find?  and in what freuqency range?
  int N_PEAKS = 3;
  float allowed_min_freq_Hz = 50.0f;
  float allowed_max_freq_Hz = 2000.0f;  
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

// Here is what is actually called from loop().  This function handles the once-per-second (or whatever)
// periodicity of the printing
void printSpectralAndCepstralPeaks(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    printSpectralAndCepstralPeaks();  //this is the function that actually prints the spectral/cepstral stuff
    
    lastUpdate_millis = curTime_millis;
  } // end if
}


// Here is Method 2, which prints all the spectrum and cepstrum values within
// a desired frequency range.   Sing or whistle and look at the values.  You'd look
// at the values via via the Arduino SerialPlotter.
//
// Note that this function needs to put the cepstrum on the same frequency basis as
// the spectrum, which is NOT the native format of the cepstrum.  Down at the fundamental
// frequency of the voice, there are many more cepstral bins than spectral bins, so
// I search over the range of cepstral bins that fit within the spectral bin and then 
// I print out the maximum cepstral value in that range.
//
// The Arduino Serial plotter plots 500 points (I think), so let's have this routine
// print however many values it is to plot and then fill out the rest of the 500 with\
// zeros so that it looks nice on the Arduino serial plotter
void printFullSpectrumAndCepstrum(void) {
  static bool firstTime = true;
  
  if (firstTime) {
    //https://diyrobocars.com/2020/05/04/arduino-serial-plotter-the-missing-manual/
    Serial.println("BinFreq_kHz:,Spectrum_dB_scaled:,Cepstrum_dB_scaled:"); //send labels for legend
    //Serial.println("Min:-10,Max:90"); //send labels for legend
    firstTime=false;
  }
  
  //define the frequencies of interest
  float min_print_Hz = 0.0;
  float max_print_Hz = 3750.0;
  int N_ARDUINO_SERIAL_PLOTTER = 500; //I think that the serial plotter shows the most recent 500 points
    

  //loop over the frequencies of interest
  int NFFT = audioAnalysisCepstrum.getNFFT();
  int min_ind = (int)(min_print_Hz / sample_rate_Hz * NFFT + 0.5); min_ind = max(1, min_ind);
  int max_ind = (int)(max_print_Hz / sample_rate_Hz * NFFT + 0.5); max_ind = min(max_ind, NFFT/2);
  int count = 0;
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

    //shift, scale and limit the values (to make pretty in the plot)
    spec_dB = spec_dB + 70.0;    // shift to better align with the SerialPlotter
    ceps_dB = ceps_dB - 20.0;    // shift to better align with the SerialPlotter
    spec_dB = max(0,min(1.0, spec_dB/90.0));      // scale relevant region to be 0.0 to 1.0
    ceps_dB = max(0,min(1.0, ceps_dB/60.0));      // scale relevant region to be 0.0 to 1.0
    spec_dB = map(spec_dB, 0.0, 1.0, 0.0, max_print_Hz); //scale values to fit within the frequency range (for pretty real-time plot)
    ceps_dB = map(ceps_dB, 0.0, 1.0, 0.0, max_print_Hz); //scale values to fit within the frequency range (for pretty real-time plot)

    //print multiple times in order to fill out the SerialPlotter window
    int n_repeat = max(1,(floor)(N_ARDUINO_SERIAL_PLOTTER / (max_ind-min_ind+1)));
    for (int Irepeat=0; Irepeat < n_repeat; Irepeat++) {   
      if (count <= N_ARDUINO_SERIAL_PLOTTER) {
        Serial.println(String(freq_Hz,1) + "," + String(spec_dB,1) + "," + String(ceps_dB,1)); 
        count++;
      }
    }
  }


  while (count < N_ARDUINO_SERIAL_PLOTTER) {
    //fill out the remaining points with zeros
    Serial.println("-1.0,0.0,0.0");
    count++;
  }
}

// Here is Method 2.5, which prints all the spectrum and cepstrum values within
// a desired frequency range...but logarithmically with interpolation to fill the
// Arduino SerialPlotter window.  Sing or whistle like before.
//
// Note that this function needs to put the cepstrum on the same frequency basis as
// the spectrum, which is NOT the native format of the cepstrum.  Down at the fundamental
// frequency of the voice, there are many more cepstral bins than spectral bins, so
// I search over the range of cepstral bins that fit within the spectral bin and then 
// I print out the maximum cepstral value in that range.
//

float getSumSpectrumOverRange(float *spec_mag2, float fs_Hz, int NFFT, float start_Hz, float end_Hz) {
  float out_mag2=0.0;
  
  //start to understand what range of indices we're going to be computing
  float start_ind_float = max(0.0,start_Hz/fs_Hz*(float)NFFT);
  float end_ind_float = min((float)NFFT/2,end_Hz/fs_Hz*(float)NFFT); //or NFFT/2 + 1 ??

  // // compute the sum for the whole points in-between the given bounds
  int start_ind = (int)ceil(start_ind_float);
  int end_ind = (int)floor(end_ind_float);
  int ind = start_ind;
  while (ind < end_ind) {
    out_mag2 += 0.5*(spec_mag2[ind] + spec_mag2[ind+1]);
    //weight_fac += 1.0; //we added one whole point to our averaging
    
    ind++; //next step
  }

  //  //now take care of the ends
  
  //lower point and its neighboring whole point
  int x1 = (int)floor(start_ind_float), x2 = x1+1; //index below and above
  float start_mag2 = map(start_ind_float, (float)x1, (float)x2, spec_mag2[x1], spec_mag2[x2]);
  float midA_mag2 = spec_mag2[(int)ceil(start_ind_float)];
  float weightA = ceil(start_ind_float) - start_ind_float;
  
  //upper point and its neigboring whole point
  x1 = (int)floor(end_ind_float), x2 = x1+1;  //index below and above
  float end_mag2 = map(start_ind_float, (float)x1, (float)x2, spec_mag2[x1], spec_mag2[x2]);
  float midB_mag2 = spec_mag2[(int)floor(end_ind_float)];
  float weightB = end_ind_float - floor(end_ind_float);
  
  //join it all together
  if (ceil(start_ind_float) > floor(end_ind_float)) {
    //there is no data point in-between...so just get the average of the two points
    float ave_mag2 = 0.5*(start_mag2 + end_mag2);
    float weight = end_ind_float - start_ind_float;
    out_mag2 += weight*ave_mag2; //weighted by how much space the start->end represents
  } else {
    //there is one data or more data points in-between
    float aveA_mag2 = 0.5*(start_mag2 + midA_mag2);
    float aveB_mag2 = 0.5*(midB_mag2 + end_mag2);
    out_mag2 +=  ((weightA*aveA_mag2)+(weightB*aveB_mag2));
  }
  return out_mag2;
}

float getMaxCepstrumOverRange(float *ceps_mag2, float fs_Hz, int NFFT, float start_Hz, float end_Hz) {
  float out_mag2=0.0;
  
  //start to understand what range of indices we're going to be computing
  float start_ind_float = max(0, fs_Hz/end_Hz);
  float end_ind_float = min(NFFT/2, fs_Hz/start_Hz); //or NFFT/2 + 1 ??

  //lowest point
  int x1 = (int)floor(start_ind_float), x2 = x1+1; //index below and above
  float start_mag2 = map(start_ind_float, (float)x1, (float)x2, ceps_mag2[x1], ceps_mag2[x2]);
  out_mag2 = max(out_mag2, start_mag2);

  // // compute the max for the whole points in-between the given bounds
  int start_ind = (int)ceil(start_ind_float);
  int end_ind = (int)floor(end_ind_float);
  int ind = start_ind;
  for (int ind=start_ind; ind <= end_ind; ind++) out_mag2 = max(out_mag2, ceps_mag2[ind]);

  //highest point
  x1 = (int)floor(end_ind_float), x2 = x1+1;  //index below and above
  float end_mag2 = map(start_ind_float, (float)x1, (float)x2, ceps_mag2[x1], ceps_mag2[x2]);
  out_mag2 = max(out_mag2, end_mag2);

  return out_mag2;
}

void printFullSpectrumAndCepstrum_Log(void) {
  static bool firstTime = true;
  
  if (firstTime) {
    //https://diyrobocars.com/2020/05/04/arduino-serial-plotter-the-missing-manual/
    Serial.println("BinFreq_kHz:,Spectrum_dB:,Cepstrum_dB:"); //send labels for legend
    //Serial.println("Min:-10,Max:90");
    firstTime=false;
  }  
  
  //define the frequencies of interest
  float min_print_Hz = 40.0;
  float max_print_Hz = 8000.0;
  int NFFT = audioAnalysisCepstrum.getNFFT();
  float fs_Hz = audioAnalysisCepstrum.getSampleRate_Hz();
  
  //define the plotting parameters
  int N_ARDUINO_SERIAL_PLOTTER = 500; //I think that the serial plotter shows the most recent 500 points
  float step_fac = expf(logf(max_print_Hz/min_print_Hz)/((float)(N_ARDUINO_SERIAL_PLOTTER-1)));
  float half_step_fac = expf(0.5f*logf(step_fac));
  
  //copy out the spectrum and cepstrum
  float spec_mag2[NFFT], ceps_mag2[NFFT];
  audioAnalysisCepstrum.getSpectrumArray_mag2(spec_mag2,NFFT);
  audioAnalysisCepstrum.getCepstrumArray_mag2(ceps_mag2,NFFT);
  
  //loop over the frequencies of interest, interpolating as needed
  int count = 0;
  float targ_freq_Hz, start_freq_Hz, end_freq_Hz;
  float spec_dB, ceps_dB;
  while (count < N_ARDUINO_SERIAL_PLOTTER) {
    targ_freq_Hz = min_print_Hz * powf(step_fac,count);

    //get the frequency bounds for our calculation
    start_freq_Hz = targ_freq_Hz / half_step_fac;
    end_freq_Hz = targ_freq_Hz * half_step_fac;
    
    //get sum of spectrum over the window
    spec_dB = 10.0f*log10f(getSumSpectrumOverRange(spec_mag2,fs_Hz,NFFT,start_freq_Hz,end_freq_Hz));

    //get cepstrum over the window...looks best if using max instead of sum?
    ceps_dB = 10.0f*log10f(getMaxCepstrumOverRange(ceps_mag2,fs_Hz,NFFT,start_freq_Hz,end_freq_Hz));

    //scale and limit the values (to make pretty) and then print to the USB Serial!
    spec_dB = spec_dB + 70.0;    // shift to align with the magnitude of the cepstral data
    spec_dB = max(0.0, min(80.0, spec_dB));  // limit the dB values
    ceps_dB = max(0.0, min(80.0, ceps_dB));  // limit the dB values
    spec_dB = map(spec_dB, 0.0, 80.0, 0, max_print_Hz); //scale values to fit within the frequency range (for pretty real-time plot)
    ceps_dB = map(ceps_dB, 0.0, 80.0, 0, max_print_Hz); //scale values to fit within the frequency range (for pretty real-time plot)
    Serial.println(String(targ_freq_Hz,1) + "," + String(spec_dB,1) + "," + String(ceps_dB,1)); 
    count++;
  }
}


// Here is what is actually called from loop().  This function handles the once-per-second (or whatever)
// periodicity of the printing
void printFullSpectrumAndCepstrum(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    if (1) {
      printFullSpectrumAndCepstrum();
    } else {
      printFullSpectrumAndCepstrum_Log();
    }
    
    lastUpdate_millis = curTime_millis;
  } // end if
}
