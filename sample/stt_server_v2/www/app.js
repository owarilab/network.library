(() => {
  const socketScheme = location.protocol === 'https:' ? 'wss:' : 'ws:';
  const socketUrl = `${socketScheme}//${location.hostname || 'localhost'}:8080/`;
  const connectionLabel = document.querySelector('#connection');
  const sessionState = document.querySelector('#sessionState');
  const audioState = document.querySelector('#audioState');
  const startButton = document.querySelector('#startButton');
  const stopButton = document.querySelector('#stopButton');
  const echoCancellation = document.querySelector('#echoCancellation');
  const noiseSuppression = document.querySelector('#noiseSuppression');
  const autoGainControl = document.querySelector('#autoGainControl');
  const clearButton = document.querySelector('#clearButton');
  const results = document.querySelector('#results');
  const emptyState = document.querySelector('#emptyState');
  const meterFill = document.querySelector('#meterFill');
  const timerLabel = document.querySelector('#timer');

  let socket;
  let stream;
  let audioContext;
  let source;
  let processor;
  let muteGain;
  let sessionId = 0;
  let recording = false;
  let stopping = false;
  let preparing = false;
  let startedAt = 0;
  let timerHandle;
  let nextChunk = [];
  let chunkSamples = 0;
  let lastResultId = 0;
  let micLevel = 0;

  function setConnection(state, text) {
    connectionLabel.dataset.state = state;
    connectionLabel.textContent = text;
  }

  function setButtons() {
    startButton.disabled = recording || stopping || preparing || !socket || socket.readyState !== WebSocket.OPEN;
    stopButton.disabled = !recording;
  }

  async function changeAudioConstraint(control, constraintName) {
    if (!stream) return;
    const track = stream.getAudioTracks()[0];
    const requestedValue = control.checked;
    if (!track || typeof track.applyConstraints !== 'function') {
      control.checked = !requestedValue;
      sessionState.textContent = 'このブラウザーでは録音中の設定変更に対応していません';
      return;
    }
    try {
      await track.applyConstraints({ [constraintName]: requestedValue });
      sessionState.textContent = 'マイク設定を変更しました';
    } catch {
      control.checked = !requestedValue;
      sessionState.textContent = 'マイクがこの設定変更を受け付けませんでした';
    }
  }

  function connect() {
    if (socket && (socket.readyState === WebSocket.OPEN || socket.readyState === WebSocket.CONNECTING)) return;
    socket = new WebSocket(socketUrl);
    socket.binaryType = 'arraybuffer';
    setConnection('offline', 'CONNECTING');
    socket.addEventListener('open', () => {
      setConnection('online', 'CONNECTED');
      sessionState.textContent = '接続しました';
      setButtons();
    });
    socket.addEventListener('message', onServerMessage);
    socket.addEventListener('close', () => {
      setConnection('offline', 'OFFLINE');
      if (recording || stopping) finishCapture();
      sessionState.textContent = '接続が終了しました';
      setButtons();
    });
    socket.addEventListener('error', () => {
      setConnection('error', 'CONNECTION ERROR');
      sessionState.textContent = 'WebSocketに接続できません';
    });
  }

  function onServerMessage(event) {
    let message;
    try { message = JSON.parse(event.data); }
    catch { return; }
    if (message.type === 'stt_ready') {
      sessionId = message.session_id;
      recording = true;
      preparing = false;
      stopping = false;
      startedAt = performance.now();
      setConnection('recording', 'RECORDING');
      sessionState.textContent = '録音中';
      audioState.textContent = 'MICROPHONE ACTIVE';
      echoCancellation.disabled = false;
      noiseSuppression.disabled = false;
      autoGainControl.disabled = false;
      timerHandle = window.setInterval(updateTimer, 200);
      setButtons();
      return;
    }
    if (message.type === 'stt_final') {
      appendResult(message);
      return;
    }
    if (message.type === 'stt_done') {
      if (message.session_id === sessionId) {
        finishCapture();
        sessionState.textContent = '文字起こしが完了しました';
      }
      return;
    }
    if (message.type === 'stt_error') {
      sessionState.textContent = `エラー: ${message.code || 'unknown'}`;
        if (recording) stopCapture(false);
        else if (stopping) finishCapture();
    }
  }

  function appendResult(message) {
    if (!Number.isFinite(message.result_id) || message.result_id <= lastResultId) return;
    lastResultId = message.result_id;
    emptyState.hidden = true;
    const row = document.createElement('li');
    row.className = 'result';
    const time = document.createElement('time');
    time.className = 'result-time';
    time.textContent = formatTime((message.start_sample || 0) / 16000);
    const text = document.createElement('p');
    text.className = 'result-text';
    text.textContent = message.text || '';
    row.append(time, text);
    results.append(row);
  }

  async function startCapture() {
    if (preparing || recording || stopping) return;
    if (!socket || socket.readyState !== WebSocket.OPEN) {
      connect();
      sessionState.textContent = 'WebSocket接続を待っています';
      return;
    }
    preparing = true;
    echoCancellation.disabled = true;
    noiseSuppression.disabled = true;
    autoGainControl.disabled = true;
    setButtons();
    try {
      stream = await navigator.mediaDevices.getUserMedia({
        audio: {
          channelCount: 1,
          echoCancellation: echoCancellation.checked,
          noiseSuppression: noiseSuppression.checked,
          autoGainControl: autoGainControl.checked
        }
      });
      audioContext = new AudioContext();
      await audioContext.audioWorklet.addModule('./pcm-worklet.js');
      source = audioContext.createMediaStreamSource(stream);
      processor = new AudioWorkletNode(audioContext, 'pcm-worklet');
      muteGain = audioContext.createGain();
      muteGain.gain.value = 0;
      processor.port.onmessage = (event) => sendResampled(event.data);
      source.connect(processor);
      processor.connect(muteGain).connect(audioContext.destination);
      nextChunk = [];
      chunkSamples = 0;
      socket.send(JSON.stringify({ type: 'stt_init', sample_rate: 16000, channels: 1, bits_per_sample: 16 }));
      sessionState.textContent = 'セッションを準備中';
      setButtons();
    } catch (error) {
      sessionState.textContent = `マイクを開始できません: ${error.message}`;
      audioState.textContent = 'MICROPHONE ERROR';
      disposeAudio();
      echoCancellation.disabled = false;
      noiseSuppression.disabled = false;
      autoGainControl.disabled = false;
      preparing = false;
      setButtons();
    }
  }

  function sendResampled(input) {
    if (!recording || !socket || socket.readyState !== WebSocket.OPEN) return;
    let inputPower = 0;
    for (const sample of input) inputPower += sample * sample;
    micLevel = Math.min(1, Math.sqrt(inputPower / input.length) * 3.2);
    const inputRate = audioContext.sampleRate;
    const ratio = inputRate / 16000;
    const outputLength = Math.floor(input.length / ratio);
    const pcm = new Int16Array(outputLength);
    for (let outputIndex = 0; outputIndex < outputLength; outputIndex++) {
      const position = outputIndex * ratio;
      const left = Math.floor(position);
      const fraction = position - left;
      const first = input[Math.min(left, input.length - 1)];
      const second = input[Math.min(left + 1, input.length - 1)];
      const sample = Math.max(-1, Math.min(1, first + (second - first) * fraction));
      pcm[outputIndex] = sample < 0 ? sample * 32768 : sample * 32767;
    }
    for (const sample of pcm) {
      nextChunk.push(sample);
      chunkSamples++;
    }
    while (chunkSamples >= 8000) {
      const frame = nextChunk.splice(0, 8000);
      socket.send(encodePcm(frame));
      chunkSamples -= 8000;
    }
  }

  function encodePcm(samples) {
    const bytes = new ArrayBuffer(samples.length * 2);
    const view = new DataView(bytes);
    samples.forEach((sample, index) => view.setInt16(index * 2, sample, true));
    return bytes;
  }

  function stopCapture(sendStop = true) {
    if (!recording || stopping) return;
    stopping = true;
    recording = false;
    if (chunkSamples > 0 && socket && socket.readyState === WebSocket.OPEN) {
      socket.send(encodePcm(nextChunk));
    }
    nextChunk = [];
    chunkSamples = 0;
    disposeAudio();
    if (sendStop && socket && socket.readyState === WebSocket.OPEN) {
      socket.send(JSON.stringify({ type: 'stt_stop' }));
      sessionState.textContent = '最後の音声区間を確定中';
    } else {
      finishCapture();
    }
    setButtons();
  }

  function finishCapture() {
    recording = false;
    stopping = false;
    preparing = false;
    sessionId = 0;
    disposeAudio();
    window.clearInterval(timerHandle);
    meterFill.style.width = '0%';
    audioState.textContent = 'MICROPHONE IDLE';
    echoCancellation.disabled = false;
    noiseSuppression.disabled = false;
    autoGainControl.disabled = false;
    if (connectionLabel.dataset.state !== 'offline' && connectionLabel.dataset.state !== 'error') {
      setConnection('online', 'CONNECTED');
    }
    setButtons();
  }

  function disposeAudio() {
    if (source) source.disconnect();
    if (processor) {
      processor.port.onmessage = null;
      processor.disconnect();
    }
    if (muteGain) muteGain.disconnect();
    if (stream) stream.getTracks().forEach((track) => track.stop());
    if (audioContext && audioContext.state !== 'closed') audioContext.close();
    source = processor = muteGain = stream = audioContext = null;
  }

  function updateTimer() {
    const elapsed = (performance.now() - startedAt) / 1000;
    timerLabel.textContent = formatTime(elapsed);
    meterFill.style.width = `${Math.round(micLevel * 100)}%`;
  }

  function formatTime(seconds) {
    const whole = Math.max(0, Math.floor(seconds));
    return `${String(Math.floor(whole / 60)).padStart(2, '0')}:${String(whole % 60).padStart(2, '0')}`;
  }

  startButton.addEventListener('click', startCapture);
  stopButton.addEventListener('click', () => stopCapture());
  echoCancellation.addEventListener('change', () => changeAudioConstraint(echoCancellation, 'echoCancellation'));
  noiseSuppression.addEventListener('change', () => changeAudioConstraint(noiseSuppression, 'noiseSuppression'));
  autoGainControl.addEventListener('change', () => changeAudioConstraint(autoGainControl, 'autoGainControl'));
  clearButton.addEventListener('click', () => {
    results.replaceChildren();
    results.append(emptyState);
    emptyState.hidden = false;
    lastResultId = 0;
  });
  connect();
})();
