var fs  = require('fs');
var Jpeg = require('../build/Release/jpeg').Jpeg;
var Buffer = require('buffer').Buffer;

var WIDTH = 400, HEIGHT = 300, QUALITY = 100;

var rgba = new Buffer(WIDTH*HEIGHT*3);

for (var i=0; i<HEIGHT; i++) {
    for (var j=0; j<WIDTH; j++) {
        rgba[i*WIDTH*3 + j*3 + 0] = 255*j/WIDTH;
        rgba[i*WIDTH*3 + j*3 + 1] = 255*i/HEIGHT;
        rgba[i*WIDTH*3 + j*3 + 2] = 0xff/2;
    }
}

var jpeg = new Jpeg(rgba, WIDTH, HEIGHT, QUALITY, 'rgb');
var jpeg_img = jpeg.encodeSync().toString('binary');

fs.writeFileSync('./jpeg-gradient.jpeg', jpeg_img, 'binary');

