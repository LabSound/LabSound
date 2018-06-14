const path = require('path');
const fs = require('fs');
Object.assign(global, require(path.join(__dirname, '..')));

const context = new AudioContext();

const WIDTH = 800;
const HEIGHT = 600;
var xPos = Math.floor(WIDTH/2);
var yPos = Math.floor(HEIGHT/2);
var zPos = 295;

var gain = context.createGain();
gain.connect(context.destination);
//gain.gain.value = 0.3;

var panner = context.createPanner();
panner.connect(gain);
panner.panningModel = 'HRTF';
panner.distanceModel = 'inverse';
panner.refDistance = 1;
panner.maxDistance = 10000;
panner.rolloffFactor = 1;
panner.coneInnerAngle = 360;
panner.coneOuterAngle = 0;
panner.coneOuterGain = 0;

var oscillator = context.createOscillator();    
oscillator.type = oscillator.SINE;
oscillator.frequency.value = 440;
oscillator.connect(panner);

var listener = context.listener;

if(listener.forwardX) {
  listener.forwardX.value = 0;
  listener.forwardY.value = 0;
  listener.forwardZ.value = -1;
  listener.upX.value = 0;
  listener.upY.value = 1;
  listener.upZ.value = 0;
} else {
  listener.setOrientation(0,0,-1,0,1,0);
}
listener.setPosition(0,0,0);

if(listener.positionX) {
  listener.positionX.value = xPos;
  listener.positionY.value = yPos;
  listener.positionZ.value = zPos;
} else {
  listener.setPosition(xPos,yPos,zPos);
}

function positionPanner() {
  if(panner.positionX) {
    panner.positionX.value = xPos + 10;
    panner.positionY.value = yPos;
    panner.positionZ.value = zPos;
  } else {
    panner.setPosition(xPos,yPos,zPos);
  }
}
positionPanner();
oscillator.start();

let tStart = Date.now();
let tSec = 0;

const _recurse = () => {
  let tNow = Date.now();
  let tSec2 = Math.floor((tNow - tStart)/1000);
  let t = (tNow - tStart)*0.001;
  let x = xPos + Math.cos(t);
  let y = yPos;
  let z = zPos + -Math.sin(t);
  if (tSec2 != tSec) {
    console.log({x, y, z, t, tSec, tSec2});
    tSec = tSec2;
  }
  panner.setPosition(x,y,z);
  process.nextTick(_recurse);
}
_recurse();

setTimeout(() => {}, 100000000);
