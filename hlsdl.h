#ifndef HLSDL_H
#define HLSDL_H

#include <QWidget>
#include "downloadthread.h"

namespace Ui {
class Hlsdl;
}

class Hlsdl : public QWidget
{
    Q_OBJECT

public:
    explicit Hlsdl(QWidget *parent = 0);
    ~Hlsdl();

private slots:
    void on_downloadBtn_clicked();

    void on_fileSaveBtn_clicked();

    void on_moreBtn_clicked();

    void on_defaultBtn_clicked();

    void showLog(QString msg, int level=DownloadThread::INFO_MSG);

    void chooseQuality(void* list);


private:
    Ui::Hlsdl *ui;

    void initArg();

    bool checkArg();

private:
    int mCustomHeaderIdx;
    DownloadThread *pDownloadThread;
};

#endif // HLSDL_H
