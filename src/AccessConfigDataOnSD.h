

#ifndef _AccessConfigDataOnSD_h
#define _AccessConfigDataOnSD_h

class AccessConfigDataOnSD {
	public: 
		virtual int trimLeadingWhitespace(char *line, int buff_len) {
			int pos = 0; 
			//find first non whitespace
			while (pos < buff_len) {
				if ((line[pos] != ' ') && (line[pos] != '\t')) break;
				pos++;
			}
			
			//shift left
			if (pos > 0) {
				int targ_ind = 0;
				int source_ind = targ_ind + pos;
				while (source_ind <= buff_len) {
					line[targ_ind] = line[source_ind];
					
					//increment in anticipation of the next step
					targ_ind++;
					source_ind = targ_ind + pos;
				}
				buff_len = targ_ind-1; //step back one because the while loop will have gone one too far
			}
			return buff_len;
		}
		virtual int trimComments(char *line, int buff_len) { //stop the line wherever a comment starts
			int pos = 0;
			while (pos < buff_len) {
				if (line[pos] == '/') 
					if (pos > 0) {
						if (line[pos-1] == '/') {
							line[pos-1] = '\0';  //terminate here back one
							line[pos] = '\0';
						}
					}
				if (line[pos] == '*') {  //this one is a bit more dangerous!
					if (pos > 0) {
						if (line[pos-1] == '/') {
							line[pos-1] = '\0';  //terminate the line back one
							line[pos] = '\0';
						}
					}
					line[pos] = '\0';//terminate the line here
					
				}
				if (line[pos] == '#') line[pos] = '\0';  //terminate the line here
				if (line[pos] == '\n')	line[pos] = '\0';//terminate the line here
				if (line[pos] == '\r')	line[pos] = '\0';//terminate the line here
			
				if (line[pos] == '\0') break;

				pos++;
				
			}
			if (pos > 0) { if (line[pos-1] == '\0') { pos = pos-1; } }
			return pos;
		}

		virtual bool isInString(char *line, int buff_len, const char *targ_str) {
			int targ_len = strlen(targ_str);
			if (targ_len <= 0) return false;
			
			const int n = buff_len;
			bool is_match = false;
			if (n >= targ_len) {  //is it long enough to fit the target string?
				for (int i=0; i < (n - targ_len); i++) { //step through the entire buffer
					is_match = true;
					for (int j=0; j < targ_len; j++) { //step through the target string
						if (line[i+j] != targ_str[j]) { //does it NOT match the target string?
							is_match = false;
							break;
						}
					}
					if (is_match) return true;
				}
			}
			return false;
		}
		
		
		int getNonEmptyLine(SdFile_Gre *file, char *line, int buff_len) {
			int n=0;
			while (n==0) { 
				if ((n = file->fgets(line, buff_len)) <= 0) return -1; 
				n=trimComments(line,n);  //stop the line wherever a commen starts
				n=trimLeadingWhitespace(line,n);  //stop the line wherever a commen starts
			}
			return n;
		}
		
		//read and parse floating point values from a line
		int readAndParseLine(SdFile_Gre *file, char *line, int buff_len, float out_val[], int n_val, int round_digit = 99) {
			int nchar;
			if ((nchar = getNonEmptyLine(file, line, buff_len)) <= 0) return -1;
			char *p_str = line;
			if (p_str[0]=='{') p_str++;
			float scale_fac = 1.0;
			if (round_digit <= 10) scale_fac = powf(10.0,round_digit); 
			for (int Ichan = 0; Ichan < n_val; Ichan++) {
				if (round_digit > 10) {	
					out_val[Ichan] = (float)strtod(p_str, &p_str);  //p_Str is a pointer inside "line" where the float value ended
				} else {
					//round to the given significant digit
					out_val[Ichan] = (float) ( ((int) ( (scale_fac * strtod(p_str, &p_str))+0.5 ) ) / scale_fac );//p_Str is a pointer inside "line" where the float value ended
				}
				if (p_str[0]==',') p_str++;
			}
			return 0;
		}

		//read and parse integer values from a line
		int readAndParseLine(SdFile_Gre *file, char *line, int buff_len, int out_val[], int n_val) {
			int nchar;
			if ((nchar = getNonEmptyLine(file, line, buff_len)) <= 0) return -1;
			char *p_str = line;
			if (p_str[0]=='{') p_str++;
			for (int Ichan = 0; Ichan < n_val; Ichan++) {
				out_val[Ichan] = (int)(strtod(p_str, &p_str)+0.5);  //p_Str is a pointer inside "line" where the float value ended
				if (p_str[0]==',') p_str++;
			}
			return 0;
		}

		int readRowsUntilTargStr(SdFile_Gre *file, char *line, int buff_len, const char *targ_str) {
			//bool found_start = false;
			int n;

			
			//Serial.println("BTNRH_WDRC: CHA_DSL: findStartOfDSL: reading file...");
			int line_count = 0;
			while ((n = file->fgets(line, buff_len)) > 0) {
				line_count++;
				
				//Serial.print(n);Serial.print(": ");
				n=trimComments(line,n);  //stop the line wherever a commen starts
				//Serial.print(n);Serial.print(": ");
				n=trimLeadingWhitespace(line,n);  //stop the line wherever a commen starts
				//Serial.print(n);Serial.print(": ");
				
				bool is_match = isInString(line, n, targ_str);					
				if (is_match) return line_count;  //find if in string
		
			}
			//Serial.println(F("\nDone"));
			return -1;
		}			
		
		void writeHeader(SdFile_Gre *file, const char *type_name, const char *var_name) {
			file->println();
			file->print(type_name);
			file->print(" ");
			file->print(var_name);
			file->print(" {");
			file->println();
		}
		
		//write integer values
		void writeValuesOnLine(SdFile_Gre *file, int out_val[], int n_val, bool trailing_comma, const char* comment, int total_in_row = -1) {
			if (total_in_row < n_val) total_in_row = n_val;
			file->print(" ");
			if (total_in_row == 1) { //write a single value
				file->print(out_val[0]); 
			} else {  //write many values
				file->print("{ ");
				for (int i=0; i < total_in_row; i++) {
					if (i < n_val) {
						file->print(out_val[i]);
					} else {
						file->print(out_val[n_val-1]);
					}
					if (i < (total_in_row-1)) file->print(", ");
				}
				file->print("}");
			}
			if (trailing_comma) file->print(",");
			file->print(" \t// "); file->print(comment);
			file->println();
		}

		//write float values
		void writeValuesOnLine(SdFile_Gre *file, float out_val[], int n_val, bool trailing_comma, const char* comment, int total_in_row = -1) {
			if (total_in_row < n_val) total_in_row = n_val;
			file->print(" ");
			if (total_in_row == 1) {  //write a single value
				file->print(out_val[0],5); 
			} else { //write many values
				file->print("{ ");
				for (int i=0; i < total_in_row; i++) {
					if (i < n_val) {
						file->print(out_val[i],5);
					} else {
						file->print(out_val[n_val-1],5);
					}
					if (i < (total_in_row-1)) file->print(", ");
				}
				file->print("}");
			}
			if (trailing_comma) file->print(",");
			file->print(" \t// "); file->print(comment);
			file->println();
		}
		void writeFooter(SdFile_Gre *file) {
			file->println("};");
			file->println();
		}
		
};


#endif