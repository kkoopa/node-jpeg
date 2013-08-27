#include <node.h>
#include <node_buffer.h>
#include <jpeglib.h>
#include <cstdlib>
#include <cstring>

#include "common.h"
#include "jpeg.h"
#include "jpeg_encoder.h"

using namespace v8;
using namespace node;

void
Jpeg::Initialize(v8::Handle<v8::Object> target)
{
    NanScope();

    Local<FunctionTemplate> t = FunctionTemplate::New(New);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    NODE_SET_PROTOTYPE_METHOD(t, "encode", JpegEncodeAsync);
    NODE_SET_PROTOTYPE_METHOD(t, "encodeSync", JpegEncodeSync);
    NODE_SET_PROTOTYPE_METHOD(t, "setQuality", SetQuality);
    NODE_SET_PROTOTYPE_METHOD(t, "setSmoothing", SetSmoothing);
    target->Set(String::NewSymbol("Jpeg"), t->GetFunction());
}

Jpeg::Jpeg(unsigned char *ddata, int wwidth, int hheight, int qquality, buffer_type bbuf_type) :
    jpeg_encoder(ddata, wwidth, hheight, qquality, bbuf_type) {}

Handle<Value>
Jpeg::JpegEncodeSync()
{
    NanScope();

    try {
        jpeg_encoder.encode();
    }
    catch (const char *err) {
        return ThrowException(Exception::Error(String::New(err)));
    }

    int jpeg_len = jpeg_encoder.get_jpeg_len();
    Local<Object> retbuf = NanNewBufferHandle(jpeg_len);
    memcpy(Buffer::Data(retbuf), jpeg_encoder.get_jpeg(), jpeg_len);
    return scope.Close(retbuf);
}

void
Jpeg::SetQuality(int q)
{
    jpeg_encoder.set_quality(q);
}

void
Jpeg::SetSmoothing(int s)
{
    jpeg_encoder.set_smoothing(s);
}

NAN_METHOD(Jpeg::New)
{
    NanScope();

    if (args.Length() < 3)
        return NanThrowError("At least three arguments required - buffer, width, height, [and buffer type]");
    if (!Buffer::HasInstance(args[0]))
        return NanThrowTypeError("First argument must be Buffer.");
    if (!args[1]->IsInt32())
        return NanThrowTypeError("Second argument must be integer width.");
    if (!args[2]->IsInt32())
        return NanThrowTypeError("Third argument must be integer height.");
    if (!args[3]->IsInt32())
        return NanThrowTypeError("Third argument must be integer quality.");

    int w = args[1]->Int32Value();
    int h = args[2]->Int32Value();
    int q = args[3]->Int32Value();

    if (w < 0)
        return NanThrowRangeError("Width can't be negative.");
    if (h < 0)
        return NanThrowRangeError("Height can't be negative.");
    if (q < 0 || q > 100)
        return NanThrowRangeError("Quality must be between 0 and 100");

    buffer_type buf_type = BUF_RGB;
    if (args.Length() == 5) {
        if (!args[4]->IsString())
            return NanThrowTypeError("Fifth argument must be a string. Either 'rgb', 'bgr', 'rgba' or 'bgra'.");

        String::AsciiValue bt(args[4]->ToString());
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

    Local<Object> buffer = args[0]->ToObject();
    Jpeg *jpeg = new Jpeg((unsigned char*) Buffer::Data(buffer), w, h, q, buf_type);
    jpeg->Wrap(args.This());
    NanReturnValue(args.This());
}

NAN_METHOD(Jpeg::JpegEncodeSync)
{
    NanScope();

    Jpeg *jpeg = ObjectWrap::Unwrap<Jpeg>(args.This());
    NanReturnValue(jpeg->JpegEncodeSync());
}

NAN_METHOD(Jpeg::SetQuality)
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

    Jpeg *jpeg = ObjectWrap::Unwrap<Jpeg>(args.This());
    jpeg->SetQuality(q);

    NanReturnUndefined();
}

NAN_METHOD(Jpeg::SetSmoothing)
{
    NanScope();

    if (args.Length() != 1)
        return NanThrowError("One argument required - quality");

    if (!args[0]->IsInt32())
        return NanThrowTypeError("First argument must be integer quality");

    int s = args[0]->Int32Value();

    if (s < 0)
        return NanThrowRangeError("Smoothing must be greater or equal to 0.");
    if (s > 100)
        return NanThrowRangeError("Smoothing must be less than or equal to 100.");

    Jpeg *jpeg = ObjectWrap::Unwrap<Jpeg>(args.This());
    jpeg->SetSmoothing(s);

    NanReturnUndefined();
}

void Jpeg::JpegEncodeWorker::Execute() {
    try {
        jpeg_obj->jpeg_encoder.encode();
        jpeg_len = jpeg_obj->jpeg_encoder.get_jpeg_len();
        jpeg = (char *)malloc(sizeof(*jpeg)*jpeg_len);
        if (!jpeg) {
            errmsg = strdup("malloc in Jpeg::JpegEncodeWorker::Execute() failed.");
        }
        else {
            memcpy(jpeg, jpeg_obj->jpeg_encoder.get_jpeg(), jpeg_len);
        }
    } catch (const char *err) {
        errmsg = strdup(err);
    }

}

void Jpeg::JpegEncodeWorker::HandleOKCallback() {
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

void Jpeg::JpegEncodeWorker::HandleErrorCallback() {
    NanScope();
    Local<Value> argv[2] = {Undefined(), v8::Exception::Error(v8::String::New(errmsg))};

    TryCatch try_catch; // don't quite see the necessity of this

    callback->Call(2, argv);

    if (try_catch.HasCaught()) {
        FatalException(try_catch);
    }

    if (jpeg) {
        free(jpeg);
    }

    jpeg_obj->Unref();
}

NAN_METHOD(Jpeg::JpegEncodeAsync)
{
    NanScope();

    if (args.Length() != 1)
        return NanThrowError("One argument required - callback function.");

    if (!args[0]->IsFunction())
        return NanThrowTypeError("First argument must be a function.");

    Local<Function> callback = Local<Function>::Cast(args[0]);
    Jpeg *jpeg = ObjectWrap::Unwrap<Jpeg>(args.This());

    NanAsyncQueueWorker(new Jpeg::JpegEncodeWorker(new NanCallback(callback), jpeg));

    jpeg->Ref();

    NanReturnUndefined();
}
