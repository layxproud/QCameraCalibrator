#include "graphicsviewcontainer.h"
#include <QGraphicsPixmapItem>

GraphicsViewContainer::GraphicsViewContainer(QWidget *parent)
    : QWidget(parent)
{
    scene = new QGraphicsScene(this);
    view = new QGraphicsView(scene);
    layout = new QVBoxLayout(this);

    layout->addWidget(view);
    scene->setSceneRect(0, 0, view->width(), view->height());
}

void GraphicsViewContainer::updateFrame(const cv::Mat &frame)
{
    cv::Mat frameCopy = frame.clone();
    QImage img(frameCopy.data, frameCopy.cols, frameCopy.rows, frameCopy.step, QImage::Format_RGB888);
    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(QPixmap::fromImage(img.rgbSwapped()));
    scene->clear();
    scene->addItem(item);
    view->fitInView(item, Qt::KeepAspectRatio);
}
