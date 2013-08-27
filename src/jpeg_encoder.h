#ifndef JPEG_ENCODER_H
#define JPEG_ENCODER_H

#include <cstdio>
#include <cstdlib>
#include <jpeglib.h>
#include "common.h"

class JpegEncoder {
    unsigned char *data;
    int width, height, quality, smoothing;
    buffer_type buf_type;

    unsigned char *jpeg;
    long unsigned int jpeg_len;

    Rect offset;

public:
    JpegEncoder(unsigned char *ddata, int wwidth, int hheight,
        int qquality, buffer_type bbuf_type);
    ~JpegEncoder();

    class EncodeWorker : public NanAsyncWorker {
    public:
        EncodeWorker(NanCallback *callback) : NanAsyncWorker(callback) {
              jpeg = NULL;
              jpeg_len = 0;
        };

    protected:
        char *jpeg;
        int jpeg_len;
    };

    void encode();
    void set_quality(int qquality);
    void set_smoothing(int ssmoothing);
    const unsigned char *get_jpeg() const;
    unsigned int get_jpeg_len() const;

    void setRect(const Rect &r);
};

#endif
