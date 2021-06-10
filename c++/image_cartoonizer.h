#ifndef _IMAGE_CARTOONIZER_H
#define _IMAGE_CARTOONIZER_H

#include "tnn_sdk_sample.h"
#include <memory>
#include <string>


namespace TNN_NS {

class ImageCartoonizerInput: public TNNSDKInput {
public:
    ImageCartoonizerInput(std::shared_ptr<Mat> mat = nullptr) : TNNSDKInput(mat) {};
    virtual ~ImageCartoonizerInput() {}
};

class ImageCartoonizerOutput: public TNNSDKOutput {
public:
    ImageCartoonizerOutput(std::shared_ptr<Mat> mat = nullptr): TNNSDKOutput(mat) {};
    virtual ~ImageCartoonizerOutput();

    ImageInfo cartoonized_image; 
};

class ImageCartoonizerOption : public TNNSDKOption {
public:
    ImageCartoonizerOption() {}
    virtual ~ImageCartoonizerOption() {}
    int input_width;
    int input_height;
    int num_thread = 1;
    int mode = 0;
};

class ImageCartoonizer: public TNN_NS::TNNSDKSample {
public:
    virtual ~ImageCartoonizer();
    virtual MatConvertParam GetConvertParamForInput(std::string tag = "");
    virtual std::shared_ptr<TNNSDKOutput> CreateSDKOutput();
    virtual Status ProcessSDKOutput(std::shared_ptr<TNNSDKOutput> output);
    virtual std::shared_ptr<TNN_NS::Mat> ProcessSDKInputMat(std::shared_ptr<TNN_NS::Mat> mat,
                                                              std::string name);
private:
    std::shared_ptr<Mat> Tensor2Image(std::shared_ptr<Mat> mat, const MatConvertParam& param);
    MatConvertParam GetConvertParamForOutput();                 
};

}

#endif