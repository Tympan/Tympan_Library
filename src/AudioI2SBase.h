/*
	AudioI2SBase

	Created: Chip Audette, Open Audio
	Purpose: Base class to be inherited by all I2S classes.  
	    Holds config values that must always be the same across
			I2S input/output classes.

	License: MIT License.  Use at your own risk.
 */

#ifndef audioI2SBase_h_
#define audioI2SBase_h_

//Base class to hold items that should be common to all I2S classes
class AudioI2SBase {
	//GUI: inputs:0, outputs:0  //this line used for automatic generation of GUI node
	public:
		AudioI2SBase(void) {};
		virtual ~AudioI2SBase() {};
		
	protected:
		static bool transferUsing32bit;  //I2S transfers using 32 bit values (instead of 16-bit).  See instantiation in the cpp file  for the default and see begin() and config_i2s() for changes
};

#endif
