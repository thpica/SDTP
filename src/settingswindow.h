#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QWidget>
#include <QSettings>
#include <QMessageBox>
#include <QProgressDialog>
#include <QIntValidator>
#include "rsakeyring.h"

namespace Ui {
class SettingsWindow;
}

class SettingsWindow : public QWidget
{
    Q_OBJECT
    
public:
    explicit SettingsWindow(QByteArray *fileKey, QWidget *parent = 0);
    ~SettingsWindow();

signals:
    void settingsUpdated();

public slots:
    void cancel();
    void rsaExport();
    void rsaGenerate();
    void rsaKeyGenFinished();
    void rsaImport();
    void save();

private:
    Ui::SettingsWindow *ui;
    QSettings *mSettings;
    QByteArray *mFileKey;
    QIntValidator mPortValidator;
    RsaKeyring mKeyring;
    QProgressDialog *mProgress;
};

#endif // SETTINGSWINDOW_H
