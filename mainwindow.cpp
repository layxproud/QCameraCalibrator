#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QShortcut>
#include <QUuid>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , workspace(new Workspace(this))
    , graphicsViewContainer(new GraphicsViewContainer(this))
    , configurationsWidget(new ConfigurationsWidget(this))
{
    ui->setupUi(this);
    resize(1180, 560);

    ui->cameraLayout->addWidget(graphicsViewContainer);
    ui->editorLayout->addWidget(configurationsWidget);
    ui->toolBox->setCurrentIndex(0);

    QShortcut *captuteFrameShortcut = new QShortcut(Qt::Key_Space, ui->captureButton);

    // Connects from GUI
    connect(ui->captureButton, &QPushButton::clicked, workspace, &Workspace::onCaptureFrame);
    QObject::connect(
        captuteFrameShortcut, &QShortcut::activated, workspace, &Workspace::onCaptureFrame);
    connect(ui->calibrateButton, &QPushButton::clicked, workspace, &Workspace::onStartCalibration);
    connect(ui->toolBox, &QToolBox::currentChanged, workspace, &Workspace::onPageChanged);
    connect(ui->exportConfigButton, &QPushButton::clicked, this, &MainWindow::onExportConfiguration);
    connect(
        ui->markerSizeInput,
        QOverload<int>::of(&QSpinBox::valueChanged),
        workspace,
        &Workspace::onMarkerSizeChanged);
    connect(
        ui->saveToConfigurationsButton,
        &QPushButton::clicked,
        this,
        &MainWindow::onSaveConfiguration);
    connect(
        ui->saveSingleConfigButton,
        &QPushButton::clicked,
        this,
        &MainWindow::onSaveSingleConfiguration);
    connect(this, &MainWindow::pointSelected, workspace, &Workspace::pointSelected);
    connect(this, &MainWindow::saveConfiguration, workspace, &Workspace::saveConfiguration);
    connect(
        this, &MainWindow::saveSingleConfiguration, workspace, &Workspace::saveSingleConfiguration);
    connect(this, &MainWindow::exportConfiguration, workspace, &Workspace::exportConfiguration);
    connect(
        configurationsWidget,
        &ConfigurationsWidget::editConfiguration,
        workspace,
        &Workspace::editConfiguration);
    connect(
        configurationsWidget,
        &ConfigurationsWidget::removeConfiguration,
        workspace,
        &Workspace::removeConfiguration);

    // Connects to GUI
    connect(
        workspace,
        &Workspace::frameReady,
        graphicsViewContainer,
        &GraphicsViewContainer::updateFrame);
    connect(workspace, &Workspace::taskFinished, this, &MainWindow::onTaskFinished);
    connect(
        workspace,
        &Workspace::calibrationParamsMissing,
        this,
        &MainWindow::onCalibrationParametersMissing);
    connect(workspace, &Workspace::newConfiguration, this, &MainWindow::onNewConfiguration);
    connect(workspace, &Workspace::configurationsUpdated, this, &MainWindow::onCofigurationsUpdated);
    connect(workspace, &Workspace::calibrationUpdated, this, &MainWindow::onCalibrationUpdated);
    connect(workspace, &Workspace::frameCaptured, this, &MainWindow::onFrameCaptured);

    workspace->init();
    configurationsWidget->setConfigurations(workspace->getConfigurations());
}

MainWindow::~MainWindow()
{
    delete ui;
}

// Detects clicks on the camera widget and sends mouse coordinates
// to workspace and markerThread
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (graphicsViewContainer->geometry().contains(event->pos())) {
            QGraphicsView *view = graphicsViewContainer->getView();
            QPointF mappedPos = view->mapToScene(view->mapFromGlobal(event->globalPos()));
            emit pointSelected(mappedPos);
        }
    }
    QMainWindow::mousePressEvent(event);
}

Configuration MainWindow::formConfiguration()
{
    Configuration newConfiguration{};
    newConfiguration.name = ui->blockNameInput->text().toStdString();
    newConfiguration.type = ui->blockTypeInput->text().toStdString();
    newConfiguration.date
        = QString(QDateTime::currentDateTime().toString("dd-MM-yyyy")).toStdString();
    if (ui->blockIdInput->text() == "") {
        QUuid uuid = QUuid::createUuid();
        QString uuidString = uuid.toString(QUuid::WithoutBraces);
        newConfiguration.id = uuidString.toStdString();
    } else {
        newConfiguration.id = ui->blockIdInput->text().toStdString();
    }
    return newConfiguration;
}

void MainWindow::onTaskFinished(bool success, const QString &message)
{
    if (success) {
        QMessageBox::information(this, tr("Успех"), message);
    } else {
        QMessageBox::warning(this, tr("Ошибка"), message);
    }
}

void MainWindow::onNewConfiguration(const Configuration &config)
{
    ui->blockIdInput->setText(QString::fromStdString(config.id));
    ui->blockDateInput->setText(QString::fromStdString(config.date));
    ui->blockNameInput->setText(QString::fromStdString(config.name));
    ui->blockTypeInput->setText(QString::fromStdString(config.type));
}

void MainWindow::onCalibrationParametersMissing()
{
    ui->toolBox->setCurrentIndex(0);
}

void MainWindow::onSaveConfiguration()
{
    Configuration newConfiguration = formConfiguration();
    emit saveConfiguration(newConfiguration);
}

void MainWindow::onSaveSingleConfiguration()
{
    Configuration newConfiguration = formConfiguration();
    QString fileName = QFileDialog::getSaveFileName(
        this, tr("Сохранение блока"), QString(), tr("YAML files (*.yml)"));
    emit saveSingleConfiguration(newConfiguration, fileName);
}

void MainWindow::onCofigurationsUpdated()
{
    configurationsWidget->setConfigurations(workspace->getConfigurations());
}

void MainWindow::onCalibrationUpdated(bool status)
{
    if (status) {
        ui->calibrationStatusLabel->setText(tr("Файл калибровки загружен"));
    } else {
        ui->calibrationStatusLabel->setText(tr("Файл калибровки отсутствует"));
    }
}

void MainWindow::onFrameCaptured(int num)
{
    ui->framesCapturedValue->setText(QString::number(num));
}

void MainWindow::onExportConfiguration()
{
    QString fileName = QFileDialog::getOpenFileName(
        this, tr("Экспорт блока"), QString(), tr("YAML files (*.yml)"));
    emit exportConfiguration(fileName);
}
