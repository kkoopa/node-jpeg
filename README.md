This is a node.js module, written in C++, that uses libjpeg to produce a JPEG
image (in memory) from a buffer of RGBA or RGB values. Since JPEG has no notion
of A (alpha), the module always uses just RGB values.

It was written by Peteris Krumins (peter@catonmat.net).
His blog is at http://www.catonmat.net  --  good coders code, great reuse.

------------------------------------------------------------------------------

The module exports three objects: `Jpeg`, `FixedJpegStack`, `DynamicJpegStack`.

Jpeg allows to create fixed size jpegs from *RGB*, *BGR*, *RGBA* or *BGRA* buffers.
`FixedJpegStack` allows to push multiple jpegs to a fixed size canvas.
`DynamicJpegStack` allows to push multiple jpegs to a dynamic size canvas (it grows as you push jpegs to it).

All objects provide synchronous and asynchronous interfaces.

#Jpeg

`Jpeg` object that takes 5 arguments in its constructor:

```js
var jpeg = new Jpeg(buffer, width, height, quality, [buffer_type]);
```

The first argument, `buffer`, is a node.js `Buffer` filled with *RGBA* or *RGB* values.
The second argument is integer width of the image.
The third argument is integer height of the image.
The fourth argument is integer quality of the image in range [0, 100].
The fifth argument is buffer type, either `'rgb'` or `'rgba'`. [Optional].

After you have constructed the object, call `.encode()` or `.encodeSync()` to produce a jpeg:
```js
var jpeg_image = jpeg.encodeSync(); // synchronous encoding (blocks node.js)
```
Or:
```js
jpeg.encode(function (image, error) {
    // jpeg image is in 'image'
});
```

See `examples/` directory for examples.

#FixedJpegStack

First you create a `FixedJpegStack` object of fixed width and height:
```js
var stack = new FixedJpegStack(width, height,[buffer_type]);
```
Then you can push individual fragments to it, for example,
```js
stack.push(buf1, 10, 11, 100, 200); // pushes buf1 to (x,y)=(10,11)
                                    // 100 and 200 are width and height.

// more pushes
```
You can set the quality by calling `setQuality`:
```js
stack.setQuality(90);
```

After you're done, call `.encode()` to produce final jpeg asynchronously or
`.encodeSync()` (just like in Jpeg object). The final jpeg will be of size
width x height.


#DynamicJpegStack

`DynamicJpegStack` is the same as `FixedJpegStack` except its canvas grows dynamically.

First, create the stack:
```js
var stack = new DynamicJpegStack([buffer_type]);
```
Next push the RGB(A) buffers to it:
```js
stack.push(buf1, 5, 10, 100, 40);
stack.push(buf2, 2, 210, 20, 20);
```
You can set the quality by calling `setQuality`:
```js
stack.setQuality(90);
```
Now you can call `encode` to produce the final jpeg:
```js
var jpeg = stack.encodeSync();
```
Now let's see what the dimensions are,
```js
var dims = stack.dimensions();
```
Same asynchronously:
```js
stack.encode(function (jpeg, dims) {
    // jpeg is the image
    // dims are its dimensions
});
```

In this particular example:

The x position `dims.x` is 2 because the 2nd jpeg is closer to the left.
The y position `dims.y` is 10 because the 1st jpeg is closer to the top.
The width `dims.width` is 103 because the first jpeg stretches from x=5 to
x=105, but the 2nd jpeg starts only at x=2, so the first two pixels are not
necessary and the width is 105-2=103.
The height `dims.height` is 220 because the 2nd jpeg is located at 210 and
its height is 20, so it stretches to position 230, but the first jpeg starts
at 10, so the upper 10 pixels are not necessary and height becomes 230-10= 220.


#How to install?

To get it compiled, you need to have libjpeg and node installed. Then just run
```bash
node-gyp rebuild
```
to build the Jpeg module. It will produce a `jpeg.node` file as the module.

------------------------------------------------------------------------------

Have fun!


Sincerely,
Peteris Krumins
http://www.catonmat.net

