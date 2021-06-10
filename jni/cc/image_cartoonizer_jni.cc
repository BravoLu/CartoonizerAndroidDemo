#include "image_cartoonizer.h"
#include "image_cartoonizer_jni.h"
#include "tnn_sdk_sample.h"
#include "helper_jni.h"
#include "kannarotate.h"
#include "yuv420sp_to_rgb_fast_asm.h"
#include <jni.h>
#include <android/bitmap.h>

static std::shared_ptr<TNN_NS::ImageCartoonizer> gCartoonizer;
static int gComputeUnitType = 0;
static jclass clsImageInfo;
static jmethodID midconstructorImageInfo;
static jfieldID fidimage_width;
static jfieldID fidimage_height;
static jfieldID fidimage_channel;
static jfieldID fiddata;

JNIEXPORT JNICALL jint TNN_CARTOONIZE(init)(JNIEnv *env, jobject thiz, 
                                            jstring modelPath, jint width, 
                                            jint height, jint computeUnitType)
{
    setBenchResult("");
    std::vector<int> nchw = {1, 3, height, width};
    gCartoonizer = std::make_shared<TNN_NS::ImageCartoonizer>();
    std::string protoContent, modelContent;
    std::string modelPathStr(jstring2string(env, modelPath));
    protoContent = fdLoadFile(modelPathStr + "/SimpleGenerator.opt.tnnproto");
    modelContent = fdLoadFile(modelPathStr + "/SimpleGenerator.opt.tnnmodel");
    LOGI("proto content size %d model content size %d", protoContent.length(), 
                                                        modelContent.length());
    TNN_NS::Status status = TNN_NS::TNN_OK;
    gComputeUnitType = computeUnitType;

    auto option = std::make_shared<TNN_NS::ImageCartoonizerOption>();
    option->compute_units = TNN_NS::TNNComputeUnitsCPU;
    option->input_shapes = {};
    option->library_path="";
    option->proto_content = protoContent;
    option->model_content = modelContent;
    option->input_width = 256;
    option->input_height = 256;
    option->mode = 1;
    if (gComputeUnitType == 1) {
        option->compute_units = TNN_NS::TNNComputeUnitsGPU;
    } else if (gComputeUnitType == 2) {
        LOGI("the device type  %d device huawei_npu" ,gComputeUnitType);
        gCartoonizer->setNpuModelPath(modelPathStr + "/");
        gCartoonizer->setCheckNpuSwitch(false);
        option->compute_units = TNN_NS::TNNComputeUnitsHuaweiNPU;
    } else {
	    option->compute_units = TNN_NS::TNNComputeUnitsCPU;
    }
    status = gCartoonizer->Init(option);

    if (status != TNN_NS::TNN_OK) {
        LOGE("cartoonizer init failed $d", (int)status);
        return -1;
    }

    if (clsImageInfo == NULL) {
        clsImageInfo = static_cast<jclass>(env->NewGlobalRef(env->FindClass("com/tencent/tnn/demo/ImageInfo")));
        midconstructorImageInfo = env->GetMethodID(clsImageInfo, "<init>", "()V");
        fidimage_width = env->GetFieldID(clsImageInfo, "image_width" , "I");
        fidimage_height = env->GetFieldID(clsImageInfo, "image_height" , "I");
        fidimage_channel = env->GetFieldID(clsImageInfo, "image_channel" , "I");
        fiddata = env->GetFieldID(clsImageInfo, "data" , "[B");
    }

    return 0;
}

JNIEXPORT jboolean TNN_CARTOONIZE(checkNpu)(JNIEnv *env, jobject thiz, jstring modelPath) {
    TNN_NS::ImageCartoonizer tmpCartoonizer;
    std::string protoContent, modelContent;
    std::string modelPathStr(jstring2string(env, modelPath));
    protoContent = fdLoadFile(modelPathStr + "/SimpleGenerator.opt.tnnproto");
    modelContent = fdLoadFile(modelPathStr + "/SimpleGenerator.opt.tnnmodel");
    auto option = std::make_shared<TNN_NS::ImageCartoonizerOption>();
    option->compute_units = TNN_NS::TNNComputeUnitsHuaweiNPU;
    option->library_path = "";
    option->proto_content = protoContent;
    option->model_content = modelContent;
    option->input_height= 768;
    option->input_width = 768;
    tmpCartoonizer.setNpuModelPath(modelPathStr + "/");
    tmpCartoonizer.setCheckNpuSwitch(true);
    TNN_NS::Status ret = tmpCartoonizer.Init(option);
    return ret == TNN_NS::TNN_OK; 
}



JNIEXPORT JNICALL jint TNN_CARTOONIZE(deinit)(JNIEnv *env, jobject thiz)
{
    gCartoonizer = nullptr;
    return 0;
}

JNIEXPORT JNICALL jobjectArray TNN_CARTOONIZE(cartoonizeFromImage)(JNIEnv *env, jobject thiz, 
                                                jobject imageSource, jint width, jint height)
{
    jobjectArray imageInfoArray;
    std::vector<TNN_NS::ImageInfo> imageInfoList;
    int ret = -1;
    AndroidBitmapInfo  sourceInfocolor;
    void*              sourcePixelscolor;

    if (AndroidBitmap_getInfo(env, imageSource, &sourceInfocolor) < 0) {
        return 0;
    }

    if (sourceInfocolor.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        return 0;
    }

    if ( AndroidBitmap_lockPixels(env, imageSource, &sourcePixelscolor) < 0) {
        return 0;
    }

    TNN_NS::BenchOption bench_option;
    bench_option.forward_count = 1;
    gCartoonizer->SetBenchOption(bench_option);
    TNN_NS::DeviceType dt = TNN_NS::DEVICE_ARM;
    TNN_NS::DimsVector target_dims = {1, 3, height, width};
    std::shared_ptr<TNN_NS::Mat> input_mat = std::make_shared<TNN_NS::Mat>(dt, TNN_NS::N8UC4, 
                                                                target_dims, sourcePixelscolor);
    // gCartoonizer->ProcessSDKInputMat(input_mat);
    std::shared_ptr<TNN_NS::TNNSDKInput> input = std::make_shared<TNN_NS::TNNSDKInput>(input_mat);
    std::shared_ptr<TNN_NS::TNNSDKOutput> output = gCartoonizer->CreateSDKOutput();
    TNN_NS::Status status = gCartoonizer->Predict(input, output);

    gCartoonizer->ProcessSDKOutput(output);
    AndroidBitmap_unlockPixels(env, imageSource);

    if (status != TNN_NS::TNN_OK) {
        return 0;
    }

    imageInfoList.push_back(dynamic_cast<TNN_NS::ImageCartoonizerOutput *>(output.get())->cartoonized_image);

    if (imageInfoList.size() > 0) {
        imageInfoArray = env->NewObjectArray(imageInfoList.size(), clsImageInfo, NULL);
        for (int i = 0; i < imageInfoList.size(); i++) {
            jobject objImageInfo = env->NewObject(clsImageInfo, midconstructorImageInfo);
            int image_width = imageInfoList[i].image_width;
            int image_height = imageInfoList[i].image_height;
            int image_channel = imageInfoList[i].image_channel;
            int dataNum = image_channel * image_width * image_height;

            env->SetIntField(objImageInfo, fidimage_width, image_width);
            env->SetIntField(objImageInfo, fidimage_height, image_height);
            env->SetIntField(objImageInfo, fidimage_channel, image_channel);

            jbyteArray jarrayData = env->NewByteArray(dataNum);
            // 设置值
            env->SetByteArrayRegion(jarrayData, 0, dataNum , (jbyte*)imageInfoList[i].data.get());
            //函数作用：
            // 　　该访问器例程系列设置对象的实例(非静态)域的值。要访问的域由通过调用SetFieldID() 而得到的域 ID 指定。
            // 参数说明：　　
            // 　　env:JNI 接口指针。
            // 　　obj:Java 对象(不能为 NULL)。
            // 　　fieldID:有效的域 ID。
            // 　　value:域的新值。
            env->SetObjectField(objImageInfo, fiddata, jarrayData);

            env->SetObjectArrayElement(imageInfoArray, i, objImageInfo);
            env->DeleteLocalRef(objImageInfo);
        }
        return imageInfoArray;
    } else {
        return 0;
    }
    // imageInfoArray = env->NewObjectArray(imageInfoList.size(), clsImageInfo, NULL);
    // jobject objImageInfo = env->NewObject(clsImageInfo, midconstructorImageInfo);
    // int image_width = imageInfoList[0].image_width;
    // int image_height = imageInfoList[0].image_height;
    // int image_channel = imageInfoList[0].image_channel;
    // int dataNum = image_channel * image_width * image_height;

    // env->SetIntField(objImageInfo, fidimage_width, image_width);
    // env->SetIntField(objImageInfo, fidimage_height, image_height);
    // env->SetIntField(objImageInfo, fidimage_channel, image_channel);

    // jbyteArray jarrayData = env->NewByteArray(dataNum);
    // env->SetByteArrayRegion(jarrayData, 0, dataNum , (jbyte*)imageInfoList[0].data.get());
    // env->SetObjectField(objImageInfo, fiddata, jarrayData);

    // env->SetObjectArrayElement(imageInfoArray, 0, objImageInfo);
    // env->DeleteLocalRef(objImageInfo);

    // return imageInfoArray;
    // return imageInfoList;
}


JNIEXPORT JNICALL jobjectArray TNN_CARTOONIZE(cartoonizeFromStream)(JNIEnv *env, jobject thiz, jbyteArray yuv420sp, jint width, jint height, jint rotate)
{
    jobjectArray imageInfoArray;
    auto asyncRefCartoonizer = gCartoonizer;
    std::vector<TNN_NS::ImageInfo> imageInfoList;
    // Convert yuv to rgb
    LOGI("detect from stream %d x %d r %d", width, height, rotate);
    unsigned char *yuvData = new unsigned char[height * width * 3 / 2];
    jbyte *yuvDataRef = env->GetByteArrayElements(yuv420sp, 0);
    int ret = kannarotate_yuv420sp((const unsigned char*)yuvDataRef, (int)width, (int)height, (unsigned char*)yuvData, (int)rotate);
    env->ReleaseByteArrayElements(yuv420sp, yuvDataRef, 0);
    unsigned char *rgbaData = new unsigned char[height * width * 4];
    yuv420sp_to_rgba_fast_asm((const unsigned char*)yuvData, height, width, (unsigned char*)rgbaData);
    TNN_NS::DeviceType dt = TNN_NS::DEVICE_ARM;
    TNN_NS::DimsVector target_dims = {1, 3, width, height};

    auto rgbTNN = std::make_shared<TNN_NS::Mat>(dt, TNN_NS::N8UC4, target_dims, rgbaData);
    std::shared_ptr<TNN_NS::TNNSDKInput> input = std::make_shared<TNN_NS::TNNSDKInput>(rgbTNN);
    std::shared_ptr<TNN_NS::TNNSDKOutput> output = gCartoonizer->CreateSDKOutput();

    TNN_NS::Status status = gCartoonizer->Predict(input, output);

    imageInfoList.push_back(dynamic_cast<TNN_NS::ImageCartoonizerOutput *>(output.get())->cartoonized_image);
    
    delete [] yuvData;
    delete [] rgbaData;
    if (status != TNN_NS::TNN_OK) {
        LOGE("failed to detect %d", (int)status);
        return 0;
    }

    if (imageInfoList.size() > 0) {
        imageInfoArray = env->NewObjectArray(imageInfoList.size(), clsImageInfo, NULL);
        for (int i = 0; i < imageInfoList.size(); i++) {
            jobject objImageInfo = env->NewObject(clsImageInfo, midconstructorImageInfo);
            int image_width = imageInfoList[i].image_width;
            int image_height = imageInfoList[i].image_height;
            int image_channel = imageInfoList[i].image_channel;
            int dataNum = image_channel * image_width * image_height;

            env->SetIntField(objImageInfo, fidimage_width, image_width);
            env->SetIntField(objImageInfo, fidimage_height, image_height);
            env->SetIntField(objImageInfo, fidimage_channel, image_channel);

            jbyteArray jarrayData = env->NewByteArray(dataNum);
            env->SetByteArrayRegion(jarrayData, 0, dataNum , (jbyte*)imageInfoList[i].data.get());
            env->SetObjectField(objImageInfo, fiddata, jarrayData);

            env->SetObjectArrayElement(imageInfoArray, i, objImageInfo);
            env->DeleteLocalRef(objImageInfo);
        }
        return imageInfoArray;
    } else {
        return 0;
    }

}