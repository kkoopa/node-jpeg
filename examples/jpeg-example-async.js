var fs  = require('fs');
var Jpeg = require('../build/Release/jpeg').Jpeg;
var Buffer = require('buffer').Buffer;

var rgba = fs.readFileSync('./rgba-terminal.dat');

var jpeg = new Jpeg(rgba, 720, 400, 100, 'rgba');
jpeg.encode(function (image) {
    fs.writeFileSync('./jpeg-async.jpeg', image.toString('binary'), 'binary');
})

