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

package com.edvince.gpt2chatbot;

import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.view.Surface;

public class CLIP
{
    public native boolean loadCLIP(AssetManager mgr, String vocab);
    public native boolean set(int num);
    public native boolean encodeImage(Bitmap bitmap, int cnt);
    public native boolean encodeText(String in);
    public native int go();
    public native boolean clear();

    static {
        System.loadLibrary("clipis");
    }
}
