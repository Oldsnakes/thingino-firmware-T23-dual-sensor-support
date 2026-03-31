#include "imp_hal.hpp"
#include "IMPSystem.hpp"
#include "Config.hpp"
#include "globals.hpp"

#define MODULE "IMP_SYSTEM"

IMPSensorInfo IMPSystem::create_sensor_info(_sensor sensor, int id)
{
    IMPSensorInfo out;
    memset(&out, 0, sizeof(IMPSensorInfo));
    strcpy(out.name, sensor.model);
    out.cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C;
    strcpy(out.i2c.type, sensor.model);
    out.i2c.addr = sensor.i2c_address;
    out.i2c.i2c_adapter_id = sensor.i2c_bus;
    out.sensor_id = id;

#if defined(PLATFORM_T40) || defined(PLATFORM_T41)
    // Additional fields required for T40/T41 platforms
    out.i2c.i2c_adapter_id = sensor.i2c_bus;
    out.rst_gpio = sensor.gpio_reset;
    out.pwdn_gpio = -1;
    out.power_gpio = -1;
    out.sensor_id = 0;
    out.video_interface = static_cast<IMPSensorVinType>(sensor.video_interface);
    out.mclk = static_cast<IMPSensorMclk>(sensor.mclk);
    out.default_boot = 0;
#endif

    return out;
}

IMPSystem *IMPSystem::createNew()
{
    return new IMPSystem();
}

int IMPSystem::init()
{
    LOG_DEBUG("IMPSystem::init()");
    int ret = 0;

    ret = IMP_OSD_SetPoolSize(cfg->general.osd_pool_size * 1024);
    LOG_DEBUG_OR_ERROR(ret, "IMP_OSD_SetPoolSize(" << (cfg->general.osd_pool_size * 1024) << ")");

    IMPVersion impVersion;
    ret = IMP_System_GetVersion(&impVersion);
    LOG_INFO("LIBIMP Version " << impVersion.aVersion);

    SUVersion suVersion;
    ret = SU_Base_GetVersion(&suVersion);
    LOG_INFO("SYSUTILS Version: " << suVersion.chr);

    cfg->sysinfo.cpu = IMP_System_GetCPUInfo();
    LOG_INFO("CPU Information: " << cfg->sysinfo.cpu);

    ret = IMP_ISP_Open();
    LOG_DEBUG_OR_ERROR_AND_EXIT(ret, "IMP_ISP_Open()");

    /* sensor */
    _sensor sensor0 = cfg->sensor;  // gc1084s1
    _sensor sensor1 = cfg->sensor1; // gc1084

    int sensor_id = 0;

    // sensor 0:  
    sinfo = create_sensor_info(sensor1, sensor_id);
    LOG_INFO("Sensor: " << sinfo.name << " address: " << sinfo.i2c.addr << " adaptor: " << sinfo.i2c.i2c_adapter_id << " id: " << sinfo.sensor_id);
    ret = hal::isp::add_sensor(&sinfo);
    LOG_DEBUG_OR_ERROR_AND_EXIT(ret, "IMP_ISP_AddSensor-0(&sinfo)");

    usleep(1000);
    sensor_id++;

    if (NUM_SENSOR == 2) {
        // sensor 1:  the I2C bus 0 is dominent channel, need to be the last sensor to add.  The prudynt.json need to have to match it with config
        sinfo = create_sensor_info(sensor0, sensor_id);
        LOG_INFO("Sensor: " << sinfo.name << " address: " << sinfo.i2c.addr << " adaptor: " << sinfo.i2c.i2c_adapter_id << " id: " << sinfo.sensor_id);
        ret = hal::isp::add_sensor(&sinfo);
        LOG_DEBUG_OR_ERROR_AND_EXIT(ret, "IMP_ISP_AddSensor-1(&sinfo)");
    }

    ret = hal::isp::enable_sensor(&sinfo);
    LOG_DEBUG_OR_ERROR_AND_EXIT(ret, "IMP_ISP_EnableSensor() sensors: " << NUM_SENSOR);

    /* system */
    ret = IMP_System_Init();
    LOG_DEBUG_OR_ERROR_AND_EXIT(ret, "IMP_System_Init()");

    ret = IMP_ISP_EnableTuning();
    LOG_DEBUG_OR_ERROR_AND_EXIT(ret, "IMP_ISP_EnableTuning()");

    IMPISPSENSORAttr sensorAttr;
    memset(&sensorAttr, 0, sizeof(IMPISPSENSORAttr));
    ret = IMP_ISP_Tuning_GetSensorAttr(&sensorAttr);
    LOG_DEBUG_OR_ERROR_AND_EXIT(ret, "IMP_ISP_Tuning_GetSensorAttr sensor");
    LOG_DEBUG("Sensor 0 size: " << sensorAttr.width << "X" << sensorAttr.height << " fps: " << sensorAttr.fps);
    if (cfg->sensor.select & 0x2) {
        ret = IMP_ISP_Tuning_GetSensorAttr_Sec(&sensorAttr);
        LOG_DEBUG_OR_ERROR_AND_EXIT(ret, "IMP_ISP_Tuning_GetSensorAttr_Sec sensor");
        LOG_DEBUG("Sensor 1 size: " << sensorAttr.width << "X" << sensorAttr.height << " fps: " << sensorAttr.fps);
    }

        IMP_ISP_Tuning_SetAeFreeze(IMPISP_TUNING_OPS_MODE_DISABLE);
        IMP_ISP_Tuning_SetISPCustomMode(IMPISP_TUNING_OPS_MODE_DISABLE);
    if (cfg->sensor.select & 0x2) {
        IMP_ISP_Tuning_SetAeFreeze_Sec(IMPISP_TUNING_OPS_MODE_DISABLE);
        IMP_ISP_Tuning_SetISPCustomMode_Sec(IMPISP_TUNING_OPS_MODE_DISABLE);
    }

#if !defined(NO_TUNINGS)

    IMPISPAEMin ae_min;
    memset(&ae_min, 0, sizeof(IMPISPAEMin));
	ae_min.min_it = 10;
	ae_min.min_again = 2048;
	ae_min.min_it_short = 10;
	ae_min.min_again_short = 2048;
    ret = IMP_ISP_Tuning_SetAeMin(&ae_min);
    LOG_DEBUG_OR_ERROR(ret, "IMP_ISP_Tuning_SetAeMin(" << ae_min.min_again << ")");
    if (cfg->sensor.select & 0x2) {
        ret = IMP_ISP_Tuning_SetAeMin_Sec(&ae_min);
        LOG_DEBUG_OR_ERROR(ret, "IMP_ISP_Tuning_SetAeMin_Sec(" << ae_min.min_again << ")");
    }

   ret = hal::isp::set_contrast(cfg->image0.contrast);
    LOG_DEBUG_OR_ERROR(ret, "SetContrast(" << cfg->image0.contrast << ")");

    ret = hal::isp::set_sharpness(cfg->image0.sharpness);
    LOG_DEBUG_OR_ERROR(ret, "SetSharpness(" << cfg->image0.sharpness << ")");

    ret = hal::isp::set_saturation(cfg->image0.saturation);
    LOG_DEBUG_OR_ERROR(ret, "SetSaturation(" << cfg->image0.saturation << ")");

    ret = hal::isp::set_brightness(cfg->image0.brightness);
    LOG_DEBUG_OR_ERROR(ret, "SetBrightness(" << cfg->image0.brightness << ")");

#if !defined(PLATFORM_T21)
    ret = hal::isp::set_sinter_strength(cfg->image0.sinter_strength);
    LOG_DEBUG_OR_ERROR(ret, "SetSinterStrength(" << cfg->image0.sinter_strength << ")");
#endif

    ret = hal::isp::set_temper_strength(cfg->image0.temper_strength);
    LOG_DEBUG_OR_ERROR(ret, "SetTemperStrength(" << cfg->image0.temper_strength << ")");

    ret = hal::isp::set_hflip(cfg->image0.hflip);
    LOG_DEBUG_OR_ERROR(ret, "SetISPHflip(" << cfg->image0.hflip << ")");

    ret = hal::isp::set_running_mode(cfg->image0.running_mode);  // cfg default
    LOG_DEBUG_OR_ERROR(ret, "SetISPRunningMode(" << cfg->image0.running_mode << ")");

    IMPISPAntiflickerAttr flickerAttr;
    memset(&flickerAttr, 0, sizeof(IMPISPAntiflickerAttr));
    ret = hal::isp::set_anti_flicker(cfg->image0.anti_flicker);
    LOG_DEBUG_OR_ERROR(ret, "SetAntiFlickerAttr(" << cfg->image0.anti_flicker << ")");

#if !defined(PLATFORM_T21)
    ret = hal::isp::set_ae_compensation(cfg->image0.ae_compensation);
    LOG_DEBUG_OR_ERROR(ret, "SetAeComp(" << cfg->image0.ae_compensation << ")");
#endif

    ret = hal::isp::set_max_again(cfg->image0.max_again);
    LOG_DEBUG_OR_ERROR(ret, "SetMaxAgain(" << cfg->image0.max_again << ")");

    ret = hal::isp::set_max_dgain(cfg->image0.max_dgain);
    LOG_DEBUG_OR_ERROR(ret, "SetMaxDgain(" << cfg->image0.max_dgain << ")");

    ret = hal::isp::set_wb(cfg->image0.core_wb_mode, cfg->image0.wb_rgain, cfg->image0.wb_bgain);
    if (ret != 0)
    {
        LOG_ERROR("Unable to set white balance. Mode: " << cfg->image0.core_wb_mode << ", rgain: "
                                                        << cfg->image0.wb_rgain << ", bgain: " << cfg->image0.wb_bgain);
    }
    else
    {
        LOG_DEBUG("Set white balance. Mode: " << cfg->image0.core_wb_mode << ", rgain: "
                                              << cfg->image0.wb_rgain << ", bgain: " << cfg->image0.wb_bgain);
    }

#if defined(PLATFORM_T23) || defined(PLATFORM_T31) || defined(PLATFORM_C100)
    ret = hal::isp::set_hue(cfg->image0.hue);
    LOG_DEBUG_OR_ERROR(ret, "SetBcshHue(" << cfg->image0.hue << ")");

    ret = hal::isp::set_defog_strength(static_cast<uint8_t>(cfg->image0.defog_strength));
    LOG_DEBUG_OR_ERROR(ret, "SetDefog_Strength(" << cfg->image0.defog_strength << ")");

#endif
#if defined(PLATFORM_T21) || defined(PLATFORM_T23) || defined(PLATFORM_T31) || defined(PLATFORM_C100)
//    ret = hal::isp::set_drc_strength(cfg->image0.drc_strength);
//    LOG_DEBUG_OR_ERROR(ret, "SetDRC_Strength(" << cfg->image0.drc_strength << ")");
#endif

#if defined(PLATFORM_T23) || defined(PLATFORM_T31) || defined(PLATFORM_C100)
    if (cfg->image0.backlight_compensation > 0)
    {
        ret = hal::isp::set_backlight_comp(cfg->image0.backlight_compensation);
        LOG_DEBUG_OR_ERROR(ret, "SetBacklightComp(" << cfg->image0.backlight_compensation << ")");
    }
    else if (cfg->image0.highlight_depress > 0)
    {
        ret = hal::isp::set_highlight_depress(cfg->image0.highlight_depress);
        LOG_DEBUG_OR_ERROR(ret, "SetHiLightDepress(" << cfg->image0.highlight_depress << ")");
    }
#elif defined(PLATFORM_T21) || defined(PLATFORM_T30)
    ret = hal::isp::set_highlight_depress(cfg->image0.highlight_depress);
    LOG_DEBUG_OR_ERROR(ret, "SetHiLightDepress(" << cfg->image0.highlight_depress << ")");
#endif
    LOG_DEBUG(" ### ISP Tuning Defaults Set.");

    // Clamp sensor FPS to a sane value based on stream0 desired FPS and sensor limits
    uint32_t desired_sensor_fps = cfg->stream0.fps > 0 ? cfg->stream0.fps : 25;
    // cfg->sensor.fps is read from /proc/jz/sensor/max_fps; min from min_fps
    if (cfg->sensor.min_fps > 0 && desired_sensor_fps < cfg->sensor.min_fps) {
        desired_sensor_fps = cfg->sensor.min_fps;
    }
    if (cfg->sensor.fps > 0 && desired_sensor_fps > (int)cfg->sensor.fps) {
        desired_sensor_fps = cfg->sensor.fps;
    }

    uint32_t den;
    uint32_t fps_num;
    desired_sensor_fps = 10;
    LOG_DEBUG("# desired_sensor_fps = " << desired_sensor_fps);
    ret = IMP_ISP_Tuning_SetSensorFPS(desired_sensor_fps, 1);
    LOG_DEBUG_OR_ERROR_AND_EXIT(ret, "IMP_ISP_Tuning_SetSensorFPS(" << desired_sensor_fps << ", 1)");
    if (cfg->sensor.select & 0x2) {
        ret = IMP_ISP_Tuning_SetSensorFPS_Sec(desired_sensor_fps, 1);
        LOG_DEBUG_OR_ERROR_AND_EXIT(ret, "IMP_ISP_Tuning_SetSensorFPS_Sec(" << desired_sensor_fps << ", 1)");
    }

    ret = IMP_ISP_Tuning_GetSensorFPS(&fps_num, &den);
    LOG_DEBUG_OR_ERROR_AND_EXIT(ret, "IMP_ISP_Tuning_GetSensorFPS(" << fps_num << ", " << den << ")");
    if (cfg->sensor.select & 0x2) {
        ret = IMP_ISP_Tuning_GetSensorFPS_Sec(&fps_num, &den);
        LOG_DEBUG_OR_ERROR_AND_EXIT(ret, "IMP_ISP_Tuning_GetSensorFPS_Sec(" << fps_num << ", " << den << ")");
    }

#if defined(PLATFORM_T21)
    //T20 T21 only set FPS if it is read after set.
    uint32_t fps_num, fps_den;
    ret = IMP_ISP_Tuning_GetSensorFPS(&fps_num, &fps_den);
    LOG_DEBUG_OR_ERROR_AND_EXIT(ret, "IMP_ISP_Tuning_GetSensorFPS(" << fps_num << ", " << fps_den << ")");
#endif

    // Set the ISP to DAY on launch
    ret = hal::isp::set_running_mode(IMPISP_RUNNING_MODE_DAY);
    LOG_DEBUG_OR_ERROR_AND_EXIT(ret, "SetISPRunningMode(" << IMPISP_RUNNING_MODE_DAY << ")");
#endif // #if !defined(NO_TUNINGS)

    #if defined(NO_TUNINGS)
    IMPISPModuleCtl ispmodule;
    memset(&ispmodule, 0, sizeof(IMPISPModuleCtl));
    ret = IMP_ISP_Tuning_GetModuleControl(&ispmodule);
    LOG_DEBUG_OR_ERROR(ret, "* IMP_ISP_Tuning_SetModuleControl-bitBypassAG(" << ispmodule.bitBypassAG << ")");
    {  // module control bits:  1 = bypass
		ispmodule.bitBypassBLC= 1;  /* [0]  BackLight Compensation/ Block Level Correction*/  
		ispmodule.bitBypassGIB= 1;  /* [1]  */
		ispmodule.bitBypassAG= 1;   /* [2]  Analog Gain, Auto Gain */
		ispmodule.bitBypassDPC= 1;  /* [4]  Digital Pulse Compression */
		ispmodule.bitBypassRDNS= 1; /* [5]	Dialogue Noise Suppression */
		ispmodule.bitBypassLSC= 1;  /* [6]  Lens Shading Correction */
		ispmodule.bitBypassADR= 1;  /* [7]	 */
		ispmodule.bitBypassDMSC= 1; /* [8]	 */
		ispmodule.bitBypassCCM= 1;  /* [9]  Color Correction Matrix */
		ispmodule.bitBypassGAMMA= 1; /* [10] Gamma */
		ispmodule.bitBypassDEFOG= 1; /* [11] DeFOG, image dehazing */
		ispmodule.bitBypassCSC= 1;  /* [12]	 */
		ispmodule.bitBypassCLM= 1;  /* [13]	 */
		ispmodule.bitBypassSP= 1;   /* [14]  */
		ispmodule.bitBypassYDNS= 1; /* [15]	 */
		ispmodule.bitBypassBCSH= 1; /* [16]	Brightness, Contrast, Saturation, Hue */
		ispmodule.bitBypassSDNS= 1; /* [17]	Dialogue Noise Suppression */
		ispmodule.bitBypassHLDC= 1; /* [18]	HighLigh Depress  */
		ispmodule.bitRsv= 12;       /* [19 ~ 30] reserved 12 bits	*/
		ispmodule.bitBypassMDNS= 1; /* [31] Dialogue Noise Suppression */
    }
    ret = IMP_ISP_Tuning_SetModuleControl(&ispmodule);
    LOG_DEBUG_OR_ERROR(ret, "# IMP_ISP_Tuning_SetModuleControl-bitBypassAG(" << ispmodule.bitBypassAG << ")");
#endif  // no_tunings

    LOG_INFO(" ### IMPSystem Done!");
    return ret;
}

int IMPSystem::destroy()
{
    int ret;

    ret = IMP_System_Exit();
    LOG_DEBUG_OR_ERROR(ret, "IMP_System_Exit()");

    ret = hal::isp::disable_sensor();
    LOG_DEBUG_OR_ERROR(ret, "IMP_ISP_DisableSensor()");

    ret = hal::isp::del_sensor(&sinfo);
    LOG_DEBUG_OR_ERROR(ret, "IMP_ISP_DelSensor()");

    ret = IMP_ISP_DisableTuning();
    LOG_DEBUG_OR_ERROR(ret, "IMP_ISP_DisableTuning()");

    ret = IMP_ISP_Close();
    LOG_DEBUG_OR_ERROR(ret, "IMP_ISP_Close()");

    return 0;
}
