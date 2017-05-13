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
#include <errno.h>
#include <sndfile.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "sndcrunch.h"

#define PROG "sndcrunch"

#define USAGE_INFO "USAGE: " PROG " [options] <in_path> <out_path>\n\n"	\
	PROG " reads the audio file in_file and writes a crunched output to \
out_path.\n\n\
OPTIONS:\n\
\t-f\tSilently overwrite output file if it exists\n\
\t-h\tPrints this help message and exits\n\
\t-l\tSets the loss level (default 10)\n\
\t-v\tPrints version info and exits\n"

#define VERSION_INFO VERSION_STRING "\n\
Copyright (C) 2015-2017 David McMackins II\n\
License AGPLv3: GNU AGPL version 3 only <http://gnu.org/licenses/agpl.html>.\n\
This is libre software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\n\
Written by David McMackins II.\n"

static void
usage(int rc)
{
	fprintf(stderr, USAGE_INFO);
	exit(rc);
}

static bool
fexists(const char *path)
{
	return 0 == access(path, F_OK);
}

static void
check_file_error(const char *path, const char *mode)
{
	FILE *f = fopen(path, mode);
	if (!f)
	{
		fprintf(stderr, PROG ": %s: %s\n", path, strerror(errno));
		exit(1);
	}

	fclose(f);
}

int
main(int argc, char *argv[])
{
	bool force = false;
	int rc = 0;
	unsigned int loss = 10;

	if (argc > 1)
	{
		int c;
		while ((c = getopt(argc, argv, ":hfl:v")) != -1)
		{
			unsigned long temploss;

			switch (c)
			{
			case 'h':
				usage(0);

			case 'f':
				force = true;
				break;

			case 'l':
				rc = sscanf(optarg, "%lu", &temploss);
				if (rc != 1)
				{
					fprintf(stderr,
						PROG ": %s is not a valid number\n",
						optarg);
					return 1;
				}

				if (temploss < 1 || temploss > SC_MAX_LOSS)
				{
					fprintf(stderr,
						PROG ": Loss level must be between 1 and %u\n",
						SC_MAX_LOSS);
					return 1;
				}

				loss = temploss;
				break;

			case 'v':
				printf(VERSION_INFO);
				return 0;

			case ':':
				fprintf(stderr,
					PROG ": option -%c requires an argument\n",
					optopt);
				// SPILLS OVER!

			case '?':
				usage(1);
			}
		}
	}

	if ((argc - optind) != 2)
		usage(1);

	const char *in_path = argv[optind];
	const char *out_path = argv[optind + 1];

	if (!force && fexists(out_path))
	{
		printf("%s exists. Overwrite? [y/N] ", out_path);
		int c = tolower(getchar());
		if ('y' != c)
			return 1;

		while (c != '\n')
			c = getchar(); // clear input line
	}

	rc = sc_crunch(in_path, out_path, loss);
	if (rc)
	{
		if (SF_ERR_SYSTEM == rc)
		{
			check_file_error(in_path, "r");
			check_file_error(out_path, "a");
		}

		fprintf(stderr, PROG ": %s\n", sc_error_string(rc));
		return 1;
	}

	sc_cleanup();
	return 0;
}
