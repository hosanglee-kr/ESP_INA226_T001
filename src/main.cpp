#include <Arduino.h>




//#define		I10_INA

#ifdef I10_INA
	#include "I10_INA226_demo_001.h"
#endif

#define     J10_MULTI_METER


#ifdef J10_MULTI_METER
	#include "J10_multi_meter/J100_main.h"
#endif

void setup(){
    Serial.begin(115200);
	Serial.println(__FILE__);
	

    #ifdef I10_INA
	    I10_init();
    #endif

    #ifdef J10_MULTI_METER
	    J100_init();
    #endif
}

void loop(){
    #ifdef I10_INA
	    I10_run();
    #endif

    #ifdef J10_MULTI_METER
	    J100_run();
    #endif
}