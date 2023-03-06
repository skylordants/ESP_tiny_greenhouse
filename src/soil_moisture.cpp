#include "soil_moisture.h"
#include "esp_log.h"

const static char *TAG = "SOIL_MOISTURE";

CapacitiveMoistureSensor::CapacitiveMoistureSensor (adc_unit_t adc, adc_channel_t channel)
	: _init_config({ .unit_id = adc, .ulp_mode = ADC_ULP_MODE_DISABLE})
	, _channel(channel)
	, _config({.atten = ADC_ATTEN_DB_11, .bitwidth = ADC_BITWIDTH_DEFAULT})
{
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&_init_config, &_adc_handle));
	ESP_ERROR_CHECK(adc_oneshot_config_channel(_adc_handle, _channel , &_config));

	_do_calibration = adc_calibration_init();
}

CapacitiveMoistureSensor::~CapacitiveMoistureSensor() {
	ESP_ERROR_CHECK(adc_oneshot_del_unit(_adc_handle));
	if (_do_calibration) {
		adc_calibration_deinit();
	}
}

bool CapacitiveMoistureSensor::adc_calibration_init() {
	esp_err_t ret = ESP_FAIL;
	bool calibrated = false;

    ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
	adc_cali_curve_fitting_config_t cali_config = {
		.unit_id = _init_config.unit_id,
		.atten = _config.atten,
		.bitwidth = _config.bitwidth,
	};

	ret = adc_cali_create_scheme_curve_fitting(&cali_config, &_adc_cali_handle);

	if (ret == ESP_OK) {
		calibrated = true;
	}

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

bool CapacitiveMoistureSensor::adc_calibration_deinit() {
	ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(_adc_cali_handle));

	return true;
}

bool CapacitiveMoistureSensor::measure() {
	ESP_ERROR_CHECK(adc_oneshot_read(_adc_handle, _channel, &_adc_raw[0]));
	ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", _init_config.unit_id + 1, _channel, _adc_raw[0]);
	if (_do_calibration) {
		ESP_ERROR_CHECK(adc_cali_raw_to_voltage(_adc_cali_handle, _adc_raw[0], &_voltage[0]));
		ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", _init_config.unit_id + 1, _channel, _voltage[0]);
	}

	return true;
}

int CapacitiveMoistureSensor::getVoltage() {
	return _voltage[0];
}
