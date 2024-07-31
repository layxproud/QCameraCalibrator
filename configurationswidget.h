#ifndef CONFIGURATIONSWIDGET_H
#define CONFIGURATIONSWIDGET_H

#include "yamlhandler.h"
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QWidget>

class ConfigurationForm : public QWidget
{
    Q_OBJECT
public:
    explicit ConfigurationForm(QWidget *parent = nullptr);
    void setData(const Configuration &config);
    Configuration getData();

signals:
    void editConfiguration(const Configuration &config);

private:
    QFormLayout *formLayout;
    QLabel *blockIdLabel;
    QLabel *blockNameLabel;
    QLabel *blockTypeLabel;
    QLabel *blockDateLabel;
    QLineEdit *blockIdInput;
    QLineEdit *blockNameInput;
    QLineEdit *blockTypeInput;
    QLineEdit *blockDateInput;
    QPushButton *editButton;

private slots:
    void onEditButton();
};

class ConfigurationsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ConfigurationsWidget(QWidget *parent = nullptr);

    void setConfigurations(const std::map<std::string, Configuration> &configurations);

signals:
    void editConfiguration(const Configuration &config);

private:
    void clearLayout(QLayout *layout);
};

#endif // CONFIGURATIONSWIDGET_H
