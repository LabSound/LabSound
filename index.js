const path = require('path');
const bindings = require('./build/Release/webaudio.node');

const {PannerNode} = bindings;
PannerNode.setPath(path.join(__dirname, 'labsound', 'assets', 'hrtf'));

module.exports = bindings;