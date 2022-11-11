# CLIP-ImageSearch-NCNN

**想法**: 这个项目我觉得非常适合永城手机自带相册的图片搜索功能，在后台偷跑每一张图进行抽特征，这样最后用的时候就能秒出结果。而且还提供每个图片的匹配概率，可以按照概率从高到低显示做成图片流的结果返回👍。

**为什么要做**：其实想做CLIP是因为我前面还想做GPT来的，当时想着既然要做GPT，为什么不把CLIP也做了呢？(不知廉耻的引一下流[GPT2-ChineseChat-NCNN](https://github.com/EdVince/GPT2-ChineseChat-NCNN))

**做什么模型**：CLIP跟GPT一样，玩法超级多，我是在awesome-CLIP里面选的，我一眼就看中[natural-language-image-search](https://github.com/haltakov/natural-language-image-search)这个项目了，给出一句话来描述想要的图片，就能从图库中搜出来符合要求的。当时看到这个项目，我就知道这是个天生就适合放在手机相册里面的功能，既然这样为何不搞呢？

**工作目标**：使用ncnn部署CLIP实现使用自然语言检索图像，目标是给出x86和android端的demo

**PS**：工作繁忙，更新缓慢，只求一个star

## Demo
android: 可直接运行的[apk](https://pan.baidu.com/s/1Q1nt_1gtcFo31goDJVOpdQ?pwd=6666)

![image](./resources/clip.jpg)

![image](./resources/bicycle.jpg)

用法：
1. 先点“扫描相册”，扫描一遍手机有的图片
2. 按照各个时间段的图片数量，选择一个合适进行特征抽取(耗时与数量成正比，麒麟970上0.5s一张图，慢慢等)
3. 在最底下的框输入你要想搜内容，一定要用英文！！！
4. 最后点击“搜”就有结果了(麒麟970上1.5s就能出来结果)

x86: 可直接运行的[exe](https://pan.baidu.com/s/1RvVxXJ1TPazJT_Kp-u4tkA?pwd=6666)

![image](./resources/x86.png)

用法：按照按钮的顺序点击就好了，举例：
1. 先点击“1.选择图库”，选择该repo的gallery文件夹
2. 然后点击“2.抽取图库特征”，抽取所选文件夹的所有图片的特征，耗时与图片数量有关
3. 在文本框内输入一句描述要想的图片的话，一定要用英文，大小写没关系
4. 最后点击“4.搜”，程序会自动返回图库中最匹配所输入文字的图片

## 工作原理
1. 使用CLIP的encode_image提取图片的特征，构建一个图库的特征向量
2. 使用CLIP的encode_text提取文字的特征，构建文本特征向量
3. 对两个特征向量求相似度，可以双向匹配，可以用文本匹配图片，也可以用图片匹配文本
4. 可以得到所有图片的相似度，我为了偷懒只显示了匹配概率最高的那一个
5. 稍微修改一下就可以做成手机相册自带搜图的那种返回一堆相关图片的功能了
6. 支持以图搜图、以字搜图、以图搜字、甚至是以字搜字。。。玩法很多，就是用那些特征求相似的问题，不赘述了
7. 最耗时间的其实是构建图库所有图片的特征向量，这里我用的是“RN50”这个模型，对图片使用resnet50进行特征抽取

## Repo结构
1. android: 所提供的apk程序的源代码
2. x86: 所提供的exe程序的源代码，基于qt
3. gallery: 一个小的图库，供测试用
4. resources: README的资源文件夹

## 工作内容
- [x] pytorch模型梳理与导出
- [x] x86 demo
    - 要编译的话，到[这里](https://pan.baidu.com/s/1qNnrFBuNiT0l67irAJgf0A?pwd=6666)下载.bin文件并放置到x86项目的assert文件夹下
- [x] android demo
    - 要编译的话，到上面x86的链接下载.bin文件放到android项目的assert文件夹下

## 参考
1. [ncnn](https://github.com/Tencent/ncnn)
3. [CLIP](https://github.com/openai/CLIP)
2. [natural-language-image-search](https://github.com/haltakov/natural-language-image-search)
