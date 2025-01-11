class ClockConfig {
	constructor() {
		console.hos = 0;
		console.device = null;
		console.characteristic = null;

		console.serviceUuid = [0xFFE0];
		//console.serviceUuid = [0xFFE0, 0x1800, '6e400001-b5a3-f393-e0a9-e50e24dcca9e'];
		console.characteristicUuid = 0xFFE1;
	}

	connect() {
		console.log('Requesting...');

		return navigator.bluetooth.requestDevice({
			filters: [{
				//services: console.serviceUuid, 
				namePrefix: 'Pico'
			}],
		})
		.then((device) => {
			console.log('Selected device: ' + device.name);
			this.device = device;
			return device.gatt.connect();
		})
		.then((server) => {
			console.log('Connected to device');
			return server.getPrimaryServices();
		})
		.then((services) => {
			
		});
	}

	disconnect() {
		if (!device) {
      return;
    }

    console.log('Disconnecting from "' + device.name + '" bluetooth device...');

    device.removeEventListener('gattserverdisconnected',
        console.boundHandleDisconnection);

    if (!device.gatt.connected) {
      console.log('"' + device.name +
          '" bluetooth device is already disconnected');
      return;
    }

    device.gatt.disconnect();

    console.log('"' + device.name + '" bluetooth device disconnected');
	}
}