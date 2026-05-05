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
		this.sttChunkMaxBytes = opts.sttChunkMaxBytes || (16 * 1024);
		this._sttRecording = false;
		this._sttTargetRate = 16000;
		this._sttPcmChunks = [];
		this._sttStream = null;
		this._sttAudioCtx = null;
		this._sttWorklet = null;
		this._sttFlushIntervalMs = opts.sttFlushIntervalMs || 500;
		this._sttFlushTimer = null;
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

	appendSttPcmChunk(int16Chunk) {
		if (!int16Chunk || int16Chunk.length === 0) {
			return;
		}
		this._sttPcmChunks.push(int16Chunk);
	}

	collectSttPcmChunks() {
		const totalSamples = this._sttPcmChunks.reduce((acc, chunk) => acc + chunk.length, 0);
		const fullPcm = new Int16Array(totalSamples);
		let offset = 0;
		for (const chunk of this._sttPcmChunks) {
			fullPcm.set(chunk, offset);
			offset += chunk.length;
		}
		this._sttPcmChunks = [];
		return fullPcm;
	}

	releaseSttResources() {
		if (this._sttFlushTimer) {
			clearInterval(this._sttFlushTimer);
			this._sttFlushTimer = null;
		}
		if (this._sttWorklet) {
			this._sttWorklet.disconnect();
			this._sttWorklet = null;
		}
		if (this._sttStream) {
			this._sttStream.getTracks().forEach((track) => track.stop());
			this._sttStream = null;
		}
		if (this._sttAudioCtx) {
			this._sttAudioCtx.close();
			this._sttAudioCtx = null;
		}
	}

	startSttFlushTimer() {
		if (this._sttFlushTimer) {
			clearInterval(this._sttFlushTimer);
		}
		this._sttFlushTimer = setInterval(() => {
			this.flushSttPcmChunks();
		}, this._sttFlushIntervalMs);
	}

	flushSttPcmChunks() {
		let totalBytesSent = 0;
		let chunkCount = 0;
		const fullPcm = this.collectSttPcmChunks();

		if (fullPcm.length === 0) {
			return { sentCount: 0, totalBytesSent: 0 };
		}

		const result = this.sendSttPcmChunks(fullPcm);
		totalBytesSent += result.totalBytesSent;
		chunkCount += result.sentCount;
		if (result.sentCount > 0) {
			console.log(`[STT] flush sent ${result.sentCount} chunks, ${result.totalBytesSent} bytes`);
		}
		return { sentCount: chunkCount, totalBytesSent };
	}

	sendSttControlMessage(type, extra = {}) {
		if (!this.isConnected()) {
			return false;
		}
		this.wsSocket.send(JSON.stringify(Object.assign({ type }, extra)));
		return true;
	}

	sendSttPcmChunks(fullPcm) {
		const samplesPerSlice = Math.floor(this.sttChunkMaxBytes / 2);
		let sentCount = 0;
		let totalBytesSent = 0;

		if (samplesPerSlice <= 0) {
			console.error('[STT] invalid sttChunkMaxBytes:', this.sttChunkMaxBytes);
			return { sentCount: 0, totalBytesSent: 0 };
		}

		for (let i = 0; i < fullPcm.length; i += samplesPerSlice) {
			const slice = fullPcm.subarray(i, i + samplesPerSlice);
			const pcmBuffer = slice.buffer.slice(slice.byteOffset, slice.byteOffset + slice.byteLength);

			if (!this.isConnected()) {
				console.error('[STT] WebSocket disconnected mid-transmission');
				break;
			}

			this.wsSocket.send(pcmBuffer);
			sentCount++;
			totalBytesSent += pcmBuffer.byteLength;
			console.log(`[STT] pcm chunk ${sentCount}: ${pcmBuffer.byteLength} bytes sent`);
		}

		return { sentCount, totalBytesSent };
	}

	/**
	 * Start microphone capture.
	 * Collects PCM data in memory; call stopSttRecording() to send PCM chunks.
	 * @param {number} targetSampleRate - target Hz for PCM (default 16000)
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
			this.appendSttPcmChunk(this.floatToInt16(resampled));
		};

		source.connect(this._sttWorklet);
		this._sttRecording = true;
		this.sendSttControlMessage('stt_init', {
			sample_rate: this._sttTargetRate,
			channels: 1,
			bits_per_sample: 16
		});
		this.startSttFlushTimer();
		this.emit('onSttStart', { manager: this, state: this.getState() });
		console.log(`[STT] recording started (target: ${targetSampleRate} Hz)`);
	}

	/**
	 * Stop recording and send the captured audio as raw PCM binary chunks.
	 * @returns {number} bytes sent, or 0 on error
	 */
	stopSttRecording() {
		let result;
		if (!this._sttRecording) {
			console.warn('[STT] not recording');
			return 0;
		}
		this._sttRecording = false;
		this.releaseSttResources();
		result = this.flushSttPcmChunks();
		this.sendSttControlMessage('stt_stop');
		console.log(`[STT] Finished: Sent ${result.sentCount} PCM chunks, total ${result.totalBytesSent} bytes.`);
		this.emit('onSttStop', {
			bytesSent: result.totalBytesSent,
			chunkCount: result.sentCount,
			manager: this,
			state: this.getState()
		});
		return result.totalBytesSent;
	}
}

window.RoomServerManager = RoomServerManager;
