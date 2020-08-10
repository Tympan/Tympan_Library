
#ifndef _BTNRH_WDRC_TYPES_H
#define _BTNRH_WDRC_TYPES_H

//include "utility/textAndStringUtils.h"

#include <SdFat_Gre.h>   //for reading and writing settings to SD card
#include "AccessConfigDataOnSD.h"

#define DSL_MXCH 8  
	
namespace BTNRH_WDRC {

	// Here are the settings for the adaptive feedback cancelation
	class CHA_AFC {
		public: 
			int default_to_active; //enable AFC at startup?  1=active. 0=disabled.
			int afl;	//length (samples) of adaptive filter for modeling feedback path.
			float mu;	//mu, scale factor for how fast the adaptive filter adapts (bigger is faster)
			float rho;	//rho, smoothing factor for estimating audio envelope (bigger is a longer average)
			float eps;	//eps, when est the audio envelope, this is the min allowed level (avoids divide-by-zero)
			AccessConfigDataOnSD r;
						
			int readFromSDFile(SdFile_Gre *file) {
				const int buff_len = 300;
				char line[buff_len];
				
				//find start of data structure
				char targ_str[] = "CHA_AFC";
				int lines_read = r.readRowsUntilTargStr(file,line,buff_len,targ_str); //file is incremented so that the next line should be the first part of the DSL data
				if (lines_read <= 0) {
					Serial.println("BTNRH_WDRC: CHA_AFC: readFromSDFile: *** could not find start of AFC data in file.");
					return -1;
				}
	
				// read the overall settings
				if (r.readAndParseLine(file, line, buff_len, &default_to_active, 1) < 0) return -1;
				if (r.readAndParseLine(file, line, buff_len, &afl, 1) < 0) return -1;
				if (r.readAndParseLine(file, line, buff_len, &mu, 1) < 0) return -1;
				if (r.readAndParseLine(file, line, buff_len, &rho, 1) < 0) return -1;
				if (r.readAndParseLine(file, line, buff_len, &eps, 1) < 0) return -1;
				
				//write to serial for debugging
				//printAllValues();
			
				return 0;
			}
		
			
			int readFromSD(SdFatSdioEX &sd, const char *filename) {
				SdFile_Gre file;
				
				//open SD
				if (!(sd.begin())) {
					Serial.println("BTNRH_WDRC: CHA_AFC: readFromSD: cannot open SD.");
					return -1;
				}
				
				//open file
				if (!(file.open(filename,O_READ))) {   //open for reading
					Serial.print("BTNRH_WDRC: CHA_AFC: readFromSD: cannot open file ");
					Serial.println(filename);
					return -1;
				}
				
				//read data
				int ret_val = readFromSDFile(&file);
				
				//close file
				file.close();
				
				//return
				return ret_val;
			}
			int readFromSD(const char *filename) {
				SdFatSdioEX sd;
				return readFromSD(sd, filename);
			}	
		
			void printAllValues(void) { printAllValues(&Serial); }
			void printAllValues(Stream *s) {
				s->println("CHA_AFC:");
				s->print("    : enable = "); s->println(default_to_active);
				s->print("    : filter length (afl) = "); s->println(afl);
				s->print("    : Adaptation speed (mu) = "); s->println(mu,6);
				s->print("    : Smooothing factor (rho) "); s->println(rho,6);
				s->print("    : Min Tolerance (eps) = "); s->println(eps,6);

			};
		
		
			void printToSDFile(SdFile_Gre *file, const char *var_name) {
				char header_str[] = "BTNRH_WDRC::CHA_AFC";
				r.writeHeader(file, header_str, var_name);
				
				r.writeValuesOnLine(file, &default_to_active, 1, true, "enable AFC at startup?", 1);
				r.writeValuesOnLine(file, &afl,	1, true, "afl, length (samples) of adaptive filter", 1);
				r.writeValuesOnLine(file, &mu,  1, true, "mu, scale factor for adaptation (bigger is faster)", 1);
				r.writeValuesOnLine(file, &rho, 1, true, "rho, smoooothing factor for envelope (bigger is longer average)", 1);
				r.writeValuesOnLine(file, &eps, 1, false, "eps, min value for env (helps avoid divide by zero)", 1);//no trailing comma on the last one 
				
				r.writeFooter(file); 
			}
		
			int printToSD(const char *filename, const char *var_name, bool deleteExisting = false) {
				SdFatSdioEX sd;
				return printToSD(sd, filename, var_name, deleteExisting);
			}
			int printToSD(SdFatSdioEX &sd, const char *filename, const char *var_name, bool deleteExisting = false) {
				SdFile_Gre file;
				
				//open SD
				if (!(sd.begin())) {
					Serial.println("BTNRH_WDRC: CHA_AFC: printToSD: cannot open SD.");
					return -1;
				}
				
				//delete existing file
				if (deleteExisting) {
					if (sd.exists(filename)) sd.remove(filename);
				}
				
				//open file
				file.open(filename,O_RDWR  | O_CREAT | O_APPEND); //open for writing
				if (!file.isOpen()) {  
					Serial.print("BTNRH_WDRC: CHA_AFC: printToSD: cannot open file ");
					Serial.println(filename);
					return -1;
				}
				
				//write data
				printToSDFile(&file, var_name);
				
				//close file
				file.close();
				
				//return
				return 0;
			}	
		
	};
          
	class CHA_DSL  {
	
		public:
			//CHA_DSL(void) {};  //no constructor means that I can use brace initialization
			static const int DSL_MAX_CHAN = DSL_MXCH;    // maximum number of channels
			float attack;               // attack time (ms)
			float release;              // release time (ms)
			float maxdB;                // maximum signal (dB SPL)
			int ear;                    // 0=left, 1=right
			int nchannel;               // number of channels
			float cross_freq[DSL_MXCH]; // cross frequencies (Hz)
			float exp_cr[DSL_MXCH];		// compression ratio for low-SPL region (ie, the expander)
			float exp_end_knee[DSL_MXCH];	// expansion-end kneepoint
			float tkgain[DSL_MXCH];     // compression-start gain
			float cr[DSL_MXCH];         // compression ratio
			float tk[DSL_MXCH];         // compression-start kneepoint
			float bolt[DSL_MXCH];       // broadband output limiting threshold
			AccessConfigDataOnSD r;
			
			int readFromSDFile(SdFile_Gre *file) { //returns zero if successful
				//Serial.println("CHA_DSL: readFromSDFile: starting...");
				const int buff_len = 400;
				char line[buff_len];
				
				//find start of data structure
				char targ_str[] = "CHA_DSL";
				int lines_read = r.readRowsUntilTargStr(file,line,buff_len,targ_str); //file is incremented so that the next line should be the first part of the DSL data
				if (lines_read <= 0) {
					Serial.println("BTNRH_WDRC: CHA_DSL: readFromSDFile: *** could not find start of DSL data in file.");
					return -1;
				}
				//Serial.print("BTNRH_WDRC: CHA_DSL: readFromSDFile: DSL Structure Starts at line "); Serial.println(lines_read);
				
				// read the overall settings
				if (r.readAndParseLine(file, line, buff_len, &attack, 1) < 0) return -1;
				if (r.readAndParseLine(file, line, buff_len, &release, 1) < 0) return -1;
				if (r.readAndParseLine(file, line, buff_len, &maxdB, 1) < 0) return -1;
				if (r.readAndParseLine(file, line, buff_len, &ear, 1) < 0) return -1;
				if (r.readAndParseLine(file, line, buff_len, &nchannel, 1) < 0) return -1;
				if (nchannel > DSL_MAX_CHAN) {
					Serial.print("BTNRH_WDRC: CHA_DSL: readFromSDFile: *** ERROR*** nchannel read as ");Serial.print(nchannel);
					nchannel = DSL_MAX_CHAN;
					Serial.print("    : Limiting to "); Serial.println(nchannel);
				}
				
				//read the per-channel settings
				if (r.readAndParseLine(file, line, buff_len, cross_freq, nchannel) < 0) return -1;				
				if (r.readAndParseLine(file, line, buff_len, exp_cr, nchannel) < 0) return -1;
				if (r.readAndParseLine(file, line, buff_len, exp_end_knee, nchannel) < 0) return -1;
				if (r.readAndParseLine(file, line, buff_len, tkgain, nchannel) < 0) return -1;
				if (r.readAndParseLine(file, line, buff_len, cr, nchannel) < 0) return -1;
				if (r.readAndParseLine(file, line, buff_len, tk, nchannel) < 0) return -1;
				if (r.readAndParseLine(file, line, buff_len, bolt, nchannel) < 0) return -1;

				Serial.println("CHA_DSL: readFromSDFile: success!");
				//write to serial for debugging
				//printAllValues();
			
				return 0;
			}
		
			
			int readFromSD(SdFatSdioEX &sd, const char *filename) {
				SdFile_Gre file;
				
				//open SD
				if (!(sd.begin())) {
					Serial.println("BTNRH_WDRC: CHA_DSL: readFromSD: cannot open SD.");
					return -1;
				}
				
				//open file
				if (!(file.open(filename,O_READ))) {   //open for reading
					Serial.print("BTNRH_WDRC: CHA_DSL: readFromSD: cannot open file ");
					Serial.println(filename);
					return -1;
				}
				
				//read data
				int ret_val = readFromSDFile(&file);
				
				//close file
				file.close();
				
				//return
				return ret_val;
			}
			int readFromSD(const char *filename) {
				SdFatSdioEX sd;
				return readFromSD(sd, filename);
			}	
		
			
			void printAllValues(void) { printAllValues(&Serial); }
			void printAllValues(Stream *s) {
				int last_chan = nchannel;
				//last_chan = DSL_MXCH;
				
				s->println("CHA_DSL:");
				s->print("    : attack (ms) = "); s->println(attack);
				s->print("    : release (ms) = "); s->println(release);
				s->print("    : maxdB (dB SPL) = "); s->println(maxdB);
				s->print("    : ear (0 = left, 1 = right) "); s->println(ear);
				s->print("    : nchannel = "); s->println(nchannel);
				s->print("    : cross_freq (Hz) = ");
				for (int i=0; i< last_chan;i++) { s->print(cross_freq[i]); s->print(", ");}; s->println();
				s->print("    : exp_cr = ");
				for (int i=0; i<last_chan;i++) { s->print(exp_cr[i]); s->print(", ");}; s->println();
				s->print("    : exp_end_knee (dB SPL) = ");
				for (int i=0; i<last_chan;i++) { s->print(exp_end_knee[i]); s->print(", ");}; s->println();
				s->print("    : tkgain (dB) = ");
				for (int i=0; i<last_chan;i++) { s->print(tkgain[i]); s->print(", ");}; s->println();
				s->print("    : cr = ");
				for (int i=0; i<last_chan;i++) { s->print(cr[i]); s->print(", ");}; s->println();
				s->print("    : tk (dB SPL) = ");
				for (int i=0; i<last_chan;i++) { s->print(tk[i]); s->print(", ");}; s->println();
				s->print("    : bolt (dB SPL) = ");
				for (int i=0; i<last_chan;i++) { s->print(bolt[i]); s->print(", ");}; s->println();
			};
			
			void printToSDFile(SdFile_Gre *file, const char *var_name) {
				char header_str[] = "BTNRH_WDRC::CHA_DSL";
				r.writeHeader(file, header_str, var_name);
				
				r.writeValuesOnLine(file, &attack,      1, true, "atttack (msec)", 1);
				r.writeValuesOnLine(file, &release,     1, true, "release (msec) ", 1);
				r.writeValuesOnLine(file, &maxdB,       1, true, "max dB", 1);
				r.writeValuesOnLine(file, &ear,         1, true, "ear (0=left, 1=right)", 1);
				r.writeValuesOnLine(file, &nchannel,    1, true, "number of channels", 1);
				r.writeValuesOnLine(file, cross_freq,   nchannel, true, "Crossover Freq (Hz)", DSL_MXCH);
				r.writeValuesOnLine(file, exp_cr,       nchannel, true, "Expansion: Compression Ratio", DSL_MXCH);
				r.writeValuesOnLine(file, exp_end_knee, nchannel, true, "Expansion: Knee (dB SPL)", DSL_MXCH);
				r.writeValuesOnLine(file, tkgain,       nchannel, true, "Linear: Gain (dB)", DSL_MXCH);
				r.writeValuesOnLine(file, cr,           nchannel, true, "Compression: Compression Ratio", DSL_MXCH);
				r.writeValuesOnLine(file, tk,           nchannel, true,"Compression: Knee (dB SPL)", DSL_MXCH);
				r.writeValuesOnLine(file, bolt,         nchannel, false, "Limiter: Knee (dB SPL)", DSL_MXCH); //no trailing comma on the last one	
				
				r.writeFooter(file); 
			}
			
			int printToSD(const char *filename, const char *var_name, bool deleteExisting = false) {
				SdFatSdioEX sd;
				return printToSD(sd, filename, var_name, deleteExisting);
			}
			int printToSD(SdFatSdioEX &sd, const char *filename, const char *var_name, bool deleteExisting = false) {
				SdFile_Gre file;
				
				//open SD
				if (!(sd.begin())) {
					Serial.println("BTNRH_WDRC: CHA_WDRC: printToSD: cannot open SD.");
					return -1;
				}
				
				//delete existing file
				if (deleteExisting) {
					if (sd.exists(filename)) sd.remove(filename);
				}
				
				
				//open file
				file.open(filename,O_RDWR  | O_CREAT | O_APPEND); //open for writing
				if (!file.isOpen()) {  
					Serial.print("BTNRH_WDRC: CHA_DSL: printToSD: cannot open file ");
					Serial.println(filename);
					return -1;
				}
				
				//write data
				printToSDFile(&file, var_name);
				
				//close file
				file.close();
				
				//return
				return 0;
			}
			
			
	};
	
	typedef struct {
		float alfa;                 // attack constant (not time)
		float beta;                 // release constant (not time
		float fs;                   // sampling rate (Hz)
		float maxdB;                // maximum signal (dB SPL)
		float tkgain;               // compression-start gain
		float tk;                   // compression-start kneepoint
		float cr;                   // compression ratio
		float bolt;                 // broadband output limiting threshold
		//float gcalfa;				// ??  Added May 1, 2018 as part of adaptie feedback cancelation
		//float gcbeta;				// ??  Added May 1, 2018 as part of adaptie feedback cancelation
		float mu;				// 1e-3, adaptive feedback cancelation  (AFC) filter-estimation step size
		float rho;				// 0.984, AFC filter-estimation forgetting factor
		float pwr;				// estimated (smoothed) signal power for modifying mu for AFC
		float fbm;				// some sort of quality metric for AFC (some sort of power residual...smaller is better?)
		
	} CHA_DVAR_t;
	
	typedef struct {
		//int cs;  //chunk size (number of samples in data block)
		//int nc;  //number of frequency bands (channels)
		//int nw;	//number of samples in data window
		int rsz;    //32, ring buffer size (samples)?  Used for AFC
		int rhd;    //last starting point in the ring buffer (head)
		int rtl;    //last ending point in the ring buffer (tail)
		int afl;	//100, adaptive filter length for AFC
		int fbl;	//100, length of simulated feedback (ie, the given feedback impulse response)
		int nqm;    //number of quality metrics:  nqm = (fbl < afl) ? fbl : afl;
		
	} CHA_IVAR_t;
	
/* 	typedef struct {
		float attack;               // attack time (ms), unused in this class
		float release;              // release time (ms), unused in this class
		float fs;                   // sampling rate (Hz), set through other means in this class
		float maxdB;                // maximum signal (dB SPL)...I think this is the SPL corresponding to signal with rms of 1.0
		float tkgain;               // compression-start gain
		float tk;                   // compression-start kneepoint
		float cr;                   // compression ratio
		float bolt;                 // broadband output limiting threshold
	} CHA_WDRC; */
	
	class CHA_WDRC {
		public: 
			float attack;               // attack time (ms), unused in this class
			float release;              // release time (ms), unused in this class
			float fs;                   // sampling rate (Hz), set through other means in this class
			float maxdB;                // maximum signal (dB SPL)...I think this is the SPL corresponding to signal with rms of 1.0
			float exp_cr;				// compression ratio for low-SPL region (ie, the expander)
			float exp_end_knee;			// expansion-end kneepoint
			float tkgain;               // compression-start gain
			float tk;                   // compression-start kneepoint
			float cr;                   // compression ratio
			float bolt;                 // broadband output limiting threshold
			AccessConfigDataOnSD r;
			
			
			int readFromSDFile(SdFile_Gre *file) {
				const int buff_len = 300;
				char line[buff_len];
				
				//find start of data structure
				char targ_str[] = "CHA_WDRC";
				int lines_read = r.readRowsUntilTargStr(file,line,buff_len,targ_str); //file is incremented so that the next line should be the first part of the DSL data
				if (lines_read <= 0) {
					Serial.println("BTNRH_WDRC: CHA_WDRC: readFromSDFile: *** could not find start of DSL data in file.");
					return -1;
				}
				//Serial.print("BTNRH_WDRC: CHA_WDRC: readFromSDFile: DSL Structure Starts at line "); Serial.println(lines_read);
				
				// read the overall settings
				if (r.readAndParseLine(file, line, buff_len, &attack, 1) < 0) return -1;
				if (r.readAndParseLine(file, line, buff_len, &release, 1) < 0) return -1;
				if (r.readAndParseLine(file, line, buff_len, &fs, 1) < 0) return -1;
				if (r.readAndParseLine(file, line, buff_len, &maxdB, 1) < 0) return -1;
				if (r.readAndParseLine(file, line, buff_len, &exp_cr, 1) < 0) return -1;
				if (r.readAndParseLine(file, line, buff_len, &exp_end_knee, 1) < 0) return -1;
				if (r.readAndParseLine(file, line, buff_len, &tkgain, 1) < 0) return -1;
				if (r.readAndParseLine(file, line, buff_len, &tk, 1) < 0) return -1;
				if (r.readAndParseLine(file, line, buff_len, &cr, 1) < 0) return -1;
				if (r.readAndParseLine(file, line, buff_len, &bolt, 1) < 0) return -1;

				//write to serial for debugging
				//printAllValues();
			
				return 0;
			}
		
			
			int readFromSD(SdFatSdioEX &sd, const char *filename) {
				SdFile_Gre file;
				
				//open SD
				if (!(sd.begin())) {
					Serial.println("BTNRH_WDRC: CHA_WDRC: readFromSD: cannot open SD.");
					return -1;
				}
				
				//open file
				if (!(file.open(filename,O_READ))) {   //open for reading
					Serial.print("BTNRH_WDRC: CHA_WDRC: readFromSD: cannot open file ");
					Serial.println(filename);
					return -1;
				}
				
				//read data
				int ret_val = readFromSDFile(&file);
				
				//close file
				file.close();
				
				//return
				return ret_val;
			}
			int readFromSD(const char *filename) {
				SdFatSdioEX sd;
				return readFromSD(sd, filename);
			}	
			
			void printAllValues(void) { printAllValues(&Serial); }
			void printAllValues(Stream *s) {
				s->println("CHA_WDRC:");
				s->print("    : attack (ms) = "); s->println(attack);
				s->print("    : release (ms) = "); s->println(release);
				s->print("    : fs (Hz) = "); s->println(fs);
				s->print("    : maxdB (dB SPL) = "); s->println(maxdB);;
				s->print("    : exp_cr = "); s->println(exp_cr);
				s->print("    : exp_end_knee (dB SPL) = "); s->println(exp_end_knee);
				s->print("    : tkgain (dB) = "); s->println(tkgain);
				s->print("    : tk (db SPL) = "); s->println(tk);
				s->print("    : cr = "); s->println(cr);
				s->print("    : bolt (dB SPL) = "); s->println(bolt);
			};
			
			
			void printToSDFile(SdFile_Gre *file, const char *var_name) {
				char header_str[] = "BTNRH_WDRC::CHA_WDRC";
				r.writeHeader(file, header_str, var_name);
				
				r.writeValuesOnLine(file, &attack, 1, true, "atttack (msec)", 1);
				r.writeValuesOnLine(file, &release, 1, true, "release (msec) ", 1);
				r.writeValuesOnLine(file, &fs, 1, true, "sample rate (ignored)", 1);
				r.writeValuesOnLine(file, &maxdB, 1, true, "max dB", 1);
				r.writeValuesOnLine(file, &exp_cr, 1, true, "Expansion: Compression Ratio", 1);
				r.writeValuesOnLine(file, &exp_end_knee, 1, true, "Expansion: Knee (dB SPL)", 1);
				r.writeValuesOnLine(file, &tkgain, 1, true, "Linear: Gain (dB)", 1);
				r.writeValuesOnLine(file, &tk, 1, true, "Compression: Knee (dB SPL)", 1);
				r.writeValuesOnLine(file, &cr, 1, true, "Compression: Compression Ratio", 1);
				r.writeValuesOnLine(file, &bolt, 1, false, "Limiter: Knee (dB SPL)", 1);  //no trailing comma on the last one
				
				r.writeFooter(file); 
			}
			
			int printToSD(const char *filename, const char *var_name, bool deleteExisting = false) {
				SdFatSdioEX sd;
				return printToSD(sd, filename, var_name, deleteExisting);
			}
			int printToSD(SdFatSdioEX &sd, const char *filename, const char *var_name, bool deleteExisting = false) {
				SdFile_Gre file;
				
				//open SD
				if (!(sd.begin())) {
					Serial.println("BTNRH_WDRC: CHA_WDRC: printToSD: cannot open SD.");
					return -1;
				}
				
				//delete existing file
				if (deleteExisting) {
					if (sd.exists(filename)) sd.remove(filename);
				}
				
				//open file
				file.open(filename,O_RDWR  | O_CREAT | O_APPEND); //open for writing
				if (!file.isOpen()) {  
					Serial.print("BTNRH_WDRC: CHA_WDRC: printToSD: cannot open file ");
					Serial.println(filename);
					return -1;
				}
				
				//write data
				printToSDFile(&file, var_name);
				
				//close file
				file.close();
				
				//return
				return 0;
			}			
	};
	
}

#endif