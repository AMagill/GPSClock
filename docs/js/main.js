const config = new ClockConfig();

const buttonConnect    = document.getElementById('connect');
const buttonDisconnect = document.getElementById('disconnect');
const buttonSave       = document.getElementById('save');
const labelTime        = document.getElementById('time');
const inputTimeZone    = document.getElementById('timezone');
const inputBrightness  = document.getElementById('brightness');

buttonDisconnect.disabled = true;

config._onGotValue = function(name, value) {
	if (name === 'Time') {
		labelTime.innerText = value;
	} else if (name === 'TimeZone') {
		inputTimeZone.value = value;
	} else if (name === 'Brightness') {
		inputBrightness.value = value;
	}
};

config._onDisconnected = function() {
	buttonConnect.disabled    = false;
	buttonDisconnect.disabled = true;
	buttonSave.disabled       = true;
	labelTime.disabled        = true;
	labelTime.innerText       = '';
	inputTimeZone.disabled    = true;
	inputTimeZone.value       = '';
	inputBrightness.disabled  = true;
	inputBrightness.value     = '';
};

buttonConnect.onclick = function(event) {
	buttonConnect.disabled = true;
	config.connect()
		.then(() => {
			buttonDisconnect.disabled = false;
			buttonSave.disabled       = false;
			labelTime.disabled        = false;
			inputTimeZone.disabled    = false;
			inputBrightness.disabled  = false;
		})
		.catch((error) => {
			console.error('Error: ' + error);
			buttonConnect.disabled = false;
		});
};

buttonDisconnect.onclick = function(event) {
	config.disconnect();
};

inputTimeZone.oninput = async function(event) {
	const timeZone = parseInt(inputTimeZone.value);

	if (timeZone >= -12 && timeZone <= 12) {
		await config.setValue('TimeZone', new Int32Array([timeZone]));
	}
}

inputBrightness.oninput = async function(event) {
	const brightness = parseInt(inputBrightness.value);

	if (brightness >= 0 && brightness <= 127) {
		await config.setValue('Brightness', new Int8Array([brightness]));
	}
}

buttonSave.onclick = function(event) {
	config.sendCommandSave();
}