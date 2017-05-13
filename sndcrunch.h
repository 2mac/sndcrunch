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

#ifndef SNDCRUNCH_H
#define SNDCRUNCH_H

#define VERSION_STRING "sndcrunch 2.0.2"

enum sc_error
{
	SC_EALLOC    = -1,
	SC_ENULLPATH = -2,
	SC_ESAMEPATH = -3,
	SC_EZEROLOSS = -4,
	SC_EHIGHLOSS = -5
};

#define SC_MAX_LOSS (1000)

const char *
sc_error_string(int rc);

int
sc_crunch(const char *in_path, const char *out_path, unsigned int loss);

void
sc_cleanup(void);

#endif
