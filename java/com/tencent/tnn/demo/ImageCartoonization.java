package com.tencent.tnn.demo;

import android.graphics.Bitmap;

public class ImageCartoonization {
    public native int init(String modelPath, int width, int height, int computeUnitType);
    public native boolean checkNpu(String modelPath);
    public native int deinit();
    public native ImageInfo[] cartoonizeFromImage(Bitmap image, int width, int height);
    public native ImageInfo[] cartoonizeFromStream(byte[] yuv420sp, int width, int height, int rotate);
}
