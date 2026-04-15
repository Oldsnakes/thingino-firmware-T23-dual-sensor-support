#include <chrono>
#include <thread>
#include <atomic>
#include <condition_variable>
#include "RTSP.hpp"
#include "Logger.hpp"
#include "globals.hpp"
#include "Config.hpp"
#include "version.hpp"
#include "ConfigWatcher.hpp"
#include "AudioWorker.hpp"
#include "BackchannelWorker.hpp"
#include "VideoWorker.hpp"
#include "JPEGWorker.hpp"
#include "WorkerUtils.hpp"
#include "IMPSystem.hpp"
#include "Motion.hpp"
#include "IMPBackchannel.hpp"

#include "IPCServer.hpp"
#include "imp_hal.hpp"
#include "Motor.hpp"

#include "ctrls_hal.hpp"

using namespace std::chrono;

#define ENV_FILE  "/tmp/environment"

std::mutex mutex_main;
std::condition_variable global_cv_worker_restart;

bool startup = true;
bool global_restart = false;

bool global_restart_rtsp = false;
bool global_restart_video = false;
bool global_restart_audio = false;

bool global_restart_motion = false;

bool global_osd_thread_signal = false;
bool global_main_thread_signal = false;
bool global_motion_thread_signal = false;
std::atomic<char> global_rtsp_thread_signal{1};

std::shared_ptr<jpeg_stream> global_jpeg[NUM_JPEG_CHANNELS] = {nullptr};
std::shared_ptr<video_stream> global_video[NUM_VIDEO_CHANNELS] = {nullptr};
#if defined(AUDIO_SUPPORT)
std::shared_ptr<audio_stream> global_audio[NUM_AUDIO_CHANNELS] = {nullptr};
std::shared_ptr<backchannel_stream> global_backchannel = nullptr;
#endif

std::shared_ptr<CFG> cfg = std::make_shared<CFG>();

std::atomic<int> global_MipiMode = 3;
std::atomic<int> global_Sensor = 0x3;

RTSP rtsp;
Motion motion;
IMPSystem *imp_system = nullptr;

bool timesync_wait()
{
    // I don't really have a better way to do this than
    // a no-earlier-than time. The most common sync failure
    // is time() == 0
    int timeout = 0;
    while (time(NULL) < 1647489843)
    {
        std::this_thread::sleep_for(seconds(1));
        ++timeout;
        if (timeout == 60)
            return false;
    }
    return true;
}

void start_video(int encChn, int encGrp, int fsChnNum)
{
    StartHelper sh{encChn, encGrp, fsChnNum};
    int ret = pthread_create(&global_video[encChn]->thread, nullptr, VideoWorker::thread_entry, static_cast<void *>(&sh));
    LOG_DEBUG_OR_ERROR(ret, "create video["<< encChn << "] thread");

    // wait for initialization done
    sh.has_started.acquire();
}

 /* Video channels 
 * (encChn) (chnNr)                (encChn)      (cfg->)   (stream)   
 * video   framesource    group    encoder       sensor    format   
 *  ch 0:  framesource 0, group 0, encoder ch 0, sensor 0, H264,     0
 *  ch 1:  framesource 3, group 1, encoder ch 1, sensor 1, H264,     1
 *  ch 2:  framesource 0, group 0, encoder ch 2, sensor 0, JPEG,     2
 *  ch 3:  framesource 3, group 1, encoder ch 3, sensor 1, JPEG,     3
 *  ch 4:  framesource 1, group 2, encoder ch 4, sensor 0, H264,     4
 *  ch 5:  framesource 4, group 3, encoder ch 5, sensor 1, H264,     5
 */
/* JPEG channels 
 *  (jpgChn)  (chnNr)              (encChn)        
 *  JPEG   framesource    group    encoder       sensor     
 *  ch 0:  framesource 0, group 0, encoder ch 2, sensor 0,  
 *  ch 1:  framesource 3, group 1, encoder ch 3, sensor 1,    
 */

int main(int argc, const char *argv[])
{
    LOG_INFO("PRUDYNT-T Next-Gen Video Daemon: " << FULL_VERSION_STRING);


    LOG_INFO("  Compiled on " << __DATE__ << " at " << __TIME__ );
	global_MipiMode = 3;  // default to dual sensor mode
    global_Sensor = 0x3;


    for (int i = 1; i < argc; ++i) {
        // Check for a specific option like "-h" or "--help"
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            std::cout << "Options:\n   -s n   : sensor switch mode,\n   -v    :  build time"  << std::endl;
            return 0; 
        }
        else if (strcmp(argv[i], "-v") == 0) {
                std::cout << VERSION << std::endl;
                return 0;
        }
        // Handle other options or arguments
        else {
            std::cout << "Unknown argument: " << argv[i] << std::endl;
        }
    }
    
    if (global_MipiMode == 3) {
        cfg->image1.alt_sensor = true;
        cfg->image0.alt_sensor = true;
    } else {
        cfg->image0.alt_sensor = false;  
        cfg->image1.alt_sensor = false;  
    }

    pthread_t cw_thread;
    pthread_t osd_thread;
    pthread_t rtsp_thread;
    pthread_t motion_thread;
    pthread_t backchannel_thread;

    if (Logger::init(cfg->general.loglevel))
    {
        LOG_ERROR("Logger initialization failed.");
        return 1;
    }
    LOG_INFO("Starting Prudynt Video Server.");

    if (!timesync_wait())
    {
        LOG_ERROR("Time is not synchronized.");
        return 1;
    }

    // get GPIO settings from environment file
    ctrls_hal::getEnv_gpio(ENV_FILE); 
    // enable memory access for GPIO registers
    int fd = 0;
    ctrls_hal::openMMIO (fd);

    if (!imp_system)
    {
        imp_system = IMPSystem::createNew();
    }

    global_video[0] = std::make_shared<video_stream>(0, &cfg->stream0, "stream0");
    global_video[1] = std::make_shared<video_stream>(1, &cfg->stream1, "stream1");
    global_jpeg[0] = std::make_shared<jpeg_stream>(2, &cfg->stream2);
    global_jpeg[1] = std::make_shared<jpeg_stream>(3, &cfg->stream3);
    global_video[4] = std::make_shared<video_stream>(4, &cfg->stream4, "stream4");
    global_video[5] = std::make_shared<video_stream>(5, &cfg->stream5, "stream5");

#if defined(AUDIO_SUPPORT)
    global_audio[0] = std::make_shared<audio_stream>(1, 0, 0);
    global_backchannel = std::make_shared<backchannel_stream>();
#endif

    // Start UNIX domain socket IPC server for local control
    static IPCServer ipc_server;
    ipc_server.start();

    pthread_create(&cw_thread, nullptr, ConfigWatcher::thread_entry, nullptr);

    while (true)
    {
        global_restart = true;
#if defined(AUDIO_SUPPORT)
        if (cfg->audio.output_enabled && (global_restart_audio || startup))
        {
             int ret = pthread_create(&backchannel_thread, nullptr, BackchannelWorker::thread_entry, NULL);
             LOG_DEBUG_OR_ERROR(ret, "create backchannel thread");
        }

        if (cfg->audio.input_enabled && (global_restart_audio || startup))
        {
            StartHelper sh{0};
            int ret = pthread_create(&global_audio[0]->thread, nullptr, AudioWorker::thread_entry, static_cast<void *>(&sh));
            LOG_DEBUG_OR_ERROR(ret, "create audio thread");
            // wait for initialization done
            sh.has_started.acquire();
        }
#endif
        // stream follow encoder  start_video(int encChn, int encGrp, int fsChnNum)
        if (global_restart_video || startup)
        {
            LOG_INFO("Start VIDEO streams.");
            if (cfg->stream0.enabled)
            {
                LOG_DEBUG("stream 0 enabled");
                start_video(0, 0 , 0);
            } else {
                LOG_DEBUG("stream 0 disabled");
            }
            if (cfg->stream1.enabled)
            {
                LOG_DEBUG("stream 1 enabled");
                start_video(1, 1, 3);
            } else {
                LOG_DEBUG("stream 1 disabled");
            }
            if (cfg->stream4.enabled)
            {
                LOG_DEBUG("stream 4 enabled");
                start_video(4, 2, 1);
            } else {
                LOG_DEBUG("stream 4 disabled");
            }
            if (cfg->stream5.enabled)
            {
                LOG_DEBUG("stream 5 enabled");
                start_video(5, 3, 4);
            } else {
                LOG_DEBUG("stream 5 disabled");
            }
            if (cfg->stream2.enabled) 
            {
                StartHelper sh{encChn:2, encGrp:0};
                LOG_DEBUG("stream 2 (jpeg) enabled");
                int ret = pthread_create(&global_jpeg[0]->thread, nullptr, JPEGWorker::thread_entry, static_cast<void *>(&sh));
                LOG_DEBUG_OR_ERROR(ret, "create jpeg thread 0");
                // wait for initialization done
                sh.has_started.acquire();
           } else {
                LOG_DEBUG("stream 2 (jpeg) disabled");
            }

            if (cfg->stream3.enabled)  
            {
                StartHelper sh2{encChn:3, encGrp:1};
                LOG_DEBUG("stream 3 (jpeg) enabled");
                int ret2 = pthread_create(&global_jpeg[1]->thread, nullptr, JPEGWorker::thread_entry, static_cast<void *>(&sh2));
                LOG_DEBUG_OR_ERROR(ret2, "create mjpeg/jpeg thread 1");
                sh2.has_started.acquire();
            } else {
                LOG_DEBUG("stream 3 (mjpeg/jpeg) disabled");
            }

            if (cfg->stream0.osd.enabled || cfg->stream1.osd.enabled || cfg->stream4.osd.enabled || cfg->stream5.osd.enabled)
            {
                LOG_DEBUG("OSD enabled");
                int ret = pthread_create(&osd_thread, nullptr, OSD::thread_entry, NULL);
                LOG_DEBUG_OR_ERROR(ret, "create osd thread");
                global_osd_thread_signal = true;
            }
        }  // start video

        // start motion detection server
        if (global_restart_video || global_restart_motion || startup)
        {
            if (cfg->motion.enabled)
            {
                LOG_DEBUG("Motion Detection enabled");
                int ret = pthread_create(&motion_thread, nullptr, Motion::run, &motion);
                LOG_DEBUG_OR_ERROR(ret, "create motion thread");
                global_motion_thread_signal = true;
            }
        }

        // start rtsp server
        if (global_rtsp_thread_signal != 0 && (global_restart_rtsp || startup))
        {
                LOG_DEBUG("RTSP enabled");
            int ret = pthread_create(&rtsp_thread, nullptr, RTSP::run, &rtsp);
            LOG_DEBUG_OR_ERROR(ret, "create rtsp thread");
            global_rtsp_thread_signal = 0;
        }

        /* we should wait a short period to ensure all services are up
         * and running, additionally we add the timespan which is configured as
         * OSD startup delay.
         */
        usleep(250000 + (cfg->stream0.osd.start_delay * 1000) + cfg->stream1.osd.start_delay * 1000);

        LOG_DEBUG("main thread is go into sleep-loop....zzz");
//        std::unique_lock lck(mutex_main);

        startup = false;
        global_restart = false;
        global_restart_video = false;
        global_restart_audio = false;
        global_restart_rtsp = false;
        global_restart_motion = false;
        while (!global_restart_rtsp && !global_restart_video && !global_restart_audio && !global_restart_motion) 
        {
            sleep(1);
//            global_cv_worker_restart.wait(lck);
        }
//        lck.unlock();

        global_restart = true;
        if (global_restart_rtsp)
        {
            LOG_DEBUG("RTSP is asking to restart!!!");
            // stop rtsp thread
            if (global_rtsp_thread_signal == 0)
            {
                global_rtsp_thread_signal = 1;
                int ret = pthread_join(rtsp_thread, NULL);
                LOG_DEBUG_OR_ERROR(ret, "join rtsp thread");
            }
        }

        if (global_restart_motion)
        {
            LOG_DEBUG("Motion is asking to restart!!!");
            // stop motion thread
            if (global_motion_thread_signal)
            {
                global_motion_thread_signal = false;
                int ret = pthread_join(motion_thread, NULL);
                LOG_DEBUG_OR_ERROR(ret, "join motion thread");
            }
        }

        // stop audio
        if (global_audio[0]->imp_audio && global_restart_audio)
        {
            LOG_DEBUG("Audio is asking to restart!!!");
            global_audio[0]->running = false;
            global_audio[0]->should_grab_frames.notify_one();
            int ret = pthread_join(global_audio[0]->thread, NULL);
            LOG_DEBUG_OR_ERROR(ret, "join audio thread");
        }

        // stop backchannel
        if (global_backchannel->imp_backchannel && global_restart_audio)
        {
             global_backchannel->running = false;
             global_backchannel->should_grab_frames.notify_one();
             int ret = pthread_join(backchannel_thread, NULL);
             LOG_DEBUG_OR_ERROR(ret, "join backchannel thread");
        }

        if (global_restart_video)
        {
            LOG_DEBUG("Video is asking to restart!!!");
            // stop motion thread
            if (global_motion_thread_signal)
            {
                global_motion_thread_signal = false;
                int ret = pthread_join(motion_thread, NULL);
                LOG_DEBUG_OR_ERROR(ret, "join motion thread");
            }

            // stop osd thread
            if (global_osd_thread_signal)
            {
                // OSD::exit();
                global_osd_thread_signal = false;
                int ret = pthread_join(osd_thread, NULL);
                LOG_DEBUG_OR_ERROR(ret, "join osd thread");
            }

            // stop jpeg
            if (global_jpeg[0]->imp_encoder)
            {
                global_jpeg[0]->running = false;
                global_jpeg[0]->should_grab_frames.notify_one();
                int ret = pthread_join(global_jpeg[0]->thread, NULL);
                LOG_DEBUG_OR_ERROR(ret, "join jpeg thread 0");
                global_jpeg[1]->running = false;
                global_jpeg[1]->should_grab_frames.notify_one();
                ret = pthread_join(global_jpeg[1]->thread, NULL);
                LOG_DEBUG_OR_ERROR(ret, "join jpeg thread 1");
            }

            // stop stream5
            if (global_video[5]->imp_encoder)
            {
                global_video[5]->running = false;
                global_video[5]->should_grab_frames.notify_one();
                int ret = pthread_join(global_video[5]->thread, NULL);
                LOG_DEBUG_OR_ERROR(ret, "join stream5 thread");
            }
            // stop stream4
            if (global_video[4]->imp_encoder)
            {
                global_video[4]->running = false;
                global_video[4]->should_grab_frames.notify_one();
                int ret = pthread_join(global_video[4]->thread, NULL);
                LOG_DEBUG_OR_ERROR(ret, "join stream4 thread");
            }
            // stop stream1
            if (global_video[1]->imp_encoder)
            {
                global_video[1]->running = false;
                global_video[1]->should_grab_frames.notify_one();
                int ret = pthread_join(global_video[1]->thread, NULL);
                LOG_DEBUG_OR_ERROR(ret, "join stream1 thread");
            }
            // stop stream0
            if (global_video[0]->imp_encoder)
            {
                global_video[0]->running = false;
                global_video[0]->should_grab_frames.notify_one();
                int ret = pthread_join(global_video[0]->thread, NULL);
                LOG_DEBUG_OR_ERROR(ret, "join stream0 thread");
            }
        }  // restart video
    } // while

    return 0;
}
