const config = new ClockConfig();

const btnConnect = document.getElementById('connect');
const btnDisconnect = document.getElementById('disconnect');

btnDisconnect.disabled = true;

config._onTimeChanged = function(date_time) {
	document.getElementById('time').innerText = date_time;
};

btnConnect.onclick = function(event) {
	btnConnect.disabled = true;
	config.connect()
		.then(() => {
			console.log('Connected');
			btnDisconnect.disabled = false;
		})
		.catch((error) => {
			console.error('Error: ' + error);
			btnConnect.disabled = false;
		});
};

btnDisconnect.onclick = function(event) {
	config.disconnect();
	btnConnect.disabled = false;
	btnDisconnect.disabled = true;
};