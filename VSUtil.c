/*
 * VSUtil.c
 * 
 * Copyright 2018  <pi@raspberrypi>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>

#include "VSUtil.h"

// Virtual studio utilities

void virtualstudio_set_dbpath(virtualstudio *vs, char *dbpath)
{
	strcpy(vs->dbpath, dbpath);
}

void virtualstudio_purge(virtualstudio *vs)
{
	sqlite3 *db;
	char *err_msg = NULL;
	char *sql;
	int rc;

	if ((rc = sqlite3_open(vs->dbpath, &db)))
	{
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
	}
	else
	{
//printf("Opened database successfully\n");
		sql = "delete from audioeffects;";
		if ((rc = sqlite3_exec(db, sql, 0, 0, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to delete data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}

		sql = "delete from audiochains;";
		if ((rc = sqlite3_exec(db, sql, 0, 0, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to delete data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}

		sql = "delete from chaineffects;";
		if ((rc = sqlite3_exec(db, sql, 0, 0, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to delete data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}
	}
	sqlite3_close(db);
}

int virtualstudio_select_max_callback(void *data, int argc, char **argv, char **azColName) 
{
	virtualstudio *vs = (virtualstudio*)data;

	vs->effid = (argv[0]?atoi((const char *)argv[0]):0);

	return 0;
}

void virtualstudio_addeffect(virtualstudio *vs, char *path)
{
	sqlite3 *db;
	char *err_msg = NULL;
	char *sql;
	int rc;
	char ins[256];

	if ((rc = sqlite3_open(vs->dbpath, &db)))
	{
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
	}
	else
	{
//printf("Opened database successfully\n");
		vs->effid = 0;

		sql = "select max(id) as id from audioeffects;";
		if ((rc = sqlite3_exec(db, sql, virtualstudio_select_max_callback, (void*)vs, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to select data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}

		vs->effid++;
		sprintf(ins, "insert into audioeffects values(%d, '%s');", vs->effid, path);
		if ((rc = sqlite3_exec(db, ins, 0, 0, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to insert data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}
	}
	sqlite3_close(db);
}

void virtualstudio_deleteeffect(virtualstudio *vs, int id)
{
	sqlite3 *db;
	char *err_msg = NULL;
	int rc;
	char del[256];

	if ((rc = sqlite3_open(vs->dbpath, &db)))
	{
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
	}
	else
	{
//printf("Opened database successfully\n");
		sprintf(del, "delete from audioeffects where id = %d;", id);
		if ((rc = sqlite3_exec(db, del, 0, 0, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to delete data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}
	}
	sqlite3_close(db);
}

int virtualstudio_select_maxchain_callback(void *data, int argc, char **argv, char **azColName) 
{
	virtualstudio *vs = (virtualstudio*)data;

	vs->effid = (argv[0]?atoi((const char *)argv[0]):0);

	return 0;
}

void virtualstudio_addchain(virtualstudio *vs, char *name)
{
	sqlite3 *db;
	char *err_msg = NULL;
	char *sql;
	int rc;
	char ins[256];

	if ((rc = sqlite3_open(vs->dbpath, &db)))
	{
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
	}
	else
	{
//printf("Opened database successfully\n");
		vs->effid = 0;

		sql = "select max(id) as id from audiochains;";
		if ((rc = sqlite3_exec(db, sql, virtualstudio_select_maxchain_callback, (void*)vs, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to select data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}

		vs->effid++;
		sprintf(ins, "insert into audiochains values(%d, '%s');", vs->effid, name);
		if ((rc = sqlite3_exec(db, ins, 0, 0, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to insert data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}
	}
	sqlite3_close(db);
}

void virtualstudio_deletechain(virtualstudio *vs, int chain)
{
	sqlite3 *db;
	char *err_msg = NULL;
	int rc;
	char del[256];

	if ((rc = sqlite3_open(vs->dbpath, &db)))
	{
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
	}
	else
	{
//printf("Opened database successfully\n");
		sprintf(del, "delete from chaineffects where chain = %d;", chain);
		if ((rc = sqlite3_exec(db, del, 0, 0, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to delete data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}

		sprintf(del, "delete from audiochains where id = %d;", chain);
		if ((rc = sqlite3_exec(db, del, 0, 0, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to delete data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}
	}
	sqlite3_close(db);
}

void virtualstudio_assign_effect(virtualstudio *vs, int chain, int effect)
{
	sqlite3 *db;
	char *err_msg = NULL;
	int rc;
	char ins[256];

	if ((rc = sqlite3_open(vs->dbpath, &db)))
	{
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
	}
	else
	{
//printf("Opened database successfully\n");
		sprintf(ins, "insert into chaineffects values(%d, %d);", chain, effect);
		if ((rc = sqlite3_exec(db, ins, 0, 0, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to delete data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}
	}
	sqlite3_close(db);
}

void virtualstudio_unassign_effect(virtualstudio *vs, int chain, int effect)
{
	sqlite3 *db;
	char *err_msg = NULL;
	int rc;
	char del[256];

	if ((rc = sqlite3_open(vs->dbpath, &db)))
	{
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
	}
	else
	{
//printf("Opened database successfully\n");
		sprintf(del, "delete from chaineffects where chain = %d and effect = %d;", chain, effect);
		if ((rc = sqlite3_exec(db, del, 0, 0, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to delete data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}
	}
	sqlite3_close(db);
}
