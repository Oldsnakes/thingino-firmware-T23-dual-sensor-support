#include "imp_hal.hpp"
#include "Config.hpp"
#include "globals.hpp"
#include "Logger.hpp"
#include "WorkerUtils.hpp"
#include <cstring>
#include <dlfcn.h>
#include <imp/imp_isp.h>
#include "Motor.hpp"
#include "Motion.hpp"
#include "IMPFramesource.hpp"

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "ctrls_hal.hpp"    // GPIO control

#undef IMP_DEBUG

extern void MakeTables(int q, uint8_t *lqt, uint8_t *cqt);

namespace hal {

static PlatformCaps g_caps = {
    // Encoder capabilities (must match struct order)
#if defined(PLATFORM_T31) || defined(PLATFORM_C100) || defined(PLATFORM_T40) || defined(PLATFORM_T41)
    .has_h265 = true,
    .has_capped_quality = true,
    .has_capped_vbr = true,
    .has_ip_pb_delta = true,
    .has_bufshare = true,
    .has_jpeg_set_qtable = false,
    .has_smart_rc = true,
    .has_super_frm = true,
    .has_intra_refresh = true,
#elif defined(PLATFORM_T30)
    .has_h265 = true,
    .has_capped_quality = false,
    .has_capped_vbr = false,
    .has_ip_pb_delta = false,
    .has_bufshare = false,
    .has_jpeg_set_qtable = true,
    .has_smart_rc = false,
    .has_super_frm = false,
    .has_intra_refresh = false,
#else
    .has_h265 = false,
    .has_capped_quality = false,
    .has_capped_vbr = false,
    .has_ip_pb_delta = false,
    .has_bufshare = false,
    .has_jpeg_set_qtable = true,
    .has_smart_rc = false,
    .has_super_frm = false,
    .has_intra_refresh = false,
#endif

    // Audio capabilities (must match struct order)
#if defined(PLATFORM_T23) || defined(PLATFORM_T40) || defined(PLATFORM_T41)
    .has_audio_aec_channel = true,
#else
    .has_audio_aec_channel = false,
#endif

#if defined(PLATFORM_T20) || defined(PLATFORM_T21) || defined(PLATFORM_T23) || defined(PLATFORM_T30) || defined(PLATFORM_T31) || defined(PLATFORM_C100) || defined(PLATFORM_T40) || defined(PLATFORM_T41)
    .has_audio_agc = true,
#else
    .has_audio_agc = false,
#endif

#if defined(PLATFORM_T21) || defined(PLATFORM_T31) || defined(PLATFORM_C100)
    .has_audio_alc = true,
#else
    .has_audio_alc = false,
#endif

#if defined(PLATFORM_T20) || defined(PLATFORM_T21) || defined(PLATFORM_T23) || defined(PLATFORM_T30) || defined(PLATFORM_T31) || defined(PLATFORM_C100) || defined(PLATFORM_T40) || defined(PLATFORM_T41)
    .has_audio_hpf = true,
    .has_audio_ns = true,
#else
    .has_audio_hpf = false,
    .has_audio_ns = false,
#endif

    // ISP capabilities (must match struct order)
#if defined(PLATFORM_T10) || defined(PLATFORM_T20) || defined(PLATFORM_T21) || defined(PLATFORM_T23) || defined(PLATFORM_T30) || defined(PLATFORM_T31) || defined(PLATFORM_C100)
    .has_isp_sinter = true,
#else
    .has_isp_sinter = false,
#endif

#if defined(PLATFORM_T10) || defined(PLATFORM_T20) || defined(PLATFORM_T21) || defined(PLATFORM_T23) || defined(PLATFORM_T30) || defined(PLATFORM_T31) || defined(PLATFORM_C100)
    .has_isp_temper = true,
#else
    .has_isp_temper = false,
#endif

#if defined(PLATFORM_T23) || defined(PLATFORM_T31) || defined(PLATFORM_C100)
    .has_isp_hue = true,
#else
    .has_isp_hue = false,
#endif

#if defined(PLATFORM_T31) || defined(PLATFORM_C100)
    .has_isp_dpc = true,
#else
    .has_isp_dpc = false,
#endif

#if defined(PLATFORM_T10) || defined(PLATFORM_T20) || defined(PLATFORM_T21) || defined(PLATFORM_T23) || defined(PLATFORM_T30) || defined(PLATFORM_T31) || defined(PLATFORM_C100)
    .has_isp_drc = true,
#else
    .has_isp_drc = false,
#endif

#if defined(PLATFORM_T23) || defined(PLATFORM_T31) || defined(PLATFORM_C100)
    .has_isp_defog = true,
    .has_isp_backlight_comp = true,
#else
    .has_isp_defog = false,
    .has_isp_backlight_comp = false,
#endif

#if defined(PLATFORM_T21) || defined(PLATFORM_T23) || defined(PLATFORM_T30) || defined(PLATFORM_T31) || defined(PLATFORM_C100)
    .has_isp_highlight_depress = true,
#else
    .has_isp_highlight_depress = false,
#endif

#if !defined(PLATFORM_T21) && !defined(PLATFORM_T40) && !defined(PLATFORM_T41)
    .has_isp_ae_comp = true,
#else
    .has_isp_ae_comp = false,
#endif

#if !defined(PLATFORM_T40) && !defined(PLATFORM_T41)
    .has_isp_max_gain = true,
#else
    .has_isp_max_gain = false,
#endif

    // ISP control capabilities
#if !defined(PLATFORM_T40) && !defined(PLATFORM_T41)
    .has_isp_hflip = true,  // T23 can use combined HVFLIP, and must used for the second sensor
    .has_isp_vflip = true,
#else
    .has_isp_hflip = false,  // T40/T41 use combined HVFLIP
    .has_isp_vflip = false,  // T40/T41 use combined HVFLIP
#endif

    .has_isp_running_mode = true,  // All platforms support running mode
    .has_isp_anti_flicker = true,  // All platforms support anti-flicker
    .has_isp_wb = true,            // All platforms support white balance

    // OSD capabilities
#if defined(PLATFORM_T31) || defined(PLATFORM_C100) || defined(PLATFORM_T40) || defined(PLATFORM_T41)
    .has_osd_region_invert = true,
#else
    .has_osd_region_invert = false,
#endif

    // Framesource capabilities
#if defined(PLATFORM_T31)
    .has_framesource_chn_rotate = true,
#else
    .has_framesource_chn_rotate = false,
#endif

    // System capabilities
#if defined(PLATFORM_T40) || defined(PLATFORM_T41)
    .uses_xburst2 = true,
    .uses_kernel_4 = true,
#else
    .uses_xburst2 = false,
    .uses_kernel_4 = false,
#endif
};

const PlatformCaps& caps() {
    return g_caps;
}

void set_jpeg_quality_qtable(int encChn, int quality, const char* cpu_hint)
{
    if (quality < 1 || quality > 100) return;

    if (!caps().has_jpeg_set_qtable) {
        // Not supported on T31/T40/T41/C100 SDKs (JPEG tables fixed by SDK)
        return;
    }

#if !(defined(PLATFORM_T31) || defined(PLATFORM_C100) || defined(PLATFORM_T40) || defined(PLATFORM_T41))
    IMPEncoderJpegeQl pst{};
    if (cpu_hint && strncmp(cpu_hint, "T10", 3) == 0) {
        pst.user_ql_en = 0;
        LOG_DEBUG("HAL JPEG: default quantization (T10 family)");
    } else {
        uint8_t lqt[64], cqt[64];
        MakeTables(quality, lqt, cqt);
        for (int i = 0; i < 64; ++i) pst.qmem_table[i] = lqt[i];
        for (int i = 0; i < 64; ++i) pst.qmem_table[64 + i] = cqt[i];
        pst.user_ql_en = 1;
        LOG_DEBUG("HAL JPEG: custom quantization tables set");
    }
    IMP_Encoder_SetJpegeQl(encChn, &pst);
#else
    (void)encChn; (void)cpu_hint; // no-op
#endif
}

int maybe_enable_bufshare(int jpegEncGrp, int srcEncChn, bool allow_shared)
{
    if (!allow_shared) return 0;
#if defined(PLATFORM_T31) || defined(PLATFORM_C100) || defined(PLATFORM_T40) || defined(PLATFORM_T41)
    typedef int (*pfn_setbufshare)(int,int);
    void* handle = dlopen(nullptr, RTLD_LAZY);
    pfn_setbufshare fn = handle ? reinterpret_cast<pfn_setbufshare>(dlsym(handle, "IMP_Encoder_SetbufshareChn")) : nullptr;
    if (fn) {
        int ret = fn(jpegEncGrp, srcEncChn);
        LOG_DEBUG_OR_ERROR(ret, "IMP_Encoder_SetbufshareChn(" << jpegEncGrp << ", " << srcEncChn << ")");
        return ret;
    } else {
        LOG_DEBUG("IMP_Encoder_SetbufshareChn not available; skipping bufshare");
        return 0;
    }
#else
    (void)jpegEncGrp; (void)srcEncChn;
    return 0;
#endif
}

#if defined(PLATFORM_T31) || defined(PLATFORM_C100) || defined(PLATFORM_T40) || defined(PLATFORM_T41)
void apply_rc_overrides(IMPEncoderCHNAttr &chnAttr, IMPEncoderRcMode rcMode, const _stream &stream)
{
    auto *rcAttr = &chnAttr.rcAttr;
    int qp_init = stream.qp_init;
    int qp_min  = stream.qp_min;
    int qp_max  = stream.qp_max;
    int ip_delta = stream.ip_delta;
    int pb_delta = stream.pb_delta;
    int max_br = stream.max_bitrate;

    switch (rcMode) {
        case IMP_ENC_RC_MODE_FIXQP:
            if (qp_init >= 0) rcAttr->attrRcMode.attrFixQp.iInitialQP = qp_init;
            break;
        case IMP_ENC_RC_MODE_CBR:
            if (qp_init >= 0) rcAttr->attrRcMode.attrCbr.iInitialQP = qp_init;
            if (qp_min >= 0)  rcAttr->attrRcMode.attrCbr.iMinQP = qp_min;
            if (qp_max >= 0)  rcAttr->attrRcMode.attrCbr.iMaxQP = qp_max;
            if (ip_delta != -1) rcAttr->attrRcMode.attrCbr.iIPDelta = ip_delta;
            if (pb_delta != -1) rcAttr->attrRcMode.attrCbr.iPBDelta = pb_delta;
            break;
        case IMP_ENC_RC_MODE_VBR:
            if (qp_init >= 0) rcAttr->attrRcMode.attrVbr.iInitialQP = qp_init;
            if (qp_min >= 0)  rcAttr->attrRcMode.attrVbr.iMinQP = qp_min;
            if (qp_max >= 0)  rcAttr->attrRcMode.attrVbr.iMaxQP = qp_max;
            if (ip_delta != -1) rcAttr->attrRcMode.attrVbr.iIPDelta = ip_delta;
            if (pb_delta != -1) rcAttr->attrRcMode.attrVbr.iPBDelta = pb_delta;
            if (max_br > 0)     rcAttr->attrRcMode.attrVbr.uMaxBitRate = max_br;
            break;
        case IMP_ENC_RC_MODE_CAPPED_VBR:
            if (qp_init >= 0) rcAttr->attrRcMode.attrCappedVbr.iInitialQP = qp_init;
            if (qp_min >= 0)  rcAttr->attrRcMode.attrCappedVbr.iMinQP = qp_min;
            if (qp_max >= 0)  rcAttr->attrRcMode.attrCappedVbr.iMaxQP = qp_max;
            if (ip_delta != -1) rcAttr->attrRcMode.attrCappedVbr.iIPDelta = ip_delta;
            if (pb_delta != -1) rcAttr->attrRcMode.attrCappedVbr.iPBDelta = pb_delta;
            if (max_br > 0)     rcAttr->attrRcMode.attrCappedVbr.uMaxBitRate = max_br;
            break;
        case IMP_ENC_RC_MODE_CAPPED_QUALITY:
            if (qp_init >= 0) rcAttr->attrRcMode.attrCappedQuality.iInitialQP = qp_init;
            if (qp_min >= 0)  rcAttr->attrRcMode.attrCappedQuality.iMinQP = qp_min;
            if (qp_max >= 0)  rcAttr->attrRcMode.attrCappedQuality.iMaxQP = qp_max;
            if (ip_delta != -1) rcAttr->attrRcMode.attrCappedQuality.iIPDelta = ip_delta;
            if (pb_delta != -1) rcAttr->attrRcMode.attrCappedQuality.iPBDelta = pb_delta;
            if (max_br > 0)     rcAttr->attrRcMode.attrCappedQuality.uMaxBitRate = max_br;
            break;
        default:
            break;
    }
}
#else
void apply_rc_overrides(IMPEncoderCHNAttr &chnAttr, int rcMode, const _stream &stream)
{
    auto *rcAttr = &chnAttr.rcAttr;
    int qp_init = stream.qp_init;
    int qp_min  = stream.qp_min;
    int qp_max  = stream.qp_max;
    int max_br  = stream.max_bitrate;

    if (chnAttr.encAttr.enType == PT_H264) {
        switch (rcMode) {
            case ENC_RC_MODE_FIXQP:
                if (qp_init >= 0) rcAttr->attrRcMode.attrH264FixQp.qp = qp_init;
                break;
            case ENC_RC_MODE_CBR:
                if (qp_min >= 0) rcAttr->attrRcMode.attrH264Cbr.minQp = qp_min;
                if (qp_max >= 0) rcAttr->attrRcMode.attrH264Cbr.maxQp = qp_max;
                break;
            case ENC_RC_MODE_VBR:
                if (qp_min >= 0) rcAttr->attrRcMode.attrH264Vbr.minQp = qp_min;
                if (qp_max >= 0) rcAttr->attrRcMode.attrH264Vbr.maxQp = qp_max;
                if (max_br > 0) rcAttr->attrRcMode.attrH264Vbr.maxBitRate = max_br;
                break;
            case ENC_RC_MODE_SMART:
                if (qp_min >= 0) rcAttr->attrRcMode.attrH264Smart.minQp = qp_min;
                if (qp_max >= 0) rcAttr->attrRcMode.attrH264Smart.maxQp = qp_max;
                if (max_br > 0) rcAttr->attrRcMode.attrH264Smart.maxBitRate = max_br;
                break;
            default:
                break;
        }
    }
#if defined(PLATFORM_T30)
    else if (chnAttr.encAttr.enType == PT_H265) {
        // Only SMART mode used in current code for H265 on T30
        if (qp_min >= 0) rcAttr->attrRcMode.attrH265Smart.minQp = qp_min;
        if (qp_max >= 0) rcAttr->attrRcMode.attrH265Smart.maxQp = qp_max;
        if (max_br > 0) rcAttr->attrRcMode.attrH265Smart.maxBitRate = max_br;
    }
#endif
}
#endif



// ============================================================================
// ISP Tuning HAL Implementation
// ### add second sensor ISP control for T23
// ### cfg->sensor.select = 0x01 for first sensor, 0x02 for second sensor, 0x03 for both sensors
// ============================================================================

namespace isp {

#if defined(PLATFORM_T40) || defined(PLATFORM_T41)
    #define IMPVI IMPVI_MAIN
#endif

int set_brightness(unsigned char val)
{
#if defined(PLATFORM_T40) || defined(PLATFORM_T41)
    return IMP_ISP_Tuning_SetBrightness(IMPVI, &val);
#elif defined(PLATFORM_T23) 
    int ret = 0;
    if (cfg->sensor.select & 0x2) ret += IMP_ISP_Tuning_SetBrightness_Sec(val);
    if (cfg->sensor.select & 0x1) ret += IMP_ISP_Tuning_SetBrightness(val); 
    return ret;
#else 
    int ret = IMP_ISP_Tuning_SetBrightness_Sec(val);
    return IMP_ISP_Tuning_SetBrightness(val) + ret;
#endif
}

int set_contrast(unsigned char val)
{
#if defined(PLATFORM_T40) || defined(PLATFORM_T41)
    return IMP_ISP_Tuning_SetContrast(IMPVI, &val);
#elif defined(PLATFORM_T23) 
    int ret = 0;
    if (cfg->sensor.select & 0x2) ret += IMP_ISP_Tuning_SetContrast_Sec(val);
    if (cfg->sensor.select & 0x1) ret += IMP_ISP_Tuning_SetContrast(val); 
    return ret;
#else
    int ret = IMP_ISP_Tuning_SetContrast_Sec(val);
    return IMP_ISP_Tuning_SetContrast(val) + ret;
#endif
}

int set_saturation(unsigned char val)
{
#if defined(PLATFORM_T40) || defined(PLATFORM_T41)
    return IMP_ISP_Tuning_SetSaturation(IMPVI, &val);
#elif defined(PLATFORM_T23) 
    int ret = 0;
    if (cfg->sensor.select & 0x2) ret += IMP_ISP_Tuning_SetSaturation_Sec(val);
    if (cfg->sensor.select & 0x1) ret += IMP_ISP_Tuning_SetSaturation(val); 
    return ret;
#else
    int ret = IMP_ISP_Tuning_SetSaturation_Sec(val);
    return IMP_ISP_Tuning_SetSaturation(val) + ret;
#endif
}

int set_sharpness(unsigned char val)
{
#if defined(PLATFORM_T40) || defined(PLATFORM_T41)
    return IMP_ISP_Tuning_SetSharpness(IMPVI, &val);
#elif defined(PLATFORM_T23) 
    int ret = 0;
    if (cfg->sensor.select & 0x2) ret += IMP_ISP_Tuning_SetSharpness_Sec(val);
    if (cfg->sensor.select & 0x1) ret += IMP_ISP_Tuning_SetSharpness(val); 
    return ret;
#else
    int ret = IMP_ISP_Tuning_SetSharpness_Sec(val);
    return IMP_ISP_Tuning_SetSharpness(val) + ret;
#endif
}

int set_sinter_strength(unsigned char val)
{
    if (!caps().has_isp_sinter) {
        LOG_DEBUG("set_sinter_strength not supported on this platform");
        return 0;
    }
#if defined(PLATFORM_T23) 
    int ret = 0;
    if (cfg->sensor.select & 0x2) ret += IMP_ISP_Tuning_SetSinterStrength_Sec(val);
    if (cfg->sensor.select & 0x1) ret += IMP_ISP_Tuning_SetSinterStrength(val); 
    return ret;
#elif defined(PLATFORM_T31) || defined(PLATFORM_C100)
    // Simple value API
    int ret = IMP_ISP_Tuning_SetSinterStrength_Sec(val);
    return IMP_ISP_Tuning_SetSinterStrength(val) + ret;
#elif defined(PLATFORM_T10) || defined(PLATFORM_T20) || defined(PLATFORM_T21) || defined(PLATFORM_T30)
    // Struct-based API
    IMPISPSinterDenoiseAttr attr;
    memset(&attr, 0, sizeof(attr));
    attr.enable = IMPISP_TUNING_OPS_MODE_ENABLE;
    attr.type = IMPISP_TUNING_OPS_MODE_MANUAL;
    attr.sinter_strength = val;
    return IMP_ISP_Tuning_SetSinterDnsAttr(&attr);
#else
    return 0;
#endif
}

int set_temper_strength(unsigned char val)
{
    if (!caps().has_isp_temper) {
        LOG_DEBUG("set_temper_strength not supported on this platform");
        return 0;
    }
    int ret = 0;
#if defined(PLATFORM_T23) 
    if (cfg->sensor.select & 0x2) ret += IMP_ISP_Tuning_SetTemperStrength_Sec(val);
    if (cfg->sensor.select & 0x1) ret += IMP_ISP_Tuning_SetTemperStrength(val); 
    return ret;
#elif defined(PLATFORM_T31) || defined(PLATFORM_C100)
    // Simple value API
    ret = IMP_ISP_Tuning_SetTemperStrength_Sec(val);
    return IMP_ISP_Tuning_SetTemperStrength(val) + ret;
#elif defined(PLATFORM_T10) || defined(PLATFORM_T20) || defined(PLATFORM_T21) || defined(PLATFORM_T30)
    // Struct-based API
    IMPISPTemperDenoiseAttr attr;
    memset(&attr, 0, sizeof(attr));
    attr.type = IMPISP_TEMPER_MANUAL;
    attr.val = val;
    return IMP_ISP_Tuning_SetTemperDnsCtl(&attr);
#else
    return 0;
#endif
}

int set_hue(unsigned char val)
{
    if (!caps().has_isp_hue) {
        LOG_DEBUG("set_hue not supported on this platform");
        return 0;
    }
    int ret = 0;
#if defined(PLATFORM_T23) 
    if (cfg->sensor.select & 0x2) ret += IMP_ISP_Tuning_SetBcshHue_Sec(val);
    if (cfg->sensor.select & 0x1) ret += IMP_ISP_Tuning_SetBcshHue(val); 
    return ret;
#elif defined(PLATFORM_T31) || defined(PLATFORM_C100)
    // Simple value API
    ret = IMP_ISP_Tuning_SetBcshHue_Sec(val);
    return IMP_ISP_Tuning_SetBcshHue(val) + ret;
#elif defined(PLATFORM_T40) || defined(PLATFORM_T41)
    return IMP_ISP_Tuning_SetBcshHue(IMPVI_MAIN, &val);
#else
    return 0; // Function doesn't exist on this platform
#endif
}

int set_hflip(bool enable)
{
#if defined(PLATFORM_T40) || defined(PLATFORM_T41)
    // T40 uses combined HVFLIP function
    // For now, we'll need to get current vflip state and set both
    // This is a limitation - we can't set H and V independently on T40
    LOG_DEBUG("set_hflip: T40 uses combined HVFLIP - feature limited");
    return 0; // TODO: implement combined flip handling
#elif defined(PLATFORM_T23)   // can be used for T40/T41
    // use HVFLIP
    IMPISPHVFLIP hvflip;
    int ret = IMP_ISP_Tuning_GetHVFlip(&hvflip);
    hvflip = enable ? (IMPISPHVFLIP) (hvflip | IMPISP_FLIP_H_MODE) : (IMPISPHVFLIP) (hvflip & ~IMPISP_FLIP_H_MODE);

    LOG_DEBUG("hflip: " << hvflip);
    ret = 0;
    if (cfg->sensor.select & 0x2) ret += IMP_ISP_Tuning_SetHVFLIP_Sec(hvflip);
    if (cfg->sensor.select & 0x1) ret += IMP_ISP_Tuning_SetHVFLIP(hvflip); 
    return ret;
#else
    IMPISPTuningOpsMode mode = enable ? IMPISP_TUNING_OPS_MODE_ENABLE : IMPISP_TUNING_OPS_MODE_DISABLE;
    return IMP_ISP_Tuning_SetISPHflip(mode);
#endif
}

int set_vflip(bool enable)
{
#if defined(PLATFORM_T40) || defined(PLATFORM_T41)
    // T40 uses combined HVFLIP function
    LOG_DEBUG("set_vflip: T40 uses combined HVFLIP - feature limited");
    return 0; // TODO: implement combined flip handling
#elif defined(PLATFORM_T23)    // can be used for T40/T41
    // use HVFLIP
    IMPISPHVFLIP hvflip;
    int ret = IMP_ISP_Tuning_GetHVFlip(&hvflip);
    hvflip = enable ? (IMPISPHVFLIP) (hvflip | IMPISP_FLIP_V_MODE) : (IMPISPHVFLIP) (hvflip & ~IMPISP_FLIP_V_MODE);

    LOG_DEBUG("hflip: " << hvflip);
    ret = 0;
    if (cfg->sensor.select & 0x2) ret += IMP_ISP_Tuning_SetHVFLIP_Sec(hvflip);
    if (cfg->sensor.select & 0x1) ret += IMP_ISP_Tuning_SetHVFLIP(hvflip); 
    return ret;
#else
    IMPISPTuningOpsMode mode = enable ? IMPISP_TUNING_OPS_MODE_ENABLE : IMPISP_TUNING_OPS_MODE_DISABLE;
    return IMP_ISP_Tuning_SetISPHflip(mode);
#endif
}

int set_running_mode(int mode)
{
    int ret = 0;
#if defined(PLATFORM_T40) || defined(PLATFORM_T41)
    IMPISPRunningMode m = (IMPISPRunningMode)mode;
    return IMP_ISP_Tuning_SetISPRunningMode(IMPVI, &m);
#elif defined(PLATFORM_T23) 
    if (cfg->sensor.select & 0x2) ret += IMP_ISP_Tuning_SetISPRunningMode_Sec((IMPISPRunningMode)mode);
    if (cfg->sensor.select & 0x1) ret += IMP_ISP_Tuning_SetISPRunningMode((IMPISPRunningMode)mode); 
    return ret;
#else
    ret = IMP_ISP_Tuning_SetISPRunningMode_Sec((IMPISPRunningMode)mode);
    return IMP_ISP_Tuning_SetISPRunningMode((IMPISPRunningMode)mode) + ret;
#endif
}

int set_isp_bypass(bool enable)
{
    IMPISPTuningOpsMode mode = enable ? IMPISP_TUNING_OPS_MODE_ENABLE : IMPISP_TUNING_OPS_MODE_DISABLE;
#if defined(PLATFORM_T40) || defined(PLATFORM_T41)
    return IMP_ISP_Tuning_SetISPBypass(IMPVI, &mode);
#else
    return IMP_ISP_Tuning_SetISPBypass(mode);
#endif
}

int set_anti_flicker(int mode)
{
    int ret = 0;
#if defined(PLATFORM_T40) || defined(PLATFORM_T41)
    IMPISPAntiflickerAttr attr;
    attr.mode = (IMPISPAntiflickerMode)mode;
    attr.freq = 50; // Default to 50Hz, could be made configurable
    return IMP_ISP_Tuning_SetAntiFlickerAttr(IMPVI, &attr);
#elif defined(PLATFORM_T23) 
    if (cfg->sensor.select & 0x2) ret += IMP_ISP_Tuning_SetAntiFlickerAttr_Sec((IMPISPAntiflickerAttr)mode);
    if (cfg->sensor.select & 0x1) ret += IMP_ISP_Tuning_SetAntiFlickerAttr((IMPISPAntiflickerAttr)mode); 
    return ret;
#else
    ret = IMP_ISP_Tuning_SetAntiFlickerAttr_Sec((IMPISPAntiflickerAttr)mode);
    return IMP_ISP_Tuning_SetAntiFlickerAttr((IMPISPAntiflickerAttr)mode) + ret;
#endif
}

int set_ae_compensation(int val)
{
    if (!caps().has_isp_ae_comp) {
        LOG_DEBUG("set_ae_compensation not supported on this platform");
        return -1;
    }
    int ret = 0;
#if !defined(PLATFORM_T40) && !defined(PLATFORM_T41)
    return IMP_ISP_Tuning_SetAeComp(val);
#elif defined(PLATFORM_T23) 
    if (cfg->sensor.select & 0x2) ret += IMP_ISP_Tuning_SetAeComp_Sec(val);
    if (cfg->sensor.select & 0x1) ret += IMP_ISP_Tuning_SetAeComp(val); 
    return ret;
#else
    LOG_DEBUG("AE Comp: " << val);
    ret = IMP_ISP_Tuning_SetAeComp_Sec(val);
    return IMP_ISP_Tuning_SetAeComp(val) + ret;
#endif
}

int set_dpc_strength(unsigned char val)
{
    if (!caps().has_isp_dpc) {
        LOG_DEBUG("set_dpc_strength not supported on this plIMP_ISP_Tuning_SetAntiFlickerAttratform");
        return -1;
    }
#if defined(PLATFORM_T31) || defined(PLATFORM_C100)
    return IMP_ISP_Tuning_SetDPC_Strength(val);
#else
    int ret = IMP_ISP_Tuning_SetDPC_Strength_Sec(val);
    return IMP_ISP_Tuning_SetDPC_Strength(val) + ret;
#endif
}

int set_drc_strength(unsigned char val)
{
    if (!caps().has_isp_drc) {
        LOG_DEBUG("set_drc_strength not supported on this platform");
        return -1;
    }
    int ret = 0;
#if defined(PLATFORM_T23) 
    ret += IMP_ISP_Tuning_EnableDRC(IMPISP_TUNING_OPS_MODE_ENABLE);
    ret += IMP_ISP_Tuning_EnableDRC_Sec(IMPISP_TUNING_OPS_MODE_ENABLE);
    if (cfg->sensor.select & 0x2) ret += IMP_ISP_Tuning_SetDRC_Strength_Sec(val);
    if (cfg->sensor.select & 0x1) ret += IMP_ISP_Tuning_SetDRC_Strength(val); 
    return ret;
#elif defined(PLATFORM_T31) || defined(PLATFORM_C100)
    // Simple value API
    ret = IMP_ISP_Tuning_SetDRC_Strength_Sec(val);
    return IMP_ISP_Tuning_SetDRC_Strength(val) + ret;
#elif defined(PLATFORM_T10) || defined(PLATFORM_T20) || defined(PLATFORM_T21) || defined(PLATFORM_T30)
    // Struct-based API
    IMPISPDrcAttr attr;
    memset(&attr, 0, sizeof(attr));
    attr.mode = IMPISP_DRC_MANUAL;
    attr.drc_strength = val;
    return IMP_ISP_Tuning_SetRawDRC(&attr);
#else
    return 0;
#endif
}

int set_defog_strength(uint8_t val)
{
    if (!caps().has_isp_defog) {
        LOG_DEBUG("set_defog_strength not supported on this platform");
        return -1;
    }
    int ret = 0;
#if defined(PLATFORM_T23) 
    if (cfg->sensor.select & 0x2) ret += IMP_ISP_Tuning_SetDefog_Strength_Sec(reinterpret_cast<uint8_t*>(&val));
    if (cfg->sensor.select & 0x1) ret += IMP_ISP_Tuning_SetDefog_Strength(reinterpret_cast<uint8_t*>(&val)); 
    return ret;
#elif defined(PLATFORM_T31) || defined(PLATFORM_C100)
    ret = IMP_ISP_Tuning_SetDefog_Strength_Sec(reinterpret_cast<uint8_t*>(&val));
    return IMP_ISP_Tuning_SetDefog_Strength(reinterpret_cast<uint8_t*>(&val)) + ret;
#else
    return 0;
#endif
}

int set_backlight_comp(unsigned char val)
{
    if (!caps().has_isp_backlight_comp) {
        LOG_DEBUG("set_backlight_comp not supported on this platform");
        return -1;
    }
    int ret = 0;
#if defined(PLATFORM_T23) 
    if (cfg->sensor.select & 0x2) ret += IMP_ISP_Tuning_SetBacklightComp_Sec(val);
    if (cfg->sensor.select & 0x1) ret += IMP_ISP_Tuning_SetBacklightComp(val); 
    return ret;
#elif defined(PLATFORM_T31) || defined(PLATFORM_C100)
    ret = IMP_ISP_Tuning_SetBacklightComp_Sec(val);
    return IMP_ISP_Tuning_SetBacklightComp(val) + ret;
#else
    return 0;
#endif
}

int set_highlight_depress(unsigned char val)
{
    if (!caps().has_isp_highlight_depress) {
        LOG_DEBUG("set_highlight_depress not supported on this platform");
        return -1;
    }
#if !defined(PLATFORM_T40) && !defined(PLATFORM_T41) && !defined(PLATFORM_T10) && !defined(PLATFORM_T20)
    int ret = IMP_ISP_Tuning_SetHiLightDepress_Sec(val);
    return IMP_ISP_Tuning_SetHiLightDepress(val) + ret;
#else
    return 0;
#endif
}

int set_max_again(unsigned char val)
{
    if (!caps().has_isp_max_gain) {
        LOG_DEBUG("set_max_again not supported on this platform");
        return -1;
    }
    int ret = 0;
#if defined(PLATFORM_T23) 
    if (cfg->sensor.select & 0x2) ret += IMP_ISP_Tuning_SetMaxAgain_Sec(val);
    if (cfg->sensor.select & 0x1) ret += IMP_ISP_Tuning_SetMaxAgain(val); 
    return ret;
#elif !defined(PLATFORM_T40) && !defined(PLATFORM_T41)
    ret = IMP_ISP_Tuning_SetMaxAgain_Sec(val);
    return IMP_ISP_Tuning_SetMaxAgain(val) + ret;
#else
    return 0;
#endif
}

int set_max_dgain(unsigned char val)
{
    if (!caps().has_isp_max_gain) {
        LOG_DEBUG("set_max_dgain not supported on this platform");
        return -1;
    }
    int ret = 0;
#if defined(PLATFORM_T23) 
    if (cfg->sensor.select & 0x2) ret += IMP_ISP_Tuning_SetMaxDgain_Sec(val);
    if (cfg->sensor.select & 0x1) ret += IMP_ISP_Tuning_SetMaxDgain(val); 
    return ret;
#elif !defined(PLATFORM_T40) && !defined(PLATFORM_T41)
    ret = IMP_ISP_Tuning_SetMaxDgain_Sec(val);
    return IMP_ISP_Tuning_SetMaxDgain(val) + ret;
#else
    return 0;
#endif
}


int set_alt_sensor(bool enable)
{
    int ret = 0;
    global_MipiMode = enable ? 3 : 0;
    LOG_DEBUG("#### global_MipiMode: " << global_MipiMode );
    IMPISPSENSORAttr attr;
    memset(&attr, 0, sizeof(IMPISPSENSORAttr));
    ret = IMP_ISP_Tuning_GetSensorAttr(&attr);
    LOG_DEBUG_OR_ERROR(ret, "Sensor 0:  hts = " << attr.hts << " vts = " << attr.vts << " width = " 
        << attr.width << " height = " << attr.height << " fps = " << attr.fps);
    IMP_ISP_Tuning_GetSensorAttr_Sec(&attr);
    LOG_DEBUG_OR_ERROR(ret, "Sensor 1:  hts = " << attr.hts << " vts = " << attr.vts << " width = " 
        << attr.width << " height = " << attr.height << " fps = " << attr.fps);
    return 0;
}

int set_again_mode(bool enable)
{
    int ret = 0;
    IMPISPAEAttr ae;
    memset(&ae, 0, sizeof(IMPISPAEAttr));
    ret = IMP_ISP_Tuning_GetAeAttr(&ae);
    IMPISPTuningOpsMode mode = enable ? IMPISP_TUNING_OPS_MODE_ENABLE : IMPISP_TUNING_OPS_MODE_DISABLE;
    ae.AeFreezenEn = IMPISP_TUNING_OPS_MODE_DISABLE;
    ae.AeAGainManualEn = mode;
#if defined(PLATFORM_T23) 
    if (cfg->sensor.select & 0x2) ret += IMP_ISP_Tuning_SetAeAttr_Sec(&ae);
    if (cfg->sensor.select & 0x1) ret += IMP_ISP_Tuning_SetAeAttr(&ae); 
    return ret;
#else
    ret = IMP_ISP_Tuning_SetAeAttr(&ae);  
    return IMP_ISP_Tuning_SetAeAttr_Sec(&ae) + ret;
#endif
}

void touch_AE_AGain()
{
    int ret;
    uint32_t ret_val;
    ret = IMP_ISP_Tuning_GetMaxAgain(&ret_val);
    LOG_DEBUG("IMP_ISP_Tuning_GetMaxAgain: " << ret_val);
    ret = IMP_ISP_Tuning_GetMaxAgain_Sec(&ret_val);
    LOG_DEBUG("IMP_ISP_Tuning_GetMaxAgain_Sec: " << ret_val);

    ret = IMP_ISP_Tuning_SetMaxAgain(ret_val);
    usleep(1000);
    ret = IMP_ISP_Tuning_SetMaxAgain_Sec(ret_val);
}

void ae_status()
{
    int ret = 0;
    unsigned int ret_val, ret_val_sec;
    int ret_val1, ret_val1_sec;

    LOG_DEBUG("  ********  AE status ********");
//    ret += IMP_ISP_Tuning_GetAE_IT_MAX(&ret_val);
//    LOG_DEBUG("IMP_ISP_Tuning_GetAE_IT_MAX     " << ret_val);
//    ret += IMP_ISP_Tuning_GetAE_IT_MAX_Sec(&ret_val);
//    LOG_DEBUG("IMP_ISP_Tuning_GetAE_IT_MAX_Sec " << ret_val);
//    ret += IMP_ISP_Tuning_GetAeComp(&ret_val1);
//    LOG_DEBUG("IMP_ISP_Tuning_GetAeComp     " << ret_val1);
//    ret += IMP_ISP_Tuning_GetAeComp_Sec(&ret_val1);
//    LOG_DEBUG("IMP_ISP_Tuning_GetAeComp_Sec " << ret_val1);
    ret += IMP_ISP_Tuning_GetTotalGain(&ret_val);
//    LOG_DEBUG("IMP_ISP_Tuning_GetTotalGain     " << ret_val);
    ret += IMP_ISP_Tuning_GetTotalGain_Sec(&ret_val_sec);
    LOG_DEBUG("IMP_ISP_Tuning_GetTotalGain " << ret_val << "," << ret_val_sec);
    ret += IMP_ISP_Tuning_GetAeLuma(&ret_val1);
//    LOG_DEBUG("IMP_ISP_Tuning_GetAeLuma     " << ret_val1);
    ret += IMP_ISP_Tuning_GetAeLuma_Sec(&ret_val1_sec);
    LOG_DEBUG("IMP_ISP_Tuning_GetAeLuma " << ret_val1<< "," << ret_val1_sec);
//    IMPISPTuningOpsMode mode;
//    ret += IMP_ISP_Tuning_GetISPCustomMode(&mode);
//    LOG_DEBUG("IMP_ISP_Tuning_GetISPCustomMode     " << ((mode == IMPISP_TUNING_OPS_MODE_ENABLE)? "Enable":"Disable"));
//    ret += IMP_ISP_Tuning_GetISPCustomMode_Sec(&mode);
//    LOG_DEBUG("IMP_ISP_Tuning_GetISPCustomMode_Sec " << ((mode == IMPISP_TUNING_OPS_MODE_ENABLE)? "Enable":"Disable"));

    IMPISPEVAttr attr;
    memset(&attr, 0, sizeof(IMPISPEVAttr));
    ret += IMP_ISP_Tuning_GetEVAttr(&attr);
    LOG_DEBUG("IMP_ISP_Tuning_GetEVAttr    : ev = " << attr.ev << "(" << attr.expr_us << ")-(" << attr.ev_log2 
        << "), again = " << attr.again << " dgain = " << attr.dgain << "=>(" << attr.gain_log2 << ")");
    ret += IMP_ISP_Tuning_GetEVAttr_Sec(&attr);
    LOG_DEBUG("IMP_ISP_Tuning_GetEVAttr_Sec: ev = " << attr.ev << "(" << attr.expr_us << ")-(" << attr.ev_log2 
        << "), again = " << attr.again << " dgain = " << attr.dgain << "=>(" << attr.gain_log2 << ")");

#if 0
    IMPISPAEAttr ae;
    memset(&ae, 0, sizeof(IMPISPAEAttr));
    ret += IMP_ISP_Tuning_GetAeAttr(&ae);
    LOG_DEBUG("IMP_ISP_Tuning_GetAeAttr    : AeFreezenEn = " << (ae.AeFreezenEn? "Enable":"Disable") 
        << ", AeItManualEn = " << (ae.AeItManualEn? "Enable":"Disable") 
        << ", AeIt = " << ae.AeIt );
    LOG_DEBUG("    " 
        << " AeAGainManualEn = " << (ae.AeAGainManualEn? "Enable":"Disable") 
        << ", AeAGain = " << ae.AeAGain 
        << ", AeDGainManualEn = " << (ae.AeDGainManualEn? "Enable":"Disable") 
        << ", AeDGain = " << ae.AeDGain);
    LOG_DEBUG("    " 
        << " AeIspDGainManualEn = " << (ae.AeIspDGainManualEn? "Enable":"Disable") 
        << ", AeIspDGain = " << ae.AeIspDGain);
    ret += IMP_ISP_Tuning_GetAeAttr_Sec(&ae);
    LOG_DEBUG("IMP_ISP_Tuning_GetAeAttr_Sec: AeFreezenEn = " << (ae.AeFreezenEn? "Enable":"Disable") 
        << ", AeItManualEn = " << (ae.AeItManualEn? "Enable":"Disable") 
        << ", AeIt = " << ae.AeIt );
    LOG_DEBUG("    " 
        << " AeAGainManualEn = " << (ae.AeAGainManualEn? "Enable":"Disable") 
        << ", AeAGain = " << ae.AeAGain 
        << ", AeDGainManualEn = " << (ae.AeDGainManualEn? "Enable":"Disable") 
        << ", AeDGain = " << ae.AeDGain);
    LOG_DEBUG("    " 
        << " AeIspDGainManualEn = " << (ae.AeIspDGainManualEn? "Enable":"Disable") 
        << ", AeIspDGain = " << ae.AeIspDGain);

    IMPISPAEMin ae_min;
    memset(&ae_min, 0, sizeof(IMPISPAEMin));
    ret += IMP_ISP_Tuning_GetAeMin(&ae_min);
    LOG_DEBUG("IMP_ISP_Tuning_GetAeMin    : min_it = " << ae_min.min_it << ", min_again = " << ae_min.min_again
        << " min_it_short = " << ae_min.min_it_short << " min_again_short = " << ae_min.min_again_short);
    ret += IMP_ISP_Tuning_GetAeMin_Sec(&ae_min);
    LOG_DEBUG("IMP_ISP_Tuning_GetAeMin_Sec: min_it = " << ae_min.min_it << ", min_again = " << ae_min.min_again
        << " min_it_short = " << ae_min.min_it_short << ", min_again_short = " << ae_min.min_again_short);
#endif
    LOG_DEBUG("  *****************************");
}

int set_core_expr_mode(bool enable)
{
    IMPISPExpr expr;
    int ret;
    memset(&expr, 0, sizeof(IMPISPExpr));                    
    ret = IMP_ISP_Tuning_GetExpr(&expr);
    LOG_DEBUG("IMP_ISP_Tuning_GetExpr    : g_attr.mode = " << (expr.g_attr.mode? "Enable":"Disable") << ", g_attr.time = " << expr.g_attr.integration_time);
    // 1.1.0 link has no GetExpr_Sec
    expr.s_attr.mode = enable ? ISP_CORE_EXPR_MODE_AUTO : ISP_CORE_EXPR_MODE_MANUAL;
    expr.s_attr.unit = ISP_CORE_EXPR_UNIT_LINE;
#if defined(PLATFORM_T23) 
    if (cfg->sensor.select & 0x2) ret += IMP_ISP_Tuning_SetExpr_Sec(&expr);
    if (cfg->sensor.select & 0x1) ret += IMP_ISP_Tuning_SetExpr(&expr) + ret; 
    touch_AE_AGain();
#else
    ret = IMP_ISP_Tuning_SetExpr_Sec(&expr);
    ret = IMP_ISP_Tuning_SetExpr(&expr) + ret;
#endif
    return ret;
}

int set_again_gain(unsigned int val)
{
    ae_status();  // debug

    IMPISPAEAttr ae;
    int ret = 0;
    memset(&ae, 0, sizeof(IMPISPAEAttr));
    ret = IMP_ISP_Tuning_GetAeAttr(&ae);
    LOG_DEBUG("get again:      " << ae.AeAGain << " ae.AeAGainManualEn = " << ae.AeAGainManualEn);
    ret = IMP_ISP_Tuning_GetAeAttr_Sec(&ae);
    LOG_DEBUG("get again sec:  " << ae.AeAGain << " ae.AeAGainManualEn = " << ae.AeAGainManualEn);
    if (ret == 0)
    {           
            ae.AeAGain = val;  
    }
#if defined(PLATFORM_T23) 
    if (cfg->sensor.select & 0x2) ret += IMP_ISP_Tuning_SetAeAttr_Sec(&ae);
    if (cfg->sensor.select & 0x1) ret += IMP_ISP_Tuning_SetAeAttr(&ae); 
    return ret;
#else
    ret += IMP_ISP_Tuning_SetAeAttr_Sec(&ae);
    usleep(1000);
    ret += IMP_ISP_Tuning_SetAeAttr(&ae);
    LOG_DEBUG_OR_ERROR(ret, "set_again_gain ret = " << ret);
    return ret;
#endif
}   

int set_core_expr_time(unsigned int val)
{
    IMPISPExpr expr;
    int ret = 0;
    memset(&expr, 0, sizeof(IMPISPExpr));
    ret = IMP_ISP_Tuning_GetExpr(&expr);
    LOG_DEBUG("get expr_time:     " << expr.g_attr.integration_time << " min: " << expr.g_attr.integration_time_min
        << " max: " << expr.g_attr.integration_time_max << " us/line: " << expr.g_attr.one_line_expr_in_us);
    //ret = IMP_ISP_Tuning_GetExpr_Sec(&expr);  // 1.1.0 link has no GetExpr_Sec
    //LOG_DEBUG("get expr_time_sec:  " << expr.g_attr.integration_time << " min: " << expr.g_attr.integration_time_min
    //    << " max: " << expr.g_attr.integration_time_max << " us/line: " << expr.g_attr.one_line_expr_in_us);
    if (ret == 0)
    {           
        expr.s_attr.time = val;
        expr.s_attr.unit = ISP_CORE_EXPR_UNIT_LINE;
    }
#if defined(PLATFORM_T23) 
    if (cfg->sensor.select & 0x2) ret += IMP_ISP_Tuning_SetExpr_Sec(&expr);
    if (cfg->sensor.select & 0x1) ret += IMP_ISP_Tuning_SetExpr(&expr); 
    LOG_DEBUG_OR_ERROR(ret, "set_core_expr_time ret = " << ret);
    return ret;
#else
    ret += IMP_ISP_Tuning_SetExpr_Sec(&expr);
    ret += IMP_ISP_Tuning_SetExpr(&expr);
    return ret;
#endif
}

int set_zoom_enable(bool enable)
{
    LOG_DEBUG("image: zoom enable <== sensor.select " << cfg->get<int>("sensor.select"));
    switch (cfg->get<int>("sensor.select")) {
        case 1: // sensor 0 (cam 1)
            set_FramesourceAttr(enable, 0);
            break;
        case 2: // sensor 1 (cam 2)
            set_FramesourceAttr(enable, 1);
            break;
        case 3: // both sensor
            set_FramesourceAttr(enable, 0);
            set_FramesourceAttr(enable, 1);
            break;
        default:
            LOG_DEBUG("Invalid image config request: " << cfg->get<int>("sensor.select"));
            break;
    }

    return 0;
}

int set_FramesourceAttr(bool enable, int sensor)
{
    int ret = 0, scale = 0;
    double zoom_factor;
    int chnNr = 0;

    _stream *stream;
    _image *image;

    if (sensor == 0) {
        chnNr = 0;
        stream = &cfg->stream0;
        image = &cfg->image0;
    } else if (sensor == 1) {
        {
            chnNr = 3;
            stream = &cfg->stream1;
            image = &cfg->image1;
        }
    } else return -1;

    IMPFSChnAttr chnAttr;
    memset(&chnAttr, 0, sizeof(IMPFSChnAttr));
    IMPFSChnAttr chnAttr_r;  // readback
    memset(&chnAttr_r, 0, sizeof(IMPFSChnAttr));

    ret = IMP_FrameSource_GetChnAttr(chnNr, &chnAttr);
    LOG_DEBUG("* Read Channel " << chnNr << " configuration current setting:");
    LOG_DEBUG("  pic: " << chnAttr_r.picWidth << "x" << chnAttr_r.picHeight);
    LOG_DEBUG("  crop.enable=" << chnAttr_r.crop.enable << " crop=" << chnAttr_r.crop.width << "x" << chnAttr_r.crop.height);
    LOG_DEBUG("      Attr pos <-" << chnAttr_r.crop.left << " ^ " << chnAttr_r.crop.top);
    LOG_DEBUG("       cfg pos <-" << image->crop_left << " ^ " << image->crop_top);
    LOG_DEBUG("  scaler.enable=" << chnAttr_r.scaler.enable << " ext=" << chnAttr_r.scaler.outwidth << "x" << chnAttr_r.scaler.outheight);
    LOG_DEBUG("  fps=" << chnAttr_r.outFrmRateNum << "/" << chnAttr_r.outFrmRateDen << " nrVBs=" << chnAttr_r.nrVBs << " pixFmt=" << chnAttr.pixFmt);

    if (enable) {
//        chnAttr.crop.enable = image->crop_enable;
        chnAttr.crop.enable = 1;
        chnAttr.crop.top = image->crop_top;
        chnAttr.crop.left = image->crop_left;
        chnAttr.crop.width = stream->width;
        chnAttr.crop.height = stream->height;

//        chnAttr.scaler.enable = image->scaler_enable;
        chnAttr.scaler.enable = 1;
        chnAttr.picWidth = stream->width;
        chnAttr.picHeight = stream->height;

        double fw, fh;
        fw = (double) stream->width / image->crop_width;
        fh = (double) stream->height / image->crop_height;

        if (fw > fh) zoom_factor = fh;
        else zoom_factor = fw;

        LOG_DEBUG("-- Zoom factors:  zoom = " << zoom_factor << " (fw = " << fw << " fh = " << fh << ")");
        // zoom is limited by scale dim at 3200x1800 / 1280x720
        if (zoom_factor > 2.5) zoom_factor = 2.5;
        if (zoom_factor < 1) zoom_factor = 1; 

        chnAttr.scaler.outwidth = (int) (stream->width * zoom_factor);
        chnAttr.scaler.outheight = (int) (stream->height * zoom_factor);
        chnAttr.crop.left *= zoom_factor;
        chnAttr.crop.top *= zoom_factor;
    } else 
    {
        chnAttr.crop.enable = 0;
        chnAttr.scaler.enable = 0;
        chnAttr.crop.top = image->crop_top;
        chnAttr.crop.left = image->crop_left;
        chnAttr.crop.width = stream->width;
        chnAttr.crop.height = stream->height;
        chnAttr.scaler.outwidth = stream->width;
        chnAttr.scaler.outheight = stream->height;
        chnAttr.picWidth = stream->width;
        chnAttr.picHeight = stream->height;
    }

    chnAttr.crop.width &= ~15; // 16 pixel boundary
    chnAttr.crop.height &= ~15; // 16 pixel boundary
    chnAttr.scaler.outwidth &= ~15; // 16 pixel boundary
    chnAttr.scaler.outheight &= ~15; // 16 pixel boundary
    chnAttr.crop.left &= ~1; // 2 pixel boundary
    chnAttr.crop.top &= ~1; // 2 pixel boundary

    if ((chnAttr.crop.left + chnAttr.crop.width) > chnAttr.scaler.outwidth)
        chnAttr.crop.left = chnAttr.scaler.outwidth - chnAttr.crop.width;
    if ((chnAttr.crop.top + chnAttr.crop.height) > chnAttr.scaler.outheight)
        chnAttr.crop.top = chnAttr.scaler.outheight - chnAttr.crop.height;

    if (chnAttr.crop.left <= 0) chnAttr.crop.left = 0;
    if (chnAttr.crop.top <= 0) chnAttr.crop.top = 0;

    chnAttr.crop.left &= ~1; // 2 pixel boundary
    chnAttr.crop.top &= ~1; // 2 pixel boundary

    image->crop_left = chnAttr.crop.left / zoom_factor;
    image->crop_top = chnAttr.crop.top / zoom_factor;
    
    image->crop_width = chnAttr.crop.width;
    image->crop_height = chnAttr.crop.height;
    image->scaler_outwidth = chnAttr.scaler.outwidth;
    image->scaler_outheight = chnAttr.scaler.outheight;

    image->zoom_factor = zoom_factor;

    LOG_DEBUG("-> Set to Channel " << chnNr << " configuration (conf-attr):");
    LOG_DEBUG("  pic: " << chnAttr.picWidth << "x" << chnAttr.picHeight);
    LOG_DEBUG("  crop.enable=" << chnAttr.crop.enable << " crop=" << chnAttr.crop.width << "x" << chnAttr.crop.height);
    LOG_DEBUG("  scaled Attr pos <-" << chnAttr.crop.left << " ^ " << chnAttr.crop.top);
    LOG_DEBUG("       cfg pos <-" << image->crop_left << " ^ " << image->crop_top);
    LOG_DEBUG("  scaler.enable=" << chnAttr.scaler.enable << " ext=" << chnAttr.scaler.outwidth << "x" << chnAttr.scaler.outheight << 
                    "(x" << zoom_factor << ")");

//    LOG_DEBUG("Channel " << chnNr << " ready to change attribute for zoom.");
    // need to enable first then change zoom settings
        ret = IMP_FrameSource_DisableChn(chnNr);
    LOG_DEBUG_OR_ERROR_AND_EXIT(ret, "IMP_FrameSource_DisableChn(" << chnNr << ")");
    usleep(10000);
    // enable 
#if 1
        ret = IMP_FrameSource_SetChnAttr(chnNr, &chnAttr);
    LOG_DEBUG_OR_ERROR(ret, "* * IMP_FrameSource_SetChnAttr(" << chnNr << ", &chnAttr) - " << ret);
#endif
        ret = IMP_FrameSource_GetChnAttr(chnNr, &chnAttr_r);
    LOG_DEBUG("@ Read Channel " << chnNr << " configuration Read back:");
    LOG_DEBUG("  pic: " << chnAttr_r.picWidth << "x" << chnAttr_r.picHeight);
    LOG_DEBUG("  crop.enable=" << chnAttr_r.crop.enable << " crop=" << chnAttr_r.crop.width << "x" << chnAttr_r.crop.height);
    LOG_DEBUG("      Attr pos <-" << chnAttr_r.crop.left << " ^ " << chnAttr_r.crop.top);
    LOG_DEBUG("  scaler.enable=" << chnAttr_r.scaler.enable << " ext=" << chnAttr_r.scaler.outwidth << "x" << chnAttr_r.scaler.outheight);
    // set new zoom
    usleep(10000);
#if 1
        ret = IMP_FrameSource_SetChnAttr(chnNr, &chnAttr);
    LOG_DEBUG_OR_ERROR(ret, "*** IMP_FrameSource_SetChnAttr(" << chnNr << ", &chnAttr) - " << ret);
#endif
    ret = IMP_FrameSource_EnableChn(chnNr);
    LOG_DEBUG_OR_ERROR_AND_EXIT(ret, "IMP_FrameSource_EnableChn(" << chnNr << ")");

    usleep(10000);
        ret = IMP_FrameSource_GetChnAttr(chnNr, &chnAttr_r);
    LOG_DEBUG("@ Read Channel " << chnNr << " configuration Read back:");
    LOG_DEBUG("  pic: " << chnAttr_r.picWidth << "x" << chnAttr_r.picHeight);
    LOG_DEBUG("  crop.enable=" << chnAttr_r.crop.enable << " crop=" << chnAttr_r.crop.width << "x" << chnAttr_r.crop.height);
    LOG_DEBUG("      Attr pos <-" << chnAttr_r.crop.left << " ^ " << chnAttr_r.crop.top);
   LOG_DEBUG("  scaler.enable=" << chnAttr_r.scaler.enable << " ext=" << chnAttr_r.scaler.outwidth << "x" << chnAttr_r.scaler.outheight);

    return ret;
}

// GPIO
bool set_gpio(std::string gpio_name, bool enable)
{
#ifdef IMP_DEBUG
    LOG_DEBUG("set_gpio: " << gpio_name << " - " << (enable ? "true":"false"));
#endif
        ctrls_hal::setGPIObyName(gpio_name, enable);
    return enable;
}

bool get_gpio(std::string gpio_name)
{
    if (gpio_name == "ircut") {
        return ctrls_hal::getIRCUT();
    } else return ctrls_hal::getGPIObyName(gpio_name);
}

int set_ircut(bool enable)
{
    ctrls_hal::setIRCUT(enable);
    return 0;
}

// motor move from idx tile to center tile
int center_tile(int idx) {
    int x, y;
    x = ((idx % MAP_H_NUM) - (MAP_H_NUM/2) ) * MAP_DX;
    y = ((idx / MAP_H_NUM) - (MAP_V_NUM/2) ) * MAP_DY;

    LOG_DEBUG("track to rel: " << x << "," << y);
    char cmd;
    int value = 0;
    cmd = MOTOR_X_POS;
    value = x;
    Motor::motor_action(cmd,value);
    cmd = MOTOR_Y_POS;
    value = y;
    Motor::motor_action(cmd,value);
    cmd = MOTOR_DIR;
    value = MOTOR_REL_POS;
    Motor::motor_action(cmd,value);
    usleep(500000);
    cmd = MOTOR_STATUS;
    value = 0;
    Motor::motor_action(cmd,value);
    return 0;
}

//  ### TODO:  T23 dual sensor firmware does not responds to WB setting, it may has migrated to the T40/T41 scheme
int set_wb(int mode, unsigned short rgain, unsigned short bgain)
{
#if defined(PLATFORM_T40) || defined(PLATFORM_T41)
    // T40 uses IMPISPWBAttr with different structure
    // For now, log and return success
    LOG_DEBUG("set_wb: T40 WB API differs - needs implementation");
    return 0; // TODO: implement T40 WB using IMPISPWBAttr
#else
    int ret = 0;
    IMPISPWB wb;
    memset(&wb, 0, sizeof(IMPISPWB));

#if 0
    IMPISPAWBCluster awb_cluster;
    memset(&awb_cluster, 0, sizeof(IMPISPAWBCluster));
    ret = IMP_ISP_Tuning_GetAwbClust(&awb_cluster);
    LOG_DEBUG_OR_ERROR(ret,"IMP_ISP_Tuning_GetAwbClust enable = " << (awb_cluster.ClusterEn? "Enable":"Disable") << " ret = " << ret);
    awb_cluster.ClusterEn = IMPISP_TUNING_OPS_MODE_DISABLE;
    ret = IMP_ISP_Tuning_GetAwbClust(&awb_cluster);
    ret = IMP_ISP_Tuning_GetWB_Statis(&wb);
    LOG_DEBUG_OR_ERROR(ret,"IMP_ISP_Tuning_GetWB_Statis mode = " << wb.mode << " rgain = " << wb.rgain << " bgain = " << wb.bgain << " ret = " << ret);
    ret = IMP_ISP_Tuning_GetWB_Statis_Sec(&wb);
    LOG_DEBUG_OR_ERROR(ret,"IMP_ISP_Tuning_GetWB_Statis_Sec mode = " << wb.mode << " rgain = " << wb.rgain << " bgain = " << wb.bgain << " ret = " << ret);
    ret = IMP_ISP_Tuning_GetWB_GOL_Statis(&wb);
    LOG_DEBUG_OR_ERROR(ret,"IMP_ISP_Tuning_GetWB_GOL_Statis mode = " << wb.mode << " rgain = " << wb.rgain << " bgain = " << wb.bgain << " ret = " << ret);
    ret = IMP_ISP_Tuning_GetWB_GOL_Statis_Sec(&wb);
    LOG_DEBUG_OR_ERROR(ret,"IMP_ISP_Tuning_GetWB_GOL_Statis_Sec mode = " << wb.mode << " rgain = " << wb.rgain << " bgain = " << wb.bgain << " ret = " << ret);
#endif

    ret = IMP_ISP_Tuning_GetWB(&wb);
    LOG_DEBUG_OR_ERROR(ret,"IMP_ISP_Tuning_GetWB mode = " << wb.mode << " rgain = " << wb.rgain << " bgain = " << wb.bgain << " ret = " << ret);
    ret = IMP_ISP_Tuning_GetWB_Sec(&wb);
    LOG_DEBUG_OR_ERROR(ret,"IMP_ISP_Tuning_GetWB_Sec mode = " << wb.mode << " rgain = " << wb.rgain << " bgain = " << wb.bgain << " ret = " << ret);

    wb.mode = (isp_core_wb_mode)mode;
    wb.rgain = rgain;
    wb.bgain = bgain;

#if defined(PLATFORM_T23)
    if (cfg->sensor.select & 0x2) {
        ret = IMP_ISP_Tuning_SetWB_Sec(&wb);
        LOG_DEBUG_OR_ERROR(ret,"IMP_ISP_Tuning_SetWB_Sec mode = " << wb.mode << " rgain = " << wb.rgain << " bgain = " << wb.bgain << " ret = " << ret);
    }
    if (cfg->sensor.select & 0x1) {
        ret += IMP_ISP_Tuning_SetWB(&wb); 
        LOG_DEBUG_OR_ERROR(ret,"IMP_ISP_Tuning_SetWB mode = " << wb.mode << " rgain = " << wb.rgain << " bgain = " << wb.bgain << " ret = " << ret);
    }
#else 
    ret = IMP_ISP_Tuning_SetWB_Sec(&wb);
    ret = IMP_ISP_Tuning_SetWB(&wb);
    LOG_DEBUG_OR_ERROR(ret,"IMP_ISP_Tuning_SetWB_Sec mode = " << wb.mode << " rgain = " << wb.rgain << " bgain = " << wb.bgain << " ret = " << ret);
#endif
    return ret;
#endif
}

int add_sensor(IMPSensorInfo* sinfo)
{
#if defined(PLATFORM_T40) || defined(PLATFORM_T41)
    return IMP_ISP_AddSensor(IMPVI_MAIN, sinfo);
#else
    return IMP_ISP_AddSensor(sinfo);
#endif
}

int enable_sensor(IMPSensorInfo* sinfo)
{
#if defined(PLATFORM_T40) || defined(PLATFORM_T41)
    return IMP_ISP_EnableSensor(IMPVI_MAIN, sinfo);
#else
    (void)sinfo; // Unused on older platforms
    return IMP_ISP_EnableSensor();
#endif
}

int disable_sensor()
{
#if defined(PLATFORM_T40) || defined(PLATFORM_T41)
    return IMP_ISP_DisableSensor(IMPVI_MAIN);
#else
    return IMP_ISP_DisableSensor();
#endif
}

int del_sensor(IMPSensorInfo* sinfo)
{
#if defined(PLATFORM_T40) || defined(PLATFORM_T41)
    return IMP_ISP_DelSensor(IMPVI_MAIN, sinfo);
#else
    return IMP_ISP_DelSensor(sinfo);
#endif
}

} // namespace isp

// ============================================================================
// Platform Capabilities Implementation
// ============================================================================



// ============================================================================
// Video Encoder Stream HAL Implementation
// ============================================================================

namespace encoder {

uint8_t* get_pack_data_start(const IMPEncoderStream& stream, int pack_index)
{
#if defined(PLATFORM_T31) || defined(PLATFORM_T40) || defined(PLATFORM_T41) || defined(PLATFORM_C100)
    return (uint8_t*)stream.virAddr + stream.pack[pack_index].offset;
#else
    return (uint8_t*)stream.pack[pack_index].virAddr;
#endif
}

uint32_t get_pack_data_length(const IMPEncoderStream& stream, int pack_index)
{
    return stream.pack[pack_index].length;
}

int get_h264_nal_type(const IMPEncoderPack& pack)
{
#if defined(PLATFORM_T31) || defined(PLATFORM_T40) || defined(PLATFORM_T41) || defined(PLATFORM_C100)
    return pack.nalType.h264NalType;
#else
    return pack.dataType.h264Type;
#endif
}

int get_h265_nal_type(const IMPEncoderPack& pack)
{
#if defined(PLATFORM_T31) || defined(PLATFORM_T40) || defined(PLATFORM_T41) || defined(PLATFORM_C100)
    return pack.nalType.h265NalType;
#elif defined(PLATFORM_T30)
    return pack.dataType.h265Type;
#else
    // H.265 not supported on T10/T20/T21/T23
    return -1;
#endif
}

} // namespace encoder
} // namespace hal

void init_encoder_channel_attr(IMPEncoderCHNAttr& chnAttr, const char* format, int width, int height)
{
#if defined(PLATFORM_T31) || defined(PLATFORM_C100) || defined(PLATFORM_T40) || defined(PLATFORM_T41)
    // T31/C100/T40/T41 use newer API - initialization handled by caller
    (void)chnAttr;
    (void)format;
    (void)width;
    (void)height;
#else
    // Older platforms need explicit initialization
    if (strcmp(format, "JPEG") == 0)
    {
        IMPEncoderAttr *encAttr = &chnAttr.encAttr;
        encAttr->enType = PT_JPEG;
        encAttr->bufSize = 0;
        encAttr->profile = 2;
        encAttr->picWidth = width;
        encAttr->picHeight = height;
    }
    else if (strcmp(format, "H264") == 0)
    {
        chnAttr.encAttr.enType = PT_H264;
    }
#if defined(PLATFORM_T30)
    else if (strcmp(format, "H265") == 0)
    {
        chnAttr.encAttr.enType = PT_H265;
    }
#endif
#endif
}

int get_encoder_rc_mode_smart()
{
#if defined(PLATFORM_T31) || defined(PLATFORM_C100) || defined(PLATFORM_T40) || defined(PLATFORM_T41)
    return IMP_ENC_RC_MODE_CAPPED_QUALITY;
#else
    return ENC_RC_MODE_SMART;
#endif
}

int get_encoder_profile_high(const char* format)
{
#if defined(PLATFORM_T31) || defined(PLATFORM_C100) || defined(PLATFORM_T40) || defined(PLATFORM_T41)
    if (strcmp(format, "H265") == 0)
    {
        return IMP_ENC_PROFILE_HEVC_MAIN;
    }
    return IMP_ENC_PROFILE_AVC_HIGH;
#else
    // Older platforms use different profile values
    return 2; // High profile
#endif
}

int get_encoder_type(const char* format)
{
    if (strcmp(format, "JPEG") == 0)
    {
        return PT_JPEG;
    }
    else if (strcmp(format, "H264") == 0)
    {
        return PT_H264;
    }
#if defined(PLATFORM_T30) || defined(PLATFORM_T31) || defined(PLATFORM_C100) || defined(PLATFORM_T40) || defined(PLATFORM_T41)
    else if (strcmp(format, "H265") == 0)
    {
        return PT_H265;
    }
#endif
    return PT_H264; // Default
}

bool supports_jpeg_quality_table()
{
#if defined(PLATFORM_T31) || defined(PLATFORM_C100) || defined(PLATFORM_T40) || defined(PLATFORM_T41)
    return hal::caps().has_jpeg_set_qtable;
#else
    return !hal::caps().has_jpeg_set_qtable;
#endif
}


