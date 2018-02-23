const path = require('path');
const fs = require('fs');
const {AudioContext, Audio} = require('.');

const audioContext = new AudioContext();

fs.readFile(path.join(__dirname, 'labsound', 'assets', 'samples', 'stereo-music-clip.wav'), (err, data) => {
  if (!err) {
    console.log('create');

    const audio = new Audio();

    console.log('load');

    audio.load(data, 'wav');
    
    const audioSourceNode = audioContext.createMediaElementSource(audio);
    const audioDestinationNode = audioContext.destination;
    audioSourceNode.connect(audioDestinationNode, 0, 0);

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
});