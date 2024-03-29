[+ AutoGen5 template c +]
/*
** Copyright (C) 1999-2004 Erik de Castro Lopo <erikd@mega-nerd.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sndfile.h>

#include "float_cast.h"
#include "utils.h"

#define	BUFFER_SIZE		(1<<15)

static void	lrintf_test (void) ;

[+ FOR data_type
+]static void	pcm_test_[+ (get "name") +]	(const char *filename, int filetype, int hash) ;
[+ ENDFOR data_type
+]
static void pcm_test_float	(const char *filename, int filetype, int hash, int replace_float) ;
static void pcm_test_double	(const char *filename, int filetype, int hash, int replace_float) ;

/* Data written to the file. */
static	double	data_out [(BUFFER_SIZE / sizeof (double)) + 1] ;

/* Data read back from the file. */
static	double	data_in	[(BUFFER_SIZE / sizeof (double)) + 1] ;

int
main (void)
{
	lrintf_test () ;

	pcm_test_bits_8	("pcm-s8.raw", SF_FORMAT_RAW | SF_FORMAT_PCM_S8, 0x9ae33814) ;
	pcm_test_bits_8	("pcm-u8.raw", SF_FORMAT_RAW | SF_FORMAT_PCM_U8, 0x651d4694) ;

	pcm_test_bits_16 ("le-pcm16.raw", SF_ENDIAN_LITTLE	| SF_FORMAT_RAW | SF_FORMAT_PCM_16, 0x16866fa0) ;
	pcm_test_bits_16 ("be-pcm16.raw", SF_ENDIAN_BIG		| SF_FORMAT_RAW | SF_FORMAT_PCM_16, 0xc571826c) ;

	pcm_test_bits_24 ("le-pcm24.raw", SF_ENDIAN_LITTLE	| SF_FORMAT_RAW | SF_FORMAT_PCM_24, 0x658e4bb6) ;
	pcm_test_bits_24 ("be-pcm24.raw", SF_ENDIAN_BIG		| SF_FORMAT_RAW | SF_FORMAT_PCM_24, 0xbf8cde4a) ;

	pcm_test_bits_32 ("le-pcm32.raw", SF_ENDIAN_LITTLE	| SF_FORMAT_RAW | SF_FORMAT_PCM_32, 0x04c84a70) ;
	pcm_test_bits_32 ("be-pcm32.raw", SF_ENDIAN_BIG		| SF_FORMAT_RAW | SF_FORMAT_PCM_32, 0x069c84f6) ;

	/* Lite remove start */
	pcm_test_float	("le-float.raw", SF_ENDIAN_LITTLE	| SF_FORMAT_RAW | SF_FORMAT_FLOAT, 0xbb836603, SF_FALSE) ;
	pcm_test_float	("be-float.raw", SF_ENDIAN_BIG		| SF_FORMAT_RAW | SF_FORMAT_FLOAT, 0x903cd8fc, SF_FALSE) ;

	pcm_test_double	("le-double.raw", SF_ENDIAN_LITTLE	| SF_FORMAT_RAW | SF_FORMAT_DOUBLE, 0xbf84448e, SF_FALSE) ;
	pcm_test_double	("be-double.raw", SF_ENDIAN_BIG	| SF_FORMAT_RAW | SF_FORMAT_DOUBLE, 0xaf3d9fb5, SF_FALSE) ;

	puts ("Test IEEE replacement code.") ;

	pcm_test_float	("le-float.raw", SF_ENDIAN_LITTLE	| SF_FORMAT_RAW | SF_FORMAT_FLOAT, 0xbb836603, SF_TRUE) ;
	pcm_test_float	("be-float.raw", SF_ENDIAN_BIG		| SF_FORMAT_RAW | SF_FORMAT_FLOAT, 0x903cd8fc, SF_TRUE) ;

	pcm_test_double	("le-double.raw", SF_ENDIAN_LITTLE	| SF_FORMAT_RAW | SF_FORMAT_DOUBLE, 0xbf84448e, SF_TRUE) ;
	pcm_test_double	("be-double.raw", SF_ENDIAN_BIG	| SF_FORMAT_RAW | SF_FORMAT_DOUBLE, 0xaf3d9fb5, SF_TRUE) ;
	/* Lite remove end */

	return 0 ;
} /* main */

/*============================================================================================
**	Here are the test functions.
*/

static void
lrintf_test (void)
{	int k, items ;
	float	*float_data ;
	int		*int_data ;

	print_test_name ("lrintf_test", "") ;

	items = 1024 ;

	float_data = (float*) data_out ;
	int_data = (int*) data_in ;

	for (k = 0 ; k < items ; k++)
		float_data [k] = (k * ((k % 2) ? 333333.0 : -333333.0)) ;

	for (k = 0 ; k < items ; k++)
		int_data [k] = lrintf (float_data [k]) ;

	for (k = 0 ; k < items ; k++)
		if (fabs (int_data [k] - float_data [k]) > 1.0)
		{	printf ("\n\nLine %d: float : Incorrect sample (#%d : %f => %d).\n", __LINE__, k, float_data [k], int_data [k]) ;
			exit (1) ;
			} ;

	printf ("ok\n") ;
} /* lrintf_test */

[+ FOR data_type
+]static void
pcm_test_[+ (get "name") +] (const char *filename, int filetype, int hash)
{	SNDFILE		*file ;
	SF_INFO		sfinfo ;
	int			k, items, zero_count ;
	short		*short_out, *short_in ;
	int			*int_out, *int_in ;
	/* Lite remove start */
	float		*float_out, *float_in ;
	double		*double_out, *double_in ;
	/* Lite remove end */

	print_test_name ("pcm_test_[+ (get "name") +]", filename) ;

	items = [+ (get "item_count") +] ;

	short_out = (short*) data_out ;
	short_in = (short*) data_in ;

	zero_count = 0 ;
	for (k = 0 ; k < items ; k++)
	{	short_out [k] = [+ (get "short_func") +] ;
		zero_count = short_out [k] ? zero_count : zero_count + 1 ;
		} ;

	if (zero_count > items / 4)
	{	printf ("\n\nLine %d: too many zeros.\n", __LINE__) ;
		exit (1) ;
		} ;

	sfinfo.samplerate	= 44100 ;
	sfinfo.frames		= 123456789 ; /* Wrong length. Library should correct this on sf_close. */
	sfinfo.channels		= 1 ;
	sfinfo.format		= filetype ;

	file = test_open_file_or_die (filename, SFM_WRITE, &sfinfo, __LINE__) ;

	test_write_short_or_die (file, 0, short_out, items, __LINE__) ;

	sf_close (file) ;

	memset (short_in, 0, items * sizeof (short)) ;

	file = test_open_file_or_die (filename, SFM_READ, &sfinfo, __LINE__) ;

	if (sfinfo.format != filetype)
	{	printf ("\n\nLine %d: Returned format incorrect (0x%08X => 0x%08X).\n", __LINE__, filetype, sfinfo.format) ;
		exit (1) ;
		} ;

	if (sfinfo.frames != items)
	{	printf ("\n\nLine %d: Incorrect number of frames in file. (%d => %ld)\n", __LINE__, items, SF_COUNT_TO_LONG (sfinfo.frames)) ;
		exit (1) ;
		} ;

	if (sfinfo.channels != 1)
	{	printf ("\n\nLine %d: Incorrect number of channels in file.\n", __LINE__) ;
		exit (1) ;
		} ;

	check_log_buffer_or_die (file, __LINE__) ;

	test_read_short_or_die (file, 0, short_in, items, __LINE__) ;

	for (k = 0 ; k < items ; k++)
		if (short_out [k] != short_in [k])
		{	printf ("\n\nLine %d: Incorrect sample (#%d : 0x%x => 0x%x).\n", __LINE__, k, short_out [k], short_in [k]) ;
			exit (1) ;
			} ;

	sf_close (file) ;

	/* Finally, check the file hash. */
	check_file_hash_or_die (filename, hash, __LINE__) ;

	/*--------------------------------------------------------------------------
	** Test sf_read/write_int ()
	*/
	zero_count = 0 ;

	int_out = (int*) data_out ;
	int_in = (int*) data_in ;
	for (k = 0 ; k < items ; k++)
	{	int_out [k] = [+ (get "int_func") +] ;
		zero_count = int_out [k] ? zero_count : zero_count + 1 ;
		} ;

	if (zero_count > items / 4)
	{	printf ("\n\nLine %d: too many zeros.\n", __LINE__) ;
		exit (1) ;
		} ;

	sfinfo.samplerate	= 44100 ;
	sfinfo.frames		= 123456789 ; /* Wrong length. Library should correct this on sf_close. */
	sfinfo.channels		= 1 ;
	sfinfo.format		= filetype ;

	file = test_open_file_or_die (filename, SFM_WRITE, &sfinfo, __LINE__) ;

	test_write_int_or_die (file, 0, int_out, items, __LINE__) ;

	sf_close (file) ;

	memset (int_in, 0, items * sizeof (int)) ;

	file = test_open_file_or_die (filename, SFM_READ, &sfinfo, __LINE__) ;

	if (sfinfo.format != filetype)
	{	printf ("\n\nLine %d: Returned format incorrect (0x%08X => 0x%08X).\n", __LINE__, filetype, sfinfo.format) ;
		exit (1) ;
		} ;

	if (sfinfo.frames != items)
	{	printf ("\n\nLine %d: Incorrect number of frames in file. (%d => %ld)\n", __LINE__, items, SF_COUNT_TO_LONG (sfinfo.frames)) ;
		exit (1) ;
		} ;

	if (sfinfo.channels != 1)
	{	printf ("\n\nLine %d: Incorrect number of channels in file.\n", __LINE__) ;
		exit (1) ;
		} ;

	check_log_buffer_or_die (file, __LINE__) ;

	test_read_int_or_die (file, 0, int_in, items, __LINE__) ;

	for (k = 0 ; k < items ; k++)
		if (int_out [k] != int_in [k])
		{	printf ("\n\nLine %d: int : Incorrect sample (#%d : 0x%x => 0x%x).\n", __LINE__, k, int_out [k], int_in [k]) ;
			exit (1) ;
			} ;

	sf_close (file) ;

	/* Lite remove start */
	/*--------------------------------------------------------------------------
	** Test sf_read/write_float ()
	*/
	zero_count = 0 ;

	float_out = (float*) data_out ;
	float_in = (float*) data_in ;
	for (k = 0 ; k < items ; k++)
	{	float_out [k] = [+ (get "float_func") +] ;
		zero_count = (fabs (float_out [k]) > 1e-10) ? zero_count : zero_count + 1 ;
		} ;

	if (zero_count > items / 4)
	{	printf ("\n\nLine %d: too many zeros (%d/%d).\n", __LINE__, zero_count, items) ;
		exit (1) ;
		} ;

	sfinfo.samplerate	= 44100 ;
	sfinfo.frames		= 123456789 ; /* Wrong length. Library should correct this on sf_close. */
	sfinfo.channels		= 1 ;
	sfinfo.format		= filetype ;

	file = test_open_file_or_die (filename, SFM_WRITE, &sfinfo, __LINE__) ;

	sf_command (file, SFC_SET_NORM_FLOAT, NULL, SF_FALSE) ;

	test_write_float_or_die (file, 0, float_out, items, __LINE__) ;

	sf_close (file) ;

	memset (float_in, 0, items * sizeof (float)) ;

	file = test_open_file_or_die (filename, SFM_READ, &sfinfo, __LINE__) ;

	if (sfinfo.format != filetype)
	{	printf ("\n\nLine %d: Returned format incorrect (0x%08X => 0x%08X).\n", __LINE__, filetype, sfinfo.format) ;
		exit (1) ;
		} ;

	if (sfinfo.frames != items)
	{	printf ("\n\nLine %d: Incorrect number of frames in file. (%d => %ld)\n", __LINE__, items, SF_COUNT_TO_LONG (sfinfo.frames)) ;
		exit (1) ;
		} ;

	if (sfinfo.channels != 1)
	{	printf ("\n\nLine %d: Incorrect number of channels in file.\n", __LINE__) ;
		exit (1) ;
		} ;

	check_log_buffer_or_die (file, __LINE__) ;

	sf_command (file, SFC_SET_NORM_FLOAT, NULL, SF_FALSE) ;

	test_read_float_or_die (file, 0, float_in, items, __LINE__) ;

	for (k = 0 ; k < items ; k++)
		if (fabs (float_out [k] - float_in [k]) > 1e-10)
		{	printf ("\n\nLine %d: float : Incorrect sample (#%d : %f => %f).\n", __LINE__, k, (double) float_out [k], (double) float_in [k]) ;
			exit (1) ;
			break ;
			} ;

	sf_close (file) ;

	/*--------------------------------------------------------------------------
	** Test sf_read/write_double ()
	*/
	zero_count = 0 ;

	double_out = (double*) data_out ;
	double_in = (double*) data_in ;
	for (k = 0 ; k < items ; k++)
	{	double_out [k] = [+ (get "float_func") +] ;
		zero_count = (fabs (double_out [k]) > 1e-10) ? zero_count : zero_count + 1 ;
		} ;

	if (zero_count > items / 4)
	{	printf ("\n\nLine %d: too many zeros (%d/%d).\n", __LINE__, zero_count, items) ;
		exit (1) ;
		} ;

	sfinfo.samplerate	= 44100 ;
	sfinfo.frames		= 123456789 ; /* Wrong length. Library should correct this on sf_close. */
	sfinfo.channels		= 1 ;
	sfinfo.format		= filetype ;

	file = test_open_file_or_die (filename, SFM_WRITE, &sfinfo, __LINE__) ;

	sf_command (file, SFC_SET_NORM_DOUBLE, NULL, SF_FALSE) ;

	test_write_double_or_die (file, 0, double_out, items, __LINE__) ;

	sf_close (file) ;

	memset (double_in, 0, items * sizeof (double)) ;

	file = test_open_file_or_die (filename, SFM_READ, &sfinfo, __LINE__) ;

	if (sfinfo.format != filetype)
	{	printf ("\n\nLine %d: Returned format incorrect (0x%08X => 0x%08X).\n", __LINE__, filetype, sfinfo.format) ;
		exit (1) ;
		} ;

	if (sfinfo.frames != items)
	{	printf ("\n\nLine %d: Incorrect number of frames in file. (%d => %ld)\n", __LINE__, items, SF_COUNT_TO_LONG (sfinfo.frames)) ;
		exit (1) ;
		} ;

	if (sfinfo.channels != 1)
	{	printf ("\n\nLine %d: Incorrect number of channels in file.\n", __LINE__) ;
		exit (1) ;
		} ;

	check_log_buffer_or_die (file, __LINE__) ;

	sf_command (file, SFC_SET_NORM_DOUBLE, NULL, SF_FALSE) ;

	test_read_double_or_die (file, 0, double_in, items, __LINE__) ;

	for (k = 0 ; k < items ; k++)
		if (fabs (double_out [k] - double_in [k]) > 1e-10)
		{	printf ("\n\nLine %d: double : Incorrect sample (#%d : %f => %f).\n", __LINE__, k, double_out [k], double_in [k]) ;
			exit (1) ;
			} ;

	sf_close (file) ;
	/* Lite remove end */
	unlink (filename) ;

	puts ("ok") ;
} /* pcm_test_[+ (get "name") +] */

[+ ENDFOR data_type
+]

/*==============================================================================
*/

static void
pcm_test_float (const char *filename, int filetype, int hash, int replace_float)
{	SNDFILE			*file ;
	SF_INFO			sfinfo ;
	int				k, items, frames ;
	int				sign ;
	double			*data, error ;

	print_test_name ("pcm_test_float", filename) ;

	items = BUFFER_SIZE / sizeof (double) ;

	data = (double*) data_out ;
	for (sign = 1, k = 0 ; k < items ; k++)
	{	data [k] = ((double) (k * sign)) / 100.0 ;
		sign = (sign > 0) ? -1 : 1 ;
		} ;

	sfinfo.samplerate	= 44100 ;
	sfinfo.frames		= items ;
	sfinfo.channels		= 1 ;
	sfinfo.format		= filetype ;

	file = test_open_file_or_die (filename, SFM_WRITE, &sfinfo, __LINE__) ;
	sf_command (file, SFC_TEST_IEEE_FLOAT_REPLACE, NULL, replace_float) ;
	if (replace_float && string_in_log_buffer (file, "Using IEEE replacement") == 0)
	{	printf ("\n\nLine %d : Float replacement code not working.\n\n", __LINE__) ;
		dump_log_buffer (file) ;
		exit (1) ;
		} ;

	test_write_double_or_die (file, 0, data, items, __LINE__) ;

	sf_close (file) ;

	check_file_hash_or_die (filename, hash, __LINE__) ;

	memset (data, 0, items * sizeof (double)) ;

	if ((filetype & SF_FORMAT_TYPEMASK) != SF_FORMAT_RAW)
		memset (&sfinfo, 0, sizeof (sfinfo)) ;

	file = test_open_file_or_die (filename, SFM_READ, &sfinfo, __LINE__) ;
	sf_command (file, SFC_TEST_IEEE_FLOAT_REPLACE, NULL, replace_float) ;
	if (replace_float && string_in_log_buffer (file, "Using IEEE replacement") == 0)
	{	printf ("\n\nLine %d : Float replacement code not working.\n\n", __LINE__) ;
		dump_log_buffer (file) ;
		exit (1) ;
		} ;

	if (sfinfo.format != filetype)
	{	printf ("\n\nError (%s:%d) Mono : Returned format incorrect (0x%08X => 0x%08X).\n", __FILE__, __LINE__, filetype, sfinfo.format) ;
		exit (1) ;
		} ;

	if (sfinfo.frames != items)
	{	printf ("\n\nError (%s:%d) Mono : Incorrect number of frames in file. (%d => %ld)\n", __FILE__, __LINE__, items, SF_COUNT_TO_LONG (sfinfo.frames)) ;
		exit (1) ;
		} ;

	if (sfinfo.channels != 1)
	{	printf ("\n\nError (%s:%d) Mono : Incorrect number of channels in file.\n", __FILE__, __LINE__) ;
		exit (1) ;
		} ;

	check_log_buffer_or_die (file, __LINE__) ;

	test_read_double_or_die (file, 0, data, items, __LINE__) ;

	for (sign = -1, k = 0 ; k < items ; k++)
	{	error = fabs (data [k] - ((double) k) / 100.0 * (sign *= -1)) ;
		if (fabs (data [k]) > 1e-100 && fabs (error / data [k]) > 1e-5)
		{	printf ("\n\nError (%s:%d) Mono : Incorrect sample (#%d : %f => %f).\n", __FILE__, __LINE__, k, ((double) k) / 100.0, data [k]) ;
			exit (1) ;
			} ;
		} ;

	/* Seek to start of file. */
	test_seek_or_die (file, 0, SEEK_SET, 0, sfinfo.channels, __LINE__) ;

	test_read_double_or_die (file, 0, data, 4, __LINE__) ;
	for (k = 0 ; k < 4 ; k++)
	{	error = fabs (data [k] - ((double) k) / 100.0 * (sign *= -1)) ;
		if (fabs (data [k]) > 1e-100 && fabs (error / data [k]) > 1e-5)
		{	printf ("\n\nError (%s:%d) Mono : Incorrect sample (#%d : %f => %f).\n", __FILE__, __LINE__, k, ((double) k) / 100.0, data [k]) ;
			exit (1) ;
			} ;
		} ;

	/* Seek to offset from start of file. */
	test_seek_or_die (file, 10, SEEK_SET, 10, sfinfo.channels, __LINE__) ;

	test_read_double_or_die (file, 0, data + 10, 4, __LINE__) ;
	for (k = 10 ; k < 14 ; k++)
	{	error = fabs (data [k] - ((double) k) / 100.0 * (sign *= -1)) ;
		if (fabs (data [k]) > 1e-100 && fabs (error / data [k]) > 1e-5)
		{	printf ("\n\nError (%s:%d) Mono : Incorrect sample (#%d : %f => %f).\n", __FILE__, __LINE__, k, ((double) k) / 100.0, data [k]) ;
			exit (1) ;
			} ;
		} ;

	/* Seek to offset from current position. */
	test_seek_or_die (file, 6, SEEK_CUR, 20, sfinfo.channels, __LINE__) ;

	test_read_double_or_die (file, 0, data + 20, 4, __LINE__) ;
	for (k = 20 ; k < 24 ; k++)
	{	error = fabs (data [k] - ((double) k) / 100.0 * (sign *= -1)) ;
		if (fabs (data [k]) > 1e-100 && fabs (error / data [k]) > 1e-5)
		{	printf ("\n\nError (%s:%d) Mono : Incorrect sample (#%d : %f => %f).\n", __FILE__, __LINE__, k, ((double) k) / 100.0, data [k]) ;
			exit (1) ;
			} ;
		} ;

	/* Seek to offset from end of file. */
	test_seek_or_die (file, -1 * (sfinfo.frames - 10), SEEK_END, 10, sfinfo.channels, __LINE__) ;

	test_read_double_or_die (file, 0, data + 10, 4, __LINE__) ;
	for (k = 10 ; k < 14 ; k++)
	{	error = fabs (data [k] - ((double) k) / 100.0 * (sign *= -1)) ;
		if (fabs (data [k]) > 1e-100 && fabs (error / data [k]) > 1e-5)
		{	printf ("\n\nError (%s:%d) Mono : Incorrect sample (#%d : %f => %f).\n", __FILE__, __LINE__, k, ((double) k) / 100.0, data [k]) ;
			exit (1) ;
			} ;
		} ;

	sf_close (file) ;

	/* Now test Stereo. */

	if ((filetype & SF_FORMAT_TYPEMASK) == SF_FORMAT_SVX) /* SVX is mono only */
	{	printf ("ok\n") ;
		return ;
		} ;

	items = BUFFER_SIZE / sizeof (double) ;

	data = (double*) data_out ;
	for (sign = -1, k = 0 ; k < items ; k++)
		data [k] = ((double) k) / 100.0 * (sign *= -1) ;

	sfinfo.samplerate	= 44100 ;
	sfinfo.frames		= items ;
	sfinfo.channels		= 2 ;
	sfinfo.format		= filetype ;

	frames = items / sfinfo.channels ;

	file = test_open_file_or_die (filename, SFM_WRITE, &sfinfo, __LINE__) ;
	sf_command (file, SFC_TEST_IEEE_FLOAT_REPLACE, NULL, replace_float) ;
	if (replace_float && string_in_log_buffer (file, "Using IEEE replacement") == 0)
	{	printf ("\n\nLine %d : Float replacement code not working.\n\n", __LINE__) ;
		dump_log_buffer (file) ;
		exit (1) ;
		} ;

	test_writef_double_or_die (file, 0, data, frames, __LINE__) ;

	sf_close (file) ;

	check_file_hash_or_die (filename, hash, __LINE__) ;

	memset (data, 0, items * sizeof (double)) ;

	if ((filetype & SF_FORMAT_TYPEMASK) != SF_FORMAT_RAW)
		memset (&sfinfo, 0, sizeof (sfinfo)) ;

	file = test_open_file_or_die (filename, SFM_READ, &sfinfo, __LINE__) ;
	sf_command (file, SFC_TEST_IEEE_FLOAT_REPLACE, NULL, replace_float) ;
	if (replace_float && string_in_log_buffer (file, "Using IEEE replacement") == 0)
	{	printf ("\n\nLine %d : Float replacement code not working.\n\n", __LINE__) ;
		dump_log_buffer (file) ;
		exit (1) ;
		} ;

	if (sfinfo.format != filetype)
	{	printf ("\n\nError (%s:%d) Stereo : Returned format incorrect (0x%08X => 0x%08X).\n", __FILE__, __LINE__, filetype, sfinfo.format) ;
		exit (1) ;
		} ;

	if (sfinfo.frames != frames)
	{	printf ("\n\nError (%s:%d) Stereo : Incorrect number of frames in file. (%d => %ld)\n", __FILE__, __LINE__, frames, SF_COUNT_TO_LONG (sfinfo.frames)) ;
		exit (1) ;
		} ;

	if (sfinfo.channels != 2)
	{	printf ("\n\nError (%s:%d) Stereo : Incorrect number of channels in file.\n", __FILE__, __LINE__) ;
		exit (1) ;
		} ;

	check_log_buffer_or_die (file, __LINE__) ;

	test_readf_double_or_die (file, 0, data, frames, __LINE__) ;
	for (sign = -1, k = 0 ; k < items ; k++)
	{	error = fabs (data [k] - ((double) k) / 100.0 * (sign *= -1)) ;
		if (fabs (data [k]) > 1e-100 && fabs (error / data [k]) > 1e-5)
		{	printf ("\n\nError (%s:%d) Stereo : Incorrect sample (#%d : %f => %f).\n", __FILE__, __LINE__, k, ((double) k) / 100.0, data [k]) ;
			exit (1) ;
			} ;
		} ;

	/* Seek to start of file. */
	test_seek_or_die (file, 0, SEEK_SET, 0, sfinfo.channels, __LINE__) ;

	test_readf_double_or_die (file, 0, data, 4, __LINE__) ;
	for (k = 0 ; k < 4 ; k++)
	{	error = fabs (data [k] - ((double) k) / 100.0 * (sign *= -1)) ;
		if (fabs (data [k]) > 1e-100 && fabs (error / data [k]) > 1e-5)
		{	printf ("\n\nError (%s:%d) Stereo : Incorrect sample (#%d : %f => %f).\n", __FILE__, __LINE__, k, ((double) k) / 100.0, data [k]) ;
			exit (1) ;
			} ;
		} ;

	/* Seek to offset from start of file. */
	test_seek_or_die (file, 10, SEEK_SET, 10, sfinfo.channels, __LINE__) ;

	test_readf_double_or_die (file, 0, data + 20, 2, __LINE__) ;
	for (k = 20 ; k < 24 ; k++)
	{	error = fabs (data [k] - ((double) k) / 100.0 * (sign *= -1)) ;
		if (fabs (data [k]) > 1e-100 && fabs (error / data [k]) > 1e-5)
		{	printf ("\n\nError (%s:%d) Stereo : Incorrect sample (#%d : %f => %f).\n", __FILE__, __LINE__, k, ((double) k) / 100.0, data [k]) ;
			exit (1) ;
			} ;
		} ;

	/* Seek to offset from current position. */
	test_seek_or_die (file, 8, SEEK_CUR, 20, sfinfo.channels, __LINE__) ;

	test_readf_double_or_die (file, 0, data + 40, 2, __LINE__) ;
	for (k = 40 ; k < 44 ; k++)
	{	error = fabs (data [k] - ((double) k) / 100.0 * (sign *= -1)) ;
		if (fabs (data [k]) > 1e-100 && fabs (error / data [k]) > 1e-5)
		{	printf ("\n\nError (%s:%d) Stereo : Incorrect sample (#%d : %f => %f).\n", __FILE__, __LINE__, k, ((double) k) / 100.0, data [k]) ;
			exit (1) ;
			} ;
		} ;

	/* Seek to offset from end of file. */
	test_seek_or_die (file, -1 * (sfinfo.frames - 10), SEEK_END, 10, sfinfo.channels, __LINE__) ;

	test_readf_double_or_die (file, 0, data + 20, 2, __LINE__) ;
	for (k = 20 ; k < 24 ; k++)
	{	error = fabs (data [k] - ((double) k) / 100.0 * (sign *= -1)) ;
		if (fabs (data [k]) > 1e-100 && fabs (error / data [k]) > 1e-5)
		{	printf ("\n\nError (%s:%d) Stereo : Incorrect sample (#%d : %f => %f).\n", __FILE__, __LINE__, k, ((double) k) / 100.0, data [k]) ;
			exit (1) ;
			} ;
		} ;

	sf_close (file) ;

	printf ("ok\n") ;
	unlink (filename) ;
} /* pcm_test_float */

static void
pcm_test_double (const char *filename, int	filetype, int hash, int replace_float)
{	SNDFILE			*file ;
	SF_INFO			sfinfo ;
	int				k, items, frames ;
	int				sign ;
	double			*data, error ;

	/* This is the best test routine. Other should be brought up to this standard. */

	print_test_name ("pcm_test_double", filename) ;

	items = BUFFER_SIZE / sizeof (double) ;

	data = (double*) data_out ;
	for (sign = 1, k = 0 ; k < items ; k++)
	{	data [k] = ((double) (k * sign)) / 100.0 ;
		sign = (sign > 0) ? -1 : 1 ;
		} ;

	sfinfo.samplerate	= 44100 ;
	sfinfo.frames		= items ;
	sfinfo.channels		= 1 ;
	sfinfo.format		= filetype ;

	file = test_open_file_or_die (filename, SFM_WRITE, &sfinfo, __LINE__) ;
	sf_command (file, SFC_TEST_IEEE_FLOAT_REPLACE, NULL, replace_float) ;
	if (replace_float && string_in_log_buffer (file, "Using IEEE replacement") == 0)
	{	printf ("\n\nLine %d : Float replacement code not working.\n\n", __LINE__) ;
		dump_log_buffer (file) ;
		exit (1) ;
		} ;

	test_write_double_or_die (file, 0, data, items, __LINE__) ;

	sf_close (file) ;

#if (defined (WIN32) || defined (_WIN32))
	/* File hashing on Win32 fails due to slighty different
	** calculated values of the sin() function.
	*/
	hash = hash ; /* Avoid compiler warning. */
#else
	check_file_hash_or_die (filename, hash, __LINE__) ;
#endif

	memset (data, 0, items * sizeof (double)) ;

	if ((filetype & SF_FORMAT_TYPEMASK) != SF_FORMAT_RAW)
		memset (&sfinfo, 0, sizeof (sfinfo)) ;

	file = test_open_file_or_die (filename, SFM_READ, &sfinfo, __LINE__) ;
	sf_command (file, SFC_TEST_IEEE_FLOAT_REPLACE, NULL, replace_float) ;
	if (replace_float && string_in_log_buffer (file, "Using IEEE replacement") == 0)
	{	printf ("\n\nLine %d : Float replacement code not working.\n\n", __LINE__) ;
		dump_log_buffer (file) ;
		exit (1) ;
		} ;

	if (sfinfo.format != filetype)
	{	printf ("\n\nError (%s:%d) Mono : Returned format incorrect (0x%08X => 0x%08X).\n", __FILE__, __LINE__, filetype, sfinfo.format) ;
		exit (1) ;
		} ;

	if (sfinfo.frames != items)
	{	printf ("\n\nError (%s:%d) Mono : Incorrect number of frames in file. (%d => %ld)\n", __FILE__, __LINE__, items, SF_COUNT_TO_LONG (sfinfo.frames)) ;
		exit (1) ;
		} ;

	if (sfinfo.channels != 1)
	{	printf ("\n\nError (%s:%d) Mono : Incorrect number of channels in file.\n", __FILE__, __LINE__) ;
		exit (1) ;
		} ;

	check_log_buffer_or_die (file, __LINE__) ;

	test_read_double_or_die (file, 0, data, items, __LINE__) ;

	for (sign = -1, k = 0 ; k < items ; k++)
	{	error = fabs (data [k] - ((double) k) / 100.0 * (sign *= -1)) ;
		if (fabs (data [k]) > 1e-100 && fabs (error / data [k]) > 1e-5)
		{	printf ("\n\nError (%s:%d) Mono : Incorrect sample (#%d : %f => %f).\n", __FILE__, __LINE__, k, ((double) k) / 100.0, data [k]) ;
			exit (1) ;
			} ;
		} ;

	/* Seek to start of file. */
	test_seek_or_die (file, 0, SEEK_SET, 0, sfinfo.channels, __LINE__) ;

	test_read_double_or_die (file, 0, data, 4, __LINE__) ;
	for (k = 0 ; k < 4 ; k++)
	{	error = fabs (data [k] - ((double) k) / 100.0 * (sign *= -1)) ;
		if (fabs (data [k]) > 1e-100 && fabs (error / data [k]) > 1e-5)
		{	printf ("\n\nError (%s:%d) Mono : Incorrect sample (#%d : %f => %f).\n", __FILE__, __LINE__, k, ((double) k) / 100.0, data [k]) ;
			exit (1) ;
			} ;
		} ;

	/* Seek to offset from start of file. */
	test_seek_or_die (file, 10, SEEK_SET, 10, sfinfo.channels, __LINE__) ;

	test_read_double_or_die (file, 0, data + 10, 4, __LINE__) ;

	test_seek_or_die (file, 0, SEEK_CUR, 14, sfinfo.channels, __LINE__) ;

	for (k = 10 ; k < 14 ; k++)
	{	error = fabs (data [k] - ((double) k) / 100.0 * (sign *= -1)) ;
		if (fabs (data [k]) > 1e-100 && fabs (error / data [k]) > 1e-5)
		{	printf ("\n\nError (%s:%d) Mono : Incorrect sample (#%d : %f => %f).\n", __FILE__, __LINE__, k, ((double) k) / 100.0, data [k]) ;
			exit (1) ;
			} ;
		} ;

	/* Seek to offset from current position. */
	test_seek_or_die (file, 6, SEEK_CUR, 20, sfinfo.channels, __LINE__) ;

	test_read_double_or_die (file, 0, data + 20, 4, __LINE__) ;
	for (k = 20 ; k < 24 ; k++)
	{	error = fabs (data [k] - ((double) k) / 100.0 * (sign *= -1)) ;
		if (fabs (data [k]) > 1e-100 && fabs (error / data [k]) > 1e-5)
		{	printf ("\n\nError (%s:%d) Mono : Incorrect sample (#%d : %f => %f).\n", __FILE__, __LINE__, k, ((double) k) / 100.0, data [k]) ;
			exit (1) ;
			} ;
		} ;

	/* Seek to offset from end of file. */
	test_seek_or_die (file, -1 * (sfinfo.frames - 10), SEEK_END, 10, sfinfo.channels, __LINE__) ;

	test_read_double_or_die (file, 0, data + 10, 4, __LINE__) ;
	for (k = 10 ; k < 14 ; k++)
	{	error = fabs (data [k] - ((double) k) / 100.0 * (sign *= -1)) ;
		if (fabs (data [k]) > 1e-100 && fabs (error / data [k]) > 1e-5)
		{	printf ("\n\nError (%s:%d) Mono : Incorrect sample (#%d : %f => %f).\n", __FILE__, __LINE__, k, ((double) k) / 100.0, data [k]) ;
			exit (1) ;
			} ;
		} ;

	sf_close (file) ;

	/* Now test Stereo. */

	if ((filetype & SF_FORMAT_TYPEMASK) == SF_FORMAT_SVX) /* SVX is mono only */
	{	printf ("ok\n") ;
		return ;
		} ;

	items = BUFFER_SIZE / sizeof (double) ;

	data = (double*) data_out ;
	for (sign = -1, k = 0 ; k < items ; k++)
		data [k] = ((double) k) / 100.0 * (sign *= -1) ;

	sfinfo.samplerate	= 44100 ;
	sfinfo.frames		= items ;
	sfinfo.channels		= 2 ;
	sfinfo.format		= filetype ;

	frames = items / sfinfo.channels ;

	file = test_open_file_or_die (filename, SFM_WRITE, &sfinfo, __LINE__) ;
	sf_command (file, SFC_TEST_IEEE_FLOAT_REPLACE, NULL, replace_float) ;
	if (replace_float && string_in_log_buffer (file, "Using IEEE replacement") == 0)
	{	printf ("\n\nLine %d : Float replacement code not working.\n\n", __LINE__) ;
		dump_log_buffer (file) ;
		exit (1) ;
		} ;

	test_writef_double_or_die (file, 0, data, frames, __LINE__) ;

	sf_close (file) ;

#if (defined (WIN32) || defined (_WIN32))
	/* File hashing on Win32 fails due to slighty different
	** calculated values.
	*/
	hash = hash ; /* Avoid compiler warning. */
#else
	check_file_hash_or_die (filename, hash, __LINE__) ;
#endif

	memset (data, 0, items * sizeof (double)) ;

	if ((filetype & SF_FORMAT_TYPEMASK) != SF_FORMAT_RAW)
		memset (&sfinfo, 0, sizeof (sfinfo)) ;

	file = test_open_file_or_die (filename, SFM_READ, &sfinfo, __LINE__) ;
	sf_command (file, SFC_TEST_IEEE_FLOAT_REPLACE, NULL, replace_float) ;
	if (replace_float && string_in_log_buffer (file, "Using IEEE replacement") == 0)
	{	printf ("\n\nLine %d : Float replacement code not working.\n\n", __LINE__) ;
		dump_log_buffer (file) ;
		exit (1) ;
		} ;

	if (sfinfo.format != filetype)
	{	printf ("\n\nError (%s:%d) Stereo : Returned format incorrect (0x%08X => 0x%08X).\n", __FILE__, __LINE__, filetype, sfinfo.format) ;
		exit (1) ;
		} ;

	if (sfinfo.frames != frames)
	{	printf ("\n\nError (%s:%d) Stereo : Incorrect number of frames in file. (%d => %ld)\n", __FILE__, __LINE__, frames, SF_COUNT_TO_LONG (sfinfo.frames)) ;
		exit (1) ;
		} ;

	if (sfinfo.channels != 2)
	{	printf ("\n\nError (%s:%d) Stereo : Incorrect number of channels in file.\n", __FILE__, __LINE__) ;
		exit (1) ;
		} ;

	check_log_buffer_or_die (file, __LINE__) ;

	test_readf_double_or_die (file, 0, data, frames, __LINE__) ;

	for (sign = -1, k = 0 ; k < items ; k++)
	{	error = fabs (data [k] - ((double) k) / 100.0 * (sign *= -1)) ;
		if (fabs (data [k]) > 1e-100 && fabs (error / data [k]) > 1e-5)
		{	printf ("\n\nError (%s:%d) Stereo : Incorrect sample (#%d : %f => %f).\n", __FILE__, __LINE__, k, ((double) k) / 100.0, data [k]) ;
			exit (1) ;
			} ;
		} ;

	/* Seek to start of file. */
	test_seek_or_die (file, 0, SEEK_SET, 0, sfinfo.channels, __LINE__) ;

	test_read_double_or_die (file, 0, data, 4, __LINE__) ;
	for (k = 0 ; k < 4 ; k++)
	{	error = fabs (data [k] - ((double) k) / 100.0 * (sign *= -1)) ;
		if (fabs (data [k]) > 1e-100 && fabs (error / data [k]) > 1e-5)
		{	printf ("\n\nError (%s:%d) Stereo : Incorrect sample (#%d : %f => %f).\n", __FILE__, __LINE__, k, ((double) k) / 100.0, data [k]) ;
			exit (1) ;
			} ;
		} ;

	/* Seek to offset from start of file. */
	test_seek_or_die (file, 10, SEEK_SET, 10, sfinfo.channels, __LINE__) ;

	test_read_double_or_die (file, 0, data + 10, 4, __LINE__) ;
	for (k = 20 ; k < 24 ; k++)
	{	error = fabs (data [k] - ((double) k) / 100.0 * (sign *= -1)) ;
		if (fabs (data [k]) > 1e-100 && fabs (error / data [k]) > 1e-5)
		{	printf ("\n\nError (%s:%d) Stereo : Incorrect sample (#%d : %f => %f).\n", __FILE__, __LINE__, k, ((double) k) / 100.0, data [k]) ;
			exit (1) ;
			} ;
		} ;

	/* Seek to offset from current position. */
	test_seek_or_die (file, 8, SEEK_CUR, 20, sfinfo.channels, __LINE__) ;

	test_readf_double_or_die (file, 0, data + 40, 4, __LINE__) ;
	for (k = 40 ; k < 44 ; k++)
	{	error = fabs (data [k] - ((double) k) / 100.0 * (sign *= -1)) ;
		if (fabs (data [k]) > 1e-100 && fabs (error / data [k]) > 1e-5)
		{	printf ("\n\nError (%s:%d) Stereo : Incorrect sample (#%d : %f => %f).\n", __FILE__, __LINE__, k, ((double) k) / 100.0, data [k]) ;
			exit (1) ;
			} ;
		} ;

	/* Seek to offset from end of file. */
	test_seek_or_die (file, -1 * (sfinfo.frames -10), SEEK_END, 10, sfinfo.channels, __LINE__) ;

	test_readf_double_or_die (file, 0, data + 20, 4, __LINE__) ;
	for (k = 20 ; k < 24 ; k++)
	{	error = fabs (data [k] - ((double) k) / 100.0 * (sign *= -1)) ;
		if (fabs (data [k]) > 1e-100 && fabs (error / data [k]) > 1e-5)
		{	printf ("\n\nError (%s:%d) Stereo : Incorrect sample (#%d : %f => %f).\n", __FILE__, __LINE__, k, ((double) k) / 100.0, data [k]) ;
			exit (1) ;
			} ;
		} ;

	sf_close (file) ;

	printf ("ok\n") ;
	unlink (filename) ;
} /* pcm_test_double */

/*==============================================================================
*/

[+ COMMENT

 Do not edit or modify anything in this comment block.
 The following line is a file identity tag for the GNU Arch 
 revision control system.

 arch-tag: cad1443b-99d9-414e-883f-178817600d40

+]
