/*
 * Copyright (c) 2012 Stefano Sabatini
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <libavutil/imgutils.h>
#include <libavutil/parseutils.h>
#include <libswscale/swscale.h>

static void fill_yuv_image(uint8_t *data[4], int linesize[4],
                           int width, int height, int frame_index)
{
    int x, y, i;

    i = frame_index;

    /* Y */
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            data[0][y * linesize[0] + x] = x + y + i * 3;

    /* Cb and Cr */
    for (y = 0; y < height / 2; y++) {
        for (x = 0; x < width / 2; x++) {
            data[1][y * linesize[1] + x] = 128 + y + i * 2;
            data[2][y * linesize[2] + x] = 64 + x + i * 5;
        }
    }
}

static void save_pgm(const char *filename,
                     const uint8_t *data, int linesize, int width, int height)
{
    FILE *f;
    int i;

    f = fopen(filename, "w");
    fprintf(f, "P5\n%d %d\n%d\n", width, height, 255);
    for (i = 0; i < height; i++)
        fwrite(data + i * linesize, 1, width, f);
    fclose(f);
}

int main(int argc, char **argv)
{
    uint8_t *src_data[4], *dst_data[4];
    int src_linesize[4], dst_linesize[4];
    int src_w = 320, src_h = 240, dst_w, dst_h;
    enum PixelFormat src_pix_fmt = PIX_FMT_YUV420P, dst_pix_fmt = PIX_FMT_GRAY8;
    const char *dst_size = argv[1];
    struct SwsContext *sws_ctx;
    int i, ret;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s output_size\n"
                "API example program to show how to scale an image with libswscale.\n"
                "This program generates a series of pictures, rescales them to the given "
                "<output size> and finally saves the rescaled pictures as PGM files named "
                "like outscale<frame number>.pgm.\n"
                "\n", argv[0]);
        exit(1);
    }

    if (av_parse_video_size(&dst_w, &dst_h, dst_size) < 0) {
        av_log(NULL, AV_LOG_ERROR,
               "Invalid size '%s', must be in the form WxH or a valid size abbreviation\n",
               dst_size);
        exit(1);
    }

    /* create scaling context */
    sws_ctx = sws_getContext(src_w, src_h, src_pix_fmt,
                             dst_w, dst_h, dst_pix_fmt,
                             SWS_BILINEAR, NULL, NULL, NULL);
    if (!sws_ctx) {
        av_log(NULL, AV_LOG_ERROR,
               "Impossible to create scale context for the conversion "
               "fmt:%s s:%dx%d -> fmt:%s s:%dx%d\n",
               av_get_pix_fmt_name(src_pix_fmt), src_w, src_h,
               av_get_pix_fmt_name(dst_pix_fmt), dst_w, dst_h);
        ret = AVERROR(EINVAL);
        goto end;
    }

    /* allocate source and destination image buffers */
    if ((ret = av_image_alloc(src_data, src_linesize,
                              src_w, src_h, src_pix_fmt, 16)) < 0)
        goto end;

    if ((ret = av_image_alloc(dst_data, dst_linesize,
                              dst_w, dst_h, dst_pix_fmt, 16)) < 0)
        goto end;

    for (i = 0; i < 100; i++) {
        char filename[1024];

        /* generate synthetic video */
        fill_yuv_image(src_data, src_linesize, src_w, src_h, i);

        /* convert to destination format */
        sws_scale(sws_ctx, (const uint8_t * const*)src_data,
                  src_linesize, 0, src_h, dst_data, dst_linesize);

        /* write Y plane to to output PGM file */
        snprintf(filename, sizeof(filename), "outscale%02d.pgm", i);
        save_pgm(filename, dst_data[0], dst_linesize[0], dst_w, dst_h);
    }

end:
    av_freep(&src_data[0]);
    av_freep(&dst_data[0]);
    sws_freeContext(sws_ctx);
    return ret < 0;
}
