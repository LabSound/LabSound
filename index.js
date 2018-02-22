const path = require('path');
const fs = require('fs');
const WebAudio = require('./build/Release/webaudio.node');
console.log('got web audio', WebAudio);
const {HTMLAudioElement} = WebAudio;

fs.readFile(path.join(__dirname, 'labsound', 'assets', 'samples', 'stereo-music-clip.wav'), (err, data) => {
  if (!err) {
    console.log('create');
    const audioNode = new HTMLAudioElement();
    console.log('load');
    audioNode.load(data, 'wav');
    console.log('play');
    audioNode.play();
    console.log('set timeout');
    setTimeout(() => {
      audioNode.pause();

      setTimeout(() => {}, 1000);
    }, 2000);
  } else {
    throw err;
  }
});