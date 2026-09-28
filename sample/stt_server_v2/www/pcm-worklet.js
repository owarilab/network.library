class PcmWorklet extends AudioWorkletProcessor {
  constructor() {
    super();
    this.pending = new Float32Array(0);
  }

  process(inputs) {
    const channel = inputs[0] && inputs[0][0];
    if (!channel || channel.length === 0) return true;
    const merged = new Float32Array(this.pending.length + channel.length);
    merged.set(this.pending);
    merged.set(channel, this.pending.length);
    const frameLength = 4096;
    let offset = 0;
    while (merged.length - offset >= frameLength) {
      this.port.postMessage(merged.slice(offset, offset + frameLength));
      offset += frameLength;
    }
    this.pending = merged.slice(offset);
    return true;
  }
}

registerProcessor('pcm-worklet', PcmWorklet);
