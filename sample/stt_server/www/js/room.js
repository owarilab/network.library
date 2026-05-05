class RoomServerManager {
	constructor(options) {
		const opts = options || {};
		this.httpBase = opts.httpBase || window.location.origin;
		this.wsBase = opts.wsBase || ((window.location.protocol === 'https:' ? 'wss://' : 'ws://') + window.location.host);
		this.autoHandshake = opts.autoHandshake !== false;
		this.wsSocket = null;
		this.selfUuid = '';
		this.selfConnectionId = '';
		this.currentJoinRoomId = '';
		this.callbacks = {};
		this.setCallbacks(opts.callbacks || {});
	}

	setCallbacks(callbacks) {
		const nextCallbacks = callbacks || {};
		this.callbacks = Object.assign({}, this.callbacks, nextCallbacks);
		return this;
	}

	setCallback(name, handler) {
		if (typeof handler !== 'function') {
			return this;
		}
		this.callbacks[name] = handler;
		return this;
	}

	getState() {
		return {
			selfUuid: this.selfUuid,
			selfConnectionId: this.selfConnectionId,
			currentJoinRoomId: this.currentJoinRoomId,
			connected: this.isConnected()
		};
	}

	isConnected() {
		return this.wsSocket !== null && this.wsSocket.readyState === WebSocket.OPEN;
	}

	connect() {
		if (this.wsSocket !== null && this.wsSocket.readyState !== WebSocket.CLOSED) {
			return this.wsSocket;
		}

		this.wsSocket = new WebSocket(this.wsBase);
		this.wsSocket.binaryType = 'arraybuffer';
		this.wsSocket.addEventListener('open', (event) => {
			if (this.autoHandshake) {
				this.selfUuid = Math.random().toString(36).slice(-8);
				this.send(this.selfUuid);
			}
			this.emit('onWsOpen', {
				event,
				manager: this,
				state: this.getState()
			});
		});

		this.wsSocket.addEventListener('message', (event) => {
			this.emit('onWsRawMessage', {
				event,
				data: event.data,
				manager: this,
				state: this.getState()
			});

			let packet;
			try {
				packet = JSON.parse(event.data);
			} catch (error) {
				this.emit('onParseError', {
					error,
					source: 'ws_message',
					data: event.data,
					manager: this,
					state: this.getState()
				});
				return;
			}

			const hasCommonFields =
				packet &&
				packet.id !== undefined &&
				packet.type !== undefined &&
				packet.message !== undefined;

			if (!hasCommonFields) {
				this.emit('onWsMessage', {
					packet,
					event,
					manager: this,
					state: this.getState()
				});
				return;
			}

			if (packet.type === 'message') {
				if (this.selfUuid && packet.message === this.selfUuid) {
					this.selfConnectionId = packet.id;
					this.emit('onSelfConnectionId', {
						connectionId: this.selfConnectionId,
						packet,
						manager: this,
						state: this.getState()
					});
				}
				this.emit('onMessage', {
					connectionId: packet.id,
					message: packet.message,
					packet,
					event,
					manager: this,
					state: this.getState()
				});
			} else if (packet.type === 'join' || packet.type === 'leave') {
				let roomInfo = null;
				if (typeof packet.message === 'string' && packet.message.length > 0) {
					try {
						roomInfo = JSON.parse(packet.message);
					} catch (error) {
						this.emit('onParseError', {
							error,
							source: packet.type,
							data: packet.message,
							manager: this,
							state: this.getState()
						});
					}
				}

				const payload = {
					connectionId: packet.id,
					roomInfo,
					message: packet.message,
					packet,
					event,
					manager: this,
					state: this.getState()
				};

				if (packet.type === 'join') {
					this.emit('onJoin', payload);
				} else {
					this.emit('onLeave', payload);
				}
			}

			this.emit('onWsMessage', {
				packet,
				event,
				manager: this,
				state: this.getState()
			});
		});

		this.wsSocket.addEventListener('close', (event) => {
			this.currentJoinRoomId = '';
			this.emit('onWsClose', {
				event,
				manager: this,
				state: this.getState()
			});
		});

		this.wsSocket.addEventListener('error', (event) => {
			this.emit('onWsError', {
				event,
				manager: this,
				state: this.getState()
			});
		});

		return this.wsSocket;
	}

	disconnect(code, reason) {
		if (this.wsSocket === null) {
			return;
		}
		this.wsSocket.close(code, reason);
	}

	send(message) {
		if (!this.isConnected()) {
			return false;
		}
		this.wsSocket.send(message);
		return true;
	}

	listRooms() {
		return this.request('GET', '/api/v1/room/list', null)
			.then((json) => {
				this.emit('onRoomList', {
					response: json,
					rooms: Array.isArray(json && json.list) ? json.list : [],
					manager: this,
					state: this.getState()
				});
				return json;
			});
	}

	createRoom(roomName) {
		const body = this.buildFormBody({ name: roomName || '' });
		return this.request('POST', '/api/v1/room/create', body)
			.then((json) => {
				this.emit('onRoomCreate', {
					response: json,
					roomId: json && json.id ? json.id : '',
					manager: this,
					state: this.getState()
				});
				return json;
			});
	}

	joinRoom(roomId, connectionId) {
		const conId = connectionId || this.selfConnectionId;
		const body = this.buildFormBody({
			room_id: roomId || '',
			connection_id: conId || ''
		});

		return this.request('POST', '/api/v1/room/join', body)
			.then((json) => {
				this.currentJoinRoomId = roomId || '';
				this.emit('onRoomJoin', {
					response: json,
					roomId: this.currentJoinRoomId,
					connectionId: conId,
					manager: this,
					state: this.getState()
				});
				return json;
			});
	}

	leaveRoom(roomId, connectionId) {
		const targetRoomId = roomId || this.currentJoinRoomId;
		const conId = connectionId || this.selfConnectionId;
		const body = this.buildFormBody({
			room_id: targetRoomId || '',
			connection_id: conId || ''
		});

		return this.request('POST', '/api/v1/room/leave', body)
			.then((json) => {
				if (targetRoomId === this.currentJoinRoomId) {
					this.currentJoinRoomId = '';
				}
				this.emit('onRoomLeave', {
					response: json,
					roomId: targetRoomId,
					connectionId: conId,
					manager: this,
					state: this.getState()
				});
				return json;
			});
	}

	request(method, path, body) {
		return new Promise((resolve, reject) => {
			const xhr = new XMLHttpRequest();
			xhr.open(method, this.httpBase + path, true);
			if (method === 'POST') {
				xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
			}

			xhr.onreadystatechange = () => {
				if (xhr.readyState !== 4) {
					return;
				}

				const responseText = xhr.responseText || '';
				if (xhr.status < 200 || xhr.status >= 300) {
					const error = {
						type: 'http_error',
						status: xhr.status,
						path,
						method,
						body,
						responseText
					};
					this.emit('onHttpError', {
						error,
						manager: this,
						state: this.getState()
					});
					reject(error);
					return;
				}

				let json = null;
				try {
					json = responseText.length > 0 ? JSON.parse(responseText) : {};
				} catch (error) {
					const parseError = {
						type: 'http_parse_error',
						error,
						status: xhr.status,
						path,
						method,
						body,
						responseText
					};
					this.emit('onParseError', {
						error: parseError,
						source: 'http_response',
						data: responseText,
						manager: this,
						state: this.getState()
					});
					reject(parseError);
					return;
				}

				resolve(json);
			};

			xhr.onerror = () => {
				const error = {
					type: 'network_error',
					status: xhr.status,
					path,
					method,
					body
				};
				this.emit('onHttpError', {
					error,
					manager: this,
					state: this.getState()
				});
				reject(error);
			};

			xhr.send(body);
		});
	}

	buildFormBody(params) {
		const keys = Object.keys(params || {});
		return keys
			.map((key) => encodeURIComponent(key) + '=' + encodeURIComponent(params[key]))
			.join('&');
	}

	emit(name, payload) {
		const callback = this.callbacks[name];
		if (typeof callback !== 'function') {
			return;
		}
		try {
			callback(payload);
		} catch (error) {
			const fallback = this.callbacks.onCallbackError;
			if (typeof fallback === 'function') {
				fallback({
					error,
					callbackName: name,
					payload,
					manager: this,
					state: this.getState()
				});
			}
		}
	}

	// ─── STT / WAV recording ───────────────────────────────────────────

	/**
	 * Build a WAV file ArrayBuffer from Int16 PCM samples.
	 * @param {Int16Array} int16Array - raw PCM samples (mono)
	 * @param {number} sampleRate     - sample rate in Hz (e.g. 16000)
	 * @returns {ArrayBuffer}
	 */
	buildWavBuffer(int16Array, sampleRate) {
		const numChannels = 1;
		const bitsPerSample = 16;
		const byteRate = sampleRate * numChannels * (bitsPerSample / 8);
		const blockAlign = numChannels * (bitsPerSample / 8);
		const dataSize = int16Array.length * 2; // bytes
		const buffer = new ArrayBuffer(44 + dataSize);
		const view = new DataView(buffer);
		const writeStr = (offset, str) => {
			for (let i = 0; i < str.length; i++) {
				view.setUint8(offset + i, str.charCodeAt(i));
			}
		};
		// RIFF chunk
		writeStr(0, 'RIFF');
		view.setUint32(4, 36 + dataSize, true);
		writeStr(8, 'WAVE');
		// fmt chunk
		writeStr(12, 'fmt ');
		view.setUint32(16, 16, true);            // chunk size
		view.setUint16(20, 1, true);             // PCM
		view.setUint16(22, numChannels, true);
		view.setUint32(24, sampleRate, true);
		view.setUint32(28, byteRate, true);
		view.setUint16(32, blockAlign, true);
		view.setUint16(34, bitsPerSample, true);
		// data chunk
		writeStr(36, 'data');
		view.setUint32(40, dataSize, true);
		new Int16Array(buffer, 44).set(int16Array);
		return buffer;
	}

	/**
	 * Nearest-neighbor downsample Float32 audio to targetRate.
	 */
	downsampleTo(audioData, sourceSampleRate, targetRate) {
		if (sourceSampleRate === targetRate) return audioData;
		const ratio = sourceSampleRate / targetRate;
		const out = new Float32Array(Math.floor(audioData.length / ratio));
		for (let i = 0; i < out.length; i++) {
			out[i] = audioData[Math.floor(i * ratio)];
		}
		return out;
	}

	/** Convert Float32[-1,1] to Int16 PCM. */
	floatToInt16(floatArray) {
		const out = new Int16Array(floatArray.length);
		for (let i = 0; i < floatArray.length; i++) {
			const s = Math.max(-1, Math.min(1, floatArray[i]));
			out[i] = s < 0 ? s * 0x8000 : s * 0x7FFF;
		}
		return out;
	}

	/**
	 * Start microphone capture.
	 * Collects PCM data in memory; call stopSttRecording() to send WAV.
	 * @param {number} targetSampleRate - target Hz for WAV (default 16000)
	 */
	async startSttRecording(targetSampleRate = 16000) {
		if (this._sttRecording) {
			console.warn('[STT] already recording');
			return;
		}
		if (!this.isConnected()) {
			console.error('[STT] WebSocket not connected');
			return;
		}
		this._sttTargetRate = targetSampleRate;
		this._sttPcmChunks = [];

		this._sttStream = await navigator.mediaDevices.getUserMedia({
			audio: { echoCancellation: true, noiseSuppression: true, autoGainControl: true }
		});

		this._sttAudioCtx = new (window.AudioContext || window.webkitAudioContext)();
		const source = this._sttAudioCtx.createMediaStreamSource(this._sttStream);

		// Inline AudioWorklet processor
		const processorCode = `
class PcmCapture extends AudioWorkletProcessor {
	process(inputs) {
		const ch = inputs[0][0];
		if (ch) this.port.postMessage(Array.from(ch));
		return true;
	}
}
registerProcessor('pcm-capture', PcmCapture);
`;
		const blob = new Blob([processorCode], { type: 'application/javascript' });
		const url = URL.createObjectURL(blob);
		await this._sttAudioCtx.audioWorklet.addModule(url);
		URL.revokeObjectURL(url);

		this._sttWorklet = new AudioWorkletNode(this._sttAudioCtx, 'pcm-capture');
		this._sttWorklet.port.onmessage = (e) => {
			if (!this._sttRecording) return;
			const f32 = new Float32Array(e.data);
			const resampled = this.downsampleTo(f32, this._sttAudioCtx.sampleRate, this._sttTargetRate);
			this._sttPcmChunks.push(this.floatToInt16(resampled));
		};

		source.connect(this._sttWorklet);
		this._sttRecording = true;
		this.emit('onSttStart', { manager: this, state: this.getState() });
		console.log(`[STT] recording started (target: ${targetSampleRate} Hz)`);
	}

	/**
	 * Stop recording and send the captured audio as a WAV binary WebSocket message.
	 * @returns {number} bytes sent, or 0 on error
	 */
	stopSttRecording() {
		if (!this._sttRecording) {
			console.warn('[STT] not recording');
			return 0;
		}
		this._sttRecording = false;

		if (this._sttWorklet) { this._sttWorklet.disconnect(); this._sttWorklet = null; }
		if (this._sttStream) { this._sttStream.getTracks().forEach(t => t.stop()); this._sttStream = null; }
		if (this._sttAudioCtx) { this._sttAudioCtx.close(); this._sttAudioCtx = null; }

		// Concatenate all PCM chunks
		const totalSamples = this._sttPcmChunks.reduce((s, c) => s + c.length, 0);
		const pcm = new Int16Array(totalSamples);
		let offset = 0;
		for (const chunk of this._sttPcmChunks) {
			pcm.set(chunk, offset);
			offset += chunk.length;
		}
		this._sttPcmChunks = [];

		if (totalSamples === 0) {
			console.warn('[STT] no audio captured');
			return 0;
		}

		const wavBuffer = this.buildWavBuffer(pcm, this._sttTargetRate);
		if (this.isConnected()) {
			this.wsSocket.send(wavBuffer);
			console.log(`[STT] WAV sent: ${wavBuffer.byteLength} bytes`);
			this.emit('onSttStop', { bytesSent: wavBuffer.byteLength, manager: this, state: this.getState() });
			return wavBuffer.byteLength;
		}
		console.error('[STT] WebSocket disconnected, WAV not sent');
		return 0;
	}
}

window.RoomServerManager = RoomServerManager;
