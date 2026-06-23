#include "VideoWorker.hpp"

#include "Config.hpp"
#include "IMPEncoder.hpp"
#include "IMPFramesource.hpp"
#include "Logger.hpp"
#include "WorkerUtils.hpp"
#include "globals.hpp"
#include "imp_hal.hpp"
#include "OSD.hpp"

#undef MODULE
#define MODULE "VideoWorker"

VideoWorker::VideoWorker(int chn)
    : encChn(chn)
{
    LOG_DEBUG("VideoWorker created for channel " << encChn);
}

VideoWorker::~VideoWorker()
{
    LOG_DEBUG("VideoWorker destroyed for channel " << encChn);
}

void VideoWorker::run()
{
    LOG_DEBUG("Start video processing run loop for stream " << encChn);

    uint32_t bps = 0;
    uint32_t fps = 0;
    uint32_t error_count = 0; // Keep track of polling errors
    unsigned long long ms = 0;
    bool run_for_jpeg = false;

    while (global_video[encChn]->running)
    {
        /* bool helper to check if this is the active jpeg channel and a jpeg is requested while
         * the channel is inactive
         */
        run_for_jpeg = (encChn == global_jpeg[0]->streamChn && global_video[encChn]->run_for_jpeg);

         /* now we need to verify that
         * 1. a client is connected (hasDataCallback)
         * 2. a jpeg is requested
         */
        if (global_video[encChn]->hasDataCallback || run_for_jpeg)
        {
            if (IMP_Encoder_PollingStream(encChn, cfg->general.imp_polling_timeout) == 0)
            {
                IMPEncoderStream stream;
                if (IMP_Encoder_GetStream(encChn, &stream, GET_STREAM_BLOCKING) != 0)
                {
                    LOG_ERROR("IMP_Encoder_GetStream(" << encChn << ") failed");
                    error_count++;
                    continue;
                }

                /* timestamp fix, can be removed if solved
                int64_t nal_ts = stream.pack[stream.packCount - 1].timestamp;
                struct timeval encoder_time;
                encoder_time.tv_sec = nal_ts / 1000000;
                encoder_time.tv_usec = nal_ts % 1000000;
                */

                for (uint32_t i = 0; i < stream.packCount; ++i)
                {
                    fps++;
                    bps += stream.pack[i].length;

                    if (global_video[encChn]->hasDataCallback)
                    {
                        uint8_t *start = hal::encoder::get_pack_data_start(stream, i);
                        uint8_t *end = start + hal::encoder::get_pack_data_length(stream, i);

                        H264NALUnit nalu;

                        /* timestamp fix, can be removed if solved
                        nalu.imp_ts = stream.pack[i].timestamp;
                        nalu.time = encoder_time;
                        */

                        // We use start+4 because the encoder inserts 4-byte MPEG
                        // 'startcodes' at the beginning of each NAL. Live555 complains.
                        nalu.data.insert(nalu.data.end(), start + 4, end);
                        if (global_video[encChn]->idr == false)
                        {
                            int h264_type = hal::encoder::get_h264_nal_type(stream.pack[i]);
                            int h265_type = hal::encoder::get_h265_nal_type(stream.pack[i]);
                            if (h264_type == 7 || h264_type == 8 || h264_type == 5)
                            {
                                global_video[encChn]->idr = true;
                            }
                            else if (h265_type == 32)
                            {
                                global_video[encChn]->idr = true;
                            }
                        }

                        if (global_video[encChn]->idr == true)
                        {
                            if (!global_video[encChn]->msgChannel->write(nalu))
                            {
                                LOG_ERROR("video " << "channel:" << encChn << ", "
                                                   << "package:" << i << " of " << stream.packCount
                                                   << ", " << "packageSize:" << nalu.data.size()
                                                   << ".  !sink clogged!");
                            }
                            else
                            {
                                std::unique_lock<std::mutex> lock_stream{
                                    global_video[encChn]->onDataCallbackLock};
                                if (global_video[encChn]->onDataCallback)
                                    global_video[encChn]->onDataCallback();
                            }
                        }
#if defined(USE_AUDIO_STREAM_REPLICATOR)
                        /* Since the audio stream is permanently in use by the stream replicator,
                         * and the audio grabber and encoder standby is also controlled by the video threads
                         * we need to wakeup the audio thread
                        */
                        if (cfg->audio.input_enabled && !global_audio[0]->active && !global_restart)
                        {
                            LOG_DDEBUG("NOTIFY AUDIO " << !global_audio[0]->active << " "
                                                       << cfg->audio.input_enabled);
                            global_audio[0]->should_grab_frames.notify_one();
                        }
#endif
                    }
                }

                IMP_Encoder_ReleaseStream(encChn, &stream);

                ms = WorkerUtils::tDiffInMs(&global_video[encChn]->stream->stats.ts);
                if (ms > 1000)
                {
                    /* currently we write into osd and stream stats,
                     * osd will be removed and redesigned in future
                    */
                    global_video[encChn]->stream->stats.bps = bps;
                    global_video[encChn]->stream->osd.stats.bps = bps;
                    global_video[encChn]->stream->stats.fps = fps;
                    global_video[encChn]->stream->osd.stats.fps = fps;

                    fps = 0;
                    bps = 0;
                    gettimeofday(&global_video[encChn]->stream->stats.ts, NULL);
                    global_video[encChn]->stream->osd.stats.ts = global_video[encChn]
                                                                     ->stream->stats.ts;
                    /*
                    IMPEncoderCHNStat encChnStats;
                    IMP_Encoder_Query(channel->encChn, &encChnStats);
                    LOG_DEBUG("ChannelStats::" << channel->encChn <<
                                ", registered:" << encChnStats.registered <<
                                ", leftPics:" << encChnStats.leftPics <<
                                ", leftStreamBytes:" << encChnStats.leftStreamBytes <<
                                ", leftStreamFrames:" << encChnStats.leftStreamFrames <<
                                ", curPacks:" << encChnStats.curPacks <<
                                ", work_done:" << encChnStats.work_done);
                    */
                    if (global_video[encChn]->idr_fix)
                    {
                        IMP_Encoder_RequestIDR(encChn);
                        global_video[encChn]->idr_fix--;
                    }
                }
            }
            else
            {
                error_count++;
                LOG_DDEBUG("IMP_Encoder_PollingStream("
                           << encChn << ", " << cfg->general.imp_polling_timeout << ") timeout !");
            }
        }
        else if (global_video[encChn]->onDataCallback == nullptr && !global_restart_video
                 && !global_video[encChn]->run_for_jpeg)
        {
            LOG_DDEBUG("VIDEO LOCK" << " channel:" << encChn << " hasCallbackIsNull:"
                                    << (global_video[encChn]->onDataCallback == nullptr)
                                    << " restartVideo:" << global_restart_video
                                    << " runForJpeg:" << global_video[encChn]->run_for_jpeg);

            global_video[encChn]->stream->stats.bps = 0;
            global_video[encChn]->stream->stats.fps = 0;
            global_video[encChn]->stream->osd.stats.bps = 0;
            global_video[encChn]->stream->osd.stats.fps = 0;

            std::unique_lock<std::mutex> lock_stream{mutex_main};
            global_video[encChn]->active = false;
            while (global_video[encChn]->onDataCallback == nullptr && !global_restart_video
                   && !global_video[encChn]->run_for_jpeg)
                global_video[encChn]->should_grab_frames.wait(lock_stream);

            global_video[encChn]->active = true;
            global_video[encChn]->is_activated.release();

            // unlock audio
            global_audio[0]->should_grab_frames.notify_one();

            LOG_DDEBUG("VIDEO UNLOCK" << " channel:" << encChn);
        }
    }
}

void *VideoWorker::thread_entry(void *arg)
{
    StartHelper *sh = static_cast<StartHelper *>(arg);
    int encChn = sh->encChn;
    int encGrp = sh->encGrp;
    int fsChnNum = sh->fsChnNum;

    LOG_DEBUG("Start stream_grabber thread for stream " << encChn);
    LOG_DEBUG("VideoWorker Stream/encoder chn: " << encChn << " Group: " << encGrp << " Framesource:  " << fsChnNum);

    int ret;
#if 0
// dual sensors:  set group and channels
		switch (encChn) {
			case 0:  /* main-sensor 0 h264 or h265 */
				encChn = 0;
                encGrp = 0;
                fsChnNum = 0;
				break;
			case 1:  /* main-sensor 1 h264 or h265 */
				encChn = 1;
                encGrp = 1;
                fsChnNum = 3;
				break;
			case 2:  /* main-sensor 0 jpeg, not used here */
				encChn = 2;
				encGrp = 0;
                fsChnNum = 0;
				break;
			case 3:  /* main-sensor 1 jpeg, not used here */
				encChn = 3;
				encGrp = 1;
                fsChnNum = 3;
				break;
			case 4:  /* sub-sensor 0, no direct mode */
				encChn = 4;
				encGrp = 2;
                fsChnNum = 1;
				break;
			case 5:  /* sub-sensor 1, no direct mode */
				encChn = 5;
				encGrp = 3;
                fsChnNum = 4;
				break;
			default:
				LOG_DEBUG("unsupported encChn: " <<  encChn);
				return 0;
		}
#endif
        global_video[encChn]->imp_framesource = IMPFramesource::createNew(global_video[encChn]->stream,
                                                                      &cfg->sensor,  // need to move, add to global_video
                                                                      fsChnNum);
        global_video[encChn]->imp_encoder = IMPEncoder::createNew(global_video[encChn]->stream,
                                                              encChn,
                                                              encGrp,
//                                                              fsChnNum,
                                                              global_video[encChn]->name);
        global_video[encChn]->run_for_jpeg = false;
#if 1

    _stream *stream = global_video[encChn]->stream;
    if (strcmp(stream->format, "JPEG") != 0)
    {
        int ret = 0;
        IMPCell fs{};
        IMPCell enc{};
        IMPCell osd_cell{};
        OSD *osd = nullptr;

        fs = {DEV_ID_FS, fsChnNum, 0};
        enc = {DEV_ID_ENC, encChn, 0};
        osd_cell = {DEV_ID_OSD, encGrp, 0};

        if (stream->osd.enabled)
        {
//            osd = OSD::createNew(stream->osd, encGrp, encChn, global_video[encChn]->name);

            ret = IMP_System_Bind(&fs, &osd_cell);
            //LOG_DEBUG_OR_ERROR_AND_EXIT(ret, "IMP_System_Bind(&fs, &osd_cell)");
            LOG_DEBUG_OR_ERROR(ret, "IMP_System_Bind-fs->osd(" << fsChnNum << "," << encGrp << ")");

            ret = IMP_System_Bind(&osd_cell, &enc);
            //LOG_DEBUG_OR_ERROR_AND_EXIT(ret, "IMP_System_Bind(&osd_cell, &enc)");
            LOG_DEBUG_OR_ERROR(ret, "IMP_System_Bind->osd->enc(" <<encGrp << "," <<  encChn << ")");
        }
        else
        {
            ret = IMP_System_Bind(&fs, &enc);
            //LOG_DEBUG_OR_ERROR_AND_EXIT(ret, "IMP_System_Bind(&fs, &enc)");
            LOG_DEBUG_OR_ERROR(ret, "IMP_System_Bind-fs->enc(" << fsChnNum << "," << encChn << ")");
        }
    }
#endif
    global_video[encChn]->imp_framesource->enable();
    // inform main that initialization is complete
    sh->has_started.release();

    ret = IMP_Encoder_StartRecvPic(encChn);
    LOG_DEBUG_OR_ERROR(ret, "IMP_Encoder_StartRecvPic(" << encChn << ")");
    if (ret != 0)
        return 0;

    /* 'active' indicates, the thread is activly polling and grabbing images
     * 'running' describes the runlevel of the thread, if this value is set to false
     *           the thread exits and cleanup all ressources
     */
    global_video[encChn]->active = true;
    global_video[encChn]->running = true;
    VideoWorker worker(encChn);
    worker.run();

    ret = IMP_Encoder_StopRecvPic(encChn);
    LOG_DEBUG_OR_ERROR(ret, "IMP_Encoder_StopRecvPic(" << encChn << ")");

    if (global_video[encChn]->imp_framesource)
    {
        global_video[encChn]->imp_framesource->disable();

        if (global_video[encChn]->imp_encoder)
        {
            global_video[encChn]->imp_encoder->deinit();
            delete global_video[encChn]->imp_encoder;
            global_video[encChn]->imp_encoder = nullptr;
        }
    }

    return 0;
}
