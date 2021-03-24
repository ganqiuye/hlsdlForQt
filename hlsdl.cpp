#include "hlsdl.h"
#include "ui_hlsdl.h"
#include "core/misc.h"
#include "core/curl.h"
#include <QMessageBox>
#include <curl/curl.h>
#include <windows.h>
#include <stdio.h>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QDir>

Hlsdl::Hlsdl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Hlsdl)
{
    ui->setupUi(this);
    memset(&hls_args, 0x00, sizeof(hls_args));
    hls_args.loglevel = 0;
    hls_args.segment_download_retries = HLSDL_MAX_RETRIES;
    hls_args.live_start_offset_sec = HLSDL_LIVE_START_OFFSET_SEC;
    hls_args.live_duration_sec = HLSDL_LIVE_DURATION;
    hls_args.open_max_retries = HLSDL_OPEN_MAX_RETRIES;
    hls_args.refresh_delay_sec = -1;
    hls_args.maxwidth = -1;
    hls_args.maxheight = -1;
    hls_args.audiolang = NULL;
    mCustomHeaderIdx = 0;
    ui->tabWidget->hide();
    ui->defaultBtn->hide();
    resize(minimumWidth(),height());
}

Hlsdl::~Hlsdl()
{
    if(hls_args.user_agent)
    {
        free(hls_args.user_agent);
    }
    if(hls_args.proxy_uri)
    {
        free(hls_args.proxy_uri);
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
        qDebug("use best quality!!");
        hls_args.use_best = true;
    }
    if(ui->ignoreDrmCb->isChecked())
    {
        qDebug("ignore Drm!!!");
        hls_args.force_ignoredrm = true;
    }
    hls_args.url = ui->m3u8linkLe->text().toLatin1().data();
    hls_args.live_start_offset_sec    = ui->offsetSecLe->text().toInt();
    hls_args.live_duration_sec        = ui->durationSecLe->text().toInt();
    hls_args.refresh_delay_sec        = ui->refreshSecLe->text().toInt();
    hls_args.segment_download_retries = ui->retriesLe->text().toInt();
    hls_args.open_max_retries = HLSDL_OPEN_MAX_RETRIES;
    hls_args.maxwidth = -1;
    hls_args.maxheight = -1;
    hls_args.audiolang = NULL;
    QString str;
    str = ui->userAgentLe->text();
    if(!str.isEmpty())
    {
        hls_args.user_agent = (char*)malloc(str.length() + 1);
        strcpy(hls_args.user_agent, str.toLatin1().data());
    }
    str = ui->proxyUriLe->text();
    if(!str.isEmpty())
    {
        hls_args.proxy_uri = (char*)malloc(str.length() + 1);
        strcpy(hls_args.proxy_uri, str.toLatin1().data());
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

    return;
}

bool Hlsdl::checkArg()
{
    QString m3u8Str = ui->m3u8linkLe->text();
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

bool Hlsdl::getDataWithRetry(char *url, char **hlsfile_source, char **finall_url, int tries)
{
    size_t size = 0;
    long http_code = 0;
    while (tries) {
        http_code = get_hls_data_from_url(url, hlsfile_source, &size, STRING, finall_url);
        if (200 != http_code || size == 0) {
            QString str = QString("%1 %2 tries %3").arg(url).arg(http_code).arg(tries);
            QMessageBox::critical(this, "Error", str);
            --tries;
            Sleep(1);
            continue;
        }
        break;
    }

    if (http_code != 200) {
        qDebug("{\"error_code\":%d, \"error_msg\":\"\"}\n", (int)http_code);
    }

    if (size == 0) {
        qDebug("{\"error_code\":-1, \"error_msg\":\"No result from server.\"}\n");
        return false;
    }
    return true;
}

int Hlsdl::selectMediaPlaylist(hls_media_playlist_t* media_playlist, hls_media_playlist_t* audio_media_playlist,
                               char *url, char *hlsfile_source, int playlist_type)
{
    if (playlist_type == MASTER_PLAYLIST) {
        hls_master_playlist_t master_playlist;
        memset(&master_playlist, 0x00, sizeof(master_playlist));
        master_playlist.source = hlsfile_source;
        master_playlist.url = url;
        url = NULL;
        if (handle_hls_master_playlist(&master_playlist) || !master_playlist.media_playlist) {
            return 1;
        }

        hls_media_playlist_t *selected = NULL;
        if (hls_args.use_best) {
            selected = master_playlist.media_playlist;
            hls_media_playlist_t *me = selected->next;
            while (me) {
                if (me->bitrate > selected->bitrate) {
                    selected = me;
                }
                me = me->next;
            }
            qDebug("Choosing best quality. (Bitrate: %d), (Resolution: %s), (Codecs: %s)\n", selected->bitrate, selected->resolution, selected->codecs);
        } else {
            // print hls master playlist
            int i = 1;
            int quality_choice = 0;

            hls_media_playlist_t *me = master_playlist.media_playlist;
            while (me) {
                qDebug("%d: Bandwidth: %d, Resolution: %s, Codecs: %s\n", i, me->bitrate, me->resolution, me->codecs);
                i += 1;
                me = me->next;
            }

            qDebug("Which Quality should be downloaded? ");
            if (scanf("%d", &quality_choice) != 1 || quality_choice <= 0 || quality_choice >= i) {
                qDebug("Wrong input!\n");
                exit(1);
            }

            i = 1;
            me = master_playlist.media_playlist;
            while (i < quality_choice) {
                i += 1;
                me = me->next;
            }

            selected = me;
        }

        if (!selected) {
            qDebug("Wrong selection!\n");
            exit(1);
        }

        if (selected->audio_grp) {
            // check if have valid group
            hls_audio_t *selected_audio = NULL;
            hls_audio_t *audio = master_playlist.audio;
            bool has_audio_playlist = false;

            while (audio) {
                if (0 == strcmp(audio->grp_id, selected->audio_grp)) {
                    if (has_audio_playlist) {
                        selected_audio = NULL; // more then one audio playlist, so selection is needed
                        break;
                    } else {
                        has_audio_playlist = true;
                        selected_audio = audio;
                    }
                }
            }

            if (has_audio_playlist) {
                // print hls master playlist
                int audio_choice = 0;
                int i = 1;

                if (!selected_audio) {
                    if (hls_args.use_best || hls_args.audiolang) {
                        i = 0;
                        audio = master_playlist.audio;
                        while (audio) {
                            if (0 == strcmp(audio->grp_id, selected->audio_grp)) {
                                i += 1;
                                if (hls_args.use_best && audio->is_default) {
                                    audio_choice = i;
                                    break;
                                }
                                if (hls_args.audiolang && audio->lang && 0 == strcmp(audio->lang, hls_args.audiolang)) {
                                    audio_choice = i;
                                    break;
                                }
                            }
                            audio = audio->next;
                        }
                    }

                    if (audio_choice == 0) {
                        audio = master_playlist.audio;
                        i = 0;
                        while (audio) {
                            if (0 == strcmp(audio->grp_id, selected->audio_grp)) {
                                qDebug("%d: Name: %s, Language: %s\n", i, audio->name, audio->lang ? audio->lang : "unknown");
                            }
                            i += 1;
                            audio = audio->next;
                        }

                        qDebug("Which Language should be downloaded? ");
                        if (scanf("%d", &audio_choice) != 1 || audio_choice < 0 || audio_choice >= i) {
                            qDebug("Wrong input!\n");
                            exit(1);
                        }
                    }

                    i = 0;
                    audio = master_playlist.audio;
                    while (audio) {
                        if (0 == strcmp(audio->grp_id, selected->audio_grp)) {
                            i += 1;
                            if (i == audio_choice) {
                                selected_audio = audio;
                                break;
                            }
                        }
                        audio = audio->next;
                    }

                    if (!selected_audio) {
                        qDebug("Wrong selection!\n");
                        exit(1);
                    }
                }

                audio_media_playlist->orig_url = strdup(selected_audio->url);
            }
        }

        // make copy of structure
        memcpy(media_playlist, selected, sizeof(hls_media_playlist_t));
        /* we will take this attrib to selected playlist */
        selected->url = NULL;
        selected->audio_grp = NULL;
        selected->resolution = NULL;
        selected->codecs = NULL;

        media_playlist->orig_url = strdup(media_playlist->url);
        master_playlist_cleanup(&master_playlist);
    } else if (playlist_type == MEDIA_PLAYLIST) {
        media_playlist->source = hlsfile_source;
        media_playlist->bitrate = 0;
        media_playlist->orig_url = strdup(hls_args.url);
        media_playlist->url      = url;
        url = NULL;

        if (hls_args.audio_url) {
            audio_media_playlist->orig_url = strdup(hls_args.audio_url);
        }
    } else {
        return 1;
    }
    return 0;
}

static size_t priv_write(const uint8_t *data, size_t len, void *opaque) {
    return fwrite(data, 1, len, (FILE*)opaque);
}

FILE *Hlsdl::getOutputFile()
{
    FILE *pFile = NULL;
    pFile = fopen(ui->savePathLe->text().toLatin1().data(), "wb");
    return pFile;
}


int Hlsdl::saveMedia(hls_media_playlist_t media_playlist, hls_media_playlist_t audio_media_playlist)
{
    int ret = -1;
    FILE *out_file = getOutputFile();
    if (out_file) {
        write_ctx_t out_ctx = {priv_write, out_file};
        if (media_playlist.is_endlist) {
            ret = download_hls(&out_ctx, &media_playlist, &audio_media_playlist);
        } else {
            ret = download_live_hls(&out_ctx, &media_playlist);
        }
        fclose(out_file);
    }
    qDebug("save success!");
    return ret ? 1 : 0;
}

int Hlsdl::dowloading()
{
    curl_global_init(CURL_GLOBAL_ALL);

    char *hlsfile_source = NULL;
    hls_media_playlist_t media_playlist;
    hls_media_playlist_t audio_media_playlist;
    memset(&media_playlist, 0x00, sizeof(media_playlist));
    memset(&audio_media_playlist, 0x00, sizeof(audio_media_playlist));

    char *url = NULL;
    if (!getDataWithRetry(hls_args.url, &hlsfile_source, &url, hls_args.open_max_retries))
    {
        return 1;
    }

    int playlist_type = get_playlist_type(hlsfile_source);
    if (playlist_type == MASTER_PLAYLIST && hls_args.audio_url)
    {
        QMessageBox::critical(this, "Error", "uri to audio media playlist was set but main playlist is not media playlist.\n");
        exit(1);
    }

    if(selectMediaPlaylist(&media_playlist, &audio_media_playlist, url, hlsfile_source, playlist_type))
        return 1;

    if (audio_media_playlist.orig_url) {

        if (!getDataWithRetry(audio_media_playlist.orig_url, &audio_media_playlist.source, &audio_media_playlist.url, hls_args.open_max_retries)) {
            return 1;
        }

        if (get_playlist_type(audio_media_playlist.source) != MEDIA_PLAYLIST) {
            qDebug("uri to audio media playlist was set but it is not media playlist.\n");
            exit(1);
        }
    }

    if (handle_hls_media_playlist(&media_playlist)) {
        return 1;
    }

    if (audio_media_playlist.url && handle_hls_media_playlist(&audio_media_playlist)) {
        return 1;
    }

    if (media_playlist.encryption) {
        qDebug("HLS Stream is %s encrypted.\n",
                  media_playlist.encryptiontype == ENC_AES128 ? "AES-128" :
                                                                "SAMPLE-AES");
    }

    qDebug("Media Playlist parsed successfully.\n");

    saveMedia(media_playlist, audio_media_playlist);
    qDebug("exit====");
    free(url);
    media_playlist_cleanup(&media_playlist);
    curl_global_cleanup();
    return 0;
}

void Hlsdl::on_downloadBtn_clicked()
{
    if(checkArg())
    {
        initArg();
        dowloading();
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
