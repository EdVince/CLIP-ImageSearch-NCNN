#pragma execution_character_set("utf-8")

#include "mainwindow.h"

#include<QDebug>

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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    setWindowTitle("ImageSearch(https://github.com/EdVince)");
    ui.extractFeat->setEnabled(false);
    ui.go->setEnabled(false);
    clip = new CLIP();
}

void MainWindow::on_chooseImageFolder_clicked()
{
    QString folder = QFileDialog::getExistingDirectory(this, tr("Open Directory"), "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    QDir dir(folder);
    if (dir.exists()) {
        QStringList nameFilters;
        nameFilters << "*.jpg" << "*.png" << "*.jpeg" << "*.bmp";
        gallery = dir.entryList(nameFilters, QDir::Files | QDir::NoDotAndDotDot);
        for (int i = 0; i < gallery.length(); i++) {
            gallery[i] = dir.absoluteFilePath(gallery[i]);
        }
        ui.showInfo->setText(QString("Í¼¿â¹²:%1ÕÅÍ¼Ïñ").arg(gallery.length()));
        ui.extractFeat->setEnabled(true);
    }
}

void MainWindow::on_extractFeat_clicked()
{
    std::vector<cv::Mat> image_features_stack(gallery.length());
    for (int i = 0; i < gallery.length(); i++)
    {
        QString qtF = gallery[i];
        std::string cvF = qtF.toLocal8Bit().toStdString();
        cv::Mat image = cv::imread(cvF);
        cv::cvtColor(image, image, cv::COLOR_BGR2RGB);
        qDebug() << "encoding image: " << qtF;
        clip->encode_image(image, image_features_stack[i]);
    }
    cv::vconcat(image_features_stack, image_features);
    ui.go->setEnabled(true);
}

void MainWindow::on_go_clicked()
{
    QString qText = ui.targetText->text();
    if (!qText.isEmpty()) {
        clip->encode_text(qText.toLower().toLocal8Bit().toStdString(), text_features);
        cv::Mat logits = 100.0f * image_features * text_features.t();
        cv::Mat prob(1, gallery.length(), CV_32FC1);
        softmax<float>((float*)logits.data, (float*)prob.data, gallery.length());
        cv::Point maxLoc;
        cv::minMaxLoc(prob, NULL, NULL, NULL, &maxLoc);
        qDebug() << "encoding text: " << qText << " match image:" << gallery[maxLoc.x] << " prob:" << prob.at<float>(maxLoc);
        {
            QString qtF = gallery[maxLoc.x];
            std::string cvF = qtF.toLocal8Bit().toStdString();
            cv::Mat image = cv::imread(cvF);
            cv::cvtColor(image, image, cv::COLOR_BGR2RGB);
            QPixmap pixmap;
            pixmap = pixmap.fromImage(MatToQImage(image));
            pixmap = pixmap.scaled(ui.showImage->width(), ui.showImage->height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            ui.showImage->setPixmap(pixmap);
        }
    }
}

inline QImage MainWindow::MatToQImage(const cv::Mat& mat)
{
    QImage image(mat.data,
        mat.cols, mat.rows,
        static_cast<int>(mat.step),
        QImage::Format_RGB888);

    return image;
}