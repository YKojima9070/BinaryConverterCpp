#include "imageviewer.h"
//#include "ui_imageviewer1680.h"
#include "ui_imageviewer1366.h"
#include <filesystem>
#include <fstream>
#include <QDebug>


ImageViewer::ImageViewer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ImageViewer)

{
    ui->setupUi(this);
    setAcceptDrops(true);
    initAllPosi();

    this->filename = QFileDialog::getOpenFileName(this, "Select Output Image", QDir::currentPath(), "*.jpg *.png *.bmp *.jpeg *.tiff");

    getImgData();
    imageProcessor();
}

ImageViewer::~ImageViewer()
{
    delete ui;
}

///////////////////
//ImageProcessing//
///////////////////

//ImageCoordinate//
void ImageViewer::initAllPosi()
{
    this->targetPosiX = 0;
    this->targetPosiY = 0;

    this->orgPosiX = 0;
    this->orgPosiY = 0;

    this->deltaPosiX = 0;
    this->deltaPosiY = 0;

    this->shiftedPosiX = 0;
    this->shiftedPosiY = 0;

    this->wheelCount = 0;
    this->imgMaguni = 1;
}

//GetImagePATH//
void ImageViewer::getImgData()
{
    /*
    if(QFile::exists(fileName))
    {
        ui->inputLineEdit->setText(fileName);
    }
    */
    this->imgData = cv::imread(this->filename.toStdString());

    this->imgDataWidth = this->imgData.cols;
    this->imgDataHeight = this->imgData.rows;
}

//DisplayImage//
void ImageViewer::imageProcessor()
{
    cv::Mat tmpImg, rectedImg;
    QImage::Format format;

    if(this->liveRadioChecked)
    {
        if(this->rectRadioChecked)
        {
            tmpImg = binaryChange(&this->imgData);
            tmpImg = rectShowImage(&tmpImg);
            format = QImage::Format_RGB888;
        }
        else
        {
            tmpImg = binaryChange(&this->imgData);
            format = QImage::Format_Grayscale8;
        }
        tmpImg = affineImg(&tmpImg);
    }
    else
    {
        tmpImg = affineImg(&this->imgData);
        format = QImage::Format_RGB888;
    }
    this->resizedImg = resizeBox(&tmpImg);

    showImageWindow(&this->resizedImg, format);
}

//ConvertToQImage//
void ImageViewer::showImageWindow(cv::Mat *img, QImage::Format format)
{
    cv::Mat pImg = *img;
    QImage image(pImg.data,
                 pImg.cols,
                 pImg.rows,
                 pImg.step,
                 format);
    QPixmap m_pix_map = QPixmap::fromImage(image.rgbSwapped());
    ui->imgLabel->setPixmap(m_pix_map);
}

//ResizeImage//
cv::Mat ImageViewer::resizeBox(cv::Mat *raw_img)
{
    cv::Mat img = *raw_img;

    double xScale = (double)this->IMAGE_CANVAS_WIDTH / (double)img.cols;
    double yScale = (double)this->IMAGE_CANVAS_HEIGHT / (double)img.rows;

    this->image_scale = (double)std::min(xScale, yScale);

    cv::resize(img, img, cv::Size(), this->image_scale, this->image_scale);

    this->resizedImgDataWidth = img.cols;
    this->resizedImgDataHeight = img.rows;

    std::cout << img.size << std::endl;


    return img;
}

//GetImagePixelData//
void ImageViewer::getPixValue(int x, int y)
{
    int calX = (((1 / this->imgMaguni) * x) - this->shiftedPosiX + (this->resizedImgDataWidth - (this->resizedImgDataWidth / this->imgMaguni)) / 2) / this->image_scale;
    int calY = (((1 / this->imgMaguni) * y) - this->shiftedPosiY + (this->resizedImgDataHeight - (this->resizedImgDataHeight / this->imgMaguni)) / 2) / this->image_scale;

    if(this->resizedImg.channels() >= 3){
        this->pixData[0] = this->imgData.at<cv::Vec3b>(calY, calX)[0]; //Blue
        this->pixData[1] = this->imgData.at<cv::Vec3b>(calY, calX)[1]; //Green
        this->pixData[2] = this->imgData.at<cv::Vec3b>(calY, calX)[2]; //Red
    }
    else this->pixData[0] = this->imgData.at<unsigned char>(calY, calX);
}

//RGB->HEX//
QString ImageViewer::colorToHex(){
    int red   = this->pixData[2];
    int green = this->pixData[1];
    int blue  = this->pixData[0];

    std::stringstream ss;
    ss << "#" << std::hex << (red << 16 | green << 8 | blue);

    return QString::fromStdString(ss.str());
}

//BGR->Binary//
cv::Mat ImageViewer::binaryChange(cv::Mat *img)
{
   cv::Mat mask;

   int b = this->pixData[0];
   int g = this->pixData[1];
   int r = this->pixData[2];

   cv::Scalar s_min = cv::Scalar(b - this->threshValueLow,
                                 g - this->threshValueLow,
                                 r - this->threshValueLow);

   cv::Scalar s_max = cv::Scalar(b + this->threshValueUpp,
                                 g + this->threshValueUpp,
                                 r + this->threshValueUpp);

   cv::inRange(*img, s_min, s_max, mask);

   return mask;
}


//DetectRect//
cv::Mat ImageViewer::rectShowImage(cv::Mat *img)
{
    this->rectCount = 0;
    this->detectedRectResult.clear();
    cv::Mat rectImg = *img;
    cv::cvtColor(*img, rectImg, cv::COLOR_GRAY2BGR);

    //SetThreshold Image Min,MaxSize//
    this->minRectSize = ui->minRectSpinBox->text().toInt();
    this->maxRectSize = ui->maxRectSpinBox->text().toInt();
    this->aspectRatio = ui->aspectRatioSpinBox->text().toDouble();

    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;

    cv::findContours(*img, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for(auto contour : contours)
    {
        cv::Rect rects = cv::boundingRect(contour);
        int x = rects.x;
        int y = rects.y;
        int w = rects.width;
        int h = rects.height;

        if(std::min(w, h) > this->minRectSize && std::max(w, h) < this->maxRectSize && ((std::max(w, h) / std::min(w, h)) < this->aspectRatio))
        {
            this->rectCount ++ ;
            this->detectedRectResult.push_back({x, y, w, h});

            cv::rectangle(rectImg, cv::Point(x, y), cv::Point(x + w, y + h), cv::Scalar(0, 0, 255), 2);
        }
    }
    ui->rectCountLabel->setText(QString::number(this->rectCount));
    return rectImg;
}


//AffineProcess
cv::Mat ImageViewer::affineImg(cv::Mat *img)
{
    cv::Mat shiftedImg;
    cv::Mat dstImg;
    cv::Mat affineMat;
    std::vector<cv::Point2f> src, dstShift, dstZoom;

    src.push_back(cv::Point2f(0, 0));
    src.push_back(cv::Point2f(0, 1));
    src.push_back(cv::Point2f(1, 0));

    dstShift.push_back(cv::Point2f(this->targetPosiX, this->targetPosiY));
    dstShift.push_back(cv::Point2f(this->targetPosiX, this->targetPosiY + 1));
    dstShift.push_back(cv::Point2f(this->targetPosiX + 1, this->targetPosiY));

    affineMat = cv::getAffineTransform(src, dstShift);
    cv::warpAffine(*img, shiftedImg, affineMat, cv::Point(this->imgDataWidth, this->imgDataHeight));

    dstZoom.push_back(cv::Point2f(-(this->imgDataWidth / 2),     -(this->imgDataHeight / 2)));
    dstZoom.push_back(cv::Point2f(-(this->imgDataWidth / 2),     -(this->imgDataHeight / 2) + 1));
    dstZoom.push_back(cv::Point2f(-(this->imgDataWidth / 2) + 1, -(this->imgDataHeight / 2)));

    for(int i=0; i < (int)dstZoom.size(); i++) dstZoom[i] = dstZoom[i] * this->imgMaguni;

    for(int i=0; i < (int)dstZoom.size(); i++) dstZoom[i].x = dstZoom[i].x + this->imgDataWidth / 2;

    for(int i=0; i < (int)dstZoom.size(); i++) dstZoom[i].y = dstZoom[i].y + this->imgDataHeight / 2;

    affineMat = cv::getAffineTransform(src, dstZoom);
    cv::warpAffine(shiftedImg, dstImg, affineMat, cv::Point(this->imgDataWidth, this->imgDataHeight));

    return dstImg;
}

////////////////
//FileHandling//
////////////////

//ExportCSVData//
void ImageViewer::on_exportCSVButton_clicked()
{
    this->savefilename = QFileDialog::getSaveFileName(this, "SelectSave", QDir::currentPath(), "*.csv");
    exportCSVFile();
}

void ImageViewer::exportCSVFile()
{
    std::ofstream exportFile;
    exportFile.open(this->savefilename.toStdString(), std::ios::out);
    exportFile << "ThreshValueLow:" << this->threshValueLow << ","
               << "ThreshValueUpp:" << this->threshValueUpp << ","
               << "MinRectSize:" << this->minRectSize << ","
               << "MaxRectSize:" << this->maxRectSize << ","
               << "AspectRatio:" << this->aspectRatio << std::endl;
    exportFile << "x," << "y," << "w," << "h" << std::endl;

    for (int i=0; i < (int)this->detectedRectResult.size(); i++ )
    {
        for (int j=0; j < (int)this->detectedRectResult.at(i).size(); j++)
        {
            exportFile << this->detectedRectResult.at(i).at(j) << ",";
            if(j==(int)this->detectedRectResult.at(i).size()-1) exportFile<<std::endl;
        }
    }
}


////////////////////////
//Mouse,EVENT Handling//
////////////////////////

//DragAndDropHandling//
void ImageViewer::dragEnterEvent(QDragEnterEvent( *event))
{

    QStringList accepterFileTypes;
    accepterFileTypes.append("jpg");
    accepterFileTypes.append("png");
    accepterFileTypes.append("bmp");
    accepterFileTypes.append("jpeg");
    accepterFileTypes.append("tiff");

    if (event->mimeData()->hasUrls() && event->mimeData()->urls().count() ==1)
    {
        QFileInfo file(event->mimeData()->urls().at(0).toLocalFile());
        if(accepterFileTypes.contains(file.suffix().toLower()))
        {
            event->acceptProposedAction();
        }
    }
}

//DropImageEvent//
void ImageViewer::dropEvent(QDropEvent *event)
{
    QFileInfo file(event->mimeData()->urls().at(0).toLocalFile());

    this->imgData = cv::imread(file.absoluteFilePath().toStdString());
    imageProcessor();
    /*
    if(pixmap.load(file.absoluteFilePath()))
    {
        ui->imgLabel->setPixmap(pixmap.scaled(ui->imgLabel->size(),
                                           Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation));
    }
    else
    {
        QMessageBox::critical(this, tr("Error"), tr("The image file cannot be read!"));
    }
    */
}

//ClickMouseEvent//
void ImageViewer::mousePressEvent(QMouseEvent *event)
{
    int x = event->x() - 6;
    int y = event->y() - 6;

    if(event->button() == Qt::RightButton)
    {
        getPixValue(x, y);

        ui->class0Label->setStyleSheet("background-color:"+ colorToHex());
    }

    if(event->button() == Qt::LeftButton && event->x() < 1400)
    {
        this->orgPosiX = x;
        this->orgPosiY = y;
    }
}

//MoveMouseEvent//
void ImageViewer::mouseMoveEvent(QMouseEvent *event)
{
    this->analyzeFlag = false;
    int x = event->x() - 6;
    int y = event->y() - 6;

     if(event->buttons() & Qt::LeftButton && event->x() < 1400)
    {
         this->imgMoveEvent = true;

         this->deltaPosiX = (x - this->orgPosiX) / this->imgMaguni;
         this->deltaPosiY = (y - this->orgPosiY) / this->imgMaguni;

         this->targetPosiX = (this->deltaPosiX + this->shiftedPosiX) / this->image_scale;
         this->targetPosiY = (this->deltaPosiY + this->shiftedPosiY) / this->image_scale;

         imageProcessor();
     }
}

//ReleaseMouseButtonEvent//
void ImageViewer::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton && event->x() < 1400 && this->imgMoveEvent)
    {
        this->shiftedPosiX += this->deltaPosiX;
        this->shiftedPosiY += this->deltaPosiY;
        this->imgMoveEvent = false;
    }
}

//Double-ClickButtonEvent//
void ImageViewer::mouseDoubleClickEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        initAllPosi();
        imageProcessor();
    }
}

//MouseWheelEvent//
void ImageViewer::wheelEvent(QWheelEvent *event)
{
    this->analyzeFlag = false;
    int degree = event->angleDelta().y();

    if(degree > 0) this->wheelCount ++;

    if(degree < 0) this->wheelCount --;

    this->imgMaguni = 1.00 + (this->wheelCount / 10.00);

    imageProcessor();
}

//LowThresholdSliderEvent//
void ImageViewer::on_threshSliderLow_sliderMoved(int position)
{
    this->analyzeFlag = true;
    ui->threshLabelLow->setText(QString::number(position));
    this->threshValueLow = position;
    imageProcessor();
}

//UpperThresholdSliderEvent//
void ImageViewer::on_threshSliderUpp_sliderMoved(int position)
{
    this->analyzeFlag = true;
    ui->threshLabelUpp->setText(QString::number(position));
    this->threshValueUpp = position;
    imageProcessor();
}

//LiveViewEventON/OFF//
void ImageViewer::on_liveCheckBox_stateChanged(int arg1)
{
    this->analyzeFlag = true;
    if(arg1 == 2) this->liveRadioChecked = true;
    else this->liveRadioChecked = false;
    imageProcessor();
}

//DisplayRectON/OFF//
void ImageViewer::on_rectCheckBox_stateChanged(int arg1)
{
    this->analyzeFlag = true;
    if(arg1 == 2) this->rectRadioChecked = true;
    else this->rectRadioChecked = false;

    imageProcessor();
}

void ImageViewer::on_exportCSVButtonBatch_clicked()
{
    std::string dirPath;
    this->filename = QFileDialog::getOpenFileName(this, "Select Output Image", QDir::currentPath(), "*.jpg *.png *.bmp *.jpeg *.tiff");
    std::string ext("*.jpg *.png *.bmp *.jpeg *.tiff");

#ifdef __APPLE__

    int path_i = this->filename.toStdString().find_last_of("/") + 1;
    dirPath = this->filename.toStdString().substr(0, path_i);

#endif

#ifdef _WIN32

    int path_i = this->filename.toStdString().find_last_of("\\") + 1;
    dirPath = this->filename.toStdString().substr(0, path_i+1);

#endif
    QDir dir = QString::fromUtf8(dirPath.c_str());
    QStringList list;

    initAllPosi();
    this->liveRadioChecked = true;
    this->rectRadioChecked = true;

    foreach(QString file, dir.entryList(QDir::Files))
    {
        list.append(QString::fromUtf8(dirPath.c_str()) + QDir::separator() + file);
    }

    for(int i=0; i<=list.size()-1; i++)
    {
        this->filename = list[i];
        this->savefilename = list[i].section(".", 0, 0);

        getImgData();
        imageProcessor();
        exportCSVFile();
    }
}
