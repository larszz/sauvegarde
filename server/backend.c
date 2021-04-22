/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 *    backend.c
 *    This file is part of "Sauvegarde" project.
 *
 *    (C) Copyright 2015 - 2017 Olivier Delhomme
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
 * @file server/backend.c
 *
 * This file contains all the functions for the backend management of
 * cdpfglserver's server.
 */

#include "server.h"


/**
 * Inits the backend with the correct functions
 * @todo write some backends !
 * @param store_smeta a function to store server_meta_data_t structure
 * @param store_data a function to store data
 * @param init_backend a function to init the backend
 * @param build_needed_hash_list a function that must build a GSList * needed hash list
 * @param get_list_of_files gets the list of saved files
 * @param retrieve_data retrieves data from a specified hash.
 * @returns a newly created backend_t structure initialized to nothing !
 */
backend_t *init_backend_structure(void *store_smeta, void *store_data, void *init_backend, void *terminate_backend,
                                  void *build_needed_hash_list, void *get_list_of_files, void *retrieve_data)
{
    backend_t *backend = NULL;

    backend = (backend_t *) g_malloc0(sizeof(backend_t));

    backend->user_data = NULL;
    backend->store_smeta = store_smeta;
    backend->store_data = store_data;
    backend->init_backend = init_backend;
    backend->terminate_backend = terminate_backend;
    backend->build_needed_hash_list = build_needed_hash_list;
    backend->get_list_of_files = get_list_of_files;
    backend->retrieve_data = retrieve_data;

    return backend;
}


/**
 * Checks if the passed backends enable all necessary methods
 * @param backend
 * @return
 */
gboolean check_backends_valid(backend_t *backend_meta, backend_t *backend_data)
{
    gboolean valid = TRUE;

    // not valid if no backend passed
    if (backend_meta == NULL && backend_data == NULL)
    {
        print_error(__FILE__, __LINE__, "No backend passed!\n");
        return FALSE;
    }

    // store_data
    if (!((backend_meta != NULL && backend_meta->store_data != NULL) ||
          (backend_data != NULL && backend_data->store_data != NULL)))
    {
        g_print("Missing method in backends: %s\n", "store_data");
        valid = FALSE;
    }

    // store_smeta
    if (!((backend_meta != NULL && backend_meta->store_smeta != NULL) ||
          (backend_data != NULL && backend_data->store_smeta != NULL)))
    {
        g_print("Missing method in backends: %s\n", "store_smeta");
        valid = FALSE;
    }

    // build_needed_hash_list
    if (!((backend_meta != NULL && backend_meta->build_needed_hash_list != NULL) ||
          (backend_data != NULL && backend_data->build_needed_hash_list != NULL)))
    {
        g_print("Missing method in backends: %s\n", "build_needed_hash_list");
        valid = FALSE;
    }

    // retrieve_data
    if (!((backend_meta != NULL && backend_meta->retrieve_data != NULL) ||
          (backend_data != NULL && backend_data->retrieve_data != NULL)))
    {
        g_print("Missing method in backends: %s\n", "retrieve_data");
        valid = FALSE;
    }

    // get_list_of_files
    if (!((backend_meta != NULL && backend_meta->get_list_of_files != NULL) ||
          (backend_data != NULL && backend_data->get_list_of_files != NULL)))
    {
        g_print("Missing method in backends: %s\n", "get_list_of_files");
        valid = FALSE;
    }


    return valid;
}


/**
 * Returns the number of the backend, corresponding to the label
 * @param label
 * @return
 */
gint get_backend_number_from_label(char *label)
{
    if (g_strcmp0(label, BACKEND_FILE_LABEL) == 0)
    {
        // default file backend
        return BACKEND_FILE_NUM;
    } else if (g_strcmp0(label, BACKEND_MONGODB_LABEL) == 0)
    {
        // MongoDB backend
        return BACKEND_MONGODB_NUM;
    } else if (g_strcmp0(label, BACKEND_MINIO_LABEL) == 0)
    {
        return BACKEND_MINIO_NUM;
    } else
    {
        // invalid
        return BACKEND_INVALID_NUM;
    }
}


/**
 * Frees possible data stored in backend and backend itself
 * @param backend
 */
void free_backend(backend_t *backend)
{
    if (backend != NULL)
    {
//        g_free(backend->user_data);
        g_free(backend);
    }
}