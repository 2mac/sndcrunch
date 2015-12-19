/*
 *  sndcrunch - A simple audio bit crunching tool
 *  Copyright (C) 2015 David McMackins II
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

#ifdef HAVE_CONFIG_H
# include "config.h"
# define VERSION_STRING PACKAGE_STRING
# define PROG PACKAGE
#else
# define VERSION_STRING "sndcrunch custom build"
# define PROG "sndcrunch"
#endif

static PrefixTree *formats = NULL;

const char *
sc_error_string (int rc)
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

      default:
	return "Unknown error";
      }

  return sf_error_number (rc);
}

static int
init_formats ()
{
  formats = pt_new ();
  if (!formats)
    return SC_EALLOC;

#define ADD(S,I) if (pt_add (formats, S, I))	\
    {						\
      pt_free (formats);			\
      formats = NULL;				\
      return SC_EALLOC;				\
    }

  ADD ("aif", SF_FORMAT_AIFF | SF_FORMAT_PCM_16);
  ADD ("aiff", SF_FORMAT_AIFF | SF_FORMAT_PCM_16);

  ADD ("au", SF_FORMAT_AU | SF_FORMAT_PCM_16);
  ADD ("snd", SF_FORMAT_AU | SF_FORMAT_PCM_16);

  ADD ("caf", SF_FORMAT_CAF | SF_FORMAT_PCM_16);

  ADD ("flac", SF_FORMAT_FLAC | SF_FORMAT_PCM_16);

  ADD ("ogg", SF_FORMAT_OGG | SF_FORMAT_VORBIS);

  ADD ("wav", SF_FORMAT_WAV | SF_FORMAT_PCM_16);
  ADD ("wave", SF_FORMAT_WAV | SF_FORMAT_PCM_16);

  return 0;
}

int
sc_crunch (const char *in_path, const char *out_path, unsigned short loss)
{
  int rc = 0;
  short *frame = NULL;
  SNDFILE *in_file = NULL, *out_file = NULL;

  if (!in_path || !out_path)
    return SC_ENULLPATH;

  if (0 == strcmp (in_path, out_path))
    return SC_ESAMEPATH;

  if (!formats)
    {
      rc = init_formats ();
      if (rc)
	goto end;
    }

  SF_INFO in_info = { .format = 0 };
  in_file = sf_open (in_path, SFM_READ, &in_info);
  if (!in_file)
    {
      rc = sf_error (NULL);
      goto end;
    }

  const char *out_ext = strrchr (out_path, '.');
  if (!out_ext)
    {
      rc = SF_ERR_UNRECOGNISED_FORMAT;
      goto end;
    }

  ++out_ext; // don't include dot
  size_t ext_len = strlen (out_ext);
  char *temp_ext = malloc ((ext_len + 1) * sizeof (char));
  if (!temp_ext)
    {
      rc = SC_EALLOC;
      goto end;
    }

  for (size_t i = 0; i <= ext_len; ++i)
    temp_ext[i] = tolower (out_ext[i]);

  const PrefixTree *ext_type = pt_search (formats, temp_ext);
  free (temp_ext);
  if (!ext_type)
    {
      rc = SF_ERR_UNRECOGNISED_FORMAT;
      goto end;
    }

  SF_INFO out_info =
    {
      .samplerate = in_info.samplerate,
      .channels = in_info.channels,
      .format = pt_data (ext_type)
    };

  out_file = sf_open (out_path, SFM_WRITE, &out_info);
  if (!out_file)
    {
      rc = sf_error (NULL);
      goto end;
    }

  for (int type = SF_STR_FIRST; type <= SF_STR_LAST; ++type)
    {
      char sw[64];
      const char *orig_string;

      if (SF_STR_SOFTWARE == type)
	{
	  sprintf (sw, "%s at loss level %hu", VERSION_STRING, loss);
	  orig_string = sw;
	}
      else
	{
	  orig_string = sf_get_string (in_file, type);
	  if (NULL == orig_string)
	    continue;
	}

      rc = sf_set_string (out_file, type, orig_string);
      if (rc)
	goto end;
    }

  frame = malloc ((in_info.channels * loss) * sizeof (short));
  if (!frame)
    {
      rc = SC_EALLOC;
      goto end;
    }

  sf_count_t read;
  while ((read = sf_readf_short (in_file, frame, loss)))
    {
      for (int channel = 0; channel < in_info.channels; ++channel)
	{
	  intmax_t avg = 0;

	  for (unsigned short i = 0; i < read; ++i)
	    avg += frame[channel + (i * in_info.channels)];

	  avg /= read;

	  for (unsigned short i = 0; i < read; ++i)
	    frame[channel + (i * in_info.channels)] = (short) avg;
	}

      sf_count_t written = sf_writef_short (out_file, frame, read);
      if (written != read)
	{
	  rc = sf_error (out_file);
	  goto end;
	}
    }

  sf_write_sync (out_file);

 end:
  if (frame)
    free (frame);

  if (in_file)
    sf_close (in_file);

  if (out_file)
    sf_close (out_file);

  if (rc)
    {
      remove (out_path);
      sc_cleanup ();
    }

  return rc;
}

void
sc_cleanup ()
{
  if (formats)
    {
      pt_free (formats);
      formats = NULL;
    }
}
