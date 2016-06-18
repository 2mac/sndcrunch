/*
 *  sndcrunch - A simple audio bit crunching tool
 *  Copyright (C) 2015-2016 David McMackins II
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

enum sc_error
  {
    SC_EALLOC    = -1,
    SC_ENULLPATH = -2,
    SC_ESAMEPATH = -3
  };

const char *
sc_error_string (int rc);

int
sc_crunch (const char *in_path, const char *out_path, unsigned int depth);

void
sc_cleanup (void);

#endif
