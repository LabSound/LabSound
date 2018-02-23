const path = require('path');
const fs = require('fs');
const WebAudio = require('.');
console.log('got web audio', WebAudio);
const {Audio} = WebAudio;

fs.readFile(path.join(__dirname, 'labsound', 'assets', 'samples', 'stereo-music-clip.wav'), (err, data) => {
  if (!err) {
    console.log('create');
    const audio = new Audio();
    console.log('load');
    audio.load(data, 'wav');
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