#ifndef FREQ_METER_H_
#define FREQ_METER_H_

extern volatile bool FreqReadyFlag;
extern volatile bool FreqCaptureFlag;
extern volatile int FrequencyHz;

#define MSG_TX_FREQUENCY 5555

extern uint32_t OscFreqHz;
extern bool OscFreqFlag;

void J300_task_freq_counter(void* pvParam);

#endif