### CLIP-ImageSearch-NCNN

**为什么要做**：其实想做CLIP是因为我前面还想做GPT来的，当时想着既然要做GPT，为什么不把CLIP也做了呢？(不知廉耻的引一下流[GPT2-ChineseChat-NCNN](https://github.com/EdVince/GPT2-ChineseChat-NCNN))

**做什么模型**：CLIP跟GPT一样，模型超级多，我是在awesome-CLIP里面选的，我一眼就看中[natural-language-image-search](https://github.com/haltakov/natural-language-image-search)这个项目了，给出一句话来描述想要的图片，就能从图库中搜出来符合要求的。当时看到这个项目，我就知道这是个天生就适合放在手机相册里面的功能，既然这样为何不搞呢？

**工作目标**：使用ncnn部署[natural-language-image-search](https://github.com/haltakov/natural-language-image-search)这个基于CLIP的使用自然语言检索图像的模型，目标是给出x86和android端的demo

**PS**：工作繁忙，更新缓慢，只求一个star

### 工作内容
- [ ] pytorch模型梳理与导出
- [ ] x86 demo
- [ ] android demo

### 参考
1. [ncnn](https://github.com/Tencent/ncnn)
2. [natural-language-image-search](https://github.com/haltakov/natural-language-image-search)