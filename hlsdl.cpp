#include "hlsdl.h"
#include "ui_hlsdl.h"
#include "core/misc.h"
#include <QMessageBox>
#include <windows.h>
#include <stdio.h>
#include <QDebug>
#include <QFileDialog>
#include <QDir>
#include "qualitydlg.h"

Hlsdl::Hlsdl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Hlsdl),
    pDownloadThread(nullptr)
{
    ui->setupUi(this);
    memset(&hls_args, 0x00, sizeof(hls_args));
    hls_args.segment_download_retries = HLSDL_MAX_RETRIES;
    hls_args.live_start_offset_sec    = HLSDL_LIVE_START_OFFSET_SEC;
    hls_args.live_duration_sec        = HLSDL_LIVE_DURATION;
    hls_args.open_max_retries         = HLSDL_OPEN_MAX_RETRIES;
    hls_args.refresh_delay_sec        = -1;
    hls_args.maxwidth  = -1;
    hls_args.maxheight = -1;
    hls_args.audiolang = NULL;
    mCustomHeaderIdx   = 0;
    ui->tabWidget->hide();
    ui->defaultBtn->hide();
    resize(minimumWidth(), height());
}

Hlsdl::~Hlsdl()
{
    if(pDownloadThread != NULL)
    {
        delete pDownloadThread;
    }
    if(hls_args.user_agent)
    {
        free(hls_args.user_agent);
    }
    if(hls_args.proxy_uri)
    {
        free(hls_args.proxy_uri);
    }
    if(hls_args.url)
    {
        free(hls_args.url);
    }
    for(int i=0; i<mCustomHeaderIdx; i++)
    {
        char * curHeader = hls_args.custom_headers[i];
        if(curHeader)
        {
            free(curHeader);
        }
    }
    delete ui;
}

void Hlsdl::initArg()
{
    if(ui->qualityCb->isChecked())
    {
        showLog("use best quality!!");
        hls_args.use_best = true;
    }
    if(ui->ignoreDrmCb->isChecked())
    {
        showLog("ignore Drm!!!");
        hls_args.force_ignoredrm = true;
    }
    hls_args.url = strdup(ui->m3u8linkLe->text().toLatin1().data());
    hls_args.live_start_offset_sec    = ui->offsetSecLe->text().toInt();
    hls_args.live_duration_sec        = ui->durationSecLe->text().toInt();
    hls_args.refresh_delay_sec        = ui->refreshSecLe->text().toInt();
    hls_args.segment_download_retries = ui->retriesLe->text().toInt();
    hls_args.open_max_retries         = HLSDL_OPEN_MAX_RETRIES;
    hls_args.maxwidth  = -1;
    hls_args.maxheight = -1;
    hls_args.audiolang = NULL;
    QString str;
    str = ui->userAgentLe->text();
    if(!str.isEmpty())
    {
        hls_args.user_agent = strdup(str.toLatin1().data());
    }
    str = ui->proxyUriLe->text();
    if(!str.isEmpty())
    {
        hls_args.proxy_uri = strdup(str.toLatin1().data());
    }
    str = ui->httpHeaderLe->text();
    if(!str.isEmpty())
    {
        if (mCustomHeaderIdx < HLSDL_MAX_NUM_OF_CUSTOM_HEADERS) {
            hls_args.custom_headers[mCustomHeaderIdx] = (char*)malloc(str.length() + 1);
            strcpy(hls_args.custom_headers[mCustomHeaderIdx], str.toLatin1().data());
            mCustomHeaderIdx += 1;
        }
    }
    if(pDownloadThread == nullptr)
    {
        pDownloadThread = new DownloadThread(ui->savePathLe->text());
        connect(pDownloadThread, SIGNAL(sendMsg(QString, int)), SLOT(showLog(QString, int)));
        connect(pDownloadThread, SIGNAL(sendMediaPlayList(void*)), SLOT(chooseQuality(void*)));
    }
    return;
}

bool Hlsdl::checkArg()
{
    QString m3u8Str  = ui->m3u8linkLe->text();
    QString saveName = ui->savePathLe->text();
    if(m3u8Str.isEmpty() || saveName.isEmpty())
        return false;
    if(!m3u8Str.endsWith(".m3u8"))
    {
        QMessageBox::warning(this, "warning", "m3u8 link is wrong, please check!");
        return false;
    }
    return true;
}

void Hlsdl::on_downloadBtn_clicked()
{
    if(checkArg())
    {
        initArg();    
        ui->downloadBtn->setEnabled(false);
        pDownloadThread->start();

    }
   else
   {
       return;
   }
}

void Hlsdl::on_fileSaveBtn_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(NULL, "Save file", ".", "*.ts");
    ui->savePathLe->setText(fileName);
}

void Hlsdl::on_moreBtn_clicked()
{
    if(ui->tabWidget->isHidden())
    {
        resize(maximumWidth(),height());
        ui->tabWidget->show();
        ui->defaultBtn->show();
        ui->moreBtn->setText("<<");
    }
    else
    {
        ui->tabWidget->hide();
        ui->defaultBtn->hide();
        ui->moreBtn->setText(">>");
        resize(minimumWidth(),height());
    }
}

void Hlsdl::on_defaultBtn_clicked()
{
    if(ui->streamTab->isVisible())
    {
        ui->offsetSecLe->setText("120");
        ui->durationSecLe->setText("-1");
        ui->refreshSecLe->setText("-1");
        ui->retriesLe->setText("30");
    }
    else if(ui->webTab->isVisible())
    {
        ui->userAgentLe->setText("");
        ui->httpHeaderLe->setText("");
        ui->proxyUriLe->setText("");
    }
}

void Hlsdl::showLog(QString msg, int level)
{
    switch(level)
    {
    case DownloadThread::INFO_MSG:
        ui->msgTB->setTextColor(Qt::black);
        break;
    case DownloadThread::WARNING_MSG:
        ui->msgTB->setTextColor(Qt::blue);
        break;
    case DownloadThread::ERROR_MSG:
        ui->msgTB->setTextColor(Qt::red);
        break;

    }
    ui->msgTB->append(msg + "\n");
}

void Hlsdl::chooseQuality(void *list)
{
    QualityDlg* dlg = new QualityDlg((hls_media_playlist_t*)list);
    while(dlg->exec());
    int quality_choice = dlg->getChosedQualityIdx();
    qDebug("-----quality_choice:%d", quality_choice);
    delete dlg;
    pDownloadThread->setHadChosedQuality(quality_choice);
}
