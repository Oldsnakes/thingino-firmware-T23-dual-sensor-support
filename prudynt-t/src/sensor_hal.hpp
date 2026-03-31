#ifndef SENSOR_HAL_HPP
#define SENSOR_HAL_HPP

#include <vector>
#include <string>
#include <cstdint>

namespace sensor_hal {

//### TW ###
int SetSensorRegister(int sensor, uint32_t reg, uint32_t value);
int GetSensorRegister(int sensor, uint32_t reg, uint32_t *value);
int sensor_set_expo(int sensor, int value);
int sensor_set_again(int sensor, int value);
int sensor_set_integration_time(int sensor, int value);
int sensor_detect(int sensor, unsigned int *ident);
//int sensor_init(int sensor, int enable);
int sensor_set_fps(int sensor, int fps);
int sensor_set_vflip(int sensor, int enable);
int sensor_set_vhflip(int sensor, int val);

int sensor_g_chip_ident(int sensor);
int sensor_s_stream(int sensor, int enable);
int sensor_write_array(int sensor, struct regval_list *vals);
} // namespace sensor_hal

#endif // SENSOR_HAL_HPP
