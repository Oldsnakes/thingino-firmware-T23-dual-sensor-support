#include "Daynight.hpp"

#include "ctrls_hal.hpp"

#undef Daynight_DEBUG

using namespace std::chrono;

bool night_mode = false;
uint32_t low_threshold;
uint32_t up_threshold;
uint32_t sample_time;
uint32_t day_to_night_hold_samples;
uint32_t night_to_day_hold_samples;
uint32_t hold_count;
/*
 *      Daynight luma detection:
 *          low_threshold:  when light fall below this value, night scene
 *          up_threshold:  when light above this value, day scene
 *          sample_time:  sample the light input every n seconds
 *          day_to_night_hold_samples/night_to_day_hold_samples:  n samples fall into the same scene
*/
void Daynight::detect()
{
    LOG_INFO("Start Daynight thread.");

    int ret;
    int luma_value, luma_value_sec, luma_avg;
    uint32_t gain_value, gain_value_sec, gain_avg;
    uint32_t value;
    bool log;

    IMPISPEVAttr attr;
    memset(&attr, 0, sizeof(IMPISPEVAttr));
    IMPISPEVAttr attr_sec;
    memset(&attr_sec, 0, sizeof(IMPISPEVAttr));

    if(init() != 0) return;

    global_daynight_thread_signal = true;
    
    while (global_daynight_thread_signal) {
        ret = 0;
        log = cfg->daynight.log;
        sample_time = cfg->daynight.sample_time;
//    sample_time = cfg->image0.hue;
//        if (log) sleep(cfg->image0.temper_strength);  // debug extened period logging
        if (cfg->daynight.enable) {
//            ret = IMP_ISP_Tuning_GetAeLuma(&luma_value);
//            ret += IMP_ISP_Tuning_GetAeLuma_Sec(&luma_value_sec);
            ret += IMP_ISP_Tuning_GetTotalGain(&gain_value);
            ret += IMP_ISP_Tuning_GetTotalGain_Sec(&gain_value_sec);
            ret += IMP_ISP_Tuning_GetEVAttr(&attr);
            ret += IMP_ISP_Tuning_GetEVAttr_Sec(&attr_sec);

            if (ret < 0)
            {
                LOG_WARN("IMP_ISP_Tuning_GetAeLuma/TotalGain error: " << ret);
                continue;
            }
            low_threshold =  cfg->daynight.low_threshold;
            up_threshold = cfg->daynight.up_threshold;
            hold_count = cfg->daynight.hold_count;

            luma_avg = (luma_value + luma_value_sec) / 2;
            gain_avg = (gain_value + gain_value_sec) / 2;
//            value = gain_avg / 100;
            value = gain_value / 100;
#ifdef Daynight_DEBUG
//            LOG_DEBUG("*** Luma = " << luma_value << ", " << luma_value_sec << " ->" << luma_avg);
//            LOG_DEBUG("*** EV = " << attr.ev << ", " << attr_sec.ev);
            if (log) LOG_DEBUG("*** Total Gain = " << gain_value << ", " << gain_value_sec << " ->[" << gain_avg << "]  EV = "
                << attr.ev << ", " << attr_sec.ev << " hold => " << day_to_night_hold_samples << "," << night_to_day_hold_samples);
#endif
            if ( !night_mode && value > low_threshold) {
                day_to_night_hold_samples += 1;
                night_to_day_hold_samples = 0;
                if (log) LOG_DEBUG("*** Total Gain = " << gain_value << ", " << gain_value_sec << " ->[" << gain_avg << "]  EV = "
                    << attr.ev << ", " << attr_sec.ev << " hold => @" << day_to_night_hold_samples << "," << night_to_day_hold_samples);
                if (day_to_night_hold_samples >= hold_count) {
                    night_mode = true;
                    action(night_mode);
                    day_to_night_hold_samples = 0;
                }
            } else
            if ( night_mode && value < up_threshold) {
                night_to_day_hold_samples += 1;
                day_to_night_hold_samples = 0;
                if (log) LOG_DEBUG("*** Total Gain = " << gain_value << ", " << gain_value_sec << " ->[" << gain_avg << "]  EV = "
                    << attr.ev << ", " << attr_sec.ev << " hold => " << day_to_night_hold_samples << ",@" << night_to_day_hold_samples);
                if (night_to_day_hold_samples >= hold_count) {
                    night_mode = false;
                    action(night_mode);
                    night_to_day_hold_samples = 0;
               }
            } else // reset flags
            {
                    day_to_night_hold_samples = 0;
                    night_to_day_hold_samples = 0;
            }
            sleep(sample_time);

        } else continue;
    }
    exit();

    LOG_DEBUG("### Exit Daynight thread.");
}

int Daynight::action(bool active) {
    LOG_INFO("--- Night mode:  " << (active? "ON *":"OFF #") << " ---");
    if (cfg->daynight.white_enable) {
        ctrls_hal::setGPIObyName("gpio_white", active);
    }
    if (cfg->daynight.ir850_enable) {
        ctrls_hal::setGPIObyName("gpio_ir850", active);
    }
    if (cfg->daynight.ir940_enable) {
        ctrls_hal::setGPIObyName("gpio_ir940", active);
    }
    if (cfg->daynight.ircut_enable) {
        hal::isp::set_ircut(!active);    
    }
    if (cfg->daynight.color_enable) {
        int pre_select = cfg->sensor.select;
        cfg->sensor.select = cfg->daynight.sensor_select;  // independent to preset
        hal::isp::set_running_mode((active? 1:0));  // running mode = night_mode
        cfg->sensor.select = pre_select;  // restore
    }
    cfg->daynight.night_mode = active;
    return 0;
}


int Daynight::init()
{
    LOG_INFO("Initialize Daynight detection.");

    if((!cfg->stream0.enabled) ||
       (!cfg->stream1.enabled)) {

        LOG_ERROR("Video streams are disabled, abort.");
        return -1;
    }

    low_threshold =  cfg->daynight.low_threshold;
    up_threshold = cfg->daynight.up_threshold;

    return 0;
}

int Daynight::exit()
{
    int ret;

    LOG_DEBUG("Exit Daynight detection.");

    return 0;
}

void *Daynight::run(void *arg)
{
    ((Daynight *)arg)->detect();
    return nullptr;
}
