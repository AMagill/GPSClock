class ClockConfig {
	constructor() {
		this._device = null;
		this._characteristic = null;
	}

	_log(...messages) {
		console.log(...messages);
	}

	_onTimeChanged(time) {
	}

	async connect() {
		this._log('Requesting devices...');

		const serviceUuid = '00000001-b0a0-475d-a2f4-a32cd026a911';

		const device = await navigator.bluetooth.requestDevice({
			filters: [{namePrefix: 'GPS Clock'}],
			optionalServices: [serviceUuid], 
		})

		this._log('Selected device: ' + device.name);
		this._device = device;
		const server = await device.gatt.connect();

		this._log('Connected to device');
		const service = await server.getPrimaryService(serviceUuid);

		this._log('Service found');
		const chTime = await service.getCharacteristic('00000002-b0a0-475d-a2f4-a32cd026a911');
		await chTime.startNotifications();
		chTime.addEventListener('characteristicvaluechanged', (event) => {
			const time = new TextDecoder().decode(event.target.value);
			this._onTimeChanged(time);
		})
			
		this._log('Characteristics found');
		this._characteristic = chTime;


	}

	disconnect() {
		if (!this._device) {
      return;
    }

    this._log('Disconnecting from "' + this._device.name + '" bluetooth device...');

    this._device.removeEventListener('gattserverdisconnected',
        console.boundHandleDisconnection);

    if (!this._device.gatt.connected) {
      this._log('"' + this._device.name +
          '" bluetooth device is already disconnected');
      return;
    }

    this._device.gatt.disconnect();

    this._log('"' + this._device.name + '" bluetooth device disconnected');
	}
}