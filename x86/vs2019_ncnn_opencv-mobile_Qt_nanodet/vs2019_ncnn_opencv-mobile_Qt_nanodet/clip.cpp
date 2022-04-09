#include "clip.h"

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



CLIP::CLIP()
{
    opt.use_packing_layout = false;
    opt.use_bf16_storage = false;
    opt.use_fp16_arithmetic = false;
    opt.use_fp16_packed = false;
    opt.use_fp16_storage = false;
    opt.num_threads = 4;

    net.opt = opt;
    net.register_custom_layer("Gather", Gather_layer_creator);
    net.load_param("assets/clip.param");
    net.load_model("assets/clip.bin");

    std::ifstream infile;
    std::string pathname = "assets/vocab.txt";
    infile.open(pathname.data());
    std::string s;
    int idx = 0;
    while (getline(infile, s)) {
        tokenizer_token2idx.insert(std::pair<std::string, int>(s, idx));
        idx++;
    }
    infile.close();
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

void show(ncnn::Mat in)
{
    int C = in.c, H = in.h, W = in.w;
    double s = 0.0;
    for (int c = 0; c < C; c++) {
        for (int i = 0; i < H * W; i++) {
            s += in.channel(c)[i];
        }
    }
    std::cout << "dim:" << in.dims << "(" << C << "," << H << "," << W << ")sum:" << s << std::endl;
}

void CLIP::encode_image(cv::Mat image, cv::Mat& feat)
{
    cv::resize(image, image, cv::Size(224, 224));
    ncnn::Mat in = ncnn::Mat::from_pixels(image.data, ncnn::Mat::PIXEL_RGB, image.cols, image.rows);
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