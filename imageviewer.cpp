#include "imageviewer.h"
#include "ui_imageviewer.h"

ImageViewer::ImageViewer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ImageViewer)

{
    ui->setupUi(this);
    setAcceptDrops(true);
    initAllPosi();

    getImgData();
    imageProcessor();
}

ImageViewer::~ImageViewer()
{
    delete ui;
}

//画像座標値初期化//
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

//開始時画像パス取得//
void ImageViewer::getImgData()
{
    this->filename = QFileDialog::getOpenFileName(this,
                                                    "Select Output Image",
                                                    QDir::currentPath(),
                                                    "*.jpg;;*.png;;*.bmp");
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

//画像表示//
void ImageViewer::imageProcessor()
{
    cv::Mat tmpImg;
    QImage::Format format;

    cv::Mat affinedImg = affineImg(&imgData);

    //二値化画像表示//
    if(this->liveRadioChecked)
    {
        affinedImg = binaryChange(&affinedImg);

        if(this->rectRadioChecked)
        {
            format = QImage::Format_RGB888;
            affinedImg = rectShowImage(&affinedImg);
        }
        else format = QImage::Format_Grayscale8;
    }
    //オリジナル画像表示//
    else
    {
        format = QImage::Format_RGB888;
    }
    this->resizedImg = resizeBox(&affinedImg);

    showImageWindow(&this->resizedImg, format);
}

//QImge変換->表示//
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

//画像リサイズ変換//
cv::Mat ImageViewer::resizeBox(cv::Mat *raw_img)
{
    cv::Mat img = *raw_img;

    if(img.cols > this->IMAGE_CANVAS_WIDTH && img.rows > this->IMAGE_CANVAS_HEIGHT)
    {
        this->image_scale = std::max((double)this->IMAGE_CANVAS_WIDTH / (double)img.cols, (double)this->IMAGE_CANVAS_HEIGHT / (double)img.rows);
    }
    else
    {
        this->image_scale = std::min((double)this->IMAGE_CANVAS_WIDTH / (double)img.cols, (double)this->IMAGE_CANVAS_HEIGHT / (double)img.rows);
    }

    cv::resize(img, img, cv::Size(), this->image_scale, this->image_scale);

    this->resizedImgDataWidth = img.cols;
    this->resizedImgDataHeight = img.rows;

    return img;
}

//ピクセル輝度値情報取得//
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

//RGB->HEX変換//
QString ImageViewer::colorToHex(){
    int red   = this->pixData[2];
    int green = this->pixData[1];
    int blue  = this->pixData[0];

    std::stringstream ss;
    ss << "#" << std::hex << (red << 16 | green << 8 | blue);

    return QString::fromStdString(ss.str());
}

//BGR->Binary変換(二値化処理)//
cv::Mat ImageViewer::binaryChange(cv::Mat *img)
{
   cv::Mat mask;

   int b = this->pixData[0];
   int g = this->pixData[1];
   int r = this->pixData[2];

   cv::Scalar s_min = cv::Scalar(b - this->threshValue,
                                 g - this->threshValue,
                                 r - this->threshValue);

   cv::Scalar s_max = cv::Scalar(b + this->threshValue,
                                 g + this->threshValue,
                                 r + this->threshValue);

   cv::inRange(*img, s_min, s_max, mask);

   return mask;
}


//矩形検出/表示アルゴリズム//
cv::Mat ImageViewer::rectShowImage(cv::Mat *img)
{
    this->rectCount = 0;
    this->detectedRectResult.clear();
    cv::Mat rectImg = *img;
    cv::cvtColor(*img, rectImg, cv::COLOR_GRAY2BGR);

    //矩形表示閾値設定, 最小・最大・W/D比//
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
            //int data[4] = {x, y, w, h};
            this->rectCount ++ ;
            this->detectedRectResult.push_back({x, y, w, h});

            cv::rectangle(rectImg, cv::Point(x, y), cv::Point(x + w, y + h), cv::Scalar(0, 0, 255), 2);
        }
    }
    ui->rectCountLabel->setText(QString::number(this->rectCount));
    return rectImg;
}


//アフィン変換(イメージシフト、ズーム処理)
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


//ドラッグアンドドロップイベント操作//
void ImageViewer::dragEnterEvent(QDragEnterEvent( *event))
{

    QStringList accepterFileTypes;
    accepterFileTypes.append("jpg");
    accepterFileTypes.append("png");
    accepterFileTypes.append("bmp");

    if (event->mimeData()->hasUrls() && event->mimeData()->urls().count() ==1)
    {
        QFileInfo file(event->mimeData()->urls().at(0).toLocalFile());
        if(accepterFileTypes.contains(file.suffix().toLower()))
        {
            event->acceptProposedAction();
        }
    }
}

//画像ドロップイベント//
void ImageViewer::dropEvent(QDropEvent *event)
{
    QFileInfo file(event->mimeData()->urls().at(0).toLocalFile());
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
}

//マウスクリックイベント操作//
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

//マウス移動イベント操作//
void ImageViewer::mouseMoveEvent(QMouseEvent *event)
{
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

//マウスリリースイベント,移動量保管//
void ImageViewer::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton && event->x() < 1400 && this->imgMoveEvent)
    {
        this->shiftedPosiX += this->deltaPosiX;
        this->shiftedPosiY += this->deltaPosiY;
        this->imgMoveEvent = false;
    }
}

//マウスダブルクリックイベント、位置初期化//
void ImageViewer::mouseDoubleClickEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        initAllPosi();
        imageProcessor();
    }
}

//画像拡大縮小、マウスホイールイベント//
void ImageViewer::wheelEvent(QWheelEvent *event)
{
    int degree = event->angleDelta().y();

    if(degree > 0) this->wheelCount ++;

    if(degree < 0) this->wheelCount --;

    this->imgMaguni = 1.00 + (this->wheelCount / 10.00);

    imageProcessor();
}

//閾値スライダー動作//
void ImageViewer::on_threshSlider_sliderMoved(int position)
{
    ui->threshLabel->setText(QString::number(position));
    this->threshValue = position;    
    imageProcessor();
}

//ライブストリームON/OFF//
void ImageViewer::on_liveCheckBox_stateChanged(int arg1)
{
    if(arg1 == 2) this->liveRadioChecked = true;
    else this->liveRadioChecked = false;
    imageProcessor();
}

//矩形表示ON/OFF//
void ImageViewer::on_rectCheckBox_stateChanged(int arg1)
{
    if(arg1 == 2) this->rectRadioChecked = true;
    else this->rectRadioChecked = false;

    imageProcessor();
}

//CSVデータ出力//
void ImageViewer::on_exportCSVButton_clicked()
{
    this->savefilename = QFileDialog::getSaveFileName(this, "SelectSave", QDir::currentPath(), "*.csv");
    std::ofstream exportFile;
    exportFile.open(this->savefilename.toStdString(), std::ios::out);
    exportFile << "THreshValue:" << this->threshValue << ","
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
