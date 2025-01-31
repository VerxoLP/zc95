#include "CFpAnalog.h"
#include <string.h>

#define ROT_A   6
#define ROT_B   7

/*
 * Front pannel which has 4x potentiometers conencted to an I2C ADC, 
 * and 1x rotary encoder connected to an I2C port expander
 */

CFpAnalog::CFpAnalog(CSavedSettings *saved_settings)
{
    memset(_power_level, 0, sizeof(_power_level));
    _last_port_exp_read = 0;
    _adjust_value = 0;
}

void CFpAnalog::interupt (port_exp exp)
{
    _interrupt = true;
}

void CFpAnalog::process(bool always_update)
{
    if (_interrupt || always_update)
    {
        _interrupt = false;
        read_adc();

        uint8_t buffer;
        buffer = read_port_expander();

        if (_last_port_exp_read != buffer)
        {
            rot_encoder_process(buffer, ROT_A, ROT_B);

            if (abs(_adjust_value < 100))
                _adjust_value += _rot_encoder.get_rotary_encoder_change();

            _last_port_exp_read = buffer;
        }
    }
}

uint16_t CFpAnalog::get_channel_power_level(uint8_t channel)
{
    if (channel >= MAX_CHANNELS)
        return 0;

    return _power_level[channel];
}


int8_t CFpAnalog::get_adjust_control_change()
{
    int8_t retval = _adjust_value;
    _adjust_value = 0;
    return retval;
}

 
void CFpAnalog::read_adc()
{
    uint8_t command = 0x04;
    int bytes_written = i2c_write_timeout_us(i2c_default, ADC_ADDR, &command, 1, false, 2000);
    if (bytes_written != 1)
    {
        printf("CFpAnalog::read_adc() write failed! i2c bytes_written = %d\n", bytes_written);
        return;
    }


    uint8_t buffer[5];
    int retval = i2c_read_timeout_us(i2c_default, ADC_ADDR, buffer, sizeof(buffer), false, 1000);
    if (retval == PICO_ERROR_GENERIC || retval == PICO_ERROR_TIMEOUT)
    {
      printf("CFpAnalog::read_adc() i2c read error!\n");
      return;
    }

    //printf("ADC: %d\t %d\t %d\t %d\n", buffer[1],buffer[2],buffer[3],buffer[4]);
    // buf : chan
    // 1   : 1
    // 2   : 4
    // 3   : 3
    // 4   : 2

    _power_level[0] = (float)1000 - (float)buffer[1] * 3.91;
    _power_level[1] = (float)1000 - (float)buffer[4] * 3.91;
    _power_level[2] = (float)1000 - (float)buffer[3] * 3.91;
    _power_level[3] = (float)1000 - (float)buffer[2] * 3.91;
}

void CFpAnalog::rot_encoder_process(uint8_t port_exp_read, uint8_t a_pos, uint8_t b_pos)
{
    uint8_t a = (port_exp_read & (1 << a_pos) ? 1 : 0);
    uint8_t b = (port_exp_read & (1 << b_pos ) ? 1 : 0);

    _rot_encoder.process(a, b);
}

uint8_t CFpAnalog::read_port_expander()
{
    uint8_t buffer[1];
    
    int retval = i2c_read_timeout_us(i2c_default, FP_ANALOG_PORT_EXP_2_ADDR, buffer, 1, false, 1000);
    if (retval == PICO_ERROR_GENERIC || retval == PICO_ERROR_TIMEOUT)
    {
      printf("CFpAnalog::read_port_expander i2c read error!\n");
      return 0;
    }

    return buffer[0];
}