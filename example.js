const path = require('path');
const fs = require('fs');
const {AudioContext, Audio, MicrophoneMediaStream} = require('.');

const audioContext = new AudioContext();

const microphoneMediaStream = new MicrophoneMediaStream();
const audioCtx = new AudioContext();
const audioSourceNode = audioCtx.createMediaStreamSource(stream);
audioSourceNode.connect(audioCtx.destination);

/* fs.readFile(path.join(__dirname, 'labsound', 'assets', 'samples', 'stereo-music-clip.wav'), (err, data) => {
  if (!err) {
    console.log('create');

    const audio = new Audio();

    console.log('load');

    audio.load(data, 'wav');
    
    const audioSourceNode = audioContext.createMediaElementSource(audio);
    const gainNode = audioContext.createGain();
    gainNode.connect(audioContext.destination);
    const pannerNode = audioContext.createPanner();
    pannerNode.connect(gainNode);
    audioSourceNode.connect(pannerNode);

    console.log('play');

    audio.play();

    console.log('set timeout');

    setTimeout(() => {
      audio.pause();

      setTimeout(() => {}, 1000);
    }, 2000);
  } else {
    throw err;
  }
}); */
