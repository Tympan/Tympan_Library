
#ifndef _BTNRH_WDRC_TYPES_H
#define _BTNRH_WDRC_TYPES_H

#include <SdFat.h>     //this was added in Teensyduino 1.54beta3
#include <SDWriter.h>  //to get macro definition of SD_CONFIG   //in Tympan_Library
#include <AccessConfigDataOnSD.h>   // in Tympan_Library

#define DSL_MXCH_TYMPAN 16  


namespace BTNRH_WDRC {

//Define those methods that all BTNRH classes should have
/* class BTNRH_Base {
	public: 
		BTNRH_Base() {};
		virtual void printAllValues(void) { printAllValues(&Serial); }
		virtual void printAllValues(Stream *s) = 0;
}; */


// Here are the settings for the adaptive feedback cancelation
class CHA_AFC { 
	public: 
		//CHA_AFC() : BTNRH_Base() {};
		int default_to_active; //enable AFC at startup?  1=active. 0=disabled.
		int afl;	//length (samples) of adaptive filter for modeling feedback path.
		float mu;	//mu, scale factor for how fast the adaptive filter adapts (bigger is faster)
		float rho;	//rho, smoothing factor for estimating audio envelope (bigger is a longer average)
		float eps;	//eps, when est the audio envelope, this is the min allowed level (avoids divide-by-zero)
			
		void printAllValues(void) { printAllValues(&Serial); }	
		void printAllValues(Stream *s) {
			s->println("CHA_AFC:");
			s->print("    : enable = "); s->println(default_to_active);
			s->print("    : filter length (afl) = "); s->println(afl);
			s->print("    : Adaptation speed (mu) = "); s->println(mu,6);
			s->print("    : Smooothing factor (rho) "); s->println(rho,6);
			s->print("    : Min Tolerance (eps) = "); s->println(eps,6);
		};
};			

	  
class CHA_DSL {
	public:
		//CHA_DSL() : BTNRH_Base() {};  //no constructor means that I can use brace initialization
		//static const int DSL_MAX_CHAN = DSL_MXCH_TYMPAN;    // maximum number of channels
		float attack;               // attack time (ms)
		float release;              // release time (ms)
		float maxdB;                // maximum signal (dB SPL)
		int ear;                    // 0=left, 1=right
		int nchannel;               // number of channels
		float cross_freq[DSL_MXCH_TYMPAN]; // cross frequencies (Hz)
		float exp_cr[DSL_MXCH_TYMPAN];		// compression ratio for low-SPL region (ie, the expander)
		float exp_end_knee[DSL_MXCH_TYMPAN];	// expansion-end kneepoint
		float tkgain[DSL_MXCH_TYMPAN];     // compression-start gain
		float cr[DSL_MXCH_TYMPAN];         // compression ratio
		float tk[DSL_MXCH_TYMPAN];         // compression-start kneepoint
		float bolt[DSL_MXCH_TYMPAN];       // broadband output limiting threshold

		int get_DSL_MAX_CHAN(void) { return DSL_MXCH_TYMPAN; }
		void printAllValues(void) { printAllValues(&Serial); }
		void printAllValues(Stream *s) {
			int last_chan = nchannel;
			//last_chan = DSL_MXCH_TYMPAN;
			
			s->println("CHA_DSL:");
			s->print("    : attack (ms) = "); s->println(attack);
			s->print("    : release (ms) = "); s->println(release);
			s->print("    : maxdB (dB SPL) = "); s->println(maxdB);
			s->print("    : ear (0 = left, 1 = right) "); s->println(ear);
			s->print("    : nchannel = "); s->println(nchannel);
			s->print("    : cross_freq (Hz) = ");
			for (int i=0; i<last_chan-1;i++) { s->print(cross_freq[i]); s->print(", ");}; s->println();delay(3); //one fewer crossover frequency than channels
			s->print("    : exp_cr = ");
			for (int i=0; i<last_chan;i++) { s->print(exp_cr[i]); s->print(", ");}; s->println();delay(3);
			s->print("    : exp_end_knee (dB SPL) = ");
			for (int i=0; i<last_chan;i++) { s->print(exp_end_knee[i]); s->print(", ");}; s->println();delay(3);
			s->print("    : tkgain (dB) = ");
			for (int i=0; i<last_chan;i++) { s->print(tkgain[i]); s->print(", ");}; s->println();delay(3);
			s->print("    : cr = ");
			for (int i=0; i<last_chan;i++) { s->print(cr[i]); s->print(", ");}; s->println();delay(3);
			s->print("    : tk (dB SPL) = ");
			for (int i=0; i<last_chan;i++) { s->print(tk[i]); s->print(", ");}; s->println();delay(3);
			s->print("    : bolt (dB SPL) = ");
			for (int i=0; i<last_chan;i++) { s->print(bolt[i]); s->print(", ");}; s->println();delay(3);
		};
};

class CHA_WDRC {
	public:
		//CHA_WDRC() : BTNRH_Base() {};
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
};

/* typedef struct {
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
 */
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


}  //close namespace



// ///////////////////////////////////////////////////////////////////////////////////////////////
//
// Here are the BTNRH types with SD read/write used for prescription saving
//
// ///////////////////////////////////////////////////////////////////////////////////////////////

// Define an interface that all SD-enabled BTNRH classes should have
class Preset_SD_Base : public AccessConfigDataOnSD {	
	public:
		Preset_SD_Base() : AccessConfigDataOnSD() {};

		virtual int beginSD_wRetry(SdFat *sd, int n_retries) {
			int count = 0; bool done = 0;
			while ((!done) && (count < n_retries)) {
				int ret_val = sd->begin(SD_CONFIG); count++;
				if (!ret_val) {
					Serial.println("BTNRH_WDRC: Preset_SD_Base: beginSD_wRetry: *** WARNING ***: cannot open SD. sd.begin(SD_CONFIG) = " + String(ret_val));
					if (count < n_retries) { 
						delay(10); 
						Serial.println("    : Trying again..."); 
					} else {
						Serial.println("    : FAILED all attempts.  Returning with error."); 
						return -1;
					}
				} else {
					if (count > 1) { Serial.println("    : Success!"); }
					done = true; //success!
				}
			}
			return 0;
		}

		virtual int readFromSDFile(SdFile *file) { return readFromSDFile(file, String("")); }
		virtual int readFromSDFile(SdFile *file, const String &var_name) = 0; //this needs to be implemented

		virtual int readFromSD(String &filename) {                         SdFat sd; return readFromSD(sd, filename, String("")); }
		virtual int readFromSD(String &filename, const String &var_name) { SdFat sd; return readFromSD(sd, filename, var_name); }
		virtual int readFromSD(SdFat &sd, String &filename) {                        return readFromSD(sd, filename, String("")); }
		virtual int readFromSD(SdFat &sd, String &filename, const String &var_name) = 0;  //this needs to be implemented
		
		virtual void printToSDFile(SdFile *file, const String &var_name) = 0; //this needs to be implemented

		virtual int printToSD(String &filename) {                         return printToSD( filename, String(""), false); }
		virtual int printToSD(String &filename, bool deleteExisting) {    return printToSD( filename, String(""), deleteExisting); }
		virtual int printToSD(String &filename, const String &var_name) { return printToSD( filename, var_name,   false); }
		virtual int printToSD(String &filename, const String &var_name, bool deleteExisting) {SdFat sd;  return printToSD(sd, filename, var_name, deleteExisting);}
		virtual int printToSD(SdFat &sd, String &filename, const String &var_name) { return printToSD(sd, filename, var_name, false); }
		virtual int printToSD(SdFat &sd, String &filename, const String &var_name, bool deleteExisting) = 0; //this needs to be implemeted
};

class CHA_AFC_SD : public BTNRH_WDRC::CHA_AFC, public Preset_SD_Base {  //look in Preset_SD_Base and in AccessConfigDataOnSD.h for more info on the methods and functions used here
	public: 
		CHA_AFC_SD() : BTNRH_WDRC::CHA_AFC(), Preset_SD_Base() {};
		CHA_AFC_SD(const BTNRH_WDRC::CHA_AFC &_afc) : BTNRH_WDRC::CHA_AFC(_afc), Preset_SD_Base() {};
		using Preset_SD_Base::readFromSDFile; //I don't really understand why these are necessary
		using Preset_SD_Base::readFromSD;
		using Preset_SD_Base::printToSD;
		
		virtual int readFromSDFile(SdFile *file, const String &var_name) {
			const int buff_len = 300;
			char line[buff_len];
			
			//find start of data structure
			char targ_str[] = "CHA_AFC";
			//int lines_read = readRowsUntilTargStr(file,line,buff_len,targ_str); //file is incremented so that the next line should be the first part of the DSL data
			int lines_read = readRowsUntilBothTargStrs(file,line,buff_len,targ_str,var_name.c_str()); //file is incremented so that the next line should be the first part of the DSL data
			if (lines_read <= 0) {
				Serial.println("BTNRH_WDRC: CHA_AFC_SD: readFromSDFile: *** could not find start of AFC data in file.");
				return -1;
			}

			// read the overall settings
			if (readAndParseLine(file, line, buff_len, &default_to_active, 1) < 0) return -1;
			if (readAndParseLine(file, line, buff_len, &afl, 1) < 0) return -1;
			if (readAndParseLine(file, line, buff_len, &mu, 1) < 0) return -1;
			if (readAndParseLine(file, line, buff_len, &rho, 1) < 0) return -1;
			if (readAndParseLine(file, line, buff_len, &eps, 1) < 0) return -1;
			
			//write to serial for debugging
			//printAllValues();
		
			return 0;
		}
		
		virtual int readFromSD(SdFat &sd, String &filename_str, const String &var_name) {
			int ret_val = 0;
			char filename[100]; filename_str.toCharArray(filename,99);
			
			SdFile file;
			
			//open SD
			ret_val = beginSD_wRetry(&sd,5); //number of retries
			if (ret_val != 0) { Serial.println("BTNRH_WDRC: CHA_AFC_SD: readFromSD: *** ERROR ***: could not sd.begin()."); return -1; }
			
			//open file
			if (!(file.open(filename,O_READ))) {   //open for reading
				Serial.print("BTNRH_WDRC: CHA_AFC_SD: readFromSD: cannot open file ");
				Serial.println(filename);
				return -1;
			}
			
			//read data
			ret_val = readFromSDFile(&file, var_name);
			
			//close file
			file.close();
			
			//return
			return ret_val;
		}

		
		void printToSDFile(SdFile *file, const String &var_name) {
			char header_str[] = "BTNRH_WDRC::CHA_AFC";
			writeHeader(file, header_str, var_name.c_str());
			
			writeValuesOnLine(file, &default_to_active, 1, true, "enable AFC at startup?", 1);
			writeValuesOnLine(file, &afl,	1, true, "afl, length (samples) of adaptive filter", 1);
			writeValuesOnLine(file, &mu,  1, true, "mu, scale factor for adaptation (bigger is faster)", 1);
			writeValuesOnLine(file, &rho, 1, true, "rho, smoooothing factor for envelope (bigger is longer average)", 1);
			writeValuesOnLine(file, &eps, 1, false, "eps, min value for env (helps avoid divide by zero)", 1);//no trailing comma on the last one 
			
			writeFooter(file); 
		}
	


		int printToSD(SdFat &sd, String &filename_str, const String &var_name, bool deleteExisting) {
			SdFile file;
			char filename[100]; filename_str.toCharArray(filename,99);
			
			//open SD
			int ret_val = beginSD_wRetry(&sd,5); //number of retries
			if (ret_val != 0) { Serial.println("BTNRH_WDRC: CHA_AFC_SD: printToSD: *** ERROR ***: could not sd.begin()."); return -1; }
			
			//delete existing file
			if (deleteExisting) {
				if (sd.exists(filename)) sd.remove(filename);
			}
			
			//open file
			file.open(filename,O_RDWR  | O_CREAT | O_APPEND); //open for writing
			if (!file.isOpen()) {  
				Serial.print("BTNRH_WDRC: CHA_AFC_SD: printToSD: cannot open file ");
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


class CHA_DSL_SD : public BTNRH_WDRC::CHA_DSL, public Preset_SD_Base {  //look in Preset_SD_Base and in AccessConfigDataOnSD.h for more info on the methods and functions used here 
	public:
		CHA_DSL_SD() : BTNRH_WDRC::CHA_DSL(), Preset_SD_Base() {};
		CHA_DSL_SD(const BTNRH_WDRC::CHA_DSL &_dsl) : BTNRH_WDRC::CHA_DSL(_dsl), Preset_SD_Base() {};
		using Preset_SD_Base::readFromSDFile; //I don't really understand why these are necessary
		using Preset_SD_Base::readFromSD;
		using Preset_SD_Base::printToSD;
			
		int readFromSDFile(SdFile *file, const String &var_name) { //returns zero if successful
			//Serial.println("CHA_DSL: readFromSDFile: starting...");
			const int buff_len = 400;
			char line[buff_len];
			
			//find start of data structure
			char targ_str[] = "CHA_DSL";
			//int lines_read = readRowsUntilTargStr(file,line,buff_len,targ_str); //file is incremented so that the next line should be the first part of the DSL data
			int lines_read = readRowsUntilBothTargStrs(file,line,buff_len,targ_str,var_name.c_str());//file is incremented so that the next line should be the first part of the DSL data
			if (lines_read <= 0) {
				Serial.println("BTNRH_WDRC: CHA_DSL_SD: readFromSDFile: *** could not find start of DSL data in file.");
				return -1;
			}
			//Serial.print("BTNRH_WDRC: CHA_DSL: readFromSDFile: DSL Structure Starts at line "); Serial.println(lines_read);
			
			// read the overall settings
			if (readAndParseLine(file, line, buff_len, &attack, 1) < 0) return -1;
			if (readAndParseLine(file, line, buff_len, &release, 1) < 0) return -1;
			if (readAndParseLine(file, line, buff_len, &maxdB, 1) < 0) return -1;
			if (readAndParseLine(file, line, buff_len, &ear, 1) < 0) return -1;
			if (readAndParseLine(file, line, buff_len, &nchannel, 1) < 0) return -1;
			if (nchannel > get_DSL_MAX_CHAN()) {
				Serial.print("BTNRH_WDRC: CHA_DSL_SD: readFromSDFile: *** ERROR*** nchannel read as ");Serial.print(nchannel);
				nchannel = get_DSL_MAX_CHAN();
				Serial.print("    : Limiting to "); Serial.println(nchannel);
			}
			
			//read the per-channel settings
			if (readAndParseLine(file, line, buff_len, cross_freq, nchannel) < 0) return -1;				
			if (readAndParseLine(file, line, buff_len, exp_cr, nchannel) < 0) return -1;
			if (readAndParseLine(file, line, buff_len, exp_end_knee, nchannel) < 0) return -1;
			if (readAndParseLine(file, line, buff_len, tkgain, nchannel) < 0) return -1;
			if (readAndParseLine(file, line, buff_len, cr, nchannel) < 0) return -1;
			if (readAndParseLine(file, line, buff_len, tk, nchannel) < 0) return -1;
			if (readAndParseLine(file, line, buff_len, bolt, nchannel) < 0) return -1;

			//write to serial for debugging
			//Serial.println("CHA_DSL_SD: readFromSDFile: success!");
			//printAllValues();
		
			return 0;
		}
		
		//virtual int readFromSD(String &filename, const String &var_name) {
		//	return Preset_SD_Base::readFromSD(filename, var_name); //why do I need to do this explicitly?  Should happen automatically by inheritance!
		//}
		int readFromSD(SdFat &sd, String &filename_str, const String &var_name) {
			SdFile file;
			int ret_val=0;
			char filename[100]; filename_str.toCharArray(filename,99);
			
			//open SD
			ret_val = beginSD_wRetry(&sd,5); //number of retries
			if (ret_val != 0) { Serial.println("BTNRH_WDRC: CHA_DSL_SD: readFromSD: *** ERROR ***: could not sd.begin()."); return -1; }
			
			//open file
			if (!(file.open(filename,O_READ))) {   //open for reading
				Serial.print("BTNRH_WDRC: CHA_DSL_SD: readFromSD: cannot open file ");
				Serial.println(filename);
				return -1;
			}
			
			//read data
			ret_val = readFromSDFile(&file,var_name);
			
			//close file
			file.close();
			
			//return
			return ret_val;
		}
	
	
		void printToSDFile(SdFile *file, const String &var_name) {
			char header_str[] = "BTNRH_WDRC::CHA_DSL";
			writeHeader(file, header_str, var_name.c_str());
			
			writeValuesOnLine(file, &attack,      1, true, "atttack (msec)", 1);
			writeValuesOnLine(file, &release,     1, true, "release (msec) ", 1);
			writeValuesOnLine(file, &maxdB,       1, true, "max dB", 1);
			writeValuesOnLine(file, &ear,         1, true, "ear (0=left, 1=right)", 1);
			writeValuesOnLine(file, &nchannel,    1, true, "number of channels", 1);
			writeValuesOnLine(file, cross_freq,   nchannel, true, "Crossover Freq (Hz)", DSL_MXCH_TYMPAN);
			writeValuesOnLine(file, exp_cr,       nchannel, true, "Expansion: Compression Ratio", DSL_MXCH_TYMPAN);
			writeValuesOnLine(file, exp_end_knee, nchannel, true, "Expansion: Knee (dB SPL)", DSL_MXCH_TYMPAN);
			writeValuesOnLine(file, tkgain,       nchannel, true, "Linear: Gain (dB)", DSL_MXCH_TYMPAN);
			writeValuesOnLine(file, cr,           nchannel, true, "Compression: Compression Ratio", DSL_MXCH_TYMPAN);
			writeValuesOnLine(file, tk,           nchannel, true,"Compression: Knee (dB SPL)", DSL_MXCH_TYMPAN);
			writeValuesOnLine(file, bolt,         nchannel, false, "Limiter: Knee (dB SPL)", DSL_MXCH_TYMPAN); //no trailing comma on the last one	
			
			writeFooter(file); 
		}
		
		
		int printToSD(SdFat &sd, String &filename_str, const String &var_name, bool deleteExisting) {
			SdFile file;
			char filename[100]; filename_str.toCharArray(filename,99);
			
			//open SD
			int ret_val = beginSD_wRetry(&sd,5); //number of retries
			if (ret_val != 0) { Serial.println("BTNRH_WDRC: CHA_DSL_SD: printToSD: *** ERROR ***: could not sd.begin()."); return -1; }
			
			//delete existing file
			if (deleteExisting) {
				if (sd.exists(filename)) sd.remove(filename);
			}
			
			
			//open file
			file.open(filename,O_RDWR  | O_CREAT | O_APPEND); //open for writing
			if (!file.isOpen()) {  
				Serial.print("BTNRH_WDRC: CHA_DSL_SD: printToSD: cannot open file ");
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


class CHA_WDRC_SD : public BTNRH_WDRC::CHA_WDRC, public Preset_SD_Base {   //look in Preset_SD_Base and in AccessConfigDataOnSD.h for more info on the methods and functions used here
	public:
		CHA_WDRC_SD() : BTNRH_WDRC::CHA_WDRC(), Preset_SD_Base() {};
		CHA_WDRC_SD(const BTNRH_WDRC::CHA_WDRC &_wdrc) : BTNRH_WDRC::CHA_WDRC(_wdrc), Preset_SD_Base() {};
		using Preset_SD_Base::readFromSDFile;
		using Preset_SD_Base::readFromSD;
		using Preset_SD_Base::printToSD;
		int readFromSDFile(SdFile *file, const String &var_name) {
			const int buff_len = 300;
			char line[buff_len];
			
			//find start of data structure
			char targ_str[] = "CHA_WDRC";
			//int lines_read = readRowsUntilTargStr(file,line,buff_len,targ_str); //file is incremented so that the next line should be the first part of the DSL data
			int lines_read = readRowsUntilBothTargStrs(file,line,buff_len,targ_str,var_name.c_str());//file is incremented so that the next line should be the first part of the DSL data
			if (lines_read <= 0) {
				Serial.println("BTNRH_WDRC: CHA_WDRC: readFromSDFile: *** could not find start of DSL data in file.");
				return -1;
			}
			//Serial.print("BTNRH_WDRC: CHA_WDRC: readFromSDFile: DSL Structure Starts at line "); Serial.println(lines_read);
			
			// read the overall settings
			if (readAndParseLine(file, line, buff_len, &attack, 1) < 0) return -1;
			if (readAndParseLine(file, line, buff_len, &release, 1) < 0) return -1;
			if (readAndParseLine(file, line, buff_len, &fs, 1) < 0) return -1;
			if (readAndParseLine(file, line, buff_len, &maxdB, 1) < 0) return -1;
			if (readAndParseLine(file, line, buff_len, &exp_cr, 1) < 0) return -1;
			if (readAndParseLine(file, line, buff_len, &exp_end_knee, 1) < 0) return -1;
			if (readAndParseLine(file, line, buff_len, &tkgain, 1) < 0) return -1;
			if (readAndParseLine(file, line, buff_len, &tk, 1) < 0) return -1;
			if (readAndParseLine(file, line, buff_len, &cr, 1) < 0) return -1;
			if (readAndParseLine(file, line, buff_len, &bolt, 1) < 0) return -1;

			//write to serial for debugging
			//printAllValues();
		
			return 0;
		}
	
		
		int readFromSD(SdFat &sd, String &filename_str, const String &var_name) {
			SdFile file;
			int ret_val=0;
			char filename[100]; filename_str.toCharArray(filename,99);
			
			//open SD
			ret_val = beginSD_wRetry(&sd,5); //number of retries
			if (ret_val != 0) { Serial.println("BTNRH_WDRC: CHA_WDRC_SD: readFromSD: *** ERROR ***: could not sd.begin()."); return -1; }
			
			//open file
			if (!(file.open(filename,O_READ))) {   //open for reading
				Serial.print("BTNRH_WDRC: CHA_WDRC_SD: readFromSD: cannot open file ");
				Serial.println(filename);
				return -1;
			}
			
			//read data
			ret_val = readFromSDFile(&file, var_name);
			
			//close file
			file.close();
			
			//return
			return ret_val;
		}	

		void printToSDFile(SdFile *file, const String &var_name) {
			char header_str[] = "BTNRH_WDRC::CHA_WDRC";
			writeHeader(file, header_str, var_name.c_str());
			
			writeValuesOnLine(file, &attack, 1, true, "atttack (msec)", 1);
			writeValuesOnLine(file, &release, 1, true, "release (msec) ", 1);
			writeValuesOnLine(file, &fs, 1, true, "sample rate (ignored)", 1);
			writeValuesOnLine(file, &maxdB, 1, true, "max dB", 1);
			writeValuesOnLine(file, &exp_cr, 1, true, "Expansion: Compression Ratio", 1);
			writeValuesOnLine(file, &exp_end_knee, 1, true, "Expansion: Knee (dB SPL)", 1);
			writeValuesOnLine(file, &tkgain, 1, true, "Linear: Gain (dB)", 1);
			writeValuesOnLine(file, &tk, 1, true, "Compression: Knee (dB SPL)", 1);
			writeValuesOnLine(file, &cr, 1, true, "Compression: Compression Ratio", 1);
			writeValuesOnLine(file, &bolt, 1, false, "Limiter: Knee (dB SPL)", 1);  //no trailing comma on the last one
			
			writeFooter(file); 
		}
		
		
		int printToSD(SdFat &sd, String &filename_str, const String &var_name, bool deleteExisting) {
			SdFile file;
			char filename[100]; filename_str.toCharArray(filename,99);
			
			//open SD
			int ret_val = beginSD_wRetry(&sd,5); //number of retries
			if (ret_val != 0) { Serial.println("BTNRH_WDRC: CHA_WDRC_SD: printToSD: *** ERROR ***: could not sd.begin()."); return -1; }
			
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

#endif