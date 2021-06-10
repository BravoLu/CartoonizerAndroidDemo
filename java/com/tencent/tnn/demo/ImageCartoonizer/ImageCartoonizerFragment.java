package com.tencent.tnn.demo.ImageCartoonizer;

import android.graphics.Bitmap;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.SurfaceHolder;
import android.view.View;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.ToggleButton;
import android.graphics.Bitmap.Config;


import com.tencent.tnn.demo.FileUtils;
import com.tencent.tnn.demo.Helper;
import com.tencent.tnn.demo.ImageCartoonization;
import com.tencent.tnn.demo.R;
import com.tencent.tnn.demo.common.fragment.BaseFragment;
import com.tencent.tnn.demo.ImageInfo;

import java.util.ArrayList;
import java.nio.ByteBuffer;

public class ImageCartoonizerFragment extends BaseFragment {

    private final static String TAG = ImageCartoonizerFragment.class.getSimpleName();
    private ImageCartoonization mImageCartoonizer = new ImageCartoonization();

    private static final String IMAGE = "tiger_cat.jpg";
    private static final String RESULT_LIST = "synset.txt";
    private static final int NET_INPUT = 256;

    private ToggleButton mGPUSwitch;
    private Button mRunButton;
    private boolean mUseGPU = false;

    private ToggleButton mHuaweiNPUswitch;
    private boolean mUseHuaweiNpu = false;
    private TextView HuaweiNpuTextView;


    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.d(TAG, "onCreate");
        super.onCreate(savedInstanceState);
        System.loadLibrary("tnn_wrapper");
        String modelPath = initModel();
//        NpuEnable = mImageCartoonizer.checkNpu(modelPath);
    }

    private String initModel()
    {
        String targetDir =  getActivity().getFilesDir().getAbsolutePath();
        String[] modelPathsCartoonizer = {
            "SimpleGenerator.tnnmodel",
            "SimpleGenerator.tnnproto",
        };

        for (int i = 0; i < modelPathsCartoonizer.length; i++) {
            String modelFilePath = modelPathsCartoonizer[i];
            String interModelFilePath = targetDir + "/" + modelFilePath;
            FileUtils.copyAsset(getActivity().getAssets(), "simplegenerator/" + modelFilePath, 
                                                interModelFilePath);
        }
        return targetDir;
    }

    @Override
    public void onClick(View view) {
        int i = view.getId();
        if (i == R.id.back_rl) {
            clickBack();
        }
    }

    private void onSwichGPU(boolean b)
    {
        if (b && mHuaweiNPUswitch.isChecked()) {
            mHuaweiNPUswitch.setChecked(false);
            mUseHuaweiNpu = false;
        }
        mUseGPU = b;
        TextView result_view = (TextView)$(R.id.result);
        result_view.setText("");
    }

    private void onSwichNPU(boolean b)
    {
        if (b && mGPUSwitch.isChecked()) {
            mGPUSwitch.setChecked(false);
            mUseGPU = false;
        }
        mUseHuaweiNpu = b;
        TextView result_view = (TextView)$(R.id.result);
        result_view.setText("");
    }

    private void clickBack() {
        if (getActivity() != null) {
            (getActivity()).finish();
        }
    }

    @Override
    public void setFragmentView() {
        Log.d(TAG, "setFragmentView");
        setView(R.layout.fragment_image_detector);
        setTitleGone();
        $$(R.id.back_rl);
        $$(R.id.gpu_switch);
        mGPUSwitch = $(R.id.gpu_switch);
        mGPUSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                onSwichGPU(b);
            }
        });
        $$(R.id.npu_switch);
        mHuaweiNPUswitch = $(R.id.npu_switch);
        mHuaweiNPUswitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                onSwichNPU(b);
            }
        });

        HuaweiNpuTextView = $(R.id.npu_text);
        if (!NpuEnable) {
            HuaweiNpuTextView.setVisibility(View.INVISIBLE);
            mHuaweiNPUswitch.setVisibility(View.INVISIBLE);
        }

        mRunButton = $(R.id.run_button);
        mRunButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                startCartoonize();
            }
        });

        final Bitmap originBitmap = FileUtils.readBitmapFromFile(getActivity().getAssets(), IMAGE);
        ImageView source = (ImageView)$(R.id.origin);
        source.setImageBitmap(originBitmap);
    }

    @Override
    public void openCamera() {

    }

    @Override
    public void startPreview(SurfaceHolder surfaceHolder) {

    }

    @Override
    public void closeCamera() {

    }

    private void startCartoonize() {
        
        final Bitmap originBitmap = FileUtils.readBitmapFromFile(getActivity().getAssets(), IMAGE);
        final Bitmap scaleBitmap = Bitmap.createScaledBitmap(originBitmap, NET_INPUT, NET_INPUT, true);
        Bitmap cartoonize_image = null;
        ImageView source = (ImageView)$(R.id.origin);
//        source.setImageBitmap(originBitmap);
        String modelPath = initModel();
        Log.d(TAG, "Init classify " + modelPath);
        int device = 0;
        if (mUseHuaweiNpu) {
            device = 2;
        } else if (mUseGPU) {
            device = 1;
        }
        int result = mImageCartoonizer.init(modelPath, NET_INPUT, NET_INPUT, device);
        if (result == 0) {
            ImageInfo[] imageInfoList = mImageCartoonizer.cartoonizeFromImage(scaleBitmap, NET_INPUT, NET_INPUT);
            Log.d(TAG, "width,height:"+imageInfoList[0].image_width+","+imageInfoList[0].image_height);
            if (imageInfoList != null && imageInfoList.length > 0) {
                source.setImageBitmap(originBitmap);
                TextView result_view = (TextView)$(R.id.result);
                String resultText = "cartoonized images";
                result_view.setText(resultText);
                cartoonize_image = Bitmap.createBitmap(
                                                        imageInfoList[0].image_width,
                                                        imageInfoList[0].image_height,
                                                        Config.ARGB_8888
                                                        );
                Log.d(TAG, "imageInfoList[0].data.length:"+imageInfoList[0].data.length);
                ByteBuffer buffer = ByteBuffer.wrap(imageInfoList[0].data, 0, imageInfoList[0].data.length);
//                buffer.rewind();
//                int[] pixels = new int[imageInfoList[0].image_height*imageInfoList[0].image_width];
//                for (int i = 0; i < imageInfoList[0].image_height*imageInfoList[0].image_width; i++){
//                    pixels[i] = buffer.get();
//                }
//                cartoonize_image.setPixels(pixels, 0 ,imageInfoList[0].image_width, 0, 0, imageInfoList[0].image_width, imageInfoList[0].image_height);
                cartoonize_image.copyPixelsFromBuffer(buffer);
            }
            Bitmap resized = Bitmap.createScaledBitmap(cartoonize_image, 768, 768, true);
            source.setImageBitmap(resized);
//            source.setImageBitmap(cartoonize_image);
            mImageCartoonizer.deinit();

        } else {
            Log.e(TAG, "failed to init model " + result);
        }

    }

    @Override
    public void onStart() {
        Log.d(TAG, "onStart");
        super.onStart();
    }

    @Override
    public void onResume() {
        super.onResume();
        Log.d(TAG, "onResume");

        getFocus();
    }

    @Override
    public void onPause() {
        Log.d(TAG, "onPause");
        super.onPause();
    }

    @Override
    public void onStop() {
        Log.i(TAG, "onStop");
        super.onStop();
    }


    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.i(TAG, "onDestroy");
    }

    private void preview() {
        Log.i(TAG, "preview");

    }

    private void getFocus() {
        getView().setFocusableInTouchMode(true);
        getView().requestFocus();
        getView().setOnKeyListener(new View.OnKeyListener() {
            @Override
            public boolean onKey(View v, int keyCode, KeyEvent event) {
            if (event.getAction() == KeyEvent.ACTION_UP && keyCode == KeyEvent.KEYCODE_BACK) {
                clickBack();
                return true;
            }
            return false;
            }
        });
    }

}
