#include <Arduino.h>



// https://github.com/RobTillaart/INA226
//#define		I10_INA
#ifdef I10_INA
	#include "I10/I10_INA226_demo_001.h" 
#endif


//#define     J10_MULTI_METER
#ifdef J10_MULTI_METER
	#include "J10_multi_meter/J100_main.h"
#endif

// https://github.com/har-in-air/ESP32_MULTI_METER
#define     K10_MULTI_METER_T2     // MULTI_METER type 2
#ifdef K10_MULTI_METER_T2
	#include "K10/K10_main_002.h"
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


    #ifdef K10_MULTI_METER_T2
        K10_init();
    #endif

}

void loop(){
    #ifdef I10_INA
	    I10_run();
    #endif

    #ifdef J10_MULTI_METER
	    J100_run();
    #endif

    #ifdef K10_MULTI_METER_T2
        K10_run();
    #endif
}
