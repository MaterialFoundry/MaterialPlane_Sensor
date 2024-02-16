#ifndef _UMS3_H
#define _UMS3_H

#include <Arduino.h>
#include <esp_adc_cal.h>
#include <soc/adc_channel.h>

#define VBAT_ADC_CHANNEL ADC1_GPIO10_CHANNEL

class UMS3 {
  public:
    UMS3() {}

    void begin() {

      esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_2_5, ADC_WIDTH_BIT_12, 0, &adc_cal);
      adc1_config_channel_atten(VBAT_ADC_CHANNEL, ADC_ATTEN_DB_2_5);

      pinMode(VBUS_SENSE, INPUT);
      pinMode(BATTERY_STAT, INPUT);
    }

    float GetBatteryVoltage() {
      uint32_t raw = adc1_get_raw(VBAT_ADC_CHANNEL);
      uint32_t millivolts = esp_adc_cal_raw_to_voltage(raw, &adc_cal);
      const uint32_t upper_divider = 442;
      const uint32_t lower_divider = 160;
      return (float)(upper_divider + lower_divider) / lower_divider / 1000 * millivolts;
    }

    bool getVbusPresent() {
      return digitalRead(VBUS_SENSE);
    }

    bool IsChargingBattery()
    {
        int measuredVal = 0;
        for ( int i = 0; i < 10; i++ )
        {
            int v = digitalRead( BATTERY_STAT );
            measuredVal += v;
        }

        return ( measuredVal == 0);
    }

  private:
    esp_adc_cal_characteristics_t adc_cal;
};

#endif
