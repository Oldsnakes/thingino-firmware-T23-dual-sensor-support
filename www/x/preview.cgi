#!/bin/haserl
<%in _common.cgi %>
<%
page_title="Dual Camera Preview"
which motors > /dev/null && has_motors="true"
%>
<%in _header.cgi %>

<div class="row preview">
<div class="col-lg-1">

<div class="d-flex flex-nowrap flex-lg-wrap align-content-around gap-1" aria-label="controls">
<input type="checkbox" class="btn-check" name="motion" id="motion" value="1">
<label class="btn btn-dark border mb-2" for="motion" title="Motion Guard"><img src="/a/motion.svg" alt="Motion Guard" class="img-fluid"></label>

<input type="checkbox" class="btn-check" name="rotate" id="rotate" value="1">
<label class="btn btn-dark border mb-2" for="rotate" title="Rotate 180°"><img src="/a/rotate.svg" alt="Rotate 180°" class="img-fluid"></label>

<input type="checkbox" class="btn-check" name="gpio_daynight" id="gpio_daynight" value="1">
<label class="btn btn-dark border mb-2" for="gpio_daynight" title="Night mode"><img src="/a/night.svg" alt="Day/Night Mode" class="img-fluid"></label>

<!--
<input type="checkbox" class="btn-check" name="color" id="color" value="1">
<label class="btn btn-dark border mb-2" for="color" title="Color mode"><img src="/a/color.svg" alt="Color mode" class="img-fluid"></label>
-->

<input type="checkbox" class="btn-check" name="image_running_mode" id="image_running_mode" value="0">
<label class="btn btn-dark border mb-2" for="image_running_mode" title="color mode"><img src="/a/color.svg" alt="Color mode" class="img-fluid"></label>
<div id="daynight_color_enable"></div>

<% if [ -n "$gpio_ircut" ]; then %>
<input type="checkbox" class="btn-check" name="gpio_ircut" id="gpio_ircut" value="1">
<label class="btn btn-dark border mb-2" for="gpio_ircut" title="IR filter"><img src="/a/ircut_filter.svg" alt="IR filter" class="img-fluid"></label>
<div id="daynight_ircut_enable"></div>
<% fi %>

<% if [ -n "$gpio_ir850" ]; then %>
<input type="checkbox" class="btn-check" name="gpio_ir850" id="gpio_ir850" value="1">
<label class="btn btn-dark border mb-2" for="gpio_ir850" title="IR LED 850 nm"><img src="/a/light_850nm.svg" alt="850nm LED" class="img-fluid"></label>
<div id="daynight_ir850_enable"></div>
<% fi %>

<!--
<input type="checkbox" class="btn-check" name="sensor_select" id="sensor_select" value="1">
<label class="btn btn-dark border mb-2" for="sensor_select" title="CAM Select"><img src="/a/cam_select.svg" alt="CAM Select" class="img-fluid"></label>
-->

<% if [ -n "$gpio_ir940_x" ]; then %>
<input type="checkbox" class="btn-check" name="gpio_ir940" id="gpio_ir940" value="1"> 
<label class="btn btn-dark border mb-2" for="gpio_ir940" title="CAM Select"><img src="/a/light_940nm.svg" alt="CAM Select" class="img-fluid"></label> 
<div id="daynight_ir940_enable"></div>
<% fi %>

<% if [ -n "$gpio_white" ]; then %>
<input type="checkbox" class="btn-check" name="gpio_white" id="gpio_white" value="1">
<label class="btn btn-dark border mb-2" for="gpio_white" title="White LED"><img src="/a/light_white.svg" alt="White light" class="img-fluid"></label>
<div id="daynight_white_enable"></div>
<% fi %>

<button type="button" class="btn btn-dark border mb-2" id="download_btn" title="Zoom" data-bs-toggle="modal" data-bs-target="#mdPreview">
<img src="/a/zoom.svg" alt="Zoom" class="img-fluid"></button>
<a id="download_image" href="image.cgi?channel=0" target="_blank" class="btn btn-dark border mb-2" title="Save image"><img src="/a/download.svg" alt="Save image" class="img-fluid"></a>
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

<div class="col-lg-10">
<div id="zoom-map" class="position-relative mb-2">
<div id="zoom-map" class="canvas-container" style="border: 2px solid #889988;">
	<img id="preview" src="/x/ch0.mjpg" class="img-fluid" alt="Image: Stream 0 Preview">
    	<canvas class="imgCanvas" id="imageCanvas"></canvas>
	<div id="boxOnImage" class="overlayLayer"></div>
</div>

<div id="image_crop_left"></div>
<div id="image_crop_top"></div>
<div id="image_crop_width"></div>
<div id="image_crop_height"></div>
<div id="image_scaler_outwidth"></div>
<div id="image_scaler_outheight"></div>

<style>
#motor { width: 25vh; height: 25vh;} 
#motor:hover .jst { visibility: visible; }
.jst { width: 100%; height: 100%; border-radius: 50%; position: relative; overflow: hidden; visibility: hidden; }
</style>
<% if [ "true" = "$has_motors" ]; then %>
	<%in _motors.cgi %>
<% fi %>
</div> 
<% if [ "true" = "$has_motors" ]; then %>
<p class="small">Move mouse cursor over the center of the image to reveal the motor controls.
Use a single click for precise positioning, double click for coarse, larger distance movement.
Hold Control/Shift key to hide it for zoom and move function. </p>
<% fi %>
<div class="row row-cols-3 row-cols-m-2">
<div class="col-2"><% field_switch "image_zoom_enable" "Zoom Enabled" %></div>
<div class="col-2"><% field_switch "image_alt_sensor" "Dual Camera" %></div>
<% if $image_alt_sensor; then %>
<div class="btn-toolbar" role="toolbar">
<div class="btn-group mb-3" role="group">
<button type="button" class="btn btn-secondary" data-bs-toggle="modal" data-bs-target="#mdPreview" title="Full-screen"><img src="/a/zoom.svg" alt="Zoom" class="img-fluid icon-sm"></button>
<input type="radio" class="btn-check" name="preview_source" id="preview_source_0" value="0" checked>
<label class="btn btn-outline-primary" for="preview_source_0">Camera 1</label>
<input type="radio" class="btn-check" name="preview_source" id="preview_source_1" value="1">
<label class="btn btn-outline-primary" for="preview_source_1">Camera 2</label>
</div>
</div>
</div>

<div id="zoom-map1" class="position-relative mb-2">
<div id="zoom-map1" class="canvas-container" style="border: 2px solid #889988;">
	<img id="preview1" src="/x/ch1.mjpg" class="img-fluid" alt="Image: Stream 1 Preview">
    	<canvas class="imgCanvas" id="imageCanvas1"></canvas>
	<div id="boxOnImage1" class="overlayLayer"></div>
</div>
</div>
<% fi %>

<div id="sensor_select"></div>

<div class="alert alert-secondary">
<p class="mb-0"><img src="/a/mute.svg" alt="Icon: No Audio" class="float-start me-2" style="height:1.75rem" title="No Audio">
Please note, there is no audio on this page. Open the RTSP stream in a player to hear audio.</p>
<b id="playrtsp" class="cb"></b>
</div>
</div>

</div>

<%in _preview.cgi %>

<script>

const gpio_params = ['ir850', 'white', 'ircut', 'ir940'];
const image_params = ['running_mode','alt_sensor',
	'crop_left', 'crop_top', 'crop_width', 'crop_height',
	'scaler_outwidth', 'scaler_outheight', 'zoom_enable'
];
// image zoom params
const zoom_params = ['crop_left', 'crop_top', 'crop_width', 'crop_height',
	'scaler_outwidth', 'scaler_outheight', 'zoom_enable'
];
const sensor_params = ['select'];
const daynight_params = ["enable", "night_mode",
	"white_enable", "ir850_enable", "ir940_enable", "ircut_enable", "color_enable"
];

const endpoint = '/x/json-prudynt.cgi';

function handleMessage(msg) {
   // image 0/1
   for (let i = 1; i >= 0; i--) {
	const domain = `image${i}`;
	data = msg[domain];

	if (data) {
		image_params.forEach((x) => {
			if (typeof(data[x]) !== 'undefined')
				setValue(data,'image', x);
		});
		if (data.hflip != 'undefined')
			$('#rotate').checked = data.hflip;
		if (data.vflipi != 'undefined')
			$('#rotate').checked = data.vflip;
                if (data.running_mode <= 1) 
                        $('#image_running_mode').checked = (data.running_mode == 0);
		if (data.zoom_enable)
			$('#image_zoom_enable').checked = data.zoom_enable;
           		if ((data.alt_sensor == true)) {
				$('#image_alt_sensor').checked = data.alt_sensor;
				$(`#preview1`).style.display = 'block'; 
				$(`#preview1`).style.visibility = 'visible'; 
				if ($(`#preview_source_0`).checked) {
					$('#preview_fullsize').src='/x/ch0.mjpg';
				} else {
					$('#preview_fullsize').src='/x/ch1.mjpg';
				}
	   		}
           		else  if (data.alt_sensor == false) {
				$('#image_alt_sensor').checked = data.alt_sensor;
				$(`#preview1`).style.visibility = 'hidden'; 
				$(`#preview1`).style.display = 'none'; 
       	   		}

		} 
    	}
	if (msg.sensor) {
                data = msg.sensor;
                if (data) {
			if (data.select !=='undfined') {
			setValue(data, 'sensor', 'select');
			getStatus();
			}
		}
	}
	if (msg.gpio) {
                data = msg.gpio;
                if (data) {
                        gpio_params.forEach((x) => {
                               	if (data[x] <= 1)
					if ($(`#gpio_${x}`))
						$(`#gpio_${x}`).checked = (data[x] == 1);
                       });
                }
	}
	if (msg.daynight) {
		data = msg.daynight;
		if (data) {
			daynight_params.forEach((x) => {
				if (typeof(data[x]) !== 'undefined')
					setValue(data, 'daynight', x);
			});
		}
	}
	if (msg.motion && msg.motion.enabled) {
		$('#motion').checked = msg.motion.enabled;
	}
	if (msg.rtsp) {
		const r = msg.rtsp;
		if (r.username && r.password && r.port && msg.stream0?.rtsp_endpoint)
			$('#playrtsp').innerHTML = `mpv rtsp://${r.username}:${r.password}@${document.location.hostname}:${r.port}/${msg.stream0.rtsp_endpoint}`;
	}
}

async function loadConfig() {
	const payload = '{'+
		'"image":{' + image_params.map((x) => `"${x}":null`).join() + '}' +
		',"gpio":{' + gpio_params.map((x) => `"${x}":null`).join() + '}' +
		',"sensor":{' + sensor_params.map((x) => `"${x}":null`).join() + '}' +
		',"daynight":{' + daynight_params.map((x) => `"${x}":null`).join() + '}' +
		',"motion":{"enabled":null}'+
		',"rtsp":{"username":null,"password":null,"port":null}'+
		',"stream0":{"rtsp_endpoint":null}'+
		',"stream1":{"rtsp_endpoint":null}'+
		',"action":{"capture":null}'+
		'}'
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

function getStatus() {

	const payload = '{'+
		'"image":{"hflip":null,"vflip":null,"running_mode":null,"zoom_enable":null}' +
		',"gpio":{' + gpio_params.map((x) => `"${x}":null`).join() + '}' +
		',"motion":{"enabled":null}'+
		'}'
	sendToEndpoint(payload);
}

async function ntoggleDayNight(state) {
			let inv_state = state ? 0:1;

			if (state == undefined) inv_state = undefined;
			const payload = '{' +
				'"daynight":{' + daynight_params.map((x) => `"${x}":null`).join() + '}' +
				',"gpio":{' + gpio_params.map((x) => `"${x}":null`).join() + '}' +
			'}';
			sendToEndpoint(payload);
			if ($('#gpio_ir850'))  
				if (daynight_ir850_enable.value == true) 
					setValueN('gpio', 'ir850',state);
			if ($('#gpio_ir940'))  
				if (daynight_ir940_enable.value == true)                                                                  
					setValueN('gpio', 'ir940',state);
			if ($('#gpio_white'))  
				if (daynight_white_enable.value == true)                                                                  
					setValueN('gpio', 'white',state);
			if ($('#gpio_ircut'))  
				if (daynight_ircut_enable.value == true)                                                                  
					setValueN('gpio', 'ircut',inv_state);
			if (daynight_color_enable.value == true)
				if ($('#image_running_mode')) {
				sendToEndpoint(`{"image":{"running_mode":${state}},"sensor":{"select":2}}`);
				sendToEndpoint(`{"image":{"running_mode":${state}},"sensor":{"select":1}}`);
			}
}

function setValueN(domain, name, value) {
	const el = $(`#${domain}_${name}`);
	if (!el) {
		// console.error(`Element #${domain}_${name} not found`);
		return;
	}

	let payload = `"${name}":${value}`
	let thread = 0;
//	let sensor_def = '';
//	if (domain == 'image') {
//		const ex = $(`#sensor_select`);
//		let sv = ex.value;
//		sensor_def = ',"sensor":{"select":'+sv+'}';
//	}

	let json_actions = '';
	if (thread > 0) json_actions = ',"action":{"restart_thread":'+thread+'}';
	//sendToEndpoint('{"'+domain+'":{'+ payload + "}" + sensor_def + json_actions +'}');
	sendToEndpoint('{"'+domain+'":{'+payload+json_actions+'}}');
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
			value = el.checked ? 0 : 1;
		else if (domain == 'gpio' )
			value = el.checked ? 1 : 0;
		else if (domain == 'sensor' && name == 'select')
			value = el.checked ? 2 : 1;
		else
			value = el.checked;
	} else {
		// domain 'image' does not need a restart
	}

	let payload = `"${name}":${value}`
	let thread = 0;
	let sensor_def = '';
	if (domain == 'image') {
		const ex = $(`#sensor_select`);
		let sv = ex.value;
		sensor_def = ',"sensor":{"select":'+sv+'}';
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
const map_h = 8;
const map_v = 6;
image_crop_left.value= 0;
image_crop_top.value= 0;
image_crop_width.value= preview.naturalWidth;
image_crop_height.value= preview.naturalHeight;
image_zoom_enable.value = $('#image_zoom_enable').checked ? true : false;

//  plane for draw box
var drawCanvas = document.getElementById('imageCanvas');
var drawCanvas1 = document.getElementById('imageCanvas1');
//  inner image
var img = document.getElementById('preview');
var box = document.getElementById("boxOnImage");

image_params.forEach((x) => {
        const el = $(`#image_${x}`);
        if (!el) {
                console.debug(`element #image_${x} not found`);
                return;
        }
        el.addEventListener('change', (_) => {
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

$('#preview_source_0').addEventListener('click', () => 
	{ 
		$('#preview').src='/x/ch0.mjpg';
		$('#preview').style="border: 5px solid orange";
		$('#preview_fullsize').src='/x/ch0.mjpg';
	   	if ($('#image_alt_sensor').checked) {
			$('#preview1').src='/x/ch1.mjpg';
		$('#preview1').style="border: 5px solid gray";
		$('#sensor_select').value = 1;
		document.getElementById("download_image").href = "image.cgi?channel=0";
		sendToEndpoint('{"sensor":{"select":1}}');
	   }
	 });
$('#preview_source_1').addEventListener('click', () => 
	{ 
	   if ($('#image_alt_sensor').checked) {
		$('#preview').src='/x/ch0.mjpg';
		$('#preview').style="border: 5px solid gray";
		$('#preview1').src='/x/ch1.mjpg';
		$('#preview1').style="border: 5px solid orange";
	   } else {
		$('#preview').src='/x/ch1.mjpg';
		$('#preview').style="border: 5px solid orange";
	   }
	   $('#preview_fullsize').src='/x/ch1.mjpg';
	   $('#sensor_select').value = 2;
	   document.getElementById("download_image").href = "image.cgi?channel=1";
	   sendToEndpoint('{"sensor":{"select":2}}');
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

$("#motion").addEventListener('change', ev =>
        sendToEndpoint('{"motion":{"enabled":'+ev.target.checked+'}}'));                                            
$('#rotate').addEventListener('change', ev =>
        sendToEndpoint('{"image":{"hflip":'+ev.target.checked+ ', "vflip":'+ev.target.checked+'}}')); 
$("#gpio_daynight").addEventListener('change', ev =>
	ev.target.checked ? ntoggleDayNight(1) : ntoggleDayNight(0));

// Init on load
loadConfig().then(() => {
	// Preview
	const timeout = 5000;
	const preview = $('#preview');
	const preview1 = $('#preview1');
	let lastLoadTime = Date.now();
	preview.src = '/x/ch0.mjpg';
	preview_fullsize.src = '/x/ch0.mjpg';
	preview.addEventListener('load', () => {
		lastLoadTime = Date.now()
	});
	// if ($(`#image_alt_sensor`).checked) {
		preview1.src = '/x/ch1.mjpg';
		preview1.addEventListener('load', () => {
			lastLoadTime = Date.now()
		});
	// }
	setInterval(() => {
		if (Date.now() - lastLoadTime > timeout) {
			// Restart stream
			// preview.src = preview.src.split('?')[0] + '?' + new Date().getTime();
			// preview1.src = preview1.src.split('?')[0] + '?' + new Date().getTime();
			lastLoadTime = Date.now();
		}
	}, 1000);
		motor.style.position = "absolute";
		motor.style.top = '250px';
                motor.style.left = '50px';
                motor.style.start = '50px';
});

function select_tile(tile) {
	sendToEndpoint(`{"motion":{"selected_tile":${tile}}}`);
} 

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
		if (e.ctrlKey) { // click to move:  send tile to motion control to move it to the center
			const dx = imageCanvas.width / map_h;
			const dy = imageCanvas.height / map_v;

			w0 = Math.trunc(x*xr/dx);
			h0 = Math.trunc(y*yr/dy);
			select_tile(w0 + (h0*map_h));
		} else {
		    if (p_function == 3) {  // was moving, end of box
			image_crop_left.value = x0;
			image_crop_top.value = y0;
			image_crop_width.value = w0;
			image_crop_height.value = h0;
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
		$('#image_zoom_enable').value = true;
		if (e.currentTarget.id == 'imageCanvas')
			sv = 1;
		else 
			sv = 2;

		sendToEndpoint(`{"image":{"zoom_factor":10}}`);
		saveZoom("image",sv);
        }
        document.addEventListener("keydown", function (e) {
		if ((e.key = "Control") || (e.key = "Shift")) {
			motor.disabled = true;
			motor.hidden = true;
		}
        });

	//  keyboard function
        document.addEventListener("keyup", function (e) {
                 // <27>: escape key Code
                 if (e.keyCode == 27) {
                        xs = -1; ys = -1; xe = -1; ye = -1;
                        removeRectangleOnImage();
                 }
		if ((e.key = "Control") || (e.key = "Shift")) {
			motor.disabled = false;
			motor.hidden = false;
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

ntoggleDayNight(0);
</script>

<div class="alert alert-dark ui-debug d-none">
<h4 class="mb-3">Debug info</h4>
</div>

<%in _footer.cgi %>

