class ClockConfig {
	constructor() {
		this._device     = null;
		this._chCommand  = null;
		this._chTimeZone = null;
		this._chBright   = null;
		this._boundHandleChNotifyTime    = this._handleChNotifyTime.bind(this);
		this._boundHandleChNotifyTimeAcc = this._handleChNotifyTimeAcc.bind(this);
		this._boundHandleDisconnect      = this._handleDisconnect.bind(this);
	}

	_log(...messages) {
		console.log(...messages);
	}

	_onGotValue(name, value) {
	}

	_onDisconnected() {
	}

	_handleChNotifyTime(event) {
		const time = new TextDecoder().decode(event.target.value);
		this._onGotValue('Time', time);
	}

	_handleChNotifyTimeAcc(event) {
		const timeAcc = event.target.value.getUint32(0, true);
		this._onGotValue('TimeAccuracy', timeAcc);
	}

	_handleDisconnect(event) {
		this._log('Unexpected disconnect!');
		this._onDisconnected();
	}

	async connect() {
		this._log('Requesting devices...');

		const serviceUuid = '00000001-b0a0-475d-a2f4-a32cd026a911';

		const device = await navigator.bluetooth.requestDevice({
			filters: [{namePrefix: 'GPS Clock'}],
			optionalServices: [serviceUuid], 
		})

		this._log('Selected device: ' + device.name);
		this._log('Connecting...');
		this._device = device;
		this._device.addEventListener('gattserverdisconnected', this._boundHandleDisconnect);
		const server = await device.gatt.connect();

		this._log('Finding service...');
		const service = await server.getPrimaryService(serviceUuid);

		this._log('Finding characteristics...');
		this._chCommand  = await service.getCharacteristic('00000002-b0a0-475d-a2f4-a32cd026a911');
		const chTime     = await service.getCharacteristic('00000003-b0a0-475d-a2f4-a32cd026a911');
		this._chTimeZone = await service.getCharacteristic('00000004-b0a0-475d-a2f4-a32cd026a911');
		this._chBright   = await service.getCharacteristic('00000005-b0a0-475d-a2f4-a32cd026a911');
		const chTimeAcc  = await service.getCharacteristic('00000006-b0a0-475d-a2f4-a32cd026a911');

		this._log('Retrieving values...');
		const time       = new TextDecoder().decode(await chTime.readValue());
		this._onGotValue('Time', time);
		const timeZone   = (await this._chTimeZone.readValue()).getInt32(0, true);
		this._onGotValue('TimeZone', timeZone);
		const brightness = (await this._chBright  .readValue()).getInt8(0);
		this._onGotValue('Brightness', brightness);

		this._log('Starting notifications...');
		await chTime.startNotifications();
		chTime.addEventListener('characteristicvaluechanged', this._boundHandleChNotifyTime)
		await chTimeAcc.startNotifications();
		chTimeAcc.addEventListener('characteristicvaluechanged', this._boundHandleChNotifyTimeAcc)

		this._log('Connected');
	}

	disconnect() {
		if (!this._device) {
      return;
    }

    this._log('Disconnecting...');

    this._device.removeEventListener('gattserverdisconnected', this._boundHandleDisconnect);

    if (!this._device.gatt.connected) {
      this._log('Device is already disconnected');
      return;
    }

    this._device.gatt.disconnect();

		this._log('Disconnected');
		this._onDisconnected();
	}

	async setValue(name, value) {
		if (!this._device || !this._device.gatt.connected) {
			return;
		}

		this._log('Setting ' + name + ' to ' + value);
		if (name === 'TimeZone') {
			const buf = new ArrayBuffer(4);
			new DataView(buf).setUint32(0, value, true);
			await this._chTimeZone.writeValueWithResponse(buf);
		} else if (name === 'Brightness') {
			const buf = new ArrayBuffer(1);
			new DataView(buf).setInt8(0, value);
			await this._chBright.writeValueWithResponse(buf);
		} else {
			this._log('Unknown value!');
		}
	}

	async sendCommandSave() {
		if (!this._device || !this._device.gatt.connected) {
			return;
		}

		this._log('Sending save command');
		const buf = new ArrayBuffer(4);
		new DataView(buf).setUint32(0, 0x31a86b97, true);
		await this._chCommand.writeValueWithResponse(buf);
	}
}