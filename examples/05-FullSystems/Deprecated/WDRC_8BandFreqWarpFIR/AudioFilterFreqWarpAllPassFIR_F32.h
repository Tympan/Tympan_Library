
/*
   MIT License.  use at your own risk.
*/



#include <arm_math.h> //ARM DSP extensions.  https://www.keil.com/pack/doc/CMSIS/DSP/html/index.html
#include <AudioStream_F32.h>
#include <math.h>

#include <Tympan_Library.h> 
#include "utility/BTNRH_rfft.h"

class FreqWarpAllPass 
{
  public:
    FreqWarpAllPass(void) { setSampleRate_Hz(AUDIO_SAMPLE_RATE_EXACT); }
    FreqWarpAllPass(float _fs_Hz) { setSampleRate_Hz(_fs_Hz);  }
    
    void setSampleRate_Hz(float fs_Hz) {
      //https://www.dsprelated.com/freebooks/sasp/Bilinear_Frequency_Warping_Audio_Spectrum.html
      float fs_kHz = fs_Hz / 1000;
      rho = 1.0674f *sqrtf( 2.0/M_PI * atan(0.06583f * fs_kHz) ) - 0.1916f;
      //rho = 1.1*rho; //WEA trial.  totally not part of the theory, but it does shift the frequencies downward!
      //b[0] = -rho; b[1] = 1.0f;
      //a[0] = 1.0f;  a[1] = -rho;
    }
    
    float update(const float &in_val) {
      float out_val = ((-rho)*in_val) + prev_in + (rho*prev_out);
      prev_in = in_val; prev_out = out_val; //save states for next time
      return out_val;
    }
    float rho;
    
  protected:
    //float b[2];
    //float a[2];
    float prev_in=0.0f;
    float prev_out=0.0f;


};

#define N_FREQWARP_SAMP (32)
#define N_FREQWARP_ALLPASS (31)  //N-1
#define N_FREQWARP_FIR (8)      //N/2+1 (or just N/2 if skipping DC) (or N/2-1 if skipping DC and Nyquist)
class AudioFilterFreqWarpAllPassFIR_F32 : public AudioStream_F32
{
   public:
    //constructor
    AudioFilterFreqWarpAllPassFIR_F32(void) : AudioStream_F32(1, inputQueueArray_f32) {
      setSampleRate_Hz(AUDIO_SAMPLE_RATE_EXACT);
      //setFirCoeff();
      setFirCoeff_viaFFT();
    }
    AudioFilterFreqWarpAllPassFIR_F32(AudioSettings_F32 settings) : AudioStream_F32(1, inputQueueArray_f32) {
      setSampleRate_Hz(settings.sample_rate_Hz);
      //setFirCoeff();
      setFirCoeff_viaFFT();
    }
    void setSampleRate_Hz(float fs_Hz) {
      for (int Ifilt=0; Ifilt < N_FREQWARP_ALLPASS; Ifilt++) freqWarpAllPass[Ifilt].setSampleRate_Hz(fs_Hz);
    };


    //here's the method that is called automatically by the Teensy Audio Library
    void update(void) {
      //Serial.println("AudioEffectMine_F32: doing update()");  //for debugging.
      audio_block_f32_t *audio_block;
      audio_block = AudioStream_F32::receiveReadOnly_f32();
      if (!audio_block) return;

      //do your work
      applyMyAlgorithm(audio_block);

      //assume that applyMyAlgorithm transmitted the block so that we just need to release audio_block
      AudioStream_F32::release(audio_block);
    }

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!11
    // Here is where you can add your algorithm.
    // This function gets called block-wise
    void applyMyAlgorithm(audio_block_f32_t *audio_block) {

      //allocate output block for each output channel
      audio_block_f32_t *out_blocks[N_FREQWARP_FIR];
      for (int I_fir=0; I_fir < N_FREQWARP_FIR; I_fir++) {
        out_blocks[I_fir] = AudioStream_F32::allocate_f32();
        if (out_blocks[I_fir]==NULL) {
          //failed to allocate! release any blocks that have been allocated, then return;
          if (I_fir > 0) { for (int J=I_fir-1; J >= 0; J--) AudioStream_F32::release(out_blocks[J]); }
          if (Serial) {
            Serial.print("AudioFilterFreqWarpAllPassFIR: applyMyAlgorithm: could only allocate ");
            Serial.print(I_fir+1);
            Serial.println(" output blocks.  Returning...");
          }
          return;
        }
        out_blocks[I_fir]->length = audio_block->length;  //set the length of the output block
      }

      //loop over each sample and do something on a point-by-point basis (when it cannot be done as a block)
      for (int Isamp=0; Isamp < (audio_block->length); Isamp++) {
        
        // increment the delay line
        incrementDelayLine(delay_line,audio_block->data[Isamp],N_FREQWARP_SAMP);
        
        //apply each FIR filter (convolution with the delay line)
        for (int I_fir = 0; I_fir < N_FREQWARP_FIR; I_fir++) {
          #if 0
            out_val = 0.0f; //initialize
            for (int I=0; I < N_FREQWARP_SAMP; I++) { //loop over each sample in the delay line
              out_val += (delay_line[I]*all_FIR_coeff[I_fir][I]);
            }
            out_blocks[I_fir]->data[Isamp] = out_val;
          #else
            //use ARM DSP to accelerate the convolution
            arm_dot_prod_f32(delay_line,all_FIR_coeff[I_fir],N_FREQWARP_SAMP,&(out_blocks[I_fir]->data[Isamp]));
          #endif
        }
      }

      //transmit each block
      for (int I_fir = 0; I_fir < N_FREQWARP_FIR; I_fir++) {
        AudioStream_F32::transmit(out_blocks[I_fir],I_fir);
        AudioStream_F32::release(out_blocks[I_fir]);
      }
      
    } //end of applyMyAlgorithms

    void setFirCoeff(void) {
      int targ_fir_ind;
      float phase_rad, dphase_rad;
      float foo_b, win[N_FREQWARP_SAMP];

      if (Serial) {
        Serial.print("AudioFilterFreqWarpAllPassFIR: setFirCoeff: computing coefficients for ");
        Serial.print(N_FREQWARP_FIR);
        Serial.println(" filters.");
      }

      //make window
      calcWindow(N_FREQWARP_SAMP, win);

      //make each FIR filter
      for (int I_fir=0; I_fir < N_FREQWARP_FIR; I_fir++) {
        
        //each FIR should be (in effect) one FFT bin higher in frequency
        //dphase_rad = ((float)I_fir) * (2.0f*M_PI) / ((float)N_FREQWARP_SAMP);
        targ_fir_ind = I_fir+1;  //skip the DC term
        dphase_rad = (2.0f*M_PI) * ((float)targ_fir_ind) / ((float)N_FREQWARP_SAMP);  //skip the DC term


        if (Serial) {
          Serial.print("setFirCoeff: FIR ");
          Serial.print(I_fir);
          Serial.print(": ");
        }
        
        for (int I = 0; I < N_FREQWARP_SAMP; I++) {
          phase_rad = ((float)I)*dphase_rad;
          phase_rad += ((float)targ_fir_ind)*M_PI;  //shift to be linear phase
          foo_b = cosf((float)phase_rad);
          foo_b /= ((float)N_FREQWARP_SAMP);
          if ((targ_fir_ind > 0) && (targ_fir_ind < ((N_FREQWARP_SAMP/2+1)-1))) foo_b *= 2.0;
          
           if (Serial) {
             Serial.print(foo_b,4);
             Serial.print(", ");
          }
        
          //scale the coefficient and save
          foo_b = foo_b*win[I];
          all_FIR_coeff[I_fir][I] = foo_b;
  
        }
        if (Serial) Serial.println();
      }
    }
    
    void setFirCoeff_viaFFT(void) {
      if (Serial) Serial.println("AudioFilterFreqWarpAllPassFIR_F32: setFirCoeff_viaFFT: start.");
    
      const int n_fft = N_FREQWARP_SAMP ;
      float win[n_fft];
      const int N_NYQUIST_REAL = (n_fft/2+1);
      const int N_NYQUIST_COMPLEX = N_NYQUIST_REAL*2;
      float foo_vector[N_NYQUIST_COMPLEX];
      //float foo_b[N_FREQWARP_SAMP];

      //make the window
      calcWindow(n_fft, win);

      //loop over each FIR filter
      for (int I_fir = 0; I_fir < N_FREQWARP_FIR; I_fir++) {
        for (int I=0; I<N_NYQUIST_COMPLEX; I++) foo_vector[I]=0.0f;  //clear

        //desired impulse response
        int ind_middle = N_NYQUIST_REAL-1;
        //foo_vector[ind_middle-1]=1.0f;  //desired impulse results
        foo_vector[ind_middle]=1.0f;  //desired impulse results
        //foo_vector[ind_middle]=0.5;  //desired impulse results

        //convert to frequency domain (result is complelx, but only up to nyquist bin
        BTNRH_FFT::cha_fft_rc(foo_vector, n_fft);

        if (0) {
          //keep just the bin(s) that we want
          int targ_bin = I_fir+1;  //here's the bin that we want to keep (skip DC)
          for (int Ibin=0; Ibin < N_NYQUIST_REAL; Ibin++) {
            if (Ibin != targ_bin) {
              //zero out the non-target bins
              foo_vector[2*Ibin] = 0.0f;   // zero out the real
              foo_vector[2*Ibin+1] = 0.0f; // zero out the complex
            }
          }
        } else {
          //weight the bins as desired
          float targ_amp[N_NYQUIST_REAL];for (int Ibin=0; Ibin<N_NYQUIST_REAL; Ibin++) targ_amp[Ibin]=0.0f;
          switch (I_fir) {
            case 0:
              targ_amp[0] = 0.6*1.03;
              targ_amp[1] = 0.85*0.7*0.95*1.0; //blue
              targ_amp[2] = 0.4*0.8*0.95*1.03; //orange
              break;
            case 1:
              targ_amp[2] = 0.65*0.6*1.042*1.015; //orange
              targ_amp[3] = 0.75*0.60*1.02*1.05; //yellow
              break;
            case 2:
              targ_amp[3] = 0.0*0.71*1.01;  //yellow
              targ_amp[4] = 1.15*0.71*1.01*0.985;  //purple 
              targ_amp[5] = 0.15*0.71*1.035; //green
              break;
            case 3:
              targ_amp[5] = 0.35*0.86*0.95*1.045*1.02; //green
              targ_amp[6] = 1.0*0.80*0.95*1.03;    //cyan
              targ_amp[7] = 0.3*0.8*0.95*1.00*0.9086;  //red
              break;
            case 4:
              targ_amp[7] = 0.35*0.85*1.01;  //red
              targ_amp[8] = 0.78*0.85*1.025;    //blue
              targ_amp[9] = 0.72*0.81*1.03;  //orange
              break;
            case 5:
              targ_amp[10] = 0.76*0.8*1.025; //yellow
              targ_amp[11] = 0.82*0.75*1.025; //purple
              break;
            case 6:
              targ_amp[12] = 0.72*0.83*1.037*1.009; //green
              targ_amp[13] = 0.82*0.72*1.006; //cyan
              break;
            case 7:
              targ_amp[14] = 0.83*0.82; //red
              targ_amp[15] = 0.61*0.89*1.01;  //blue
              targ_amp[16] = 0.80*0.89;  //orange
              break;
          }
//          if (Serial) {
//            Serial.print("setFirCoeff_viaFFT: FIR ");
//            Serial.print(I_fir);
//            Serial.print(": targ_amp: ");
//            for (int Ibin=0; Ibin<N_NYQUIST_REAL; Ibin++) {
//              Serial.print(targ_amp[Ibin]);
//              Serial.print(", ");
//            }
//            Serial.println();
//          }
          for (int Ibin=0; Ibin<N_NYQUIST_REAL; Ibin++) {
            foo_vector[2*Ibin] *= targ_amp[Ibin];   // real
            foo_vector[2*Ibin+1] *= targ_amp[Ibin]; // complex
          }
        }

        //inverse fft to come back into the time domain (not complex)
        BTNRH_FFT::cha_fft_cr(foo_vector, n_fft); //IFFT back into the time domain

        //apply window and save
        if (Serial) {
          Serial.print("setFirCoeff_viaFFT: FIR ");
          Serial.print(I_fir);
          Serial.print(": ");
        }
        for (int I=0; I < n_fft; I++) {
          //all_FIR_coeff[I_fir][I] = foo_vector[I];
          all_FIR_coeff[I_fir][I] = foo_vector[I]*win[I];
          if (Serial) {
             Serial.print(foo_vector[I],4);
             Serial.print(", ");
          }
        }
        if (Serial) Serial.println();
        
      }
    }
    void printRho(void) {
      if (Serial) {
        Serial.print("AudioFilterFreqWarpAllPassFIR_F32: rho = ");
        Serial.println(freqWarpAllPass[0].rho);
      }
    }
  private:
    //state-related variables
    audio_block_f32_t *inputQueueArray_f32[1]; //memory pointer for the input to this module
    FreqWarpAllPass freqWarpAllPass[N_FREQWARP_ALLPASS];
    float delay_line[N_FREQWARP_SAMP];
    float all_FIR_coeff[N_FREQWARP_FIR][N_FREQWARP_SAMP];

  

    void calcWindow(int n,float *win) {
      //if (Serial) Serial.print("calcWindow: ");
      for (int I = 0; I < n; I++) {
        switch (1) {
          case 0:
            //no windowing
            win[I] = 1.0;
            break;
          case 1:
            //hanning
            win[I] = (1.0 - cos(2.0*M_PI*((float(I))/((float)(n-1)))))/2.0f;  //hanning
            win[I] /= 0.6219; //make up for lost gain in the hanning window
            break;
          case 2:
            //hanning with non-zero end points
            win[I] = (1.0 - cos(2.0*M_PI*((float(I+1))/((float)(n+1)))))/2.0f;  //hanning
            win[I] /= 0.6672; //make up for lost gain in the hanning window
            break;            
       }
       //if (Serial) { Serial.print(win[I]); Serial.print(", "); }
       
      }
      //if (Serial) Serial.println();
    }

    void incrementDelayLine(float *delay_line_state, const float &new_val, const int N_delay_line) {          
      //increment the delay line
      #define INCREMENT_MODE 2
      #if (INCREMENT_MODE == 1)
        //frequency warped delay line...start and the end and work backwards
        for (int Idelay=N_delay_line-1; Idelay > 0; Idelay--) {
          delay_line_state[Idelay] = freqWarpAllPass[Idelay-1].update(delay_line_state[Idelay-1]); //delay line is one longer than freqWarpAllPass
        }
        delay_line_state[0] = new_val;//put the newest value into the start of the delay line
      #elif (INCREMENT_MODE == 2)
         delay_line_state[0] = new_val;//put the newest value into the start of the delay line
         for (int Idelay=1; Idelay < N_delay_line; Idelay++) {
          delay_line_state[Idelay] = freqWarpAllPass[Idelay-1].update(delay_line_state[Idelay-1]); //delay line is one longer than freqWarpAllPass
        }
      #elif (INCREMENT_MODE == 0)
        //regular (linear) delay line         
        for (int Idelay=N_delay_line-1; Idelay > 0; Idelay--) {
          delay_line_state[Idelay]=delay_line_state[Idelay-1];  //shift by one
        }
        delay_line_state[0] = new_val;//put the newest value into the start of the delay line
      #endif

    }
    
  

};  //end class definition for AudioEffectMine_F32

