#include "sensor_hal.hpp"
#include <imp/imp_isp.h>

#include "Logger.hpp"
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype> 
#include <fstream> 
#include <unistd.h>
#include <fcntl.h>
#include <cstddef>
#include <stdio.h>

#undef SENSOR_DEBUG

#define SENSOR_NAME "gc1084"
#define SENSOR_NAME_1 "gc1084s1"
#define SENSOR_VERSION "H20231012a"
#define SENSOR_CHIP_ID 0x1084
#define SENSOR_CHIP_ID_H (0x10)
#define SENSOR_CHIP_ID_L (0x84)
#define SENSOR_BUS_TYPE TX_SENSOR_CONTROL_INTERFACE_I2C
#define SENSOR_I2C_BUS 0x00
#define SENSOR_I2C_ADDRESS 0x37
#define SENSOR_MAX_WIDTH 1280
#define SENSOR_MAX_HEIGHT 720
#define SENSOR_REG_END 0xffff
#define SENSOR_REG_DELAY 0xfffe
#define SENSOR_SUPPORT_30FPS_SCLK (49500000) /* 2200 * 750 *30 */
#define SENSOR_OUTPUT_MAX_FPS 30
#define SENSOR_OUTPUT_MIN_FPS 5

namespace sensor_hal {

struct regval_list {
	uint16_t reg_num;
	unsigned char value;
};

struct again_lut {
	int index;
	unsigned char reg0d1;
	unsigned char reg0d0;
	unsigned char regdc1;
	unsigned char reg0b8;
	unsigned char reg0b9;
	unsigned char reg155;
	unsigned int gain;
};

struct again_lut sensor_again_lut[] = {
	{0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0},             //1.000000
	{0x01, 0x0a, 0x00, 0x00, 0x01, 0x0c, 0x00, 16208},         //1.187500
	{0x02, 0x00, 0x01, 0x00, 0x01, 0x1a, 0x00, 32217},         //1.406250
	{0x03, 0x0a, 0x01, 0x00, 0x01, 0x2a, 0x00, 47690},         //1.656250
	{0x04, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 65536},         //2.000000
	{0x05, 0x0a, 0x02, 0x00, 0x02, 0x18, 0x00, 81784},         //2.375000
	{0x06, 0x00, 0x03, 0x00, 0x02, 0x33, 0x00, 97213},         //2.796875
	{0x07, 0x0a, 0x03, 0x00, 0x03, 0x14, 0x00, 113226},        //3.312500
	{0x08, 0x00, 0x04, 0x00, 0x04, 0x00, 0x02, 131072},        //4.000000
	{0x09, 0x0a, 0x04, 0x00, 0x04, 0x2f, 0x02, 147001},        //4.734375
	{0x0a, 0x00, 0x05, 0x00, 0x05, 0x26, 0x02, 162766},        //5.593750
	{0x0b, 0x0a, 0x05, 0x00, 0x06, 0x29, 0x02, 178990},        //6.640625
	{0x0c, 0x00, 0x06, 0x00, 0x08, 0x00, 0x02, 196608},        //8.000000
	{0x0d, 0x0a, 0x06, 0x00, 0x09, 0x1f, 0x04, 212696},        //9.484375
	{0x0e, 0x12, 0x46, 0x00, 0x0b, 0x0d, 0x04, 228311},        //11.203125
	{0x0f, 0x19, 0x66, 0x00, 0x0d, 0x12, 0x06, 244312},        //13.265625
	{0x10, 0x00, 0x04, 0x01, 0x10, 0x00, 0x06, 262144},        //16.000000
	{0x11, 0x0a, 0x04, 0x01, 0x12, 0x3e, 0x08, 278232},        //18.953125
	{0x12, 0x00, 0x05, 0x01, 0x16, 0x1a, 0x0a, 293982},        //22.406250
	{0x13, 0x0a, 0x05, 0x01, 0x1a, 0x23, 0x0c, 310012},        //26.546875
	{0x14, 0x00, 0x06, 0x01, 0x20, 0x00, 0x0c, 327680},        //32.000000
	{0x15, 0x0a, 0x06, 0x01, 0x25, 0x3b, 0x0f, 343731},        //37.921875
	{0x16, 0x12, 0x46, 0x01, 0x2c, 0x33, 0x12, 359419},        //44.796875
	{0x17, 0x19, 0x66, 0x01, 0x35, 0x06, 0x14, 375411},        //53.093750
	{0x18, 0x20, 0x06, 0x01, 0x3f, 0x3f, 0x15, 393216},        //64.000000
	{0x19, 0x0a, 0x04, 0x01, 0x4c, 0x38, 0x17, 467589},        //75.840
	{0x1a, 0x00, 0x04, 0x01, 0x59, 0x26, 0x18, 551000},        //89.582
	{0x1b, 0x0a, 0x05, 0x01, 0x69, 0x0c, 0x1a, 643415},        //106.155
	{0x1c, 0x00, 0x05, 0x01, 0x80, 0x00, 0x1b, 744532},        //128.000
};

uint32_t max_again = 744532;    // - 128x
uint32_t max_dgain = 0;
uint32_t min_integration_time = 2;
uint32_t min_integration_time_native = 2;
uint32_t max_integration_time_native = 734;
uint32_t integration_time_limit = 734;
uint32_t total_width = 2200;
uint32_t total_height = 750;
uint32_t max_integration_time = 734;
uint32_t one_line_expr_in_us = 30;
uint32_t integration_time_apply_delay = 2;
uint32_t again_apply_delay = 2;
uint32_t dgain_apply_delay = 0;

#if 0
static struct tx_isp_sensor_win_setting sensor_win_sizes[] = {
	{
		.width = 1280,
		.height = 720,
		.fps = 30 << 16 | 1,
		.mbus_code = V4L2_MBUS_FMT_SGRBG10_1X10,
		.colorspace = V4L2_COLORSPACE_SRGB,
		.regs = sensor_init_regs_1280_720_30fps_mipi,
	},
};

struct tx_isp_sensor_win_setting *wsize = &sensor_win_sizes[0];
#endif

static struct regval_list sensor_stream_on_mipi[] = {
	{SENSOR_REG_END, 0x00},
};

static struct regval_list sensor_stream_off_mipi[] = {
	{SENSOR_REG_END, 0x00},
};

int SetSensorRegister(int sensor, uint32_t reg, uint32_t value)
{
    int ret = -1;
    switch (sensor) {
        case 0:
            ret = IMP_ISP_SetSensorRegister(reg, value);
            break;
        case 1:
            ret = IMP_ISP_SetSensorRegister_Sec(reg, value);
            break;
    }
    return ret;
}

int GetSensorRegister(int sensor, uint32_t reg, uint32_t *value)
{
    int ret = -1;
    switch (sensor) {
        case 0:
            ret = IMP_ISP_GetSensorRegister(reg, value);
            break;
        case 1:
            ret = IMP_ISP_GetSensorRegister_Sec(reg, value);
            break;
    }
    return ret;
}

//  set AGain and integration time
int sensor_set_expo(int sensor, int value)
{
	int ret = 0;
	int it = (value & 0xffff);
//	int again = (value >> 0x10) * 0x20;
	int again = (value >> 11);
//	int again = (value & 0xffff0000) >> 16;
	struct again_lut *val_lut = sensor_again_lut;

	LOG_DEBUG("sensor_set_expo: again = " << again << " it = " << it);

	sensor_write_array(0, sensor_stream_off_mipi);
   /* int IMP_ISP_SetSensorRegister(uint32_t reg, uint32_t value); */
//	sensor_s_stream(0,0);
#if 0
	// Get only shown defaults
	uint32_t  v;
	LOG_DEBUG("sensor_set_expo: sensor " << sensor << " stream off");
	ret = GetSensorRegister(sensor, 0x03f0, &v);
	LOG_DEBUG("sensor_set_expo: 0x03f0 = " << v);
	ret = GetSensorRegister(sensor, 0x03f1, &v);
	LOG_DEBUG("sensor_set_expo: 0x03f1 = " << v);
	ret = GetSensorRegister(sensor, 0x00d1, &v);
	LOG_DEBUG("sensor_set_expo: 0x00d1 = " << v);
	ret = GetSensorRegister(sensor, 0x00d0, &v);
	LOG_DEBUG("sensor_set_expo: 0x00d0 = " << v);
	ret = GetSensorRegister(sensor, 0x031d, &v);
	LOG_DEBUG("sensor_set_expo: 0x031d = " << v);
	ret = GetSensorRegister(sensor, 0x0dc1, &v);
	LOG_DEBUG("sensor_set_expo: 0x0dc1 = " << v);
	ret = GetSensorRegister(sensor, 0x00b8, &v);
	LOG_DEBUG("sensor_set_expo: 0x00b8 = " << v);
	ret = GetSensorRegister(sensor, 0x00b9, &v);
	LOG_DEBUG("sensor_set_expo: 0x00b9 = " << v);
	ret = GetSensorRegister(sensor, 0x0155, &v);
	LOG_DEBUG("sensor_set_expo: 0x0155 = " << v);
#endif

	/*set analog gain*/
#if 1
#ifdef SENSOR_DEBUG
	LOG_DEBUG("sensor_set_expo: reg set 0x00d1 = " << val_lut[again].reg0d1);
#endif
    ret += SetSensorRegister(sensor, 0x00d1, val_lut[again].reg0d1);
#ifdef SENSOR_DEBUG
	LOG_DEBUG("sensor_set_expo: reg set 0x00d0 = " << val_lut[again].reg0d0);
#endif
    ret += SetSensorRegister(sensor, 0x00d0, val_lut[again].reg0d0);
#ifdef SENSOR_DEBUG
	LOG_DEBUG("sensor_set_expo: reg set 0x031d = " << 0x2d);
#endif
    ret += SetSensorRegister(sensor, 0x031d, 0x2d);
#ifdef SENSOR_DEBUG
	LOG_DEBUG("sensor_set_expo: reg set 0x0dc1 = " << val_lut[again].regdc1);
#endif
    ret += SetSensorRegister(sensor, 0x0dc1, val_lut[again].regdc1);
#ifdef SENSOR_DEBUG
	LOG_DEBUG("sensor_set_expo: reg set 0x031d = " << 0x28);
#endif
    ret += SetSensorRegister(sensor, 0x031d, 0x28);
#ifdef SENSOR_DEBUG
	LOG_DEBUG("sensor_set_expo: reg set 0x00b8 = " << val_lut[again].reg0b8);
#endif
    ret += SetSensorRegister(sensor, 0x00b8, val_lut[again].reg0b8);
#ifdef SENSOR_DEBUG
	LOG_DEBUG("sensor_set_expo: reg set 0x00b9 = " << val_lut[again].reg0b9);
#endif
    ret += SetSensorRegister(sensor, 0x00b9, val_lut[again].reg0b9);
#ifdef SENSOR_DEBUG
	LOG_DEBUG("sensor_set_expo: reg set 0x0155 = " << val_lut[again].reg155);
#endif
    ret += SetSensorRegister(sensor, 0x0155, val_lut[again].reg155);
#endif

    /*integration time*/
#if 0
	ret = GetSensorRegister(sensor, 0x0d03, &v);
	LOG_DEBUG("sensor_set_expo: 0x0d03 = " << v);
	ret = GetSensorRegister(sensor, 0x0d04, &v);
	LOG_DEBUG("sensor_set_expo: 0x0d04 = " << v);
#endif
#if 1
#ifdef SENSOR_DEBUG
	LOG_DEBUG("sensor_set_expo: reg set 0x0d03 = " << ((it >> 8) & 0xff));
#endif
    ret += SetSensorRegister(sensor, 0x0d03, (unsigned char) ((it >> 8) & 0xff));
#ifdef SENSOR_DEBUG
	LOG_DEBUG("sensor_set_expo: reg set 0x0d04 = " << (it & 0xff));
	ret += SetSensorRegister(sensor, 0x0d04, (unsigned char) (it & 0xff));
#endif
#endif

	sensor_write_array(0, sensor_stream_on_mipi);

#ifdef SENSOR_DEBUG
	LOG_DEBUG("sensor_set_expo: sensor " << sensor << " stream on");
#endif

	if (0 != ret)
		LOG_DEBUG_OR_ERROR(ret,"Sensor reg write error: (total: " << ret << ")");

	return ret;
}

//  set AGain 
int sensor_set_again(int sensor, int value)
{
	int ret = 0;
	int again = (value) >> 11;
	struct again_lut *val_lut = sensor_again_lut;

	if (again > 0x1c) again = 0x1c;

	sensor_write_array(0, sensor_stream_off_mipi);
	/*set analog gain*/
	LOG_DEBUG("sensor_set_again: again = " << again << " value = " << value);
#ifdef SENSOR_DEBUG
	uint32_t  v;

//	LOG_DEBUG("sensor_set_again: sensor " << sensor << " stream off");
	// Get only shown defaults
	ret = GetSensorRegister(sensor, 0x03f0, &v);
	LOG_DEBUG("sensor_set_again: 0x03f0 = " << v);
	ret = GetSensorRegister(sensor, 0x03f1, &v);
	LOG_DEBUG("sensor_set_again: 0x03f1 = " << v);
	ret = GetSensorRegister(sensor, 0x00d1, &v);
	LOG_DEBUG("sensor_set_again: 0x00d1 = " << v);
	ret = GetSensorRegister(sensor, 0x00d0, &v);
	LOG_DEBUG("sensor_set_again: 0x00d0 = " << v);
	ret = GetSensorRegister(sensor, 0x031d, &v);
	LOG_DEBUG("sensor_set_again: 0x031d = " << v);
	ret = GetSensorRegister(sensor, 0x0dc1, &v);
	LOG_DEBUG("sensor_set_again: 0x0dc1 = " << v);
	ret = GetSensorRegister(sensor, 0x00b8, &v);
	LOG_DEBUG("sensor_set_again: 0x00b8 = " << v);
	ret = GetSensorRegister(sensor, 0x00b9, &v);
	LOG_DEBUG("sensor_set_again: 0x00b9 = " << v);
	ret = GetSensorRegister(sensor, 0x0155, &v);
	LOG_DEBUG("sensor_set_again: 0x0155 = " << v);
#endif
#ifdef SENSOR_DEBUG
	LOG_DEBUG("sensor_set_again: reg set 0x00d1 = " << val_lut[again].reg0d1);
#endif
    ret += SetSensorRegister(sensor, 0x00d1, val_lut[again].reg0d1);
#ifdef SENSOR_DEBUG
	LOG_DEBUG("sensor_set_again: reg set 0x00d0 = " << val_lut[again].reg0d0);
#endif
    ret += SetSensorRegister(sensor, 0x00d0, val_lut[again].reg0d0);
#ifdef SENSOR_DEBUG
	LOG_DEBUG("sensor_set_again: reg set 0x031d = " << 0x2d);
#endif
    ret += SetSensorRegister(sensor, 0x031d, 0x2d);
#ifdef SENSOR_DEBUG
	LOG_DEBUG("sensor_set_again: reg set 0x0dc1 = " << val_lut[again].regdc1);
#endif
    ret += SetSensorRegister(sensor, 0x0dc1, val_lut[again].regdc1);
#ifdef SENSOR_DEBUG
	LOG_DEBUG("sensor_set_again: reg set 0x031d = " << 0x28);
#endif
    ret += SetSensorRegister(sensor, 0x031d, 0x28);
#ifdef SENSOR_DEBUG
	LOG_DEBUG("sensor_set_again: reg set 0x00b8 = " << val_lut[again].reg0b8);
#endif
    ret += SetSensorRegister(sensor, 0x00b8, val_lut[again].reg0b8);
#ifdef SENSOR_DEBUG
	LOG_DEBUG("sensor_set_again: reg set 0x00b9 = " << val_lut[again].reg0b9);
#endif
    ret += SetSensorRegister(sensor, 0x00b9, val_lut[again].reg0b9);
#ifdef SENSOR_DEBUG
	LOG_DEBUG("sensor_set_again: reg set 0x0155 = " << val_lut[again].reg155);
#endif
    ret += SetSensorRegister(sensor, 0x0155, val_lut[again].reg155);

	sensor_write_array(0, sensor_stream_on_mipi);
#ifdef SENSOR_DEBUG
//	LOG_DEBUG("sensor_set_again: sensor " << sensor << " stream on");
#endif
	if (0 != ret)
		LOG_DEBUG_OR_ERROR(ret,"Sensor reg write error: (total: " << ret << ")");

	return ret;
}

//  set AGain and integration time
int sensor_set_integration_time(int sensor, int value)
{
	int ret = 0;
	int it = value;

	sensor_write_array(0, sensor_stream_off_mipi);

    /*integration time*/
#ifdef SENSOR_DEBUG
	LOG_DEBUG("sensor_set_integration_time: it = " << it);
	uint32_t  v;

	// Get only shown defaults
	ret = GetSensorRegister(sensor, 0x0d03, &v);
	LOG_DEBUG("sensor_set_integration_time: 0x0d03 = " << v);
	ret = GetSensorRegister(sensor, 0x0d04, &v);
	LOG_DEBUG("sensor_set_integration_time: 0x0d04 = " << v);
#endif
#ifdef SENSOR_DEBUG
	LOG_DEBUG("sensor_set_integration_time: reg set 0x0d03 = " << ((it >> 8) & 0xff));
#endif
    ret += SetSensorRegister(sensor, 0x0d03, (unsigned char) ((it >> 8) & 0xff));
#ifdef SENSOR_DEBUG
	LOG_DEBUG("sensor_set_integration_time: reg set 0x0d04 = " << (it & 0xff));
#endif
	ret += SetSensorRegister(sensor, 0x0d04, (unsigned char) (it & 0xff));

	sensor_write_array(0, sensor_stream_on_mipi);
#ifdef SENSOR_DEBUG
//	LOG_DEBUG("sensor_set_integration_time: sensor " << sensor << " stream on");
#endif

	if (0 != ret)
		LOG_DEBUG_OR_ERROR(ret,"Sensor reg write error: (total: " << ret << ")");

	return ret;
}

int sensor_detect(int sensor, unsigned int *ident)
{
	int ret;
	uint32_t  v;

	ret = GetSensorRegister(sensor, 0x03f0, &v);
	LOG_DEBUG_OR_ERROR(ret,"- sensor_detect: " << v);
	if (ret < 0)
		return ret;

	if (v != SENSOR_CHIP_ID_H)
		return -ENODEV;

	*ident = v;

	ret = GetSensorRegister(sensor, 0x03f1, &v);
	LOG_DEBUG_OR_ERROR(ret,"- sensor_detect: " << v);
	if (ret < 0)
		return ret;

	if (v != SENSOR_CHIP_ID_L)
		return -ENODEV;

	*ident = (*ident << 8) | v;

	return 0;
}

int sensor_write_array(int sensor, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != SENSOR_REG_END) {
		if (vals->reg_num == SENSOR_REG_DELAY) {
			usleep(vals->value * 1000);
		} else {
			ret = SetSensorRegister(sensor, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}
		vals++;
	}

	return 0;
}

#if 0
int sensor_init(int sensor, int enable)
{
	int ret = 0;

	if (!enable)
		return 0;

	sensor->video.mbus.width = wsize->width;
	sensor->video.mbus.height = wsize->height;
	sensor->video.mbus.code = wsize->mbus_code;
	sensor->video.mbus.field = V4L2_FIELD_NONE;
	sensor->video.mbus.colorspace = wsize->colorspace;
	sensor->video.fps = wsize->fps;

	//sensor_update_actual_fps((wsize->fps >> 16) & 0xffff);

	ret = sensor_write_array(sensor, wsize->regs);
	if (ret)
		return ret;

	//ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);
	sensor->priv = wsize;

	return 0;
}
#endif

int sensor_s_stream(int sensor, int enable)
{
	int ret = 0;

	if (enable) {
		ret = sensor_write_array(sensor, sensor_stream_on_mipi);
		LOG_INFO(SENSOR_NAME << " stream on");
	} else {
		ret = sensor_write_array(sensor, sensor_stream_off_mipi);
		LOG_INFO(SENSOR_NAME << " stream off");
	}

	return ret;
}

int sensor_set_fps(int sensor, int fps)
{
    unsigned int wpclk = SENSOR_SUPPORT_30FPS_SCLK;
	unsigned short vts = 0;
	unsigned short hts = 0;
	uint32_t tmp;
	unsigned int newformat = 0; //the format is 24.8
	int ret = 0;

	newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
	if (newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)) {
		LOG_DEBUG("warn: fps " <<  fps << "not in range");
		return -1;
	}

	ret += GetSensorRegister(sensor, 0xd05, &tmp);
	hts = tmp;
	ret += GetSensorRegister(sensor, 0xd06, &tmp);
	if (ret < 0)
		return -1;

	hts = ((hts << 8) + tmp);

	vts = wpclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
	ret += SetSensorRegister(sensor, 0x0d41, ((vts & 0xff00) >> 8));
	ret += SetSensorRegister(sensor, 0x0d42, (vts & 0xff));
	if (ret < 0)
		return -1;
	
	return 0;
}

int sensor_g_chip_ident(int sensor)
{
	unsigned int ident = 0;
	int ret = 0;

	ret = sensor_detect(sensor, &ident);
	if (ret) {
			LOG_DEBUG_OR_ERROR(ret,"chip found @ " << SENSOR_I2C_ADDRESS << " (" 
                << SENSOR_I2C_BUS << ") is not an " << SENSOR_NAME << " chip.");

	}

	LOG_DEBUG("chip found @ " << SENSOR_NAME);

	LOG_DEBUG("sensor driver version " << SENSOR_VERSION);

	return 0;
}

int sensor_set_vflip(int sensor, int enable)
{
	int ret = -1;
	uint32_t  val = 0x0;
	uint32_t col_start = 0x0;

	ret = GetSensorRegister(sensor, 0x0d15, &val);
	LOG_DEBUG(" * set_vflip_sensor: val = " <<  val );
	if (enable & 0x2) {
		val |= 0x02;
		col_start = 0x01;
	} else {
		val &= 0xfd;
		col_start = 0x00;
	}
	ret += SetSensorRegister(sensor, 0x0d15, val);
	ret += SetSensorRegister(sensor, 0x0192, col_start);
	LOG_DEBUG(" # set_vflip_sensor: val = " <<  val << " col = " << col_start);

	return ret;
}

int sensor_set_vhflip(int sensor, int enable)
{
	int ret = 0;
	uint32_t row_start = 0x0;
	uint32_t col_start = 0x0;
	uint32_t  val = 0x0;

//	ret = sensor_write_array(sensor, sensor_stream_off_mipi);
	ret = GetSensorRegister(sensor, 0x0d15, &val);
	LOG_DEBUG(" * set_vflip_sensor: val = " <<  val );
	if (enable & 0x2) {
		val |= 0x02;
		col_start = 0x01;
	} else {
		val &= 0xfd;
		col_start = 0x00;
	}
	if (enable & 0x1) {
		val |= 0x01;
		row_start = 0x01;
	} else {
		val &= 0xfe;
		row_start = 0x00;
	}

	ret += SetSensorRegister(sensor, 0x0d15, val);
//	ret += SetSensorRegister(sensor, 0x0190, row_start);
	ret += SetSensorRegister(sensor, 0x0192, col_start);
	LOG_DEBUG("set_vhflip_sensor: val = " <<  val << " row = " << row_start << " col = " << col_start);
//	ret += sensor_write_array(sensor, sensor_stream_on_mipi);	
	return ret;
}


} // namespace sensor_hal
