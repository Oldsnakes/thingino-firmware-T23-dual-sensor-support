#!/bin/haserl
<%in _common.cgi %>
<%
page_title="Daynight New"
%>
<%in _header.cgi %>

<div class="col mb-3">
<p class="small">Double-click on a range element will restore its default value.</p>
</div>
    <style>
        .box {
            width: 500px;
            height: 85px;
            background-color: #303030; 
            border: 2px solid #505050;
            padding: 10px;
            margin: 10px;
        }
        .box1 {
            width: 500px;
            height: 120px;
            background-color: #303030; 
            border: 2px solid #505050;
            padding: 10px;
            margin: 10px;
        }
    </style>
<div class="col mb3">
<% field_switch "daynight_enable" "Enable Day-Night Switching" %>
<div class="col-2"
<p id="daynight_night_mode" ></p>
</div>
<div class="box">
<div class="row g-3">
<div class="col-4"><% field_switch "daynight_white_enable" "White" %></div>
<div class="col-4"><% field_switch "daynight_ir850_enable" "IR-850" %></div>
<div class="col-4"><% field_switch "daynight_ir940_enable" "IR-940" %></div>
<div class="col-4"><% field_switch "daynight_ircut_enable" "IR Cut" %></div>
<div class="col-4"><% field_switch "daynight_color_enable" "B/W" %></div>
</div>
</div>
<div class="row row-cols-2 row-cols-lg-2 g-2">
<div class="col-3"><% field_range "daynight_up_threshold" "To-Day <" "1,255,1" %></div>
<div class="col-3"><% field_range "daynight_low_threshold" "To-Night >" "10,255,1" %></div>
<div class="col-3"><% field_range "daynight_hold_count" "Hold Count" "0,25,1" %></div>
<div class="col-3"><% field_range "daynight_sample_time" "Sample Time(s)" "0,100,1" %></div>
</div>

<div class="col mb-3">
<div class="d-flex flex-wrap align-content-around gap-1">
<button type="button" class="btn btn-secondary" id="save-prudynt-config">Save config</button>
</div>

<script>

const endpoint = '/x/json-prudynt.cgi';

// gpio
const gpio_params = ['ir850', 'white', 'ircut', 'ir940'];

// sensor
const sensor_params = ['select'];

// daynight
const daynight_params = ["enable", "night_mode",
	"white_enable", "ir850_enable", "ir940_enable", "ircut_enable", "color_enable",
	"low_threshold", "up_threshold", "sample_time", "hold_count"
];

// daynight status
const status_params = ["night_mode","white_enable", "ir850_enable", "ir940_enable", "ircut_enable", "color_enable"];

DEFAULT_VALUES = {
	'audio_input_agc_compression_gain_db': 0,
        'daynight_white_enable': false,
	'daynight_ir850_enable': false, 
	'daynight_ir940_enable': false, 
	'daynight_ircut_enable': false, 
	'daynight_color_enable': false, 
        'daynight_low_threshold': 15, 
	'daynight_up_threshold': 100,
	'daynight_sample_time': 3,
	'daynight_hold_count': 5

}

async function handleMessage(msg) {
	if (msg.action && msg.action.capture == 'initiated') return;

	let data;

	// DAYNIGHT
	{
		data = msg.daynight;
		if (data) {
			daynight_params.forEach((x) => {
				if (typeof(data[x]) !== 'undefined')
					setValue(data, 'daynight', x);
			});
			// if (data.night_mode !== 'undefined')
			{
				let night_mode = document.getElementById("daynight_night_mode");
				night_mode.style.textAlign="center";
				if (daynight_night_mode.value) {
					night_mode.style.backgroundColor="black"; 
					night_mode.style.color="white"; 
					night_mode.innerText = '-- Night --';
				} else
				{
					night_mode.style.backgroundColor="white"; 
					night_mode.style.color="black"; 
					night_mode.innerText = '*** Day ***';
				}
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
}

async function loadConfig() {
	const payload = '{' +
		'"daynight":{' + daynight_params.map((x) => `"${x}":null`).join() + '}' +
		',"gpio":{' + gpio_params.map((x) => `"${x}":null`).join() + '}' +
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

async function getStatus() {
	const payload = '{' +
		'"daynight":{' + status_params.map((x) => `"${x}":null`).join() + '}' +
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
		console.error('getStatus error', err);
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


function saveValue(domain, name) {
	const el = $(`#${domain}_${name}`);
	if (!el) {
		// console.error(`Element #${domain}_${name} not found`);
		return;
	}

	let value;
	if (el.type == "checkbox") {
			value = el.checked;
	}

	let payload = `"${name}":${value}`
	payload += `,"night_mode":null`
	let thread = 0;

	let json_actions = '';
	sendToEndpoint('{"'+domain+'":{'+payload+json_actions+'}}');
}

daynight_params.forEach((x) => {
	const el = $(`#daynight_${x}`);
	if (!el) {
		console.debug(`element #daynight_${x} not found`);
		return;
	}
	el.addEventListener('change', (_) => {
		saveValue('daynight', x);
	});
	el.addEventListener('dblclick', (_) => {
		const v = DEFAULT_VALUES[`daynight_${x}`];
		el.value = v;
		$(`#daynight_${x}-show`).textContent = v;
		saveValue('daynight',x);
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
	});
});

// setInterval(getStatus, 20000); 

$('#save-prudynt-config').addEventListener('click', ev => {
	sendToEndpoint('{"action":{"save_config":null}}');
});

loadConfig();

</script>

<%in _footer.cgi %>


