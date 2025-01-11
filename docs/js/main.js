const config = new ClockConfig();

const btnConnect = document.getElementById('connect');
const btnDisconnect = document.getElementById('disconnect');

btnDisconnect.disabled = true;

btnConnect.onclick = function(event) {
	config.connect()
		.then(() => {
			console.log('Connected');
			btnConnect.disabled = true;
			btnDisconnect.disabled = false;
		})
		.catch((error) => {
			console.error('Error: ' + error);
		});
};

btnDisconnect.onclick = function(event) {
	config.disconnect();
	btnConnect.disabled = false;
	btnDisconnect.disabled = true;
};