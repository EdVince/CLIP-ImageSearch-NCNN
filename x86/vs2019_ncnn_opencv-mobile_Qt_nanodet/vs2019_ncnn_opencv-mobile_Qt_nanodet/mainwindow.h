#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_mainwindow.h"

#include <opencv2/opencv.hpp>
#include <QFileDialog>

#include "clip.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = Q_NULLPTR);

private:
    inline cv::Mat QImageToMat(const QImage& image);
    inline QImage MatToQImage(const cv::Mat& mat);

private slots:
    void on_chooseImageFolder_clicked();
    void on_extractFeat_clicked();
    void on_go_clicked();

private:
    Ui::MainWindowClass ui;

    CLIP* clip;

    QStringList gallery;

    cv::Mat image_features;
    cv::Mat text_features;
};
