class SttManager {
	constructor(options) {
		const opts = options || {};
		this.wsBase = opts.wsBase || ((window.location.protocol === 'https:' ? 'wss://' : 'ws://') + window.location.host);
		this.wsSocket = null;
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

	isConnected() {
		return this.wsSocket !== null && this.wsSocket.readyState === WebSocket.OPEN;
	}

	connect() {
		if (this.wsSocket !== null && this.wsSocket.readyState !== WebSocket.CLOSED) {
			return this.wsSocket;
		}

		this.wsSocket = new WebSocket(this.wsBase);
		this.wsSocket.binaryType = 'arraybuffer';
		this.wsSocket.addEventListener('open', () => {
			this.emit('onWsOpen');
		});

		this.wsSocket.addEventListener('message', (event) => {
			this.emit('onWsRawMessage', { data: event.data });
		});

		this.wsSocket.addEventListener('close', () => {
			this.emit('onWsClose');
		});

		this.wsSocket.addEventListener('error', () => {
			this.emit('onWsError');
		});

		return this.wsSocket;
	}

	disconnect(code, reason) {
		if (this.wsSocket === null) return;
		this.wsSocket.close(code, reason);
	}

	send(message) {
		if (!this.isConnected()) return false;
		this.wsSocket.send(message);
		return true;
	}

	emit(name, payload) {
		const callback = this.callbacks[name];
		if (typeof callback !== 'function') return;
		try {
			callback(payload);
		} catch (error) {
			const fallback = this.callbacks.onCallbackError;
			if (typeof fallback === 'function') {
				fallback({ error, callbackName: name });
			}
		}
	}

	// ─── STT / WAV recording ───────────────────────────────────────────

	downsampleTo(audioData, sourceSampleRate, targetRate) {
		if (sourceSampleRate === targetRate) return audioData;
		const ratio = sourceSampleRate / targetRate;
		const out = new Float32Array(Math.floor(audioData.length / ratio));
		for (let i = 0; i < out.length; i++) {
			out[i] = audioData[Math.floor(i * ratio)];
		}
		return out;
	}

	floatToInt16(floatArray) {
		const out = new Int16Array(floatArray.length);
		for (let i = 0; i < floatArray.length; i++) {
			const s = Math.max(-1, Math.min(1, floatArray[i]));
			out[i] = s < 0 ? s * 0x8000 : s * 0x7FFF;
		}
		return out;
	}

	appendSttPcmChunk(int16Chunk) {
		if (!int16Chunk || int16Chunk.length === 0) return;
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
		if (this._sttFlushTimer) clearInterval(this._sttFlushTimer);
		this._sttFlushTimer = setInterval(() => {
			this.flushSttPcmChunks();
		}, this._sttFlushIntervalMs);
	}

	flushSttPcmChunks() {
		const fullPcm = this.collectSttPcmChunks();
		if (fullPcm.length === 0) return { sentCount: 0, totalBytesSent: 0 };
		return this.sendSttPcmChunks(fullPcm);
	}

	sendSttControlMessage(type, extra = {}) {
		if (!this.isConnected()) return false;
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
		}

		return { sentCount, totalBytesSent };
	}

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
			audio: {
				echoCancellation: false,
				noiseSuppression: false,
				autoGainControl: false,
				channelCount: 1,
				sampleRate: targetSampleRate
			}
		});

		this._sttAudioCtx = new (window.AudioContext || window.webkitAudioContext)();
		const source = this._sttAudioCtx.createMediaStreamSource(this._sttStream);

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
		this.emit('onSttStart');
		console.log(`[STT] recording started (target: ${targetSampleRate} Hz)`);
	}

	stopSttRecording() {
		if (!this._sttRecording) {
			console.warn('[STT] not recording');
			return 0;
		}
		this._sttRecording = false;
		this.releaseSttResources();
		const result = this.flushSttPcmChunks();
		this.sendSttControlMessage('stt_stop');
		console.log(`[STT] Finished: Sent ${result.sentCount} PCM chunks, total ${result.totalBytesSent} bytes.`);
		this.emit('onSttStop', {
			bytesSent: result.totalBytesSent,
			chunkCount: result.sentCount
		});
		return result.totalBytesSent;
	}
}

window.SttManager = SttManager;
