// Tencent is pleased to support the open source community by making ncnn available.
//
// Copyright (C) 2021 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#include "clip.h"
#include <map>
#include <fstream>
#include <string>
#include <cstdlib>
#include <wchar.h>
#include <iostream>
#include <codecvt>
#include <ctime>
#include <algorithm>
#include <functional>
#include <numeric>
#include <time.h>

#include <android/log.h>

#include "cpu.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO , "GPT2", __VA_ARGS__)

class Gather : public ncnn::Layer
{
public:
    Gather()
    {
        one_blob_only = false;
    }

    virtual int forward(const std::vector<ncnn::Mat>& bottom_blobs, std::vector<ncnn::Mat>& top_blobs, const ncnn::Option& opt) const
    {
        int w = bottom_blobs[1].w;
        int vocab_size = bottom_blobs[0].h;
        int n_embd = bottom_blobs[0].w;

        ncnn::Mat& top_blob = top_blobs[0];
        top_blob.create(n_embd, w, 4u, 1, opt.blob_allocator);
        if (top_blob.empty())
            return -100;

        float* dst = top_blob;
        const float* in = bottom_blobs[1];
        const float* weight = bottom_blobs[0];

#pragma omp parallel for num_threads(opt.num_threads)
        for (int c = 0; c < w; c++) {
            int idx = std::round(*in) * n_embd;
            memcpy(dst, weight + idx, n_embd * 4);
            in++;
            dst += n_embd;
        }


        return 0;
    }
};
DEFINE_LAYER_CREATOR(Gather)

std::string WStringToString(const std::wstring& wstr)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;
    return converterX.to_bytes(wstr);
}

std::wstring StringToWString(const std::string& str)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;
    return converterX.from_bytes(str);
}

static std::u16string UTF8StringToUTF16String(const std::string& string) {
    return std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}
            .from_bytes(string);
}

static std::string UTF16StringToUTF8String(const char16_t* chars, size_t len) {
    std::u16string u16_string(chars, len);
    return std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}
            .to_bytes(u16_string);
}

CLIP::CLIP()
{
    blob_pool_allocator.set_size_compare_ratio(0.f);
    workspace_pool_allocator.set_size_compare_ratio(0.f);
}

int CLIP::load(AAssetManager* mgr, std::string vocab)
{
    net.clear();
    blob_pool_allocator.clear();
    workspace_pool_allocator.clear();

    ncnn::set_cpu_powersave(2);
    ncnn::set_omp_num_threads(ncnn::get_big_cpu_count());

    net.opt = ncnn::Option();
#if NCNN_VULKAN
    net.opt.use_vulkan_compute = true;
#endif
    net.opt.lightmode = true;
    net.opt.use_packing_layout = false;
    net.opt.num_threads = 4;
    net.opt.blob_allocator = &blob_pool_allocator;
    net.opt.workspace_allocator = &workspace_pool_allocator;

    net.register_custom_layer("Gather", Gather_layer_creator);
    net.load_param(mgr, "clip.param");
    net.load_model(mgr, "clip.bin");

    LOGI("load ncnn model ok!");

    std::ifstream infile;
    infile.open(vocab.data());
    std::string s;
    int idx = 0;
    while (getline(infile, s)) {
        tokenizer_token2idx.insert(std::pair<std::string, int>(s.substr(0,s.length()-1), idx));
        idx++;
    }

    infile.close();

    LOGI("load vocab: %d\n", idx);

    return 0;
}

void CLIP::encode_image(ncnn::Mat in, cv::Mat& feat)
{
    in.substract_mean_normalize(mean_vals, norm_vals);

    ncnn::Mat image_features;
    {
        ncnn::Mat subout;
        ncnn::Extractor ex = net.create_extractor();
        ex.input(".\\encode_image-fp16/0", in);
        ex.extract(".\\encode_image-fp16/652", subout);
        image_features = ncnn::Mat(1024, subout.row(0));
    }

    {
        float norm = 0.f;
        for (int i = 0; i < 1024; i++) {
            norm += image_features[i] * image_features[i];
        }
        norm = std::sqrt(norm);
        for (int i = 0; i < 1024; i++) {
            image_features[i] /= norm;
        }
    }

    cv::Mat ifeat(1, 1024, CV_32FC1, (void*)image_features);

    feat = ifeat.clone();
}

std::vector<std::string> CLIP::stringSplit(const std::string& str, char delim) {
    std::vector<std::string> elems;
    auto lastPos = str.find_first_not_of(delim, 0);
    auto pos = str.find_first_of(delim, lastPos);
    while (pos != std::string::npos || lastPos != std::string::npos) {
        elems.push_back(str.substr(lastPos, pos - lastPos));
        lastPos = str.find_first_not_of(delim, pos);
        pos = str.find_first_of(delim, lastPos);
    }
    return elems;
}

std::vector<int> CLIP::tokenize(std::string token)
{
    std::vector<int> idx;
    idx.push_back(49406);
    {
        std::vector<std::string> tokens = stringSplit(token, ' ');
        for (auto t : tokens) {
            idx.push_back(tokenizer_token2idx[t + "</w>"]);
        }
    }
    idx.push_back(49407);
    return idx;
}

void CLIP::encode_text(std::string text, cv::Mat& feat)
{
    std::vector<int> _text_ = tokenize(text);
    encode_text(_text_, feat);
}

void CLIP::encode_text(std::vector<int> text, cv::Mat& feat)
{
    int len = text.size() - 1;
    ncnn::Mat in(77); in.fill(0.0f);
    for (int i = 0; i < text.size(); i++) {
        in[i] = (float)text[i];
    }

    ncnn::Mat text_features;
    {
        ncnn::Mat subout;

        ncnn::Extractor ex = net.create_extractor();
        ex.input(".\\encode_text-fp16/input.1", in);
        ex.extract(".\\encode_text-fp16/1745", subout);

        ncnn::Mat subin(512, subout.row(len));
        ex.input(".\\encode_text-fp16/1751", subin);
        ex.extract(".\\encode_text-fp16/1752", text_features);
    }

    {
        float norm = 0.f;
        for (int i = 0; i < 1024; i++) {
            norm += text_features[i] * text_features[i];
        }
        norm = std::sqrt(norm);
        for (int i = 0; i < 1024; i++) {
            text_features[i] /= norm;
        }
    }

    cv::Mat tfeat(1, 1024, CV_32FC1, (void*)text_features);

    feat = tfeat.clone();
}


