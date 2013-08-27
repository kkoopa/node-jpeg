#include <node.h>
#include <node_buffer.h>
#include <jpeglib.h>
#include <cstdlib>
#include <cstring>

#include "common.h"
#include "dynamic_jpeg_stack.h"
#include "jpeg_encoder.h"

using namespace v8;
using namespace node;

void
DynamicJpegStack::Initialize(v8::Handle<v8::Object> target)
{
    NanScope();

    Local<FunctionTemplate> t = FunctionTemplate::New(New);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    NODE_SET_PROTOTYPE_METHOD(t, "encode", JpegEncodeAsync);
    NODE_SET_PROTOTYPE_METHOD(t, "encodeSync", JpegEncodeSync);
    NODE_SET_PROTOTYPE_METHOD(t, "push", Push);
    NODE_SET_PROTOTYPE_METHOD(t, "reset", Reset);
    NODE_SET_PROTOTYPE_METHOD(t, "setBackground", SetBackground);
    NODE_SET_PROTOTYPE_METHOD(t, "setQuality", SetQuality);
    NODE_SET_PROTOTYPE_METHOD(t, "dimensions", Dimensions);
    target->Set(String::NewSymbol("DynamicJpegStack"), t->GetFunction());
}

DynamicJpegStack::DynamicJpegStack(buffer_type bbuf_type) :
    quality(60), buf_type(bbuf_type),
    dyn_rect(-1, -1, 0, 0),
    bg_width(0), bg_height(0), data(NULL) {}

DynamicJpegStack::~DynamicJpegStack()
{
    free(data);
}

void
DynamicJpegStack::update_optimal_dimension(int x, int y, int w, int h)
{
    if (dyn_rect.x == -1 || x < dyn_rect.x)
        dyn_rect.x = x;
    if (dyn_rect.y == -1 || y < dyn_rect.y)
        dyn_rect.y = y;

    if (dyn_rect.w == 0)
        dyn_rect.w = w;
    if (dyn_rect.h == 0)
        dyn_rect.h = h;

    int ww = w - (dyn_rect.w - (x - dyn_rect.x));
    if (ww > 0)
        dyn_rect.w += ww;

    int hh = h - (dyn_rect.h - (y - dyn_rect.y));
    if (hh > 0)
        dyn_rect.h += hh;
}

Handle<Value>
DynamicJpegStack::JpegEncodeSync()
{
    NanScope();

    try {
        JpegEncoder jpeg_encoder(data, bg_width, bg_height, quality, BUF_RGB);
        jpeg_encoder.setRect(Rect(dyn_rect.x, dyn_rect.y, dyn_rect.w, dyn_rect.h));
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
DynamicJpegStack::Push(unsigned char *data_buf, int x, int y, int w, int h)
{
    update_optimal_dimension(x, y, w, h);

    int start = y*bg_width*3 + x*3;

    switch (buf_type) {
    case BUF_RGB:
        for (int i = 0; i < h; i++) {
            unsigned char *datap = &data[start + i*bg_width*3];
            for (int j = 0; j < w; j++) {
                *datap++ = *data_buf++;
                *datap++ = *data_buf++;
                *datap++ = *data_buf++;
            }
        }
        break;

    case BUF_BGR:
        for (int i = 0; i < h; i++) {
            unsigned char *datap = &data[start + i*bg_width*3];
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
            unsigned char *datap = &data[start + i*bg_width*3];
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
            unsigned char *datap = &data[start + i*bg_width*3];
            for (int j = 0; j < w; j++) {
                *datap++ = *(data_buf+2);
                *datap++ = *(data_buf+1);
                *datap++ = *data_buf;
                data_buf += 4;
            }
        }
        break;

    default:
        throw "Unexpected buf_type in DynamicJpegStack::Push";
    }
}

void
DynamicJpegStack::SetBackground(unsigned char *data_buf, int w, int h)
{
    if (data) {
        free(data);
        data = NULL;
    }

    switch (buf_type) {
    case BUF_RGB:
        data = (unsigned char *)malloc(sizeof(*data)*w*h*3);
        if (!data) throw "malloc failed in DynamicJpegStack::SetBackground";
        memcpy(data, data_buf, w*h*3);
        break;

    case BUF_BGR:
        data = bgr_to_rgb(data_buf, w*h*3);
        if (!data) throw "malloc failed in DynamicJpegStack::SetBackground";
        break;

    case BUF_RGBA:
        data = rgba_to_rgb(data_buf, w*h*4);
        if (!data) throw "malloc failed in DynamicJpegStack::SetBackground";
        break;

    case BUF_BGRA:
        data = bgra_to_rgb(data_buf, w*h*4);
        if (!data) throw "malloc failed in DynamicJpegStack::SetBackground";
        break;

    default:
        throw "Unexpected buf_type in DynamicJpegStack::SetBackground";
    }
    bg_width = w;
    bg_height = h;
}

void
DynamicJpegStack::SetQuality(int q)
{
    quality = q;
}

void
DynamicJpegStack::Reset()
{
    dyn_rect = Rect(-1, -1, 0, 0);
}

Handle<Value>
DynamicJpegStack::Dimensions()
{
    NanScope();

    Local<Object> dim = Object::New();
    dim->Set(String::NewSymbol("x"), Integer::New(dyn_rect.x));
    dim->Set(String::NewSymbol("y"), Integer::New(dyn_rect.y));
    dim->Set(String::NewSymbol("width"), Integer::New(dyn_rect.w));
    dim->Set(String::NewSymbol("height"), Integer::New(dyn_rect.h));

    return scope.Close(dim);
}

NAN_METHOD(DynamicJpegStack::New)
{
    NanScope();

    if (args.Length() > 1)
        return NanThrowError("One argument max - buffer type.");

    buffer_type buf_type = BUF_RGB;
    if (args.Length() == 1) {
        if (!args[0]->IsString())
            return NanThrowTypeError("First argument must be a string. Either 'rgb', 'bgr', 'rgba' or 'bgra'.");

        String::AsciiValue bt(args[0]->ToString());
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

    DynamicJpegStack *jpeg = new DynamicJpegStack(buf_type);
    jpeg->Wrap(args.This());
    NanReturnValue(args.This());
}

NAN_METHOD(DynamicJpegStack::JpegEncodeSync)
{
    NanScope();
    DynamicJpegStack *jpeg = ObjectWrap::Unwrap<DynamicJpegStack>(args.This());
    NanReturnValue(jpeg->JpegEncodeSync());
}

NAN_METHOD(DynamicJpegStack::Push)
{
    NanScope();

    if (args.Length() != 5)
        return NanThrowError("Five arguments required - buffer, x, y, width, height.");

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

    DynamicJpegStack *jpeg = ObjectWrap::Unwrap<DynamicJpegStack>(args.This());

    if (!jpeg->data)
        return NanThrowError("No background has been set, use setBackground or setSolidBackground to set.");

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
    if (x >= jpeg->bg_width)
        return NanThrowRangeError("Coordinate x exceeds DynamicJpegStack's background dimensions.");
    if (y >= jpeg->bg_height)
        return NanThrowRangeError("Coordinate y exceeds DynamicJpegStack's background dimensions.");
    if (x+w > jpeg->bg_width)
        return NanThrowRangeError("Pushed fragment exceeds DynamicJpegStack's width.");
    if (y+h > jpeg->bg_height)
        return NanThrowRangeError("Pushed fragment exceeds DynamicJpegStack's height.");

    jpeg->Push((unsigned char *)Buffer::Data(data_buf), x, y, w, h);

    NanReturnUndefined();
}

NAN_METHOD(DynamicJpegStack::SetBackground)
{
    NanScope();

    if (args.Length() != 3)
        return NanThrowError("Four arguments required - buffer, width, height");
    if (!Buffer::HasInstance(args[0]))
        return NanThrowTypeError("First argument must be Buffer.");
    if (!args[1]->IsInt32())
        return NanThrowTypeError("Second argument must be integer width.");
    if (!args[2]->IsInt32())
        return NanThrowTypeError("Third argument must be integer height.");

    DynamicJpegStack *jpeg = ObjectWrap::Unwrap<DynamicJpegStack>(args.This());
    Local<Object> data_buf = args[0]->ToObject();
    int w = args[1]->Int32Value();
    int h = args[2]->Int32Value();

    if (w < 0)
        return NanThrowRangeError("Coordinate x smaller than 0.");
    if (h < 0)
        return NanThrowRangeError("Coordinate y smaller than 0.");

    try {
        jpeg->SetBackground((unsigned char *)Buffer::Data(data_buf), w, h);
    }
    catch (const char *err) {
        return NanThrowError(err);
    }

    NanReturnUndefined();
}

NAN_METHOD(DynamicJpegStack::Reset)
{
    NanScope();

    DynamicJpegStack *jpeg = ObjectWrap::Unwrap<DynamicJpegStack>(args.This());
    jpeg->Reset();
    NanReturnUndefined();
}

NAN_METHOD(DynamicJpegStack::Dimensions)
{
    NanScope();

    DynamicJpegStack *jpeg = ObjectWrap::Unwrap<DynamicJpegStack>(args.This());
    NanReturnValue(jpeg->Dimensions());
}

NAN_METHOD(DynamicJpegStack::SetQuality)
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

    DynamicJpegStack *jpeg = ObjectWrap::Unwrap<DynamicJpegStack>(args.This());
    jpeg->SetQuality(q);

    NanReturnUndefined();
}


void DynamicJpegStack::DynamicJpegEncodeWorker::Execute() {
    try {
        Rect &dyn_rect = jpeg_obj->dyn_rect;
        JpegEncoder encoder(jpeg_obj->data, jpeg_obj->bg_width, jpeg_obj->bg_height, jpeg_obj->quality, BUF_RGB);
        encoder.setRect(Rect(dyn_rect.x, dyn_rect.y, dyn_rect.w, dyn_rect.h));
        encoder.encode();
        jpeg_len = encoder.get_jpeg_len();
        jpeg = (char *)malloc(sizeof(*jpeg)*jpeg_len);
        if (!jpeg) {
            errmsg = strdup("malloc in DynamicJpegStack::DynamicJpegEncodeWorker::Execute() failed.");
        }
        else {
            memcpy(jpeg, encoder.get_jpeg(), jpeg_len);
        }
    }
    catch (const char *err) {
        errmsg = strdup(err);
    }
}

void DynamicJpegStack::DynamicJpegEncodeWorker::HandleOKCallback() {
    NanScope();

    Local<Object> buf = NanNewBufferHandle(jpeg_len);
    memcpy(Buffer::Data(buf), jpeg, jpeg_len);
    Local<Value> argv[3] = {buf, jpeg_obj->Dimensions(), Undefined()};

    TryCatch try_catch; // don't quite see the necessity of this

    callback->Call(3, argv);

    if (try_catch.HasCaught()) {
        FatalException(try_catch);
    }

    free(jpeg);
    jpeg = NULL;

    jpeg_obj->Unref();
}

void DynamicJpegStack::DynamicJpegEncodeWorker::HandleErrorCallback() {
    NanScope();
    Local<Value> argv[3] = {Undefined(), Undefined(), v8::Exception::Error(v8::String::New(errmsg))};

    TryCatch try_catch; // don't quite see the necessity of this

    callback->Call(3, argv);

    if (try_catch.HasCaught()) {
        FatalException(try_catch);
    }

    if (jpeg) {
        free(jpeg);
        jpeg = NULL;
    }

    jpeg_obj->Unref();
}

NAN_METHOD(DynamicJpegStack::JpegEncodeAsync)
{
    NanScope();

    if (args.Length() != 1)
        return NanThrowError("One argument required - callback function.");

    if (!args[0]->IsFunction())
        return NanThrowTypeError("First argument must be a function.");

    Local<Function> callback = Local<Function>::Cast(args[0]);
    DynamicJpegStack *jpeg = ObjectWrap::Unwrap<DynamicJpegStack>(args.This());

    NanAsyncQueueWorker(new DynamicJpegStack::DynamicJpegEncodeWorker(new NanCallback(callback), jpeg));

    jpeg->Ref();

    NanReturnUndefined();
}
