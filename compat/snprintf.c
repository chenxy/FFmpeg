/*
 * C99-compatible snprintf() and vsnprintf() implementations
 * Copyright (c) 2012 Ronald S. Bultje <rsbultje@gmail.com>
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

#include <limits.h>
#include <string.h>

#include "libavutil/error.h"
#include "snprintf.h"

#undef snprintf
int avpriv_snprintf(char *restrict s, size_t n, const char *restrict fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = avpriv_vsnprintf(s, n, fmt, ap);
    va_end(ap);

    return ret;
}

#undef vsnprintf
int avpriv_vsnprintf(char *restrict s, size_t n, const char *restrict fmt,
                     va_list ap)
{
    int ret;

    if (n == 0)
        return 0;
    else if (n > INT_MAX)
        return AVERROR(EINVAL);

    /* we use n - 1 here because if the buffer is not big enough, the MS
     * runtime libraries don't add a terminating zero at the end. MSDN
     * recommends to provide _snprintf/_vsnprintf() a buffer size that
     * is one less than the actual buffer, and zero it before calling
     * _snprintf/_vsnprintf() to workaround this problem.
     * See http://msdn.microsoft.com/en-us/library/1kt27hek(v=vs.80).aspx */
    memset(s, 0, n);
    ret = vsnprintf(s, n - 1, fmt, ap);
    if (ret == -1)
        ret = n;

    return ret;
}
