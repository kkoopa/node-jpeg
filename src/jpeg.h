#ifndef JPEG_H
#define JPEG_H

#include <node.h>
#include <node_buffer.h>

#include "jpeg_encoder.h"

class Jpeg : public node::ObjectWrap {
    JpegEncoder jpeg_encoder;

    class JpegEncodeWorker : public JpegEncoder::EncodeWorker {
    public:
        JpegEncodeWorker(NanCallback *callback, Jpeg *jpeg) : EncodeWorker(callback), jpeg_obj(jpeg) {
        };

        void Execute();
        void HandleOKCallback();

    private:
        Jpeg *jpeg_obj;
    };

public:
    static void Initialize(v8::Handle<v8::Object> target);
    Jpeg(unsigned char *ddata, int wwidth, int hheight, int qquality, buffer_type bbuf_type);
    v8::Handle<v8::Value> JpegEncodeSync();
    void SetQuality(int q);
    void SetSmoothing(int s);

    static NAN_METHOD(New);
    static NAN_METHOD(JpegEncodeSync);
    static NAN_METHOD(JpegEncodeAsync);
    static NAN_METHOD(SetQuality);
    static NAN_METHOD(SetSmoothing);
};

#endif

