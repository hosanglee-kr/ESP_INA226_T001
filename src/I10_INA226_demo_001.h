
#pragma once

//
//    FILE: INA226_demo.ino
//  AUTHOR: Rob Tillaart
// PURPOSE: demo
//     URL: https://github.com/RobTillaart/INA226

#include "INA226.h"

INA226 g_I10_INA(0x40);


void I10_INA225_init(){
	Wire.begin();
	if (!g_I10_INA.begin()) {
		Serial.println("could not connect. Fix and Reboot");
	}

	g_I10_INA.setMaxCurrentShunt(1, 0.002);
}

void I10_init() {
	Serial.print("INA226_LIB_VERSION: ");
	Serial.println(INA226_LIB_VERSION);

	I10_INA225_init();
	
}

void I10_run() {
	Serial.println("\nBUS\tSHUNT\tCURRENT\tPOWER");
	for (int i = 0; i < 20; i++) {
		Serial.print(g_I10_INA.getBusVoltage(), 3);
		Serial.print("\t");
		Serial.print(g_I10_INA.getShuntVoltage_mV(), 3);
		Serial.print("\t");
		Serial.print(g_I10_INA.getCurrent_mA(), 3);
		Serial.print("\t");
		Serial.print(g_I10_INA.getPower_mW(), 3);
		Serial.println();
		delay(1000);
	}
}

//  -- END OF FILE --
