#include "downloadthread.h"
#include <curl/curl.h>
#include "core/curl.h"
#include "core/misc.h"
#include <QMessageBox>
#include <QDebug>

DownloadThread::DownloadThread(QString file):
    mFileName(file),
    pFile(nullptr)
{
    mQualityIndex = 1;
    mQualitySem = new QSemaphore(0);
}

DownloadThread::~DownloadThread()
{
    media_playlist_cleanup(&media_playlist);
    curl_global_cleanup();
    if(pFile)
    {
        fclose(pFile);
    }
    terminate();
}

void DownloadThread::run()
{
    curl_global_init(CURL_GLOBAL_ALL);

    char *hlsfile_source = NULL;
    memset(&media_playlist, 0x00, sizeof(media_playlist));
    memset(&audio_media_playlist, 0x00, sizeof(audio_media_playlist));
    QString str = QString("url:%1").arg(hls_args.url);
    sendMsg(str);
    char *url = NULL;
    if (!getDataWithRetry(hls_args.url, &hlsfile_source, &url, hls_args.open_max_retries))
    {
        //return 1;
        qDebug("get Data fail!!!");
        return;
    }
    //qDebug()<<"hlsfile_source: "<<hlsfile_source;
    int playlist_type = get_playlist_type(hlsfile_source);
    if (playlist_type == MASTER_PLAYLIST && hls_args.audio_url)
    {
        //QMessageBox::critical(this, "Error", "uri to audio media playlist was set but main playlist is not media playlist.\n");
        //exit(1);
        sendMsg("uri to audio media playlist was set but main playlist is not media playlist.\n", ERROR_MSG);
        return;
    }

    if(selectMediaPlaylist(&media_playlist, &audio_media_playlist, url, hlsfile_source, playlist_type))
    {
        qDebug("select medaiplay list fail");
        //return 1;
        return;
    }

    if (audio_media_playlist.orig_url) {

        if (!getDataWithRetry(audio_media_playlist.orig_url, &audio_media_playlist.source, &audio_media_playlist.url, hls_args.open_max_retries)) {
            //return 1;
            qDebug("get data fail 222");
            return;
        }

        if (get_playlist_type(audio_media_playlist.source) != MEDIA_PLAYLIST) {
            qDebug("uri to audio media playlist was set but it is not media playlist.\n");
            exit(1);
        }
    }

    if (handle_hls_media_playlist(&media_playlist)) {
        //return 1;
        qDebug("haldle medai play list faile");
        return;
    }

    if (audio_media_playlist.url && handle_hls_media_playlist(&audio_media_playlist)) {
        //return 1;
        qDebug("audio media playlist faile");
        return;
    }

    if (media_playlist.encryption) {
        qDebug("HLS Stream is %s encrypted.\n",
                  media_playlist.encryptiontype == ENC_AES128 ? "AES-128" :
                                                                "SAMPLE-AES");
    }

    qDebug("Media Playlist parsed successfully.\n");
    saveMedia(&media_playlist, &audio_media_playlist);
    sendMsg("exit=====");
    free(url);
    media_playlist_cleanup(&media_playlist);
    curl_global_cleanup();
    //return 0;
    return;
}

void DownloadThread::setHadChosedQuality(int index)
{
    mQualityIndex = index;
    mQualitySem->release();
}

bool DownloadThread::getDataWithRetry(char *url, char **hlsfile_source, char **finall_url, int tries)
{
    size_t size = 0;
    long http_code = 0;
    while (tries) {
        http_code = get_hls_data_from_url(url, hlsfile_source, &size, STRING, finall_url);
        if (200 != http_code || size == 0) {
            QString str = QString("%1 %2 tries %3").arg(url).arg(http_code).arg(tries);
            sendMsg(str, ERROR_MSG);
            --tries;
            Sleep(1);
            continue;
        }
        break;
    }

    if (http_code != 200) {
        QString str = QString("{\"error_code\":%1, \"error_msg\":\"\"}\n").arg(http_code);
        sendMsg(str, ERROR_MSG);
    }

    if (size == 0) {
        sendMsg("{\"error_code\":-1, \"error_msg\":\"No result from server.\"}\n", ERROR_MSG);
        return false;
    }
    return true;
}

int DownloadThread::selectMediaPlaylist(hls_media_playlist_t *media_playlist, hls_media_playlist_t *audio_media_playlist, char *url, char *hlsfile_source, int playlist_type)
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
            QString str = QString("Choosing best quality. (Bitrate: %1), (Resolution: %2), (Codecs: %3)\n")
                    .arg(selected->bitrate).arg(selected->resolution).arg(selected->codecs);
            sendMsg(str);
             } else {
            // print hls master playlist
            hls_media_playlist_t *me = master_playlist.media_playlist;
            sendMediaPlayList((void*)me);
            mQualitySem->acquire();

            int i = 1;
            me = master_playlist.media_playlist;
            while (i < mQualityIndex) {
                i += 1;
                me = me->next;
            }
            selected = me;
            QString str = QString("You Choose the quality %1: Bandwidth: %2, Resolution: %3, Codecs: %4\n")
                    .arg(i)
                    .arg(me->bitrate)
                    .arg(me->resolution)
                    .arg(me->codecs);
            sendMsg(str);
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
        media_playlist->source   = hlsfile_source;
        media_playlist->bitrate  = 0;
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

int DownloadThread::saveMedia(hls_media_playlist_t* media_playlist, hls_media_playlist_t *audio_media_playlist)
{
    sendMsg("saving.....");
    int ret = -1;
    pFile = fopen(mFileName.toLatin1().data(), "wb");
    if (pFile) {
        write_ctx_t out_ctx = {priv_write, pFile};
        if (media_playlist->is_endlist) {
            ret = download_hls(&out_ctx, media_playlist, audio_media_playlist);
        } else {
            ret = download_live_hls(&out_ctx, media_playlist);
        }
        fclose(pFile);
        pFile = nullptr;
    }
    sendMsg("save success!");
    return ret ? 1 : 0;
}
