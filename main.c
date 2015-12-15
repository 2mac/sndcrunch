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

#include <errno.h>
#include <limits.h>
#include <sndfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "sndcrunch.h"

#ifdef HAVE_CONFIG_H
# include "config.h"
# define VERSION_STRING PACKAGE_STRING
# define PROG PACKAGE
#else
# define VERSION_STRING "sndcrunch custom build"
# define PROG "sndcrunch"
#endif

#define USAGE_INFO "USAGE: " PROG " [options] <in_path> <out_path>\n\n"	\
  PROG " reads the audio file in_file and writes a crunched output to \
out_path.\n\n\
OPTIONS:\n\
\t-h, --help\t\tPrints this help message and exits\n\
\t-l, --loss=LEVEL\tSets the loss level (default 10)\n\
\t-v, --version\t\tPrints version info and exits\n"

#define VERSION_INFO VERSION_STRING "\n\
Copyright (C) 2015 David McMackins II\n\
License AGPLv3: GNU AGPL version 3 only <http://gnu.org/licenses/agpl.html>.\n\
This is libre software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\n\
Written by David McMackins II."

static void
usage (int rc)
{
  puts (USAGE_INFO);
  exit (rc);
}

static void
check_file_error (const char *path, const char *mode)
{
  FILE *f = fopen (path, mode);
  if (!f)
    {
      int rc = errno;
      fprintf (stderr, PROG ": %s: %s\n", path, strerror (rc));
      exit (rc);
    }

  fclose (f);
}

int
main (int argc, char *argv[])
{
  int rc = 0;
  unsigned short loss = 10;

  struct option longopts[] =
    {
      {"help", no_argument, 0, 'h'},
      {"loss", required_argument, 0, 'l'},
      {"version", no_argument, 0, 'v'},
      {0, 0, 0, 0}
    };

  if (argc > 1)
    {
      long temploss;
      int c, longindex;
      while ((c = getopt_long (argc, argv, "hl:v", longopts, &longindex))
	     != -1)
	{
	  switch (c)
	    {
	    case 'h':
	      usage (0);

	    case 'l':
	      rc = sscanf (optarg, "%lu", &temploss);
	      if (rc != 1)
		{
		  fprintf (stderr, PROG ": %s is not a valid number\n",
			   optarg);
		  return 3;
		}

	      if (temploss < 1 || temploss > USHRT_MAX)
		{
		  fprintf (stderr, PROG ": Loss level must be between 1 and "
			   "%hu\n", USHRT_MAX);
		  return 3;
		}

	      loss = temploss;
	      break;

	    case 'v':
	      puts (VERSION_INFO);
	      return 0;

	    case '?':
	      return 2;
	    }
	}
    }

  if ((argc - optind) != 2)
    usage (1);

  rc = sc_crunch (argv[optind], argv[optind + 1], loss);
  if (rc)
    {
      if (SF_ERR_SYSTEM == rc)
	{
	  check_file_error (argv[optind], "r");
	  check_file_error (argv[optind + 1], "a");
	}

      fprintf (stderr, PROG ": %s\n", sc_error_string (rc));
      return rc;
    }

  sc_cleanup ();
  return 0;
}
