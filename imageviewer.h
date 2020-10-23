#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QFileDialog>
#include <QPixmap>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QMessageBox>
#include <QResizeEvent>
#include <QMouseEvent>
#include "opencv2/opencv.hpp"
#include <QPalette>

QT_BEGIN_NAMESPACE
namespace  Ui { class ImageViewer; }
QT_END_NAMESPACE

class ImageViewer : public QMainWindow
{
    Q_OBJECT

public:
    ImageViewer(QWidget *parent = nullptr);
    ~ ImageViewer();

private slots:
    void on_threshSlider_sliderMoved(int position);
    void on_liveCheckBox_stateChanged(int arg1);
    void on_rectCheckBox_stateChanged(int arg1);

private:
    Ui::ImageViewer *ui;
    QPixmap pixmap;
    QString filename;

    int IMAGE_CANVAS_WIDTH = 1400;
    int IMAGE_CANVAS_HEIGHT = 940;

    int pixData[3];
    int imgDataWidth;
    int imgDataHeight;

    cv::Mat imgData;
    cv::Mat resizedImg;

    void getImgData();
    void getPixValue(int x, int y);
    void initAllPosi();
    void imageProcessor();
    void showImageWindow(cv::Mat *img, QImage::Format format);

    QString colorToHex();
    cv::Mat resizeBox(cv::Mat *img);
    cv::Mat binaryChange(cv::Mat *img);
    cv::Mat rectShowImage(cv::Mat *img);
    cv::Mat affineImg(cv::Mat *img);

    //画像座標値計算用//
    int targetPosiX, targetPosiY;   //位置変更ターゲット座標
    int imgPosiX, imgPosiY;         //画像座標使用
    int orgPosiX, orgPosiY;         //移動前座標
    int deltaPosiX, deltaPosiY;     //ドラッグ移動時差分計算用
    int shiftedPosiX, shiftedPosiY; //画像移動後、座標算出用

    //マウス、イメージ倍率関連//
    int threshValue;
    int wheelCount = 0;
    float imgMaguni = 1;
    double image_scale;

    //マウス操作イベント関連//
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    bool imgMoveEvent = false;

    //GUIイベント用//
    bool liveRadioChecked;
    bool rectRadioChecked = false;

};


#endif // IMAGEVIEWER_H