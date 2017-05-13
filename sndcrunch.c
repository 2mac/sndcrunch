/*
 *  sndcrunch - A simple audio bit crunching tool
 *  Copyright (C) 2015-2017 David McMackins II
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, version 3 only.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ctype.h>
#include <pfxtree.h>
#include <sndfile.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sndcrunch.h"

#define MAX_EXT_LEN (4)

static PrefixTree *formats = NULL;

const char *
sc_error_string(int rc)
{
	if (0 == rc)
		return "No error";

	if (rc < 0)
		switch (rc)
		{
		case SC_EALLOC:
			return "Out of memory";

		case SC_ENULLPATH:
			return "Null path supplied";

		case SC_ESAMEPATH:
			return "Input and output paths are the same";

		case SC_EZEROLOSS:
			return "Loss level is zero";

		case SC_EHIGHLOSS:
			return "Loss level is too high";

		default:
			return "Unknown error";
		}

	return sf_error_number(rc);
}

static int
init_formats()
{
	formats = pt_new();
	if (!formats)
		return SC_EALLOC;

#define ADD(S,I) if (pt_add (formats, (S), (I))) goto fail;

	ADD("aif", SF_FORMAT_AIFF | SF_FORMAT_PCM_16);
	ADD("aiff", SF_FORMAT_AIFF | SF_FORMAT_PCM_16);

	ADD("au", SF_FORMAT_AU | SF_FORMAT_PCM_16);
	ADD("snd", SF_FORMAT_AU | SF_FORMAT_PCM_16);

	ADD("caf", SF_FORMAT_CAF | SF_FORMAT_PCM_16);

	ADD("flac", SF_FORMAT_FLAC | SF_FORMAT_PCM_16);

	ADD("ogg", SF_FORMAT_OGG | SF_FORMAT_VORBIS);

	ADD("wav", SF_FORMAT_WAV | SF_FORMAT_PCM_16);
	ADD("wave", SF_FORMAT_WAV | SF_FORMAT_PCM_16);

	ADD("xi", SF_FORMAT_XI | SF_FORMAT_DPCM_16);

	return 0;

fail:
	pt_free(formats);
	formats = NULL;
	return SC_EALLOC;
}

int
sc_crunch(const char *in_path, const char *out_path, unsigned int loss)
{
	int rc = 0;
	SF_INFO in_info, out_info;
	SNDFILE *in_file = NULL, *out_file = NULL;

	if (0 == loss)
		return SC_EZEROLOSS;

	if (loss > SC_MAX_LOSS)
		return SC_EHIGHLOSS;

	if (!in_path || !out_path)
		return SC_ENULLPATH;

	if (0 == strcmp(in_path, out_path))
		return SC_ESAMEPATH;

	if (!formats)
	{
		rc = init_formats();
		if (rc)
			goto end;
	}

	in_info.format = 0;
	in_file = sf_open(in_path, SFM_READ, &in_info);
	if (!in_file)
	{
		rc = sf_error(NULL);
		goto end;
	}

	const char *out_ext = strrchr(out_path, '.');
	if (!out_ext)
	{
		rc = SF_ERR_UNRECOGNISED_FORMAT;
		goto end;
	}

	++out_ext; // don't include dot
	size_t ext_len = strlen(out_ext);
	if (ext_len > MAX_EXT_LEN)
	{
		rc = SF_ERR_UNRECOGNISED_FORMAT;
		goto end;
	}

	char temp_ext[MAX_EXT_LEN + 1];
	for (size_t i = 0; i <= ext_len; ++i)
		temp_ext[i] = tolower(out_ext[i]);

	const PrefixTree *ext_type = pt_search(formats, temp_ext);
	if (!ext_type)
	{
		rc = SF_ERR_UNRECOGNISED_FORMAT;
		goto end;
	}

	out_info.samplerate = in_info.samplerate;
	out_info.channels = in_info.channels;
	out_info.format = pt_data(ext_type);

	out_file = sf_open(out_path, SFM_WRITE, &out_info);
	if (!out_file)
	{
		rc = sf_error(NULL);
		if (rc != SF_ERR_UNRECOGNISED_FORMAT || 1 == out_info.channels)
			goto end;

		rc = 0;
		out_info.channels = 1;
		out_file = sf_open(out_path, SFM_WRITE, &out_info);
		if (!out_file)
		{
			rc = sf_error(NULL);
			goto end;
		}
	}

	for (int type = SF_STR_FIRST; type <= SF_STR_LAST; ++type)
	{
		char sw[64];
		const char *orig_string;

		if (SF_STR_SOFTWARE == type)
		{
			sprintf(sw, "%s at loss level %u", VERSION_STRING,
				loss);
			orig_string = sw;
		}
		else
		{
			orig_string = sf_get_string(in_file, type);
			if (NULL == orig_string)
				continue;
		}

		sf_set_string(out_file, type, orig_string); // ignore errors
	}

	int frames[SC_MAX_LOSS * 2];
	sf_count_t num_read;

	while ((num_read = sf_readf_int(in_file, frames, loss)))
	{
		if (1 == out_info.channels && 2 == in_info.channels)
		{
			int *p;
			sf_count_t i;

			for (i = 0, p = frames; i < num_read; ++i, p += 2)
			{
				intmax_t sum = p[0];
				sum += p[1];
				sum /= 2;

				frames[i] = (int) sum;
			}
		}

		for (int channel = 0; channel < out_info.channels; ++channel)
		{
			intmax_t avg = 0;

			for (sf_count_t i = 0; i < num_read; ++i)
				avg += frames[channel
					+ (i * out_info.channels)];

			avg /= num_read;

			for (sf_count_t i = 0; i < num_read; ++i)
				frames[channel
					+ (i * out_info.channels)] = (int) avg;
		}

		sf_count_t num_written = sf_writef_int(out_file, frames,
						num_read);
		if (num_written != num_read)
		{
			rc = sf_error (out_file);
			goto end;
		}
	}

	sf_write_sync(out_file);

end:
	if (in_file)
		sf_close(in_file);

	if (out_file)
		sf_close(out_file);

	if (rc)
	{
		remove(out_path);
		sc_cleanup();
	}

	return rc;
}

void
sc_cleanup()
{
	if (formats)
	{
		pt_free(formats);
		formats = NULL;
	}
}
