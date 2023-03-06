#ifndef __SOIL_MOISTURE_H__
#define __SOIL_MOISTURE_H__

#include "esp_adc\adc_oneshot.h"
#include "esp_adc\adc_cali.h"
#include "esp_adc\adc_cali_scheme.h"

class CapacitiveMoistureSensor {
public:
	CapacitiveMoistureSensor(adc_unit_t adc, adc_channel_t channel);
	CapacitiveMoistureSensor();
	~CapacitiveMoistureSensor();

	bool measure();
	int getVoltage();

private:
	bool adc_calibration_init();
	bool adc_calibration_deinit();
	
	
	adc_oneshot_unit_handle_t _adc_handle;
	adc_oneshot_unit_init_cfg_t _init_config;
	adc_channel_t _channel;
	adc_oneshot_chan_cfg_t _config;

	adc_cali_handle_t _adc_cali_handle;

	bool _do_calibration;

	int _adc_raw[10];
	int _voltage[10];
};

#endif