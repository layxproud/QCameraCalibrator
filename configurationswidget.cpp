#include "configurationswidget.h"
#include <QDateTime>

ConfigurationForm::ConfigurationForm(QWidget *parent)
    : QWidget(parent)
{
    blockIdLabel = new QLabel(tr("ID блока"), this);
    blockNameLabel = new QLabel(tr("Имя блока"), this);
    blockTypeLabel = new QLabel(tr("Тип блока"), this);
    blockDateLabel = new QLabel(tr("Дата создания"), this);

    blockIdInput = new QLineEdit(this);
    blockIdInput->setReadOnly(true);
    blockNameInput = new QLineEdit(this);
    blockTypeInput = new QLineEdit(this);
    blockDateInput = new QLineEdit(this);
    blockDateInput->setReadOnly(true);

    formLayout = new QFormLayout(this);

    formLayout->addRow(blockIdLabel, blockIdInput);
    formLayout->addRow(blockNameLabel, blockNameInput);
    formLayout->addRow(blockTypeLabel, blockTypeInput);
    formLayout->addRow(blockDateLabel, blockDateInput);

    setLayout(formLayout);
}

void ConfigurationForm::setData(const Configuration &config)
{
    blockIdInput->setText(QString::number(config.id));
    blockTypeInput->setText(QString::fromStdString(config.type));
    blockNameInput->setText(QString::fromStdString(config.name));
    blockDateInput->setText(QString::fromStdString(config.date));
}

Configuration ConfigurationForm::getData()
{
    Configuration config{};
    config.name = blockNameInput->text().toStdString();
    config.type = blockTypeInput->text().toStdString();
    config.date = QString(QDateTime::currentDateTime().toString("dd-MM-yyyy")).toStdString();
    return config;
}

ConfigurationsWidget::ConfigurationsWidget(QWidget *parent)
    : QStackedWidget(parent)
{}

void ConfigurationsWidget::setConfigurations(
    const std::map<std::string, Configuration> &configurations)
{
    for (int i = this->count(); i >= 0; i--) {
        QWidget *widget = this->widget(i);
        this->removeWidget(widget);
    }

    if (configurations.empty()) {
        QWidget *placeholderWidget = new QWidget();
        QVBoxLayout *layout = new QVBoxLayout(placeholderWidget);
        QLabel *messageLabel
            = new QLabel(tr("Файл конфигураций не обнаружен или файл пуст"), placeholderWidget);
        layout->addWidget(messageLabel);
        this->addWidget(placeholderWidget);
    } else {
        for (const auto &pair : configurations) {
            QWidget *pageContainer = new QWidget();
            QVBoxLayout *pageLayout = new QVBoxLayout(pageContainer);

            ConfigurationForm *widget = new ConfigurationForm();
            widget->setData(pair.second);
            pageLayout->addWidget(widget);

            QPushButton *editButton = new QPushButton(tr("Редактировать"), pageContainer);
            connect(editButton, &QPushButton::clicked, this, &ConfigurationsWidget::onEditButton);
            pageLayout->addWidget(editButton);

            QPushButton *backButton = new QPushButton(tr("Назад"), pageContainer);
            QPushButton *forwardButton = new QPushButton(tr("Вперед"), pageContainer);

            connect(backButton, &QPushButton::clicked, this, &ConfigurationsWidget::goBack);
            connect(forwardButton, &QPushButton::clicked, this, &ConfigurationsWidget::goForward);

            QHBoxLayout *buttonLayout = new QHBoxLayout();
            buttonLayout->addWidget(backButton);
            buttonLayout->addWidget(forwardButton);
            pageLayout->addLayout(buttonLayout);

            this->addWidget(pageContainer);
        }
    }
}

void ConfigurationsWidget::goBack()
{
    int currentIndex = this->currentIndex();
    if (currentIndex > 0) {
        this->setCurrentIndex(currentIndex - 1);
    }
}

void ConfigurationsWidget::goForward()
{
    int currentIndex = this->currentIndex();
    int totalWidgets = this->count();
    if (currentIndex < totalWidgets - 1) {
        this->setCurrentIndex(currentIndex + 1);
    }
}

void ConfigurationsWidget::onEditButton()
{
    QWidget *currentPageContainer = this->currentWidget();

    if (!currentPageContainer) {
        return;
    }

    foreach (QObject *obj, currentPageContainer->children()) {
        ConfigurationForm *currentConfigForm = qobject_cast<ConfigurationForm *>(obj);

        if (currentConfigForm != nullptr) {
            emit editConfiguration(currentConfigForm->getData());
            break;
        }
    }
}
