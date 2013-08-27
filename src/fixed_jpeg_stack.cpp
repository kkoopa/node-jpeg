#include <node.h>
#include <node_buffer.h>
#include <jpeglib.h>
#include <cstdlib>
#include <cstring>

#include "common.h"
#include "fixed_jpeg_stack.h"
#include "jpeg_encoder.h"

using namespace v8;
using namespace node;

void
FixedJpegStack::Initialize(v8::Handle<v8::Object> target)
{
    NanScope();

    Local<FunctionTemplate> t = FunctionTemplate::New(New);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    NODE_SET_PROTOTYPE_METHOD(t, "encode", JpegEncodeAsync);
    NODE_SET_PROTOTYPE_METHOD(t, "encodeSync", JpegEncodeSync);
    NODE_SET_PROTOTYPE_METHOD(t, "push", Push);
    NODE_SET_PROTOTYPE_METHOD(t, "setQuality", SetQuality);
    target->Set(String::NewSymbol("FixedJpegStack"), t->GetFunction());
}

FixedJpegStack::FixedJpegStack(int wwidth, int hheight, buffer_type bbuf_type) :
    width(wwidth), height(hheight), quality(60), buf_type(bbuf_type)
{
    data = (unsigned char *)calloc(width*height*3, sizeof(*data));
    if (!data) throw "calloc in FixedJpegStack::FixedJpegStack failed!";
}

Handle<Value>
FixedJpegStack::JpegEncodeSync()
{
    NanScope();

    try {
        JpegEncoder jpeg_encoder(data, width, height, quality, BUF_RGB);
        jpeg_encoder.encode();
        int jpeg_len = jpeg_encoder.get_jpeg_len();
        Local<Object> retbuf = NanNewBufferHandle(jpeg_len);
        memcpy(Buffer::Data(retbuf), jpeg_encoder.get_jpeg(), jpeg_len);
        return scope.Close(retbuf);
    }
    catch (const char *err) {
        return ThrowException(Exception::Error(String::New(err)));
    }
}

void
FixedJpegStack::Push(unsigned char *data_buf, int x, int y, int w, int h)
{
    int start = y*width*3 + x*3;

    switch (buf_type) {
    case BUF_RGB:
        for (int i = 0; i < h; i++) {
            unsigned char *datap = &data[start + i*width*3];
            for (int j = 0; j < w; j++) {
                *datap++ = *data_buf++;
                *datap++ = *data_buf++;
                *datap++ = *data_buf++;
            }
        }
        break;

    case BUF_BGR:
        for (int i = 0; i < h; i++) {
            unsigned char *datap = &data[start + i*width*3];
            for (int j = 0; j < w; j++) {
                *datap++ = *(data_buf+2);
                *datap++ = *(data_buf+1);
                *datap++ = *data_buf;
                data_buf+=3;
            }
        }
        break;

    case BUF_RGBA:
        for (int i = 0; i < h; i++) {
            unsigned char *datap = &data[start + i*width*3];
            for (int j = 0; j < w; j++) {
                *datap++ = *data_buf++;
                *datap++ = *data_buf++;
                *datap++ = *data_buf++;
                data_buf++;
            }
        }
        break;

    case BUF_BGRA:
        for (int i = 0; i < h; i++) {
            unsigned char *datap = &data[start + i*width*3];
            for (int j = 0; j < w; j++) {
                *datap++ = *(data_buf+2);
                *datap++ = *(data_buf+1);
                *datap++ = *data_buf;
                data_buf += 4;
            }
        }
        break;

    default:
        throw "Unexpected buf_type in FixedJpegStack::Push";
    }
}


void
FixedJpegStack::SetQuality(int q)
{
    quality = q;
}

NAN_METHOD(FixedJpegStack::New)
{
    NanScope();

    if (args.Length() < 2)
        return NanThrowError("At least two arguments required - width, height, [and buffer type]");
    if (!args[0]->IsInt32())
        return NanThrowTypeError("First argument must be integer width.");
    if (!args[1]->IsInt32())
        return NanThrowTypeError("Second argument must be integer height.");

    int w = args[0]->Int32Value();
    int h = args[1]->Int32Value();

    if (w < 0)
        return NanThrowRangeError("Width can't be negative.");
    if (h < 0)
        return NanThrowRangeError("Height can't be negative.");

    buffer_type buf_type = BUF_RGB;
    if (args.Length() == 3) {
        if (!args[2]->IsString())
            return NanThrowTypeError("Third argument must be a string. Either 'rgb', 'bgr', 'rgba' or 'bgra'.");

        String::AsciiValue bt(args[2]->ToString());
        if (!(str_eq(*bt, "rgb") || str_eq(*bt, "bgr") ||
            str_eq(*bt, "rgba") || str_eq(*bt, "bgra")))
        {
            return NanThrowTypeError("Buffer type must be 'rgb', 'bgr', 'rgba' or 'bgra'.");
        }

        if (str_eq(*bt, "rgb"))
            buf_type = BUF_RGB;
        else if (str_eq(*bt, "bgr"))
            buf_type = BUF_BGR;
        else if (str_eq(*bt, "rgba"))
            buf_type = BUF_RGBA;
        else if (str_eq(*bt, "bgra"))
            buf_type = BUF_BGRA;
        else
            return NanThrowTypeError("Buffer type wasn't 'rgb', 'bgr', 'rgba' or 'bgra'.");
    }

    try {
        FixedJpegStack *jpeg = new FixedJpegStack(w, h, buf_type);
        jpeg->Wrap(args.This());
        NanReturnValue(args.This());
    }
    catch (const char *err) {
        return NanThrowError(err);
    }
}

NAN_METHOD(FixedJpegStack::JpegEncodeSync)
{
    NanScope();
    FixedJpegStack *jpeg = ObjectWrap::Unwrap<FixedJpegStack>(args.This());
    NanReturnValue(jpeg->JpegEncodeSync());
}

NAN_METHOD(FixedJpegStack::Push)
{
    NanScope();

    if (!Buffer::HasInstance(args[0]))
        return NanThrowTypeError("First argument must be Buffer.");
    if (!args[1]->IsInt32())
        return NanThrowTypeError("Second argument must be integer x.");
    if (!args[2]->IsInt32())
        return NanThrowTypeError("Third argument must be integer y.");
    if (!args[3]->IsInt32())
        return NanThrowTypeError("Fourth argument must be integer w.");
    if (!args[4]->IsInt32())
        return NanThrowTypeError("Fifth argument must be integer h.");

    FixedJpegStack *jpeg = ObjectWrap::Unwrap<FixedJpegStack>(args.This());
    Local<Object> data_buf = args[0]->ToObject();
    int x = args[1]->Int32Value();
    int y = args[2]->Int32Value();
    int w = args[3]->Int32Value();
    int h = args[4]->Int32Value();

    if (x < 0)
        return NanThrowRangeError("Coordinate x smaller than 0.");
    if (y < 0)
        return NanThrowRangeError("Coordinate y smaller than 0.");
    if (w < 0)
        return NanThrowRangeError("Width smaller than 0.");
    if (h < 0)
        return NanThrowRangeError("Height smaller than 0.");
    if (x >= jpeg->width)
        return NanThrowRangeError("Coordinate x exceeds FixedJpegStack's dimensions.");
    if (y >= jpeg->height)
        return NanThrowRangeError("Coordinate y exceeds FixedJpegStack's dimensions.");
    if (x+w > jpeg->width)
        return NanThrowRangeError("Pushed fragment exceeds FixedJpegStack's width.");
    if (y+h > jpeg->height)
        return NanThrowRangeError("Pushed fragment exceeds FixedJpegStack's height.");

    jpeg->Push((unsigned char *)Buffer::Data(data_buf), x, y, w, h);

    NanReturnUndefined();
}

NAN_METHOD(FixedJpegStack::SetQuality)
{
    NanScope();

    if (args.Length() != 1)
        return NanThrowError("One argument required - quality");

    if (!args[0]->IsInt32())
        return NanThrowTypeError("First argument must be integer quality");

    int q = args[0]->Int32Value();

    if (q < 0)
        return NanThrowRangeError("Quality must be greater or equal to 0.");
    if (q > 100)
        return NanThrowRangeError("Quality must be less than or equal to 100.");

    FixedJpegStack *jpeg = ObjectWrap::Unwrap<FixedJpegStack>(args.This());
    jpeg->SetQuality(q);

    NanReturnUndefined();
}

void FixedJpegStack::FixedJpegEncodeWorker::Execute() {
    try {
        JpegEncoder encoder(jpeg_obj->data, jpeg_obj->width, jpeg_obj->height, jpeg_obj->quality, BUF_RGB);
        encoder.encode();
        jpeg_len = encoder.get_jpeg_len();
        jpeg = (char *)malloc(sizeof(*jpeg)*jpeg_len);
        if (!jpeg) {
            errmsg = strdup("malloc in FixedJpegStack::FixedJpegEncodeWorker::Execute() failed.");
        }
        else {
            memcpy(jpeg, encoder.get_jpeg(), jpeg_len);
        }
    }
    catch (const char *err) {
        errmsg = strdup(err);
    }
}

void FixedJpegStack::FixedJpegEncodeWorker::HandleOKCallback() {
    NanScope();

    Local<Object> buf = NanNewBufferHandle(jpeg_len);
    memcpy(Buffer::Data(buf), jpeg, jpeg_len);
    Local<Value> argv[2] = {buf, Undefined()};

    TryCatch try_catch; // don't quite see the necessity of this

    callback->Call(2, argv);

    if (try_catch.HasCaught()) {
        FatalException(try_catch);
    }

    free(jpeg);
    jpeg = NULL;

    jpeg_obj->Unref();
}

void FixedJpegStack::FixedJpegEncodeWorker::HandleErrorCallback() {
    NanScope();
    Local<Value> argv[2] = {Undefined(), v8::Exception::Error(v8::String::New(errmsg))};

    TryCatch try_catch; // don't quite see the necessity of this

    callback->Call(2, argv);

    if (try_catch.HasCaught()) {
        FatalException(try_catch);
    }

    if (jpeg) {
        free(jpeg);
        jpeg = NULL;
    }

    jpeg_obj->Unref();
}


NAN_METHOD(FixedJpegStack::JpegEncodeAsync)
{
    NanScope();

    if (args.Length() != 1)
        return NanThrowError("One argument required - callback function.");

    if (!args[0]->IsFunction())
        return NanThrowTypeError("First argument must be a function.");

    Local<Function> callback = Local<Function>::Cast(args[0]);
    FixedJpegStack *jpeg = ObjectWrap::Unwrap<FixedJpegStack>(args.This());

    NanAsyncQueueWorker(new FixedJpegStack::FixedJpegEncodeWorker(new NanCallback(callback), jpeg));

    jpeg->Ref();

    NanReturnUndefined();
}
