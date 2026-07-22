#!/bin/haserl --upload-limit=1024 --upload-dir=/tmp
<%in _common.cgi %>
<%
page_title="Streamer/OSD"
which motors > /dev/null && has_motors="true"

if [ "restart" = "$GET_do" ]; then
	service restart prudynt >/dev/null
	sleep 3
	redirect_to $SCRIPT_NAME
fi

OSD_FONT_PATH="/usr/share/fonts"
if [ "POST" = "$REQUEST_METHOD" ]; then
	error=""
	if [ -z "$HASERL_fontfile_path" ]; then
		set_error_flag "File upload failed. No font selected?"
	elif [ $(stat -c%s $HASERL_fontfile_path) -eq 0 ]; then
		set_error_flag "File upload failed. Empty file?"
	else
		mv "$HASERL_fontfile_path" "$OSD_FONT_PATH/uploaded.ttf"
	fi
	redirect_to $SCRIPT_NAME
fi

AUDIO_FORMATS="AAC G711A G711U G726 OPUS PCM"

AUDIO_SAMPLING="8000,12000,16000,24000,48000"

AUDIO_BITRATES=$(seq 6 2 256)

FONTS=$(ls -1 $OSD_FONT_PATH)

ts=$(date +%s)

if [ "t30" = "$soc_family" ] || [ "t31" = "$soc_family" -a "t31lc" != "$soc_model" ]; then
	FORMATS="H264,H265"
else
	FORMATS="H264"
fi

modes="CBR VBR FIXQP"
case "$soc_family" in
	t31) modes="$modes CAPPED_VBR CAPPED_QUALITY" ;;
	  *) modes="$modes SMART" ;;
esac

prudynt_config=/etc/prudynt.json

rtsp_username=$(awk -F: '/Streaming Service/{print $1}' /etc/passwd)
default_for rtsp_username $(jct $prudynt_config get rtsp.username)
default_for rtsp_password $(jct $prudynt_config get rtsp.password)
default_for rtsp_password "thingino"
%>
<%in _header.cgi %>

<nav class="navbar navbar-expand-lg mb-4 p-1">
<button class="navbar-toggler" type="button" data-bs-toggle="collapse" data-bs-target="#nbStreamer" aria-controls="nbStreamer" aria-label="Toggle navigation"><span class="navbar-toggler-icon"></span></button>
<div class="collapse navbar-collapse" id="nbStreamer">
<ul class="navbar-nav nav-underline" role="tablist">
<li class="nav-item"><a href="#" data-bs-toggle="tab" data-bs-target="#tab1-pane" class="nav-link active" aria-current="page">Common</a></li>
<li class="nav-item"><a href="#" data-bs-toggle="tab" data-bs-target="#tab2-pane" class="nav-link">Stream 0</a></li>
<li class="nav-item"><a href="#" data-bs-toggle="tab" data-bs-target="#tab3-pane" class="nav-link">Stream 1</a></li>
<li class="nav-item"><a href="#" data-bs-toggle="tab" data-bs-target="#tab4-pane" class="nav-link">Audio</a></li>
</ul>
</div>
</nav>

<div class="row row-cols-2 row-cols-lg-2">
<div class="col mb-3">

<div class="btn-toolbar" role="toolbar">
<div class="btn-group mb-3" role="group">
<button type="button" class="btn btn-secondary" data-bs-toggle="modal" data-bs-target="#mdPreview" title="Full-screen"><img src="/a/zoom.svg" alt="Zoom" class="img-fluid icon-sm"></button>
<input type="radio" class="btn-check" name="preview_source" id="preview_source_0" value="0" checked>
<label class="btn btn-outline-primary" for="preview_source_0">Camera 1</label>
<input type="radio" class="btn-check" name="preview_source" id="preview_source_1" value="1">
<label class="btn btn-outline-primary" for="preview_source_1">Camera 2</label>
</div>
</div>

<style>
.canvas-container {
    position: relative;
    display: inline-block;
}

.overlayLayer{
	display: none;
	position: absolute;
	pointerEvents: none;
	border:2px solid #ff6666;
	backgroundColor:transparent;
    	top: 0;
    	left: 0;
    	width: 100%; 
    	height: 100%;
}

.imgCanvas {
    position: absolute;
    top: 0;
    left: 0;
    width: 100%; 
    height: 100%;
    border: 1px solid black; /* Optional: adds a border to the canvas area */
}
</style>

<div id="zoom-map" class="position-relative ">
<div id="zoom-map" class="canvas-container" style="border: 2px solid #889988;">
	<img id="preview" src="/x/ch0.mjpg" class="img-fluid" alt="Image: Stream 0 Preview">
    	<canvas class="imgCanvas" id="imageCanvas"></canvas>
	<div id="boxOnImage" class="overlayLayer"></div>
</div>
</div>

<div id="Zoom-status">
   Zoom Box   X: <span id="image_crop_left"></span>
   , Y: <span id="image_crop_top"></span>
   - ( <span id="image_crop_width"></span>
   x <span id="image_crop_height"></span>
   ) ==> [ <span id="image_scaler_outwidth"></span>
   x <span id="image_scaler_outheight"></span>
   ]
</div>

</div>

<div class="col mb-3">
<div class="d-flex flex-wrap align-content-around gap-1">
<a class="btn btn-warning" href="<%= $SCRIPT_NAME %>?do=restart">Restart streamer</a>
<button type="button" class="btn btn-secondary" id="save-prudynt-config">Save config</button>
<a class="btn btn-secondary" href="tool-file-manager.cgi?dl=/etc/prudynt.json">Download config</a>
</div>

<p class="small"></p>
<div id="zoom-map1" class="position-relative ">
<div id="zoom-map1" class="canvas-container" style="border: 2px solid #889988;">
	<img id="preview1" src="/x/ch1.mjpg" class="img-fluid" alt="Image: Stream 1 Preview">
    	<canvas class="imgCanvas" id="imageCanvas1"></canvas>
	<div id="boxOnImage1" class="overlayLayer"></div>
</div>
</div>

</div>
</div>
    <style>
        .box {
            width: 500px;
            height: 170px;
            background-color: #303030; 
            border: 2px solid #505050;
            padding: 10px;
            margin: 10px;
        }
        .box1 {
            width: 650px;
            height: 515px;
            background-color: #303030; 
            border: 2px solid #505050;
            padding: 10px;
            margin: 10px;
        }
    </style>
<div class="row row-cols-2 row-cols-m-2">
</div>
<div class="col mb-3">
<p class="small">Double-click on a range element will restore its default value.</p>

<div class="tab-content" id="streamer-tabs">
<div class="tab-pane fade show active" id="tab1-pane" role="tabpanel" aria-labelledby="tab1">
<div class="row row-cols-2 row-cols-lg-2">
<div class="row g-2">

<% if [ "true" = "$has_motors" ]; then %>
<p class="medium">----- Pan and Tilt Control -----</p>
<%in _motors.cgi %>
<% fi %>

<div class="box">
<div class="row g-2">
<div class="col-6"><% field_switch "image_alt_sensor" "Dual Camera" %></div>
<div class=col-6><% field_switch "image_zoom_enable" "Zoom Enabled" %></div>
<p class="large">----- Light Control -----</p>
<div class="col-6"><% field_switch "gpio_white" "White Light" %></div>
<div class="col-6"><% field_switch "gpio_ir850" "IR-850 Light" %></div>
<div class="col-6"><% field_switch "gpio_ir940" "IR-940 Light" %></div>
<div class="col-6"><% field_switch "gpio_ircut" "IR Cut" %></div>
</div>
</div>

</div>

<div class="col mb3">
<div class="row g-2">
<div class="col-4">
<div class="mb-2 select" id="sensor_select_wrap">
<label for="sensor_select" class="form-label">Camera Setting Select</label>
<select class="form-select" id="sensor_select" name="sensor_select">
<option value="3">Both 1/2</option>
<option value="1">Camera 1</option>
<option value="2">Camera 2</option>
</select>
</div>
</div>
<div class="col-6"> <p class="large">----- Image Control -----</p> </div>
</div>
<div class="box1">
<div class="row g-2">
<div class="col-3">
<div class="mb-2 select" id="image_core_wb_mode_wrap">
<label for="image_core_wb_mode" class="form-label">White Balance </label>
<select class="form-select" id="image_core_wb_mode" name="image_core_wb_mode">
<option value="0">AUTO</option>
<option value="1">MANUAL</option>
<option value="2">DAY LIGHT</option>
<option value="3">CLOUDY</option>
<option value="4">INCANDESCENT</option>
<option value="5">FLOURESCENT</option>
<option value="6">TWILIGHT</option>
<option value="7">SHADE</option>
<option value="8">WARM FLOURESCENT</option>
<option value="9">CUSTOM</option>
</select>
</div>
</div>
<div class="col-3"><% field_range "image_wb_bgain" "Blue channel gain" "0,1024,1" %></div>
<div class="col-3"><% field_range "image_wb_rgain" "Red channel gain" "0,1024,1" %></div>
<div class="col-3"><% field_range "image_ae_compensation" "<abbr title=\"Automatic Exposure\">AE</abbr> compensation" "0,255,1" %></div>
<div class="col-2"><% field_switch "image_core_expr_mode" "Auto Exp" %></div>
<div class="col-4"><% field_range "image_core_expr_time" "Time - msec ( 0 - 2234)" "0,2234,1" %></div>
<div class="col-2"><% field_switch "image_again_mode" "Manual Gain" %></div>
<div class="col-4"><% field_range "image_again_gain" "     Analog Gain (0-100,000)" "0,100000,100" %></div>
<div class="col-4"><% field_switch "image_running_mode" "B/W mode" %></div>
<div class="col-4"><% field_switch "image_hflip" "Flip horizontally" %></div>
<div class="col-4"><% field_switch "image_vflip" "Flip vertically" %></div>
</div>
<div class="row row-cols-1 row-cols-lg-4 g-2">
<div class="col"><% field_range "image_brightness" "Brightness" "0,255,1" %></div>
<div class="col"><% field_range "image_contrast" "Contrast" "0,255,1" %></div>
<div class="col"><% field_range "image_saturation" "Saturation" "0,255,1" %></div>
<div class="col"><% field_range "image_hue" "Hue" "0,255,1" %></div>
<div class="col"><% field_range "image_sharpness" "Sharpness" "0,255,1" %></div>
<div class="col"><% field_range "image_defog_strength" "Defog" "0,255,1" %></div>
<div class="col"><% field_range "image_sinter_strength" "Sinter" "0,255,1" %></div>
<div class="col"><% field_range "image_temper_strength" "Temper" "0,255,1" %></div>
<div class="col"><% field_range "image_dpc_strength" "<abbr title=\"Dead Pixel Compensation\">DPC</abbr> strength" "0,255,1" %></div>
<div class="col"><% field_range "image_drc_strength" "<abbr title=\"Dynamic Range Compression\">DRC</abbr> strength" "0,255,1" %></div>
<div class="col"><% field_range "image_max_again" "Max. analog gain" "0,160,1" %></div>
<div class="col"><% field_range "image_max_dgain" "Max. digital gain" "0,160,1" %></div>
<div class="col"><% field_range "image_backlight_compensation" "Backlight comp." "0,10,1" %></div>
<div class="col"><% field_range "image_highlight_depress" "Highlight depress" "0,255,1" %></div>
<div class="col"><% field_range "image_anti_flicker" "Anti-flicker" "0,2,1" %></div>
</div>
</div>

</div>
</div>
</div>

<% for i in 0 1; do domain="stream$i" %>
<div class="tab-pane fade" id="tab<%= $((i+2)) %>-pane" role="tabpanel" aria-labelledby="tab<%= $((i+2)) %>">
<div class="row row-cols-2 row-cols-lg-2">
<div g-2>
<% field_switch "${domain}_enabled" "Stream Enable" %>
<div class="row g-2">
<div class="col-3"><% field_text "${domain}_width" "Width" %></div>
<div class="col-3"><% field_text "${domain}_height" "Height" %></div>
<div class="col-6"><% field_range "${domain}_fps" "FPS" "$sensor_min_fps,$sensor_max_fps,1" %></div>
</div>
<div class="row g-2">
<div class="col-3"><% field_select "${domain}_format" "Format" $FORMATS %></div>
<div class="col-3"><% field_text "${domain}_bitrate" "Bitrate" %></div>
<div class="col-6"><% field_select "${domain}_mode" "Mode" "$modes" %></div>
</div>
<div class="row g-2">
<div class="col"><% field_text "${domain}_buffers" "Buffers" %></div>
<div class="col"><% field_text "${domain}_gop" "GOP" %></div>
<div class="col"><% field_text "${domain}_max_gop" "Max. GOP" %></div>
<div class="col"><% field_text "${domain}_profile" "Profile" %></div>
</div>
<div class="row g-2">
<div class="col-9"><% field_text "${domain}_rtsp_endpoint" "Endpoint" %></div>
</div>
<% field_switch "${domain}_audio_enabled" "Audio in the stream" %>
<div class="alert alert-dark">RTSP stream URL:
<div class="cb">rtsp://<%= $rtsp_username %>:<%= $rtsp_password %>@<%= $network_address %>/ch<%= $i %></div>
</div>
</div>

<div class="col mb3">
<% field_switch "osd${i}_enabled" "OSD enable" %>
<div class="row g-1">
<div class="col-6">
<label class="form-label" for="osd<%= $i %>_fontname">Font</label>
<div class="input-group mb-3">
<button class="btn btn-secondary" type="button" data-bs-toggle="modal" data-bs-target="#mdFont" title="Upload a font">
<img src="/a/upload.svg" alt="Upload" class="img-fluid icon-sm">
</button>
<select class="form-select" id="osd<%= $i %>_fontname">
<% for f in $FONTS; do %><option><%= $f %></option><% done %></select></div></div>
<div class="col-3"><% field_range "osd${i}_fontsize" "Font size" "10,80,1" %></div>
<div class="col-3"><% field_range "osd${i}_fontstrokesize" "Shadow size" "0,5,1" %></div>
</div>
<div class="accordion" id="#osd${i}Elements">
<div class="accordion-item">
<div class="accordion-header" id="headingLogo">
<button class="accordion-button collapsed" type="button" data-bs-toggle="collapse" data-bs-target="#collapseLogo" aria-controls="collapseLogo" aria-expanded="false">Logo</button>
</div>
<div id="collapseLogo" class="accordion-collapse collapse" aria-labelledby="headingLogo" data-bs-parent="#osd${i}Elements">
<div class="accordion-body">
<% field_switch "osd${i}_logo_enabled" "Display" %>
</div>
</div>
</div>
<div class="accordion-item">
<div class="accordion-header" id="headingTime">
<button class="accordion-button collapsed" type="button" data-bs-toggle="collapse" data-bs-target="#collapseTime" aria-controls="collapseTime" aria-expanded="false">Time</button>
</div>
<div id="collapseTime" class="accordion-collapse collapse" aria-labelledby="headingTime" data-bs-parent="#osd${i}Elements">
<div class="accordion-body">
<% field_switch "osd${i}_time_enabled" "Display" %>
<div class="row g-1">
<div class="col col-4"><% field_color "osd${i}_time_fontcolor" "Color" %></div>
<div class="col col-4"><% field_color "osd${i}_time_fontstrokecolor" "Shadow" %></div>
<div class="col col-4"><% field_text "osd${i}_time_format" "Format" "$STR_SUPPORTS_STRFTIME" %></div>
</div>
</div>
</div>
</div>
<div class="accordion-item">
<div class="accordion-header" id="headingUptime">
<button class="accordion-button collapsed" type="button" data-bs-toggle="collapse" data-bs-target="#collapseUptime" aria-controls="collapseUptime" aria-expanded="false">Uptime</button>
</div>
<div id="collapseUptime" class="accordion-collapse collapse" aria-labelledby="headingUptime" data-bs-parent="#osd${i}Elements">
<div class="accordion-body">
<% field_switch "osd${i}_uptime_enabled" "Display" %>
<div class="row g-1">
<div class="col col-4"><% field_color "osd${i}_uptime_fontcolor" "Color" %></div>
<div class="col col-4"><% field_color "osd${i}_uptime_fontstrokecolor" "Shadow" %></div>
</div>
</div>
</div>
</div>
<div class="accordion-item">
<div class="accordion-header" id="headingUsertext">
<button class="accordion-button collapsed" type="button" data-bs-toggle="collapse" data-bs-target="#collapseUsertext" aria-controls="collapseUsertext" aria-expanded="false">User Text</button>
</div>
<div id="collapseUsertext" class="accordion-collapse collapse" aria-labelledby="headingUsertext" data-bs-parent="#osd${i}Elements">
<div class="accordion-body">
<% field_switch "osd${i}_usertext_enabled" "Display" %>
<div class="row g-1">
<div class="col col-4"><% field_color "osd${i}_usertext_fontcolor" "Color" %></div>
<div class="col col-4"><% field_color "osd${i}_usertext_fontstrokecolor" "Shadow" %></div>
<div class="col col-4"><% field_text "osd${i}_usertext_format" "Format" "$STR_usertext_FMT" %></div>
</div>
</div>
</div>
</div>
</div>
</div>
</div>
</div>
<% done %>


<div class="tab-pane fade" id="tab4-pane" role="tabpanel" aria-labelledby="tab4">
<% field_switch "audio_input_enabled" "Input Enabled" %>
<div class="row g-2">
<div class="col"><% field_select "audio_input_format" "Codec" "$AUDIO_FORMATS" %></div>
<div class="col"><% field_select "audio_input_sample_rate" "Sampling, Hz" "$AUDIO_SAMPLING" %></div>
<div class="col"><% field_select "audio_input_bitrate" "Bitrate, kbps" "$AUDIO_BITRATES" %></div>
</div>
<div class="row g-2">
<div class="col"><% field_range "audio_input_vol" "Input volume" "-30,120,1" %></div>
<div class="col"><% field_range "audio_input_gain" "Input gain" "0,31,1" %></div>
<div class="col"><% field_range "audio_input_alc_gain" "<abbr title=\"Automatic Level Control\">ALC</abbr> gain" "0,7,1" %></div>
</div>
<br>
<div class="row g-2">
<div class="col"><% field_switch "audio_input_agc_enabled" "<abbr title=\"Automatic gain control\">AGC</abbr> Enabled" %></div>
<div class="col"><% field_switch "audio_input_high_pass_filter" "High pass filter" %></div>
</div>
<div class="row g-2">
<div class="col"><% field_range "audio_input_noise_suppression" "Noise suppression level" "0,3,1" %></div>
</div>
<div class="row g-2">
<div class="col"><% field_range "audio_input_agc_compression_gain_db" "Compression gain, dB" "0,90,1" %></div>
<div class="col"><% field_range "audio_input_agc_target_level_dbfs" "Target level, dBfs" "0,31,1" %></div>
</div>

<% field_switch "audio_output_enabled" "Output Enabled" %>

<button type="button" class="btn btn-secondary" id="restart-audio">Restart Audio</button>
</div>

</div>
</div>

<div class="alert alert-danger mt-5">
<p>If you made changes that messed up the settings, you can restore the original configuration and start from scratch.</p>
<% button_restore_from_rom "/etc/prudynt.json" %>
</div>

<div class="modal fade" id="mdFont" tabindex="-1" aria-labelledby="mdlFont" aria-hidden="true">
<div class="modal-dialog"><div class="modal-content"><div class="modal-header">
<h1 class="modal-title fs-4" id="mdlFont">Upload font file</h1>
<button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
</div><div class="modal-body text-center">
<form action="<%= $SCRIPT_NAME %>" method="post" class="mb-4" enctype="multipart/form-data">
<% field_file "fontfile" "Upload a TTF file" %>
<% button_submit %></form>
</div></div></div></div>

<%in _preview.cgi %>

<script>
const soc = "<%= $soc_family %>";

const endpoint = '/x/json-prudynt.cgi';

function ts() {
	return Math.floor(Date.now());
}

if (soc == "t31") {
	DEFAULT_ENC_MODE_0 = "FIXQP"
	DEFAULT_ENC_MODE_1 = "CAPPED_QUALITY"
	DEFAULT_BUFFERS_0 = 4
	DEFAULT_BUFFERS_1 = 2
	DEFAULT_SINTER = 128
	DEFAULT_TEMPER = 128
} else if (soc == "t23") {
	DEFAULT_ENC_MODE_0 = "SMART"
	DEFAULT_ENC_MODE_1 = "SMART"
	DEFAULT_BUFFERS_0 = 2
	DEFAULT_BUFFERS_1 = 2
	DEFAULT_SINTER = 128
	DEFAULT_TEMPER = 128
} else {
	DEFAULT_ENC_MODE_0 = "SMART"
	DEFAULT_ENC_MODE_1 = "SMART"
	DEFAULT_BUFFERS_0 = 2
	DEFAULT_BUFFERS_1 = 2
	DEFAULT_SINTER = 50
	DEFAULT_TEMPER = 50
}

DEFAULT_VALUES = {
	'audio_input_agc_compression_gain_db': 0,
	'audio_input_agc_target_level_dbfs': 10,
	'audio_input_alc_gain': 0,
	'audio_input_gain': 25,
	'audio_input_noise_suppression': 0,
	'audio_input_sample_rate':  16000,
	'audio_input_vol': 80,
	'image_ae_compensation': 128,
	'image_anti_flicker': 2,
	'image_backlight_compensation': 0,
	'image_brightness': 128,
	'image_contrast': 128,
	'image_core_wb_mode': 0,
	'image_defog_strength': 128,
	'image_dpc_strength': 128,
	'image_drc_strength': 128,
	'image_highlight_depress': 0,
	'image_hue': 128,
	'image_max_again': 160,
	'image_max_dgain': 80,
	'image_running_mode': 0,
	'image_saturation': 128,
	'image_sharpness': 128,
	'image_sinter_strength': DEFAULT_SINTER,
	'image_temper_strength': DEFAULT_TEMPER,
	'image_wb_bgain': 0,
	'image_wb_rgain': 0,
	'image_hflip': false,
	'image_vflip': false,
	'image_alt_sensor': false,
	'image_alt_speed': 2000,
	'image_again_mode': false,
	'image_again_gain': 15000,
	'image_core_expr_mode': true,
	'image_core_expr_time': 300,

	'stream0_fps': 25,
	'stream1_fps': 25,
}

// audio
const audio_params = [
	'input_agc_compression_gain_db', 'input_agc_enabled',
	'input_agc_target_level_dbfs', 'input_alc_gain', 'input_bitrate',
	'input_enabled', 'input_format', 'input_gain', 'input_high_pass_filter',
	'input_noise_suppression', 'input_sample_rate', 'input_vol',
	'output_enabled'
];

// image
const image_params = [
	'ae_compensation', 'anti_flicker', 'backlight_compensation',
	'brightness', 'contrast', 'core_wb_mode', 'defog_strength',
	'dpc_strength', 'drc_strength', 'hflip', 'highlight_depress', 'hue',
	'max_again', 'max_dgain', 'running_mode', 'saturation', 'sharpness',
	'alt_sensor', 'alt_speed','again_mode', 'again_gain', 'core_expr_mode', 'core_expr_time',
	'sinter_strength', 'temper_strength', 'vflip', 'wb_bgain', 'wb_rgain',
	'crop_left', 'crop_top', 'crop_width', 'crop_height',
	'scaler_outwidth', 'scaler_outheight', 'zoom_enable'
];

const zoom_params = ['crop_left', 'crop_top', 'crop_width', 'crop_height',
	'scaler_outwidth', 'scaler_outheight', 'zoom_enable'
];

// gpio
const gpio_params = ['ir850', 'white', 'ircut', 'ir940'];

// sensor
const sensor_params = ['select'];

// motion
const motion_params = [
	'debounce_time', 'post_time', 'ivs_polling_timeout', 'cooldown_time',
	'init_time', 'min_time', 'sensitivity', 'skip_frame_count',
	'frame_width', 'frame_height', 'monitor_stream', 'roi_0_x', 'roi_0_y',
	'roi_1_x', 'roi_1_y', 'roi_count'
];

// stream [0, 1]
const stream_params = [
	'audio_enabled', 'bitrate', 'buffers', 'enabled', 'format', 'fps',
	'gop', 'height', 'max_gop', 'mode', 'profile', 'rtsp_endpoint', 'width'
];

// stream 2/3
const stream2_params = ['jpeg_channel'];
const stream3_params = ['jpeg_channel'];

// OSD
const osd_params = [
	'enabled', 'font_path', 'font_size', 'font_stroke_size', 'logo_enabled',
	'time_enabled', 'time_font_color', 'time_font_stroke_color',
	'time_format', 'uptime_enabled', 'uptime_font_color',
	'uptime_font_stroke_color', 'usertext_enabled', 'usertext_font_color',
	'usertext_stroke_color', 'usertext_format'
];

let sts;

function rgba2color(hex8) {
	return hex8.substring(0, 7);
}

function rgba2alpha(hex8) {
	const alphaHex = hex8.substring(7, 9);
	const alpha = parseInt(alphaHex, 16);
	return alpha;
}

async function handleMessage(msg) {
	if (msg.action && msg.action.capture == 'initiated') return;

	let data;

	// Video
	for (const i in [0, 1]) {
		const domain = `stream${i}`;
		data = msg[domain];
		if (data) {
			stream_params.forEach((x) => {
				if (typeof(data[x]) !== 'undefined')
					setValue(data, domain, x);
			});
			if (data.osd) {
				if (data.osd.enabled) {
					$(`#osd${i}_enabled`).checked = data.osd.enabled;
					toggleWrappers(i);
				}
				if (data.osd.font_path)
					$(`#osd${i}_fontname`).value = data.osd.font_path.split('/').reverse()[0];
				if (data.osd.font_size) {
					$(`#osd${i}_fontsize-show`).textContent = data.osd.font_size;
					$(`#osd${i}_fontsize`).value = data.osd.font_size;
				}
				if (data.osd.font_stroke_size) {
					$(`#osd${i}_fontstrokesize-show`).textContent = data.osd.font_stroke_size;
					$(`#osd${i}_fontstrokesize`).value = data.osd.font_stroke_size;
				}

				if (data.osd.logo_enabled)
					$(`#osd${i}_logo_enabled`).checked = data.osd.logo_enabled;

				if (data.osd.time_enabled)
					$(`#osd${i}_time_enabled`).checked = data.osd.time_enabled;
				if (data.osd.time_format)
					$(`#osd${i}_time_format`).value = data.osd.time_format;

				if (data.osd.time_font_color) {
					$(`#osd${i}_time_fontcolor`).value = rgba2color(data.osd.time_font_color);
					$(`#osd${i}_time_fontcolor-alpha`).value = rgba2alpha(data.osd.time_font_color);
				}

				if (data.osd.time_font_stroke_color) {
					$(`#osd${i}_time_fontstrokecolor`).value = rgba2color(data.osd.time_font_stroke_color);
				}

				if (data.osd.uptime_enabled)
					$(`#osd${i}_uptime_enabled`).checked = data.osd.uptime_enabled;

				if (data.osd.uptime_font_color) {
					$(`#osd${i}_uptime_fontcolor`).value = rgba2color(data.osd.uptime_font_color);
					$(`#osd${i}_uptime_fontcolor-alpha`).value = rgba2alpha(data.osd.uptime_font_color);
				}

				if (data.osd.uptime_font_stroke_color) {
					$(`#osd${i}_uptime_fontstrokecolor`).value = rgba2color(data.osd.uptime_font_stroke_color);
				}

				if (data.osd.usertext_enabled)
					$(`#osd${i}_usertext_enabled`).checked = data.osd.usertext_enabled;

				if (data.osd.usertext_font_color) {
					$(`#osd${i}_usertext_fontcolor`).value = rgba2color(data.osd.usertext_font_color);
					$(`#osd${i}_usertext_fontcolor-alpha`).value = rgba2alpha(data.osd.usertext_font_color);
				}

				if (data.osd.usertext_font_stroke_color) {
					$(`#osd${i}_usertext_fontstrokecolor`).value = rgba2color(data.osd.usertext_font_stroke_color);
				}

				if (data.osd.usertext_format)
					$(`#osd${i}_usertext_format`).value = data.osd.usertext_format;
			}
		}
	}

	// Audio
	{
		data = msg.audio;
		if (data) {
			audio_params.forEach((x) => {
				if (typeof(data[x]) !== 'undefined')
					setValue(data, 'audio', x);
			});
		}
	}

	// Image
	{
	   for (let i = 1; i >= 0; i--) {
		const domain = `image${i}`;
		data = msg[domain];
	   	if (data) {
           		if ((data.alt_sensor == true)) {
				$(`#preview1`).style.display = 'block'; 
				$(`#preview1`).style.visibility = 'visible'; 
				if ($(`#preview_source_0`).checked) {
					$('#preview_fullsize').src='/x/ch0.mjpg';
				} else {
					$('#preview_fullsize').src='/x/ch1.mjpg';
				}
	   		}
           		else  if (data.alt_sensor == false) {
				$(`#preview1`).style.visibility = 'hidden'; 
				$(`#preview1`).style.display = 'none'; 
       	   		}
		}
	   }

	   // for (const i in [1, 0]) {
	   for (let i = 1; i >= 0; i--) {
		const domain = `image${i}`;
		data = msg[domain];
		if (data) {
                	if (data.running_mode <= 1) 
                        	$('#image_running_mode').checked = (data.running_mode == 1);

			if (data.zoom_enable)
				$('#image_zoom_enable').checked = data.zoom_enable;
			zoom_params.forEach((x) => {
				if (typeof(data[x]) !== 'undefined') {
					setValue(data, 'image', x);
					($(`#image_${x}`)).textContent = data[x];
					($(`#image_${x}`)).value = data[x];
				}	
			});
			image_params.forEach((x) => {
					if (typeof(data[x]) !== 'undefined')
						setValue(data, 'image', x);
			});
		}
	   }
	}

	// GPIO
	{
		data = msg.gpio;
		if (data) {
			gpio_params.forEach((x) => {
				if (typeof(data[x]) !== 'undefined')
					setValue(data, 'gpio', x);
					// $(`#gpio_${x}`).checked = (data[x] == 1);

			});
		}
	}

	// SENSOR
	{
		data = msg.sensor;
		if (data) {
			if (data.select !=='undfined') {
				setValue(data, 'sensor', 'select');
				const d = await loadConfigImage();
				if (data.select == 2)
					$(`#preview_source_1`).click();
				else 
					$('#preview_source_0').click();
			}
		}
	}
}

async function loadConfig() {
	const stream_rq = '{' +
		stream_params.map((x) => `"${x}":null`).join() +
		',"osd":{' + osd_params.map((x) => `"${x}":null`).join() + '}' +
		'}';
	const payload = '{' +
		'"stream0":' + stream_rq +
		',"stream1":' + stream_rq +
		',"stream2":{' + stream2_params.map((x) => `"${x}":null`).join() + '}' +
		',"stream3":{' + stream3_params.map((x) => `"${x}":null`).join() + '}' +
		',"audio":{' + audio_params.map((x) => `"${x}":null`).join() + '}' +
		',"image":{' + image_params.map((x) => `"${x}":null`).join() + '}' +
		',"gpio":{' + gpio_params.map((x) => `"${x}":null`).join() + '}' +
		',"sensor":{' + sensor_params.map((x) => `"${x}":null`).join() + '}' +
		'}';
	console.log('===>', payload);
	try {
		const response = await fetch(endpoint, {
			method: 'POST',
			headers: { 'Content-Type': 'application/json' },
			body: payload
		});
		if (!response.ok) throw new Error(`HTTP ${response.status}`);
		const contentType = response.headers.get('content-type');
		if (contentType?.includes('application/json')) {
			const msg = await response.json();
			console.log(ts(), '<===', JSON.stringify(msg));
			handleMessage(msg);
		}
	} catch (err) {
		console.error('Load config error', err);
	}
}

async function loadConfigImage() {
	const stream_rq = '{' +
		stream_params.map((x) => `"${x}":null`).join() +
		',"osd":{' + osd_params.map((x) => `"${x}":null`).join() + '}' +
		'}';
	const payload = '{' +
		'"image":{' + image_params.map((x) => `"${x}":null`).join() + '}' +
		'}';
	console.log('===>', payload);
	try {
		const response = await fetch(endpoint, {
			method: 'POST',
			headers: { 'Content-Type': 'application/json' },
			body: payload
		});
		if (!response.ok) throw new Error(`HTTP ${response.status}`);
		const contentType = response.headers.get('content-type');
		if (contentType?.includes('application/json')) {
			const msg = await response.json();
			console.log(ts(), '<===', JSON.stringify(msg));
			handleMessage(msg);
		}
	} catch (err) {
		console.error('Load config error', err);
	}
}


async function sendToEndpoint(payload) {
	console.log(ts(), '===>', payload);
	try {
		const response = await fetch(endpoint, {
			method: 'POST',
			headers: { 'Content-Type': 'application/json' },
			body: payload
		});
		if (!response.ok) throw new Error(`HTTP ${response.status}`);
		const contentType = response.headers.get('content-type');
		if (contentType?.includes('application/json')) {
			const msg = await response.json();
			console.log(ts(), '<===', JSON.stringify(msg));
			handleMessage(msg);
		}
	} catch (err) {
		console.error('Send error', err);
	}
}

// n - stream #,
// el - osd element
function setFontColor(n, el) {
	const fontcolor = $(`#osd${n}_${el}_fontcolor`).value;
	const fontcolor_alpha = parseInt($(`#osd${n}_${el}_fontcolor-alpha`).value).toString(16);
	const fontstrokecolor = $(`#osd${n}_${el}_fontstrokecolor`).value;
	const fontstrokecolor_alpha = parseInt($(`#osd${n}_${el}_fontstrokecolor-alpha`).value).toString(16);

	if (fontcolor == '' || fontstrokecolor == '') return;
	sendToEndpoint('{"stream'+n+'":{"osd":{'+
		'"'+el+'_font_color":"'+fontcolor+fontcolor_alpha+'",'+
		'"'+el+'_font_stroke_color":"'+fontstrokecolor+fontstrokecolor_alpha+'"'+
		'}},"action":{"restart_thread":10}}');
}

function toggleOSDElement(el) {
	const status = el.checked ? 'true' : 'false';
	const stream_id = el.id.substr(3, 1);
	const id = el.id.replace('osd0_', '').replace('osd1_', '');
	sendToEndpoint('{"stream'+stream_id+'":{"osd":{'+
		'"'+id+'":'+status+
		'}},"action":{"restart_thread":10}}');
}

function toggleWrappers(id) {
	const wrappers = $$(`#osd${id}_fontname_wrap,#osd${id}_fontsize_wrap,#osd${id}_fontfile_wrap`);
	if ($(`#osd${id}_enabled`).checked) {
		wrappers.forEach(el => el.classList.remove('d-none'));
	} else {
		wrappers.forEach(el => el.classList.add('d-none'));
	}
}

function saveValue(domain, name) {
	const el = $(`#${domain}_${name}`);
	if (!el) {
		// console.error(`Element #${domain}_${name} not found`);
		return;
	}

	let value;
	if (el.type == "checkbox") {
		if (domain == 'image' && name == 'running_mode')
			value = el.checked ? 1 : 0;
		else if (domain == 'gpio')
			value = el.checked ? 1 : 0;
		else
			value = el.checked;
	} else {
		value = el.value;
		if (["format", "input_format", "mode", "rtsp_endpoint"].includes(name)) {
			value = `"${value}"`;
		}
	}

	let payload = `"${name}":${value}`
	let thread = 0;
	let sensor_def = '';
	if (domain == 'audio') {
		thread += ThreadAudio;
		console.log(name, value);
		if (name == 'input_format') {
			if (value == '"G711A"' || value == '"G711U"') {
				payload += ',"input_sample_rate":8000'
			} else if (value == '"G726"') {
				payload += ',"input_sample_rate":16000'
			} else if (value == '"OPUS"') {
				payload += ',"input_sample_rate":48000'
			}
		}
	} else if (domain == 'stream0' || domain == 'stream1') {
		thread += ThreadRtsp;
		thread += ThreadVideo;
	} else {
		if (domain == 'image') {
			const ex = $(`#sensor_select`);
			let sv = ex.value;
			sensor_def = ',"sensor":{"select":'+sv+'}';
			// domain 'image' does not need a restart
		}
	}

	let json_actions = '';
	if (thread > 0) json_actions = ',"action":{"restart_thread":'+thread+'}';
	sendToEndpoint('{"'+domain+'":{'+ payload + "}" + sensor_def + json_actions +'}');
}

function saveZoom(domain,sv) {
	sensor_def = '"sensor":{"select":'+sv+'}';
	const payload =	'"image":{' + zoom_params.map((x) => `"${x}":` + $(`#${domain}_${x}`).value).join() + '}';
	sendToEndpoint("{" + payload + "," + sensor_def + "}");
}

image_zoom_enable = true;
var xr = 1;
var yr = 1;
var g_x0 = 0;
var g_y0 = 0;
var g_w0 = 300;
var g_h0 = 200;
image_crop_left.value= 0;
image_crop_top.value= 0;
image_crop_width.value= preview.naturalWidth;
image_crop_height.value= preview.naturalHeight;
image_zoom_enable.value = $('#image_zoom_enable').checked ? true : false;

//  plane for draw box
var drawCanvas = document.getElementById('imageCanvas');
var drawCanvas1 = document.getElementById('imageCanvas1');
// var ctx = drawCanvas.getContext('2d');
//  inner image
var img = document.getElementById('preview');
var box = document.getElementById("boxOnImage");

for (const i in [0, 1]) {
	stream_params.forEach((x) => {
		const el = $(`#stream${i}_${x}`);
		if (!el) {
			console.debug(`element #stream${i}_${x} not found`);
			return;
		}
		el.addEventListener('change', (_) => {
			saveValue(`stream${i}`, x);
		});
		el.addEventListener('dblclick', (_) => {
			const v = DEFAULT_VALUES[`stream${i}_${x}`];
			el.value = v;
			$(`#stream${i}_${x}-show`).textContent = v;
			saveValue(`stream${i}`, x);
		});
	});
}

audio_params.forEach((x) => {
	const el = $(`#audio_${x}`);
	if (!el) {
		console.debug(`element #image_${x} not found`);
		return;
	}
	el.addEventListener('change', (_) => {
		saveValue('audio', x);
	});
	el.addEventListener('dblclick', (_) => {
		const v = DEFAULT_VALUES[`audio_${x}`];
		el.value = v;
		$(`#audio_${x}-show`).textContent = v;
		saveValue('audio', x);
	});
});

//$("#image_zoom_enable").addEventListener('change', ev =>
//        sendToEndpoint('{"image":{"zoom_enable":'+ev.target.checked+'}}')); 


image_params.forEach((x) => {
	const el = $(`#image_${x}`);
	if (!el) {
		console.debug(`element #image_${x} not found`);
		return;
	}
	el.addEventListener('change', (_) => {
		saveValue('image', x);
	});
	el.addEventListener('dblclick', (_) => {
		const v = DEFAULT_VALUES[`image_${x}`];
		el.value = v;
		$(`#image_${x}-show`).textContent = v;
		saveValue('image', x);
	});
});

gpio_params.forEach((x) => {
	const el = $(`#gpio_${x}`);
	if (!el) {
		console.debug(`element #gpio_${x} not found`);
		return;
	}
	el.addEventListener('change', (_) => {
		saveValue('gpio', x);
	});
});

sensor_params.forEach((x) => {
	const el = $(`#sensor_${x}`);
	if (!el) {
		console.debug(`element #sensor_${x} not found`);
		return;
	}
	el.addEventListener('change', (_) => {
		saveValue('sensor', x);
		// loadConfigImage();
	});
});

$('#save-prudynt-config').addEventListener('click', ev => {
	sendToEndpoint('{"action":{"save_config":null}}');
});

$('#restart-audio').addEventListener('click', ev => {
	sendToEndpoint('{"action":{"restart_thread":' + ThreadAudio + '}}');
});

for (const i in [0, 1]) {
	$('#osd'+i+'_fontname').onchange = () => setFont(i);
	$('#osd'+i+'_fontsize').onchange = () => setFont(i);
	$('#osd'+i+'_fontstrokesize').onchange = () => setFont(i);

	$('#osd'+i+'_enabled').onchange = (ev) => sendToEndpoint('{"stream'+i+'":{"osd":{"enabled":'+ev.target.checked+'}},"action":{"restart_thread":10}}}');
	$('#osd'+i+'_logo_enabled').onchange = (ev) => toggleOSDElement(ev.target);

	$('#osd'+i+'_time_enabled').onchange = (ev) => toggleOSDElement(ev.target);
	$('#osd'+i+'_time_fontcolor').onchange = () => setFontColor(i, 'time');
	$('#osd'+i+'_time_fontcolor-alpha').onchange = () => setFontColor(i, 'time');
	$('#osd'+i+'_time_fontstrokecolor').onchange = () => setFontColor(i, 'time');
	$('#osd'+i+'_time_fontstrokecolor-alpha').onchange = () => setFontColor(i, 'time');
	$('#osd'+i+'_time_format').onchange = (ev) => sendToEndpoint('{"stream'+i+'":{"osd":{"time_format":"'+ev.target.value+'"}},"action":{"restart_thread":10}}}');

	$('#osd'+i+'_uptime_enabled').onchange = (ev) => toggleOSDElement(ev.target);
	$('#osd'+i+'_uptime_fontcolor').onchange = () => setFontColor(i, 'uptime');
	$('#osd'+i+'_uptime_fontcolor-alpha').onchange = () => setFontColor(i, 'uptime');
	$('#osd'+i+'_uptime_fontstrokecolor').onchange = () => setFontColor(i, 'uptime');
	$('#osd'+i+'_uptime_fontstrokecolor-alpha').onchange = () => setFontColor(i, 'uptime');

	$('#osd'+i+'_usertext_enabled').onchange = (ev) => toggleOSDElement(ev.target);
	$('#osd'+i+'_usertext_fontcolor').onchange = () => setFontColor(i, 'usertext');
	$('#osd'+i+'_usertext_fontcolor-alpha').onchange = () => setFontColor(i, 'usertext');
	$('#osd'+i+'_usertext_fontstrokecolor').onchange = () => setFontColor(i, 'usertext');
	$('#osd'+i+'_usertext_fontstrokecolor-alpha').onchange = () => setFontColor(i, 'usertext');
	$('#osd'+i+'_usertext_format').onchange = (ev) => sendToEndpoint('{"stream'+i+'":{"osd":{"usertext_format":"'+ev.target.value+'"}},"action":{"restart_thread":10}}}');
}

$('#preview_source_0').addEventListener('click', () => 
	{ 
		$('#preview').src='/x/ch0.mjpg';
		$('#preview').style="border: 3px solid orange";
		$('#preview_fullsize').src='/x/ch0.mjpg';
	   if ($('#image_alt_sensor').checked) {
			$('#preview1').src='/x/ch1.mjpg';
		$('#preview1').style="border: 3px solid gray";
	   }
	 });
$('#preview_source_1').addEventListener('click', () => 
	{ 
	   if ($('#image_alt_sensor').checked) {
		$('#preview').src='/x/ch0.mjpg';
		$('#preview').style="border: 3px solid gray";
		$('#preview1').src='/x/ch1.mjpg';
		$('#preview1').style="border: 3px solid orange";
	   } else {
		$('#preview').src='/x/ch1.mjpg';
		$('#preview').style="border: 3px solid orange";
	   }
	   $('#preview_fullsize').src='/x/ch1.mjpg';
	 });

img.addEventListener('load', 
    function get_current () {
	//  load once
	img.removeEventListener('load', get_current, false);
	drawCanvas.width = img.naturalWidth;
	drawCanvas.height = img.naturalHeight;
	drawCanvas1.offsetTop = 0;
	drawCanvas1.width = img.naturalWidth;
	drawCanvas1.height = img.naturalHeight;
 	xr = drawCanvas.width / drawCanvas.offsetWidth;
 	yr = drawCanvas.height / drawCanvas.offsetHeight;
}, false);


loadConfig().then(() => 
	{ 
		$('#preview').src = '/x/ch0.mjpg'; 
 		if ($(`#image_alt_sensor`).checked) {
			$('#preview1').src = '/x/ch1.mjpg';
		 }
		motor.style.visibility = "visible";
		motor.style.position = "relative";
		motor.style.top = '20px';
                motor.style.left = '10px';
                motor.style.width = '18vh';
                motor.style.height= '18vh';

		//motor.style.backgroundColor = "#444433ff"; 
	});

//const Canvases = document.querySelectorAll('.imgCanvas');

//Canvases.forEach(canvas => {
//    canvas.addEventListener('dblclick', handleMouseDoubleClick);
//});

window.onload = 
    function() {
	var box = document.getElementById("boxOnImage");
	//box.style.pointerEvents = "none";

        function isInImage(x, y) {
        	var innerImage = document.getElementById("imageCanvas");

                if (innerImage.offsetHeight < y || innerImage.offsetWidth < x) {
                        return false;
                } else {
                        return true;
                }

        }

        var top = 0, left = 0, width = 0, height = 0;
	let xs = -1, ys = -1, xe = -1, ye = -1;
	var moveEnable = false;
	var p_function = 0;  // previous mouse function

	// mouse functions
        imageCanvas.addEventListener("mousemove", function (e) {
		box = document.getElementById("boxOnImage");
		handleMouseMove(e);
        });
        imageCanvas1.addEventListener("mousemove", function (e) {
		box = document.getElementById("boxOnImage1");
		handleMouseMove(e);
        });
	function handleMouseMove(e) {
		if (!moveEnable) return;
	    	p_function = 3;
                var x = e.offsetX;
                var y = e.offsetY;
                if (isInImage(x, y)) {
                     if ((xe == -1 && ye == -1) && (xs != -1 && ys != -1)) {
                            createRectangleOnImage(xs, x, ys, y);
                     }
                } else {
			p_function = 0;
			moveEnable = false;
                	xs = -1; ys = -1; xe = -1; ye = -1;
                	removeRectangleOnImage();
	      	}
	}
	//
        imageCanvas.addEventListener("mousedown", function (e) {
		box = document.getElementById("boxOnImage");
		handleMouseDown(e);
	});
        imageCanvas1.addEventListener("mousedown", function (e) {
		box = document.getElementById("boxOnImage1");
		handleMouseDown(e);
	});
	function handleMouseDown(e) {
	    	if (p_function == 2) p_function = 1;
                var x = e.offsetX;
                var y = e.offsetY;
                if (isInImage(x, y)) {
    			//ctx.lineWidth = 6; // Border thickness
			//ctx.clearRect(g_x0-2, g_y0-2, g_w0+6, g_h0+6);
                        if (xs == -1 && ys == -1) {
                            xs = x;
                            ys = y;
                        } else if (xe == -1 && ye == -1) {
                            xe = x;
                            ye = y;
                            createRectangleOnImage(xs, xe, ys, ye);
                        } else {
                            xe = x;
                            ye = y;
                            createRectangleOnImage(xs, xe, ys, ye);
                        }
                }
		moveEnable = true;
	}
	//
        imageCanvas.addEventListener("mouseup", function (e) {
		box = document.getElementById("boxOnImage");
		handleMouseUp(e);
	});
        imageCanvas1.addEventListener("mouseup", function (e) {
		box = document.getElementById("boxOnImage1");
		handleMouseUp(e);
	});
	function handleMouseUp(e) {
		// let i, j;
		// let xsc, ysc, xec, yec;	

            var x = e.offsetX;
            var y = e.offsetY;
         if (isInImage(x, y)) {
	    let x0, y0, x1, y1, w0, h0;
	    moveEnable = false;
            xs = -1; ys = -1; xe = -1; ye = -1;
            removeRectangleOnImage();
	    if (p_function == 3) {  // was moving, end of box
 		xr = drawCanvas.width / drawCanvas.offsetWidth;
 		yr = drawCanvas.height / drawCanvas.offsetHeight;
 		// xr = image_scaler_outwidth.value / canvas.offsetWidth;
 		// yr = image_scaler_outheight.value / canvas.offsetHeight;
		x0 = Math.trunc(left * xr);
		y0 = Math.trunc(top * yr);
		w0 = Math.trunc(width * xr);
		h0 = Math.trunc(height * yr);
		x1 = x0 + w0 -1;
		y1 = y0 + h0 -1;
		g_x0 = x0; g_y0 = y0; g_w0 = w0; g_h0 = h0;
		image_crop_left.value = x0;
		image_crop_top.value = y0;
		image_crop_width.value = w0;
		image_crop_height.value = h0;
		image_crop_left.textContent= x0;
		image_crop_top.textContent = y0;
		image_crop_width.textContent = w0;
		image_crop_height.textContent = h0;
		$('#image_zoom_enable').value = true;
		if (e.currentTarget.id == 'imageCanvas')
			sv = 1;
		else 
			sv = 2;

		saveZoom("image",sv);
	   } else {
		if (e.currentTarget.id == 'imageCanvas')
			$(`#preview_source_0`).click();
		else 
			$('#preview_source_1').click();
		}
	} 
	    p_function = 2;
        }
	//  double click the mouse, set for full box/MAP
        imageCanvas.addEventListener("dblclick", function (e) {
		box = document.getElementById("boxOnImage");
		handleMouseDoubleClick(e);
	});
        imageCanvas1.addEventListener("dblclick", function (e) {
		box = document.getElementById("boxOnImage1");
		handleMouseDoubleClick(e);
	});
	function handleMouseDoubleClick(e) {
                xs = -1; ys = -1; xe = -1; ye = -1;
		let x0, y0, w0, h0;
                removeRectangleOnImage();
                width = img.naturalWidth;
                height = img.naturalHeight;
                x0 = 0;
                y0 = 0;
		image_crop_left.value = 0;
		image_crop_top.value = 0;
		image_crop_width.value = width;
		image_crop_height.value = height;
		image_scaler_outwidth.value = width;
		image_scaler_outheight.value = height;
		image_crop_left.textContent= 0;
		image_crop_top.textContent = 0;
		image_crop_width.textContent = width;
		image_crop_height.textContent = height;
		image_scaler_outwidth.textContent = width;
		image_scaler_outheight.textContent = height;
		$('#image_zoom_enable').value = true;
		if (e.currentTarget.id == 'imageCanvas')
			sv = 1;
		else 
			sv = 2;

		sendToEndpoint(`{"image":{"zoom_factor":10}}`);
		saveZoom("image",sv);
        }
	//  keyboard function
        document.addEventListener("keyup", function (e) {
                 // <27>: escape key Code
                 if (e.keyCode == 27) {
                        xs = -1; ys = -1; xe = -1; ye = -1;
                        removeRectangleOnImage();
                 }
        });
	// draw mouse move box image 
        function createRectangleOnImage(xs, xe, ys, ye) {
		box.style.pointerEvents = "none";
                if (box.style.display != 'block') {
                       box.style.display = 'block';
                }
                top = 0; left = 0; width = 0; height = 0;
		let t;
                if (ye > ys) {
                	height = (ye - ys);
                        top = ys;
                } else {
                        height = (ys - ye);
                        top = ye;
                }

                if (xe > xs) {
                        width = (xe - xs);
                        left = xs;
                } else {
                        width = (xs - xe);
                        left = xe;
                }

                box.style.top = Number(top) + "px";
                box.style.left = Number(left) + "px";
                box.style.width = Number(width) + "px";
                box.style.height = Number(height) + "px";
                if (box.style.display != 'block') {
                       box.style.display = 'block';
                }

        }
	// erase mouse move box image
        function removeRectangleOnImage() {
                    box.style.display = "none";
        }
}();

</script>

<%in _footer.cgi %>

