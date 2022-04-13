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

#ifndef GPT2_H
#define GPT2_H

#include <map>
#include <net.h>
#include <vector>
#include "opencv2/opencv.hpp"

class CLIP
{
public:
    CLIP();

    int load(AAssetManager* mgr, std::string vocab);
    void encode_image(ncnn::Mat in, cv::Mat& feat);
    void encode_text(std::string text, cv::Mat& feat);
    void encode_text(std::vector<int> text, cv::Mat& feat);

private:
    std::vector<std::string> stringSplit(const std::string& str, char delim);
    std::vector<int> tokenize(std::string token);

private:
    ncnn::Net net;
    ncnn::UnlockedPoolAllocator blob_pool_allocator;
    ncnn::PoolAllocator workspace_pool_allocator;

    std::map<std::string, int> tokenizer_token2idx;

    const float mean_vals[3] = { 0.48145466f * 255.f, 0.4578275f * 255.f, 0.40821073f * 255.f };
    const float norm_vals[3] = { 1 / 0.26862954f / 255.f, 1 / 0.26130258f / 255.f, 1 / 0.27577711f / 255.f };
};

#endif // NANODET_H
