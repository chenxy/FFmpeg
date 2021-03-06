/*
 * Image format
 * Copyright (c) 2000, 2001, 2002 Fabrice Bellard
 * Copyright (c) 2004 Michael Niedermayer
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "libavutil/intreadwrite.h"
#include "libavutil/avstring.h"
#include "libavutil/log.h"
#include "libavutil/opt.h"
#include "avformat.h"
#include "avio_internal.h"
#include "internal.h"
#include "libavutil/opt.h"

typedef struct {
    const AVClass *class;  /**< Class for private options. */
    int img_number;
    int is_pipe;
    int split_planes;       /**< use independent file for each Y, U, V plane */
    char path[1024];
    int updatefirst;
} VideoMuxData;

static int write_header(AVFormatContext *s)
{
    VideoMuxData *img = s->priv_data;
    const char *str;

    av_strlcpy(img->path, s->filename, sizeof(img->path));

    /* find format */
    if (s->oformat->flags & AVFMT_NOFILE)
        img->is_pipe = 0;
    else
        img->is_pipe = 1;

    str = strrchr(img->path, '.');
    img->split_planes = str && !av_strcasecmp(str + 1, "y");
    return 0;
}

static int write_packet(AVFormatContext *s, AVPacket *pkt)
{
    VideoMuxData *img = s->priv_data;
    AVIOContext *pb[3];
    char filename[1024];
    AVCodecContext *codec= s->streams[ pkt->stream_index ]->codec;
    int i;

    if (!img->is_pipe) {
        if (av_get_frame_filename(filename, sizeof(filename),
                                  img->path, img->img_number) < 0 && img->img_number>1 && !img->updatefirst) {
            av_log(s, AV_LOG_ERROR,
                   "Could not get frame filename number %d from pattern '%s'\n",
                   img->img_number, img->path);
            return AVERROR(EINVAL);
        }
        for(i=0; i<3; i++){
            if (avio_open2(&pb[i], filename, AVIO_FLAG_WRITE,
                           &s->interrupt_callback, NULL) < 0) {
                av_log(s, AV_LOG_ERROR, "Could not open file : %s\n",filename);
                return AVERROR(EIO);
            }

            if(!img->split_planes)
                break;
            filename[ strlen(filename) - 1 ]= 'U' + i;
        }
    } else {
        pb[0] = s->pb;
    }

    if(img->split_planes){
        int ysize = codec->width * codec->height;
        avio_write(pb[0], pkt->data        , ysize);
        avio_write(pb[1], pkt->data + ysize, (pkt->size - ysize)/2);
        avio_write(pb[2], pkt->data + ysize +(pkt->size - ysize)/2, (pkt->size - ysize)/2);
        avio_flush(pb[1]);
        avio_flush(pb[2]);
        avio_close(pb[1]);
        avio_close(pb[2]);
    }else{
        if(ff_guess_image2_codec(s->filename) == AV_CODEC_ID_JPEG2000){
            AVStream *st = s->streams[0];
            if(st->codec->extradata_size > 8 &&
               AV_RL32(st->codec->extradata+4) == MKTAG('j','p','2','h')){
                if(pkt->size < 8 || AV_RL32(pkt->data+4) != MKTAG('j','p','2','c'))
                    goto error;
                avio_wb32(pb[0], 12);
                ffio_wfourcc(pb[0], "jP  ");
                avio_wb32(pb[0], 0x0D0A870A); // signature
                avio_wb32(pb[0], 20);
                ffio_wfourcc(pb[0], "ftyp");
                ffio_wfourcc(pb[0], "jp2 ");
                avio_wb32(pb[0], 0);
                ffio_wfourcc(pb[0], "jp2 ");
                avio_write(pb[0], st->codec->extradata, st->codec->extradata_size);
            }else if(pkt->size >= 8 && AV_RB32(pkt->data) == 0xFF4FFF51){
                //jpeg2000 codestream
            }else if(pkt->size < 8 ||
                     (!st->codec->extradata_size &&
                      AV_RL32(pkt->data+4) != MKTAG('j','P',' ',' '))){ // signature
            error:
                av_log(s, AV_LOG_ERROR, "malformed JPEG 2000 codestream %X\n", AV_RB32(pkt->data));
                return -1;
            }
        }
        avio_write(pb[0], pkt->data, pkt->size);
    }
    avio_flush(pb[0]);
    if (!img->is_pipe) {
        avio_close(pb[0]);
    }

    img->img_number++;
    return 0;
}

#define OFFSET(x) offsetof(VideoMuxData, x)
#define ENC AV_OPT_FLAG_ENCODING_PARAM
static const AVOption muxoptions[] = {
    { "updatefirst",  "", OFFSET(updatefirst),  AV_OPT_TYPE_INT,    {.dbl = 0},    0, 1, ENC },
    { "start_number", "first number in the sequence", OFFSET(img_number), AV_OPT_TYPE_INT, {.dbl = 1}, 1, INT_MAX, ENC },
    { NULL },
};

#if CONFIG_IMAGE2_MUXER
static const AVClass img2mux_class = {
    .class_name = "image2 muxer",
    .item_name  = av_default_item_name,
    .option     = muxoptions,
    .version    = LIBAVUTIL_VERSION_INT,
};

AVOutputFormat ff_image2_muxer = {
    .name           = "image2",
    .long_name      = NULL_IF_CONFIG_SMALL("image2 sequence"),
    .extensions     = "bmp,dpx,jls,jpeg,jpg,ljpg,pam,pbm,pcx,pgm,pgmyuv,png,"
                      "ppm,sgi,tga,tif,tiff,jp2,j2c,xwd,sun,ras,rs,im1,im8,im24,"
                      "sunras,xbm",
    .priv_data_size = sizeof(VideoMuxData),
    .video_codec    = AV_CODEC_ID_MJPEG,
    .write_header   = write_header,
    .write_packet   = write_packet,
    .flags          = AVFMT_NOTIMESTAMPS | AVFMT_NODIMENSIONS | AVFMT_NOFILE,
    .priv_class     = &img2mux_class,
};
#endif
#if CONFIG_IMAGE2PIPE_MUXER
AVOutputFormat ff_image2pipe_muxer = {
    .name           = "image2pipe",
    .long_name      = NULL_IF_CONFIG_SMALL("piped image2 sequence"),
    .priv_data_size = sizeof(VideoMuxData),
    .video_codec    = AV_CODEC_ID_MJPEG,
    .write_header   = write_header,
    .write_packet   = write_packet,
    .flags          = AVFMT_NOTIMESTAMPS | AVFMT_NODIMENSIONS
};
#endif
