#include "graphicsviewcontainer.h"

GraphicsViewContainer::GraphicsViewContainer(QWidget *parent)
    : QWidget(parent)
    , pixmapItem(new QGraphicsPixmapItem())
{
    scene = new QGraphicsScene(this);
    view = new QGraphicsView(scene);
    layout = new QVBoxLayout(this);
    scene->addItem(pixmapItem);

    layout->addWidget(view);
    scene->setSceneRect(0, 0, view->width(), view->height());
}

void GraphicsViewContainer::updateFrame(const cv::Mat &frame)
{
    cv::Mat frameCopy = frame.clone();
    QImage img(frameCopy.data, frameCopy.cols, frameCopy.rows, frameCopy.step, QImage::Format_RGB888);
    pixmapItem->setPixmap(QPixmap::fromImage(img.rgbSwapped()));
    view->update();
}
