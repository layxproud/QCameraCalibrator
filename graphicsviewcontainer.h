#ifndef GRAPHICSVIEWCONTAINER_H
#define GRAPHICSVIEWCONTAINER_H

#include <opencv2/opencv.hpp>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QVBoxLayout>
#include <QWidget>

class GraphicsViewContainer : public QWidget
{
    Q_OBJECT

public:
    explicit GraphicsViewContainer(QWidget *parent = nullptr);

    QGraphicsView *getView() { return view; }

public slots:
    void updateFrame(const cv::Mat &frame);

private:
    QGraphicsScene *scene;
    QGraphicsView *view;
    QVBoxLayout *layout;
};

#endif // GRAPHICSVIEWCONTAINER_H
