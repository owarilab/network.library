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
}

window.RoomServerManager = RoomServerManager;
