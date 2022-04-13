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

#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>

#include <android/log.h>

#include <jni.h>

#include <string>
#include <vector>
#include <locale>
#include <codecvt>
#include <fstream>
#include <map>

#include <platform.h>
#include <benchmark.h>

#include "clip.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO , "read txt", __VA_ARGS__)

#if __ARM_NEON
#include <arm_neon.h>
#endif // __ARM_NEON

static CLIP* g_clip = 0;
static ncnn::Mutex lock;

static cv::Mat image_features;
static cv::Mat text_features;

std::vector<cv::Mat> image_features_stack;

template<typename _Tp>
int softmax(_Tp* src, _Tp* dst, int length)
{
    const _Tp alpha = *std::max_element(src, src + length);
    _Tp denominator{ 0 };
    for (int i = 0; i < length; ++i) {
        dst[i] = std::exp(src[i] - alpha);
        denominator += dst[i];
    }
    for (int i = 0; i < length; ++i) {
        dst[i] /= denominator;
    }
    return 0;
}

static std::string UTF16StringToUTF8String(const char16_t* chars, size_t len) {
    std::u16string u16_string(chars, len);
    return std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}
            .to_bytes(u16_string);
}

std::string JavaStringToString(JNIEnv* env, jstring str) {
    if (env == nullptr || str == nullptr) {
        return "";
    }
    const jchar* chars = env->GetStringChars(str, NULL);
    if (chars == nullptr) {
        return "";
    }
    std::string u8_string = UTF16StringToUTF8String(
            reinterpret_cast<const char16_t*>(chars), env->GetStringLength(str));
    env->ReleaseStringChars(str, chars);
    return u8_string;
}

static std::u16string UTF8StringToUTF16String(const std::string& string) {
    return std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}
            .from_bytes(string);
}

jstring StringToJavaString(JNIEnv* env, const std::string& u8_string) {
    std::u16string u16_string = UTF8StringToUTF16String(u8_string);
    auto result =env->NewString(reinterpret_cast<const jchar*>(u16_string.data()),
                                u16_string.length());
    return result;
}

extern "C" {

JNIEXPORT jboolean JNICALL Java_com_edvince_gpt2chatbot_CLIP_loadCLIP(JNIEnv* env, jobject thiz, jobject assetManager, jstring jvocab)
{
    AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);
    std::string vocab = JavaStringToString(env,jvocab);

    {
        ncnn::MutexLockGuard g(lock);
        if (!g_clip)
            g_clip = new CLIP;
        g_clip->load(mgr,vocab);
    }

    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_com_edvince_gpt2chatbot_CLIP_clear(JNIEnv* env, jobject thiz)
{
    image_features = cv::Mat::zeros(0,1024,CV_32FC1);
    text_features = cv::Mat::zeros(0,1024,CV_32FC1);
    image_features_stack.clear();

    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_com_edvince_gpt2chatbot_CLIP_set(JNIEnv* env, jobject thiz, jint num)
{
    image_features_stack.clear();
    image_features_stack.resize((int)num);

    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_com_edvince_gpt2chatbot_CLIP_encodeImage(JNIEnv* env, jobject thiz, jobject bitmap, jint cnt)
{
    double start_time = ncnn::get_current_time();

    ncnn::Mat image = ncnn::Mat::from_android_bitmap_resize(env, bitmap, ncnn::Mat::PIXEL_RGB,224,224);
    g_clip->encode_image(image,image_features_stack[(int)cnt]);

    if(cnt == image_features_stack.size()-1){
        cv::vconcat(image_features_stack, image_features);
    }

    LOGI("image: %d, cost: %.3lfms", cnt, ncnn::get_current_time()-start_time);

    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_com_edvince_gpt2chatbot_CLIP_encodeText(JNIEnv* env, jobject thiz, jstring in)
{
    double start_time = ncnn::get_current_time();

    std::string cpp_in = JavaStringToString(env, in);
    g_clip->encode_text(cpp_in,text_features);

    LOGI("Text cost: %.3lfms", ncnn::get_current_time()-start_time);

    return JNI_TRUE;
}

JNIEXPORT jint JNICALL Java_com_edvince_gpt2chatbot_CLIP_go(JNIEnv* env, jobject thiz)
{
    double start_time = ncnn::get_current_time();

    cv::Mat logits = 100.0f * image_features * text_features.t();
    cv::Point maxLoc;
    cv::Mat prob(1, image_features_stack.size(), CV_32FC1);
    softmax<float>((float*)logits.data, (float*)prob.data, image_features_stack.size());
    cv::minMaxLoc(prob, NULL, NULL, NULL, &maxLoc);

    LOGI("Search prob: %f, cost: %.3lfms", prob.at<float>(0,maxLoc.x), ncnn::get_current_time()-start_time);

    return maxLoc.x;
}

}
