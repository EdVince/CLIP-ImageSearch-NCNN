#pragma once
#include <algorithm>
#include <vector>
#include <iostream>
#include <numeric>
#include <fstream>
#include <map>
#include <limits>
#include <algorithm>
#include <functional>
#include <cstdlib>
#include <ctime>

#include "opencv2/opencv.hpp"

#include "net.h"
#include "layer.h"

class CLIP
{
public:
    CLIP();
    
    void encode_image(cv::Mat image, cv::Mat& feat);
    void encode_text(std::string text, cv::Mat& feat);
    void encode_text(std::vector<int> text, cv::Mat& feat);

    std::vector<std::string> stringSplit(const std::string& str, char delim);
    std::vector<int> tokenize(std::string token);

    std::map<std::string, int> tokenizer_token2idx;

    ncnn::Option opt;
    ncnn::Net net;

    const float mean_vals[3] = { 0.48145466f * 255.f, 0.4578275f * 255.f, 0.40821073f * 255.f };
    const float norm_vals[3] = { 1 / 0.26862954f / 255.f, 1 / 0.26130258f / 255.f, 1 / 0.27577711f / 255.f };
};