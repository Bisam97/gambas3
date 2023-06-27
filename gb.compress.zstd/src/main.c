/***************************************************************************

	main.c

  (c) 2019-present Laurent Carlier <lordheavym@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
	MA 02110-1301, USA.

***************************************************************************/

#define __MAIN_C

#include "gb_common.h"

// Use 64 bits I/O
#if (__WORDSIZE == 64) && (!_LARGEFILE64_SOURCE)
#define _LARGEFILE64_SOURCE
#endif

// fix some building errors with zstd prior v1.3.5?
// - tagged as experimental on these releases -
#define ZSTD_STATIC_LINKING_ONLY 1

#include <errno.h>
#include <zstd.h>

#include "main.h"

#define MODE_READ 0
#define MODE_WRITE 1

// since zstd v1.3.5
#ifndef ZSTD_CLEVEL_DEFAULT
#define ZSTD_CLEVEL_DEFAULT 3
#endif

GB_INTERFACE EXPORT GB;
COMPRESS_INTERFACE EXPORT COMPRESSION;

static COMPRESS_DRIVER _driver;

GB_STREAM_DESC ZSTDStream = {
	.open = ZSTD_stream_open,
	.close = ZSTD_stream_close,
	.read = ZSTD_stream_read,
	.write = ZSTD_stream_write,
	.seek = ZSTD_stream_seek,
	.tell = ZSTD_stream_tell,
	.flush = ZSTD_stream_flush,
	.eof = ZSTD_stream_eof,
	.lof = ZSTD_stream_lof
};

typedef 
	struct
	{
		uint8_t mode;
	} 
	handleInfo; 

typedef
	struct {
		GB_STREAM_BASE base;
		handleInfo *info;
		}
	STREAM_COMPRESS;


// /*****************************************************************************
// 
// 	The driver interface
// 
// *****************************************************************************/

static int ZSTD_max_compression(void)
{
	return ZSTD_maxCLevel();
}

static int ZSTD_min_compression(void)
{
    #if ZSTD_VERSION_NUMBER < 10308 // ubuntu 18.04 LTS - zstd v1.3.3 / debian stable is v1.3.8
    return 1;
    #else
	return ZSTD_minCLevel();
    #endif
}

static int ZSTD_default_compression(void)
{
	return ZSTD_CLEVEL_DEFAULT;
}

/*****************************************************************************

	Compression 

*****************************************************************************/

static void ZSTD_c_String(char **target,unsigned int *lent,char *source,unsigned int len,int level)
{
	size_t retValue;
    
	*lent = ZSTD_compressBound(len);
	GB.Alloc((void**)target, sizeof(char)*(*lent));

	retValue = ZSTD_compress((void *)(*target), *lent, (const void *)source, len, level);

	if (ZSTD_isError(retValue))
	{
		GB.Free((void**)target);
		*lent = 0;
		*target = NULL;
		GB.Error("Unable to compress string: &1", ZSTD_getErrorName(retValue));
		return;
	}
    
	*lent = (uint)retValue;
}

static void ZSTD_c_File(char *source,char *target,int level)
{
	FILE *f_src, *f_dst;
    void *buffIn, *buffOut;
    
	if ((f_src=fopen(source, "rb")) == NULL)
    {
		GB.Error("Unable to open file for reading");
		return;
	}

	if ((f_dst=fopen(target, "wb")) == NULL)
    {
		fclose(f_src);
		GB.Error("Unable to open file for writing");
		return;
	}
    
    size_t const buffInSize = ZSTD_CStreamInSize();
    size_t const buffOutSize = ZSTD_CStreamOutSize();
    GB.Alloc(&buffIn, buffInSize);
    GB.Alloc(&buffOut, buffOutSize);
    
    // TODO: it is recommended to allocate a context only once, and re-use it for each successive compression operation.
    ZSTD_CCtx *cctx = ZSTD_createCCtx();
    
    if (cctx == NULL)
    {
        GB.Error("Error while compressing file: ZSTD_createCCtx() failed!");
        goto error_cfile;
    }

    #if ZSTD_VERSION_NUMBER < 10308 // ubuntu 18.04 LTS - zstd v1.3.3 / debian stable is v1.3.8
    size_t const retValue = ZSTD_initCStream(cctx, level);
    
    if (ZSTD_isError(retValue))
    {
        GB.Error("Error while compressing file, ZSTD_initCStream() failed:  &1", ZSTD_getErrorName(retValue));
        goto error_cfile;
    }
    
    while(!feof(f_src))
	{
        size_t len = fread(buffIn, 1, buffInSize, f_src);
		if (len < buffInSize)
		{
			if (ferror(f_src))
			{
				GB.Error("Error while reading data: &1", strerror(errno));
				goto error_cfile;
			}
        }
        if (len > 0)
		{
            ZSTD_inBuffer input = { buffIn, len, 0 };
            int finished;
            do {
                ZSTD_outBuffer output = { buffOut, buffOutSize, 0 };
                size_t const remaining = ZSTD_compressStream(cctx, &output , &input);

                if (ZSTD_isError(remaining))
                {
                    GB.Error("Error while compressing file: &1", ZSTD_getErrorName(remaining));
                    goto error_cfile;
                }
                if (fwrite(buffOut, 1, output.pos, f_dst) != output.pos)
                {
                    GB.Error("Error while writing data: &1", strerror(errno));
                    goto error_cfile;
                }
                finished = remaining;
            } while (!finished);
            
            ZSTD_outBuffer output = { buffOut, buffOutSize, 0 };
            size_t const remaining = ZSTD_endStream(cctx, &output);
            
 			if (remaining)
			{
				GB.Error("Error while compressing file: not fully flushed!");
				goto error_cfile;
			}
            if (fwrite(buffOut, 1, output.pos, f_dst) != output.pos)
            {
                GB.Error("Error while writing flushed data: &1", strerror(errno));
                goto error_cfile;
            }
        }
    }
    
    #else
    // TODO: check errors ?
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, level);
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_checksumFlag, 1);
    ZSTD_EndDirective mode = ZSTD_e_continue;

	while(!feof(f_src))
	{
        size_t len = fread(buffIn, 1, buffInSize, f_src);
		if (len < buffInSize)
		{
			if (ferror(f_src))
			{
				GB.Error("Error while reading data: &1", strerror(errno));
				goto error_cfile;
			}
			mode = ZSTD_e_end;
		}
		if (len > 0)
		{
            ZSTD_inBuffer input = { buffIn, len, 0 };
            int finished;
            do {
                ZSTD_outBuffer output = { buffOut, buffOutSize, 0 };
                size_t const remaining = ZSTD_compressStream2(cctx, &output , &input, mode);

                if (ZSTD_isError(remaining))
                {
                    GB.Error("Error while compressing file: &1", ZSTD_getErrorName(remaining));
                    goto error_cfile;
                }
                if (fwrite(buffOut, 1, output.pos, f_dst) != output.pos)
                {
                    GB.Error("Error while writing data: &1", strerror(errno));
                    goto error_cfile;
                }
                finished = (len < buffInSize) ? (remaining == 0) : (input.pos == input.size);
            } while (!finished);
		}
	}
    #endif /* ZSTD_VERSION_NUMBER < 10308 */
    
error_cfile:
    if (cctx != NULL)
        ZSTD_freeCCtx(cctx);
    GB.Free(&buffIn);
    GB.Free(&buffOut);
    fclose(f_src);
    fclose(f_dst);
    return;
}


static void ZSTD_c_Open(char *path,int level, STREAM_COMPRESS *stream)
{
	GB.Error("Not yet implemented!");
	return;
}

/*****************************************************************************

	Uncompression 

*****************************************************************************/

static void ZSTD_u_String(char **target,unsigned int *lent,char *source,unsigned int len)
{
	*lent = ZSTD_getFrameContentSize((const void *) source, len);
	size_t dSize = 0;
	
	if (*lent == ZSTD_CONTENTSIZE_UNKNOWN)
	{
		GB.Error("Size unknown - maybe needs streaming - not yet implemented!");
		return;
	}
	if (*lent == ZSTD_CONTENTSIZE_ERROR)
	{
		GB.Error("Not compressed by zstd!");
		return;
	}
	
	GB.Alloc((void **) target, sizeof(char)*(*lent));
	dSize = ZSTD_decompress((void *)(*target), *lent, (const void *)source, len);
	
	if (ZSTD_isError(dSize))
	{
		GB.Free((void**)target);
		*lent=0;
		*target=NULL;
		GB.Error("Unable to uncompress string: &1", ZSTD_getErrorName(dSize));
		return;
	}

	*lent = (uint)dSize;
}

static void ZSTD_u_File(char *source,char *target)
{
	FILE *f_src, *f_dst;
    void *buffIn, *buffOut;
    
	if ((f_src=fopen(source, "rb")) == NULL)
    {
		GB.Error("Unable to open file for reading");
		return;
	}

	if ((f_dst=fopen(target, "wb")) == NULL)
    {
		fclose(f_src);
		GB.Error("Unable to open file for writing");
		return;
	}
    
    size_t const buffInSize = ZSTD_DStreamInSize();
    size_t const buffOutSize = ZSTD_DStreamOutSize();
    GB.Alloc(&buffIn, buffInSize);
    GB.Alloc(&buffOut, buffOutSize);

    // TODO: it is recommended to allocate a context only once, and re-use it for each successive decompression operation.
    ZSTD_DCtx *dctx = ZSTD_createDCtx();
    
    if (dctx == NULL)
    {
        GB.Error("Error while decompressing file: ZSTD_createDCtx() failed!");
        goto error_ufile;
    }
    
    #if ZSTD_VERSION_NUMBER < 10308 // ubuntu 18.04 LTS - zstd v1.3.3 / debian stable is v1.3.8
    size_t const retValue = ZSTD_initDStream(dctx);
    if (ZSTD_isError(retValue))
    {
        GB.Error("Error while decompressing file, ZSTD_initDStream() failed:  &1", ZSTD_getErrorName(retValue));
        goto error_ufile;
    }
    #endif /* ZSTD_VERSION_NUMBER < 10308 */
	while(!feof(f_src))
	{
    size_t len = fread(buffIn, 1, buffInSize, f_src);

		if (len < buffInSize)
		{
            if (len == 0)
            {
                GB.Error("Error: input file is empty");
                goto error_ufile;
            }
            if (ferror(f_src))
			{
				GB.Error("Error while reading data: &1", strerror(errno));
				goto error_ufile;
			}
		}

		if (len > 0)
		{
            ZSTD_inBuffer input = { buffIn, len, 0 };
            while (input.pos < input.size)
            {
                ZSTD_outBuffer output = { buffOut, buffOutSize, 0 };
                size_t const ret = ZSTD_decompressStream(dctx, &output , &input);

                if (ZSTD_isError(ret))
                {
                    GB.Error("Error while decompressing file: &1", ZSTD_getErrorName(ret));
                    goto error_ufile;
                }

                if (fwrite(buffOut, 1, output.pos, f_dst) != output.pos)
                {
                    GB.Error("Error while writing data: &1", strerror(errno));
                    goto error_ufile;
                }
            }
		}
	}
     
error_ufile:
    if (dctx != NULL)
        ZSTD_freeDCtx(dctx);
    GB.Free(&buffIn);
    GB.Free(&buffOut);
    fclose(f_src);
    fclose(f_dst);
    return;
}

static void ZSTD_u_Open(char *path, STREAM_COMPRESS *stream)
{
	GB.Error("Not yet implemented!");
	return;
}

/*************************************************************************
Stream related stuff
**************************************************************************/
/* TODO */
static int ZSTD_stream_open(GB_STREAM *stream, const char *path, int mode, void *data) {return -1;}
static int ZSTD_stream_close(GB_STREAM *stream) {return -1;}
static int ZSTD_stream_read(GB_STREAM *stream, char *buffer, int len) {return -1;}
static int ZSTD_stream_write(GB_STREAM *stream, char *buffer, int len) {return -1;}
static int ZSTD_stream_seek(GB_STREAM *stream, int64_t offset, int whence) {return -1;}
static int ZSTD_stream_tell(GB_STREAM *stream, int64_t *npos) {return -1;}
static int ZSTD_stream_flush(GB_STREAM *stream) {return -1;}
static int ZSTD_stream_eof(GB_STREAM *stream) {return -1;}
static int ZSTD_stream_lof(GB_STREAM *stream, int64_t *len) {return -1;}
/* end of TODO */

static COMPRESS_DRIVER _driver =
{
		"zstd",

		(void*)ZSTD_max_compression,
		(void*)ZSTD_min_compression,
		(void*)ZSTD_default_compression,

		{
			(void*)ZSTD_c_String,
			(void*)ZSTD_c_File,
			(void*)ZSTD_c_Open,
			(void*)ZSTD_stream_close,
		},

		{
			(void*)ZSTD_u_String,
			(void*)ZSTD_u_File,
			(void*)ZSTD_u_Open,
			(void*)ZSTD_stream_close
		}
};

/*****************************************************************************

	The component entry and exit functions.

*****************************************************************************/

int EXPORT GB_INIT(void)
{
	GB.GetInterface("gb.compress", COMPRESS_INTERFACE_VERSION, &COMPRESSION);
	COMPRESSION.Register(&_driver);

	return 0;
}

void EXPORT GB_EXIT()
{
}
