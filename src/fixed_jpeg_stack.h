#ifndef STACKED_JPEG_H
#define STACKED_JPEG_H

#include <node.h>
#include <node_buffer.h>

#include "common.h"
#include "jpeg_encoder.h"

class FixedJpegStack : public node::ObjectWrap {
    int width, height, quality;
    buffer_type buf_type;

    unsigned char *data;

    static void EIO_JpegEncode(uv_work_t *req);
    static int EIO_JpegEncodeAfter(uv_work_t *req);

public:
    static void Initialize(v8::Handle<v8::Object> target);
    FixedJpegStack(int wwidth, int hheight, buffer_type bbuf_type);
    v8::Handle<v8::Value> JpegEncodeSync();
    void Push(unsigned char *data_buf, int x, int y, int w, int h);
    void SetQuality(int q);

    class FixedJpegEncodeWorker : public JpegEncoder::EncodeWorker {
    public:
        FixedJpegEncodeWorker(NanCallback *callback, FixedJpegStack *jpeg) : JpegEncoder::EncodeWorker(callback), jpeg_obj(jpeg) {
        };

        void Execute();
        void HandleOKCallback();

    private:
        FixedJpegStack *jpeg_obj;
    };

    static NAN_METHOD(New);
    static NAN_METHOD(JpegEncodeSync);
    static NAN_METHOD(JpegEncodeAsync);
    static NAN_METHOD(Push);
    static NAN_METHOD(SetQuality);
};


#endif

