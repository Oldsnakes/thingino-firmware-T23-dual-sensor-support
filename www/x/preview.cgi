#!/bin/haserl
<%in _common.cgi %>
<%
page_title="Camera Preview"
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

<% if [ -n "$gpio_ircut" ]; then %>
<input type="checkbox" class="btn-check" name="gpio_ircut" id="gpio_ircut" value="1">
<label class="btn btn-dark border mb-2" for="gpio_ircut" title="IR filter"><img src="/a/ircut_filter.svg" alt="IR filter" class="img-fluid"></label>
<% fi %>

<% if [ -n "$gpio_ir850" ]; then %>
<input type="checkbox" class="btn-check" name="gpio_ir850" id="gpio_ir850" value="1">
<label class="btn btn-dark border mb-2" for="gpio_ir850" title="IR LED 850 nm"><img src="/a/light_850nm.svg" alt="850nm LED" class="img-fluid"></label>
<% fi %>

<input type="checkbox" class="btn-check" name="sensor_select" id="sensor_select" value="1">
<label class="btn btn-dark border mb-2" for="sensor_select" title="CAM Select"><img src="/a/cam_select.svg" alt="CAM Select" class="img-fluid"></label>

<% if [ -n "$gpio_ir940_x" ]; then %>
<input type="checkbox" class="btn-check" name="gpio_ir940" id="gpio_ir940" value="1"> 
<label class="btn btn-dark border mb-2" for="gpio_ir940" title="CAM Select"><img src="/a/light_940nm.svg" alt="CAM Select" class="img-fluid"></label> 
<% fi %>

<% if [ -n "$gpio_white" ]; then %>
<input type="checkbox" class="btn-check" name="gpio_white" id="gpio_white" value="1">
<label class="btn btn-dark border mb-2" for="gpio_white" title="White LED"><img src="/a/light_white.svg" alt="White light" class="img-fluid"></label>
<% fi %>

<button type="button" class="btn btn-dark border mb-2" title="Zoom" data-bs-toggle="modal" data-bs-target="#mdPreview">
<img src="/a/zoom.svg" alt="Zoom" class="img-fluid"></button>
</div>

</div>
<div class="col-lg-10">
<div id="frame" class="position-relative mb-2">
<img id="preview" src="/a/nostream.webp" class="img-fluid" alt="Image: Preview">
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
<p class="small">Move mouse cursor over the center of the preview image to reveal the motor controls.
Use a single click for precise positioning, double click for coarse, larger distance movement.</p>
<% fi %>

<% if [  -n "$gpio_sensor_switch" ]; then %>
<div class="col-3"><% field_switch "image_alt_sensor" "Dual Camera" %></div>
<div id="frame" class="position-relative mb-2">
<img id="preview1" src="/a/nostream.webp" class="img-fluid" alt="Image: Preview" style="display:none ;">
</div>
<% fi %>

<div class="alert alert-secondary">
<p class="mb-0"><img src="/a/mute.svg" alt="Icon: No Audio" class="float-start me-2" style="height:1.75rem" title="No Audio">
Please note, there is no audio on this page. Open the RTSP stream in a player to hear audio.</p>
<b id="playrtsp" class="cb"></b>
</div>
</div>

<div class="col-lg-1">
<div class="d-flex flex-nowrap flex-lg-wrap align-content-around gap-1" aria-label="controls">
<a href="image.cgi" target="_blank" class="btn btn-dark border mb-2" title="Save image"><img src="/a/download.svg" alt="Save image" class="img-fluid"></a>
<button type="button" class="btn btn-dark border mb-2" title="Send to email" data-sendto="email"><img src="/a/email.svg" alt="Email" class="img-fluid"></button>
<button type="button" class="btn btn-dark border mb-2" title="Send to Telegram" data-sendto="telegram"><img src="/a/telegram.svg" alt="Telegram" class="img-fluid"></button>
<button type="button" class="btn btn-dark border mb-2" title="Send to FTP" data-sendto="ftp"><img src="/a/ftp.svg" alt="FTP" class="img-fluid"></button>
<button type="button" class="btn btn-dark border mb-2" title="Send to MQTT" data-sendto="mqtt"><img src="/a/mqtt.svg" alt="MQTT" class="img-fluid"></button>
<button type="button" class="btn btn-dark border mb-2" title="Send to Webhook" data-sendto="webhook"><img src="/a/webhook.svg" alt="Webhook" class="img-fluid"></button>
<button type="button" class="btn btn-dark border mb-2" title="Send to Ntfy" data-sendto="ntfy"><img src="/a/ntfy.svg" alt="Ntfy" class="img-fluid"></button>
</div>
</div>

</div>

<%in _preview.cgi %>

<script>
<%
for i in email ftp mqtt telegram webhook ntfy; do
	continue
#	[ "true" = $(eval echo \$${i}_enabled) ] && continue
%>
{
	let a = document.createElement('a')
	a.href = 'tool-send2<%= $i %>.cgi'
	a.classList.add('btn','btn-outline-danger','mb-2')
	a.title = 'Configure sent2<%= $i%> plugin'
	a.append($('button[data-sendto=<%= $i %>] img'))
	$('button[data-sendto=<%= $i %>]').replaceWith(a);
}
<% done %>

const ImageBlackMode = 1
const ImageColorMode = 0
const gpio_params = ['ir850', 'white', 'ircut', 'ir940'];
const image_params = ['running_mode','alt_sensor'];
const AltSensor = 0

const endpoint = '/x/json-prudynt.cgi';

function handleMessage(msg) {
   for (let i = 1; i >= 0; i--) {
	const domain = `image${i}`;
	data = msg[domain];

	if (data) {
		if (data.hflip != 'undefined')
			$('#rotate').checked = data.hflip;
		if (data.vflipi != 'undefined')
			$('#rotate').checked = data.vflip;
                if (data.running_mode <= 1) {
                        $('#image_running_mode').checked = (data.running_mode == 0);
                }
               	if (data.alt_sensor == true) {
			$(`#preview1`).style.display = 'block'; 
			$(`#preview1`).style.visibility = 'visible'; 
                        $('#image_alt_sensor').checked = 1;
		}
		else  if (data.alt_sensor == false) {
			$(`#preview1`).style.display = 'none'; 
			$(`#preview1`).style.visibility = 'hidden'; 
                        $('#image_alt_sensor').checked = 0;
		}
	} 
    }
	if (msg.sensor) {
                data = msg.sensor;
                if (data) {
               	   if (data.select != 'undefined') {
			if (data.select == true) {
				$(`#preview`).src = '/x/ch0.mjpg';
				$(`#preview1`).src = '/x/ch1.mjpg';
				$(`#preview_fullsize`).src = '/x/ch0.mjpg';
			} else {
				$(`#preview`).src = '/x/ch1.mjpg';
				$(`#preview1`).src = '/x/ch0.mjpg';
				$(`#preview_fullsize`).src = '/x/ch1.mjpg';
			}	
        		sendToEndpoint('{'+
				'"image":{"hflip":null,"vflip":null,'+
				   '"running_mode":null, "alt_sensor":null}'+
				'}');
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
		'"image":{"hflip":null,"vflip":null,'+
		   '"running_mode":null, "alt_sensor":null},'+
		'"gpio":{"ir850":null,"white":null,"ircut":null,"ir940":null,"daynight":null},'+
		'"sensor":{"select":null},'+
		'"motion":{"enabled":null},'+
		'"rtsp":{"username":null,"password":null,"port":null},'+
		'"stream0":{"rtsp_endpoint":null},'+
		'"action":{"capture":null}'+
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

async function toggleButton(el) {
	if (!el) return;
	const url = '/x/json-imp.cgi?' +
		new URLSearchParams({'cmd': el.id, 'val': (el.checked ? 1 : 0)}).toString();
	console.log(url)
	await fetch(url)
		.then(res => res.json())
		.then(data => {
			console.log(data.message)
			el.checked = data.message[el.id] == 1
		})
}

async function toggleDayNight(mode = 'read') {
	url = '/x/json-imp.cgi?' + new URLSearchParams({'cmd': 'daynight', 'val': mode}).toString()
	console.log(url)
	await fetch(url)
		.then(res => res.json())
		.then(data => {
			console.log(data.message)
			$('#daynight').checked = (data.message.daynight == 'night')
			if ($('#ir850')) $('#ir850').checked = (data.message.ir850 == 1)
			if ($('#ir940')) $('#ir940').checked = (data.message.ir940 == 1)
			if ($('#white')) $('#white').checked = (data.message.white == 1)
			if ($('#ircut')) $('#ircut').checked = (data.message.ircut == 1)
			if ($('#color')) $('#color').checked = (data.message.color == 1)
		})
}

async function ntoggleDayNight(state) {
			let inv_state = state ? 0:1;

			setValue('gpio', 'daynight',state);
			if (state == undefined) inv_state = undefined;
			if ($('#gpio_ir850'))  setValue('gpio', 'ir850',state);
			if ($('#gpio_ir940'))  setValue('gpio', 'ir940',state);
			if ($('#gpio_white'))  setValue('gpio', 'white',state);
			if ($('#gpio_ircut'))  setValue('gpio', 'ircut',inv_state);
			if ($('#image_running_mode'))  setValue('image', 'running_mode',state);
}

function setValue(domain, name, value) {
	const el = $(`#${domain}_${name}`);
	if (!el) {
		// console.error(`Element #${domain}_${name} not found`);
		return;
	}

	let payload = `"${name}":${value}`
	let thread = 0;

	let json_actions = '';
	if (thread > 0) json_actions = ',"action":{"restart_thread":'+thread+'}';
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
	}

	let payload = `"${name}":${value}`
	let thread = 0;

	let json_actions = '';
	if (thread > 0) json_actions = ',"action":{"restart_thread":'+thread+'}';
	sendToEndpoint('{"'+domain+'":{'+payload+json_actions+'}}');
}

image_params.forEach((x) => {
        const el = $(`#image_${x}`);
        if (!el) {
                console.debug(`element #image_${x} not found`);
                return;
        }
        el.addEventListener('click', (_) => {
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

$("#motion").addEventListener('change', ev =>
        sendToEndpoint('{"motion":{"enabled":'+ev.target.checked+'}}'));                                            
$('#rotate').addEventListener('change', ev =>
        sendToEndpoint('{"image":{"hflip":'+ev.target.checked+ ', "vflip":'+ev.target.checked+'}}')); 
$("#gpio_daynight").addEventListener('change', ev =>
	ev.target.checked ? ntoggleDayNight(1) : ntoggleDayNight(0));

$('#sensor_select').addEventListener('click', () => 
	{ 
		saveValue('sensor','select');
//		$('#preview').src='/x/ch0.mjpg';
//		$('#preview_fullsize').src='/x/ch0.mjpg';
//		if ($(`#image_alt_sensor`).checked) {
//			$('#preview1').src='/x/ch1.mjpg';
//		}
	 });

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

ntoggleDayNight();
</script>

<div class="alert alert-dark ui-debug d-none">
<h4 class="mb-3">Debug info</h4>
</div>

<%in _footer.cgi %>

