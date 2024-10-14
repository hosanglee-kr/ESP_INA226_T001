#ifndef CONFIG_H_
#define CONFIG_H_

#define CORE_0				 0
#define CORE_1				 1

#define pinFET1				 18	 // switch in the 1.0ohm shunt
#define pinFET05			 19	 // switch in the 0.05ohm shunt
#define pinSDA				 22	 // I2C interface to INA226
#define pinSCL				 21	 //        -do-
#define pinGate				 4	 // external current monitor gate signal
#define pinAlert			 5

#define pinLED				 14


typedef enum {
	  MODE_CURRENT_VOLTAGE
	, MODE_FREQUENCY
	, MODE_INVALID
} MeasureMode;

// int main ( ) {
// 	MeasureMode ds1; //ok
// }
 

#define MODE_CURRENT_VOLTAGE 11
#define MODE_FREQUENCY		 22
#define MODE_INVALID		 33

typedef struct {
	// input
	uint16_t cfg;
	int		 scale;
	int		 nSamples;
	uint32_t periodUs;
	
	// output
	float sampleRate;  // Hz
	float vavg;		   // volts
	float vmax;
	float vmin;
	float iavgma;  // mA
	float imaxma;
	float iminma;
} CV_MEASURE_t;

typedef struct {
	int frequencyHz;
} FREQ_MEASURE_t;

typedef struct {
	int mode;
	union {
		CV_MEASURE_t   cv_meas;
		FREQ_MEASURE_t f_meas;
	} m;
} MEASURE_t;

extern volatile MEASURE_t Measure;
extern volatile int16_t*  Buffer;

#endif