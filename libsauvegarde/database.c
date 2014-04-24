/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 *    database.c
 *    This file is part of "Sauvegarde" project.
 *
 *    (C) Copyright 2014 Olivier Delhomme
 *     e-mail : olivier.delhomme@free.fr
 *
 *    "Sauvegarde" is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    "Sauvegarde" is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with "Sauvegarde".  If not, see <http://www.gnu.org/licenses/>
 */

/**
 * @file database.c
 * This file contains the functions to wrap database calls in all the
 * programs of "Sauvegarde" project.
 */

#include "libsauvegarde.h"

static void print_db_error(sqlite3 *db, const char *format, ...);
static void exec_sql_cmd(db_t *database, gchar *sql_cmd, gchar *format_message);
static int table_callback(void *num, int nbCol, char **data, char **nomCol);
static void verify_if_tables_exists(db_t *database);
static file_row_t *new_file_row_t(void);
static void free_file_row_t(file_row_t *row);
static int get_file_callback(void *a_row, int nb_col, char **data, char **name_col);
static file_row_t *get_file_id(db_t *database, meta_data_t *meta);
static int get_data_callback(void *a_data, int nb_col, char **data, char **name_col);
static void insert_file_checksums(db_t *database, meta_data_t *meta, hashs_t *hashs, guint64 file_id, hashs_t *inserted_hashs);
static data_t *get_data_from_checksum(db_t *database, gchar *encoded_hash);

/**
 * @returns a string containing the version of the database used.
 */
gchar *db_version(void)
{
    return (gchar *) sqlite3_libversion();
}


/**
 * Prints an error message to stderr and exit (db errors are considered as
 * fatal for now).
 * @param db : file connexion to the database.
 * @param format : the format of the message (as in printf)
 * @param ... : va_list of variable that are to be printed into format.
 */
static void print_db_error(sqlite3 *db, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);

    sqlite3_close(db);
    exit(EXIT_FAILURE);
}


/**
 * Executes the SQL command onto the database without any callback
 * @param database : the db_t * structure that contains the database connexion
 * @param sql_cmd : a gchar * SQL command to be executed onto the database
 * @param format_message : a gchar * format message to be used in case of an error
 */
static void exec_sql_cmd(db_t *database, gchar *sql_cmd, gchar *format_message)
{
    char *error_message = NULL;
    int result = 0;

    result = sqlite3_exec(database->db, sql_cmd, NULL, 0, &error_message);

    if (result != SQLITE_OK)
        {
            print_db_error(database->db, format_message, error_message);
        }
}


/**
 * Counts the number of row that we have by incrementing i.
 * @param num is an integer that will count the number of rows in the result.
 * @param nb_col gives the number of columns in this row.
 * @param data contains the data of each column.
 * @param name_col contains the name of each column.
 * @returns always 0.
 */
static int table_callback(void *num, int nb_col, char **data, char **name_col)
{
    int *i = (int *) num;

    *i = *i + 1;

    return 0;
}


/**
 * Verifies if the tables are created whithin the database and creates
 * them if there is no tables at all.
 * @param database : the structure to manage database's connexion.
 */
static void verify_if_tables_exists(db_t *database)
{
    char *error_message = NULL;
    int result = 0;
    int *i = NULL;               /**< Used to count the number of row */

    i = (int *) g_malloc0(sizeof(int));
    *i = 0;

    /* Trying to get all the tables that are in the database */
    result = sqlite3_exec(database->db, "SELECT * FROM sqlite_master WHERE type='table';", table_callback, i, &error_message);

    if (*i == 0)  /* No row (0) means that there is no table */
        {
            print_debug(stdout, N_("Creating tables into the database\n"));

            /* The database does not contain any tables. So we have to create them.         */
            /* Creation of checksum table that contains checksums and their associated data */
            exec_sql_cmd(database, "CREATE TABLE data (checksum TEXT PRIMARY KEY, size INTEGER, data TEXT);", N_("Error while creating database table 'data': %s\n"));

            /* Creation of buffers table that contains checksums and their associated data */
            exec_sql_cmd(database, "CREATE TABLE buffers (file_id INTEGER, buf_order INTEGER, checksum TEXT);", N_("Error while creating database table 'buffers': %s\n"));

            /* Creation of files table that contains everything about a file */
            exec_sql_cmd(database, "CREATE TABLE files (file_id  INTEGER PRIMARY KEY AUTOINCREMENT, type INTEGER, file_user TEXT, file_group TEXT, uid INTEGER, gid INTEGER, atime INTEGER, ctime INTEGER, mtime INTEGER, mode INTEGER, name TEXT);", N_("Error while creating database table 'files': %s\n"));
        }
}


/**
 * Says whether a file is in allready in the cache or not
 * @param database is the structure that contains everything that is
 *        related to the database (it's connexion for instance).
 * @param meta is the file's metadata that we want to know if it's already
 *        in the cache.
 * @returns a boolean that says TRUE if the file is already in the cache
 *          and FALSE if not.
 */
gboolean is_file_in_cache(db_t *database, meta_data_t *meta)
{
    file_row_t *row = NULL;

    if (meta != NULL && database != NULL)
        {

            row = get_file_id(database, meta);

            if (row != NULL && row->nb_row == 0) /* No row has been returned. It means that the file isn't in the cache */
                {
                    free_file_row_t(row);
                    return FALSE;
                }
            else
                {
                    free_file_row_t(row);
                    return TRUE;
                }

        }
    else
        {
            return FALSE;
        }
}


/**
 * Gets file_ids from returned rows.
 * @param a_row is a file_row_t * structure
 * @param nb_col gives the number of columns in this row.
 * @param data contains the data of each column.
 * @param name_col contains the name of each column.
 * @returns always 0.
 */
static int get_file_callback(void *a_row, int nb_col, char **data, char **name_col)
{
    file_row_t *row = (file_row_t *) a_row;

    row->nb_row = row->nb_row + 1;
    row->file_id_list = g_slist_append(row->file_id_list, g_strdup(data[0]));

    return 0;
}


/**
 * Returns the file_id for the specified file.
 * @param database is the structure that contains everything that is
 *        related to the database (it's connexion for instance).
 * @param meta is the file's metadata that we want to insert into the
 *        cache.
 * @returns a file_row_t structure filed with values returned by the
 *          database.
 */
static file_row_t *get_file_id(db_t *database, meta_data_t *meta)
{
    file_row_t *row = NULL;
    char *error_message = NULL;
    gchar *sql_command = NULL;
    int db_result = 0;

    row = new_file_row_t();

    sql_command = g_strdup_printf("SELECT file_id from files WHERE name='%s' AND type=%d AND uid=%d AND gid=%d AND atime=%ld AND ctime=%ld AND mtime=%ld AND mode=%d;", meta->name, meta->file_type, meta->uid, meta->gid, meta->atime, meta->ctime, meta->mtime, meta->mode);

    db_result = sqlite3_exec(database->db, sql_command, get_file_callback, row, &error_message);

    free_variable(sql_command);

    if (db_result == SQLITE_OK)
        {
           return row;
        }
    else
        {
            print_db_error(database->db, N_("Error while searching into the table 'files': %s\n"), error_message);
            return NULL; /* to avoid a compilation warning as we exited with failure in print_db_error */
        }
}


/**
 * Creates and inits a new file_row_t * structure.
 * @returns an empty file_row_t * structure.
 */
static file_row_t *new_file_row_t(void)
{
    file_row_t *row = NULL;

    row = (file_row_t *) g_malloc0(sizeof(file_row_t));

    row->nb_row = 0;
    row->file_id_list = NULL;

    return row;
}


/**
 * Frees everything whithin the file_row_t structure
 * @param row is the variable to be freed totaly
 */
static void free_file_row_t(file_row_t *row)
{
    if (row != NULL)
        {
            g_slist_free(row->file_id_list);
            free_variable(row);
        }
}


/**
 * Gets file_ids from returned rows.
 * @param a_data is a data_t * structure
 * @param nb_col gives the number of columns in this row.
 * @param data contains the data of each column.
 * @param name_col contains the name of each column.
 * @returns always 0.
 */
static int get_data_callback(void *a_data, int nb_col, char **data, char **name_col)
{
    data_t *my_data = (data_t *) a_data;

    if (data != NULL)
        {
            my_data->read = g_ascii_strtoull(data[0], NULL, 10);
            my_data->buffer = (guchar *) g_strdup(data[1]);
        }

    return 0;
}


/**
 * Gets data from a checksum
 * @param database is the structure that contains everything that is
 *        related to the database (it's connexion for instance).
 * @param encoded_hash is the checksum base64 encoded.
 * @returns a newly allocated data_t structure which may contain the data
 *          for the specified base64 encoded hash (checksum).
 */
static data_t *get_data_from_checksum(db_t *database, gchar *encoded_hash)
{
    data_t *a_data = NULL;
    char *error_message = NULL;
    gchar *sql_command = NULL;
    int db_result = 0;

    a_data = new_data_t_structure(NULL, 0);

    sql_command = g_strdup_printf("SELECT size, data FROM data WHERE checksum='%s' ;", encoded_hash);

    db_result = sqlite3_exec(database->db, sql_command, get_data_callback, a_data, &error_message);

    free_variable(sql_command);

    if (db_result == SQLITE_OK)
        {
           return a_data;
        }
    else
        {
            print_db_error(database->db, N_("Error while searching into the table 'data': %s\n"), error_message);
            return NULL; /* to avoid a compilation warning as we exited with failure in print_db_error */
        }
}

/**
 *
 */
static gboolean is_checksum_in_db(db_t *database, hashs_t *inserted_hashs, guint8 *a_hash)
{
    guint8 *hash = NULL;

    if (inserted_hashs != NULL)
        {
            hash = g_tree_lookup(inserted_hashs->tree_hash, a_hash);

            if (hash == NULL)
                {
                    return FALSE;
                }
            else
                {
                    return TRUE;
                }
        }
    else
        {
            return FALSE;
        }

}

/**
 * @param all_hashs is a hashs_t * structure
 * @param nb_col gives the number of columns in this row.
 * @param data contains the data of each column.
 * @param name_col contains the name of each column.
 * @returns always 0.
 */
static int get_all_checksum_callback(void *inserted_hashs, int nb_col, char **data, char **name_col)
{
    hashs_t *all_hashs = (hashs_t *) inserted_hashs;
    guint8 *a_hash = NULL;
    gsize len = 0;

    if (data != NULL)
        {
            a_hash = g_base64_decode(data[0], &len);
            g_tree_insert(all_hashs->tree_hash, a_hash, "1");
        }

    return 0;
}


/**
 * Gets all encoded hashs allready inserted into the 'data' table from the
 * database.
 * @param database is the structure that contains everything that is
 *        related to the database (it's connexion for instance).
 */
hashs_t *get_all_inserted_hashs(db_t *database)
{
    hashs_t *inserted_hashs = NULL;
    char *error_message = NULL;
    gchar *sql_command = NULL;
    int db_result = 0;

    inserted_hashs = new_hash_struct();

    sql_command = g_strdup_printf("SELECT checksum FROM data;");

    db_result = sqlite3_exec(database->db, sql_command, get_all_checksum_callback, inserted_hashs, &error_message);

    free_variable(sql_command);

    if (db_result == SQLITE_OK)
        {
           return inserted_hashs;
        }
    else
        {
            print_db_error(database->db, N_("Error while searching into the table 'data': %s\n"), error_message);
            return NULL; /* to avoid a compilation warning as we exited with failure in print_db_error */
        }
}


/**
 * Inserts the checksums related to the file into the database.
 * @param database is the structure that contains everything that is
 *        related to the database (it's connexion for instance).
 * @param meta is the file's metadata that we want to insert into the
 *        cache.
 * @param hashs : a balanced binary tree that stores hashs.
 * @todo find a more efficient way to know if a hash has already been
 *       inserted with it's data into the database. A way may be to store
 *       inserted hashs into a balanced binary tree for the session and to
 *       retreive them at the beginning of the session.
 */
static void insert_file_checksums(db_t *database, meta_data_t *meta, hashs_t *hashs, guint64 file_id, hashs_t *inserted_hashs)
{
    GSList *head = NULL;
    guint8 *a_hash = NULL;
    gchar *encoded_hash = NULL;  /**< base64 encoded hash        */
    gchar *encoded_data = NULL;  /**< base64 encoded data buffer */
    data_t *a_data = NULL;
    guint64 i = 0;
    gchar *sql_command = NULL;

    if (database != NULL && meta != NULL)
        {
            head = meta->hash_list;
            i = 0;
            while (head != NULL)
                {
                    a_hash = head->data;
                    encoded_hash = g_base64_encode((guchar*) a_hash, HASH_LEN);

                    /* Inserting the hash and it's order into buffers table */
                    sql_command = g_strdup_printf("INSERT INTO buffers (file_id, buf_order, checksum) VALUES (%ld, %ld, '%s');", file_id, i, encoded_hash);

                    exec_sql_cmd(database, sql_command,  N_("Error while inserting into the table 'buffers': %s\n"));

                    free_variable(sql_command);

                    a_data = g_tree_lookup(hashs->tree_hash, a_hash);


                    /* Is encoded hash already in the database ? */
                    if (is_checksum_in_db(database, inserted_hashs, a_hash) == FALSE) /* encoded_hash is not in the database */
                        {
                            /* Inserting checksum and the corresponding data into data table */

                            encoded_data = g_base64_encode((guchar*) a_data->buffer, a_data->read);

                            sql_command = g_strdup_printf("INSERT INTO data (checksum, size, data) VALUES ('%s', %ld, '%s');", encoded_hash, a_data->read, encoded_data);

                            exec_sql_cmd(database, sql_command,  N_("Error while inserting into the table 'data': %s\n"));

                            free_variable(sql_command);
                            free_variable(encoded_data);
                        }

                    /* The hash is already in the database or has just been inserted.
                     * We can safely free the data from the tree_hash structure but we
                     * need to keep the key into the tree (the hash) and can keep the
                     * old size of the data
                     */
                    free_variable(a_data->buffer);

                    free_variable(encoded_hash);

                    head = g_slist_next(head);
                    i = i + 1;
                }
        }
}


/**
 * Insert file into cache. One should have verified that the file
 * does not already exists in the database.
 * @param database is the structure that contains everything that is
 *        related to the database (it's connexion for instance).
 * @param meta is the file's metadata that we want to insert into the
 *        cache.
 * @param hashs : a balanced binary tree that stores hashs.
 */
void insert_file_into_cache(db_t *database, meta_data_t *meta, hashs_t *hashs, hashs_t *inserted_hashs)
{
    gchar *sql_command = NULL;     /**< Command to be executed          */
    file_row_t *row = NULL;
    guint64 file_id = 0;

    if (meta != NULL && database != NULL)
        {
            /* Inserting the file into the files table */
            sql_command = g_strdup_printf("INSERT INTO files (type, file_user, file_group, uid, gid, atime, ctime, mtime, mode, name) VALUES (%d, '%s', '%s', %d, %d, %ld, %ld, %ld, %d, '%s');", meta->file_type, meta->owner, meta->group, meta->uid, meta->gid, meta->atime, meta->ctime, meta->mtime, meta->mode, meta->name);

            exec_sql_cmd(database, sql_command,  N_("Error while inserting into the table 'files': %s\n"));

            free_variable(sql_command);

            /* getting its autoincrement file_id */
            row = get_file_id(database, meta); /* Seeking for the file_id which has been associated to this file */

            if (row != NULL)
                {
                    print_debug(stdout, _("file_id = %s ; list length = %d\n"), row->file_id_list->data, g_slist_length(row->file_id_list));
                    file_id = g_ascii_strtoull(row->file_id_list->data, NULL, 10);
                    insert_file_checksums(database, meta, hashs, file_id, inserted_hashs);
                }
            else  /* if we have no answer that means that something went wrong */
                {
                    print_db_error(database->db, _("Error while trying to get file_id of %s file\n"), meta->name);
                    exit(EXIT_FAILURE);
                }

        }
}


/**
 * Returns a database connexion or NULL.
 * @param database_name is the filename of the file that contains the
 *        database
 * @result returns a db_t * filled with the database connexion or NULL
 *         in case of an error.
 */
db_t *open_database(gchar *database_name)
{
    db_t *database = NULL;
    sqlite3 *db = NULL;
    int result = 0;

    database = (db_t *) g_malloc0(sizeof(db_t));

    result = sqlite3_open(database_name, &db);

    if (result != SQLITE_OK)
        {
            print_db_error(db, _("Error while trying to open %s database: %s\n"), database_name, sqlite3_errmsg(db));
            return NULL;
        }
    else
        {
            database->db = db;
            verify_if_tables_exists(database);
            return database;
        }
}




