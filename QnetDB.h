#pragma once
/*
 *   Copyright (C) 2020 by Thomas Early N7TAE
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <sqlite3.h>

class CQnetDB {
public:
	CQnetDB() : db(NULL) {}
	~CQnetDB() { if (db) sqlite3_close(db); }
	bool Open(const char *name);
	bool Update(const char *mycall, const char *sfx, const char *urcall, const char *module, const char *gateway);

private:
	sqlite3 *db;
};
