
#ifndef ANDROID_IMAGE_CARTOONIZER_JNI_H
#define ANDROID_IMAGE_CARTOONIZER_JNI_H

#include <jni.h>
#define TNN_CARTOONIZE(sig) Java_com_tencent_tnn_demo_ImageCartoonization_##sig
#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT JNICALL jint TNN_CARTOONIZE(init)(JNIEnv *env, jobject thiz, jstring modelPath, jint width, jint height, jint computeUnitType);
JNIEXPORT JNICALL jint TNN_CARTOONIZE(deinit)(JNIEnv *env, jobject thiz);
JNIEXPORT JNICALL jboolean TNN_CARTOONIZE(checkNpu)(JNIEnv *env, jobject thiz, jstring modelPath);
JNIEXPORT JNICALL jobjectArray TNN_CARTOONIZE(cartoonizeFromImage)(JNIEnv *env, jobject thiz, jobject imageSource, jint width, jint height);
JNIEXPORT JNICALL jobjectArray TNN_CARTOONIZE(cartoonizeFromStream)(JNIEnv *env, jobject thiz, jbyteArray yuv420sp, jint width, jint height, jint rotate);
#ifdef __cplusplus
}
#endif

#endif // ANDROID_IMAGE_CARTOONIZER_JNI_H

