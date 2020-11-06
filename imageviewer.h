//#ifndef MAINWINDOW_H
//#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QFileDialog>
#include <QPixmap>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPalette>
#include <QMimeData>

#include <string>
#include <fstream>

#include "opencv2/opencv.hpp"

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
    void on_threshSliderLow_sliderMoved(int position);
    void on_liveCheckBox_stateChanged(int arg1);
    void on_rectCheckBox_stateChanged(int arg1);
    void on_threshSliderUpp_sliderMoved(int position);
    void on_exportCSVButton_clicked();
    void on_exportCSVButtonBatch_clicked();

private:
    Ui::ImageViewer *ui;
    QPixmap pixmap;
    QString filename;
    QString savefilename;

     /* DisplaySize
    int IMAGE_CANVAS_WIDTH = 1400;
    int IMAGE_CANVAS_HEIGHT = 940;
    */

    int IMAGE_CANVAS_WIDTH = 1100;
    int IMAGE_CANVAS_HEIGHT = 763;

    int pixData[3];
    int imgDataWidth;
    int imgDataHeight;
    int resizedImgDataWidth;
    int resizedImgDataHeight;

    cv::Mat imgData;
    cv::Mat resizedImg;

    void getImgData();
    void getPixValue(int x, int y);
    void initAllPosi();
    void imageProcessor();
    void showImageWindow(cv::Mat *img, QImage::Format format);
    void exportCSVFile();

    QString colorToHex();
    cv::Mat resizeBox(cv::Mat *img);
    cv::Mat binaryChange(cv::Mat *img);
    cv::Mat rectShowImage(cv::Mat *img);
    cv::Mat affineImg(cv::Mat *img);

    //ImageCoordinate//
    int targetPosiX, targetPosiY;
    int orgPosiX, orgPosiY;
    int deltaPosiX, deltaPosiY;
    int shiftedPosiX, shiftedPosiY;

    //Mouse,Image,Magunification//
    int threshValueLow;
    int threshValueUpp;
    int wheelCount = 0;
    float imgMaguni = 1;
    double image_scale;

    //MouseEvent//
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    bool imgMoveEvent = false;

    //GUIEvent//
    bool liveRadioChecked;
    bool rectRadioChecked = false;
    bool analyzeFlag = false;

    //Rect//
    int rectCount;
    int minRectSize;
    int maxRectSize;
    double aspectRatio;
    std::vector<std::vector<int>> detectedRectResult;

};
//#endif // IMAGEVIEWER_H
