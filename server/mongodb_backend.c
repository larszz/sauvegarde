//
// Created by work on 14.04.21.
//

#include "server.h"


/**
 * Prints the format and further parameters to "stdout" with the MongoDB log prefix
 * @param format
 * @param ...
 */
static void mongodb_print_info(const char *format, ...)
{
    va_list ap;

    fprintf(stdout, MONGODB_LOG_PREFIX);
    va_start(ap, format);
    vfprintf(stdout, format, ap);
    va_end(ap);
}


/**
 * Prints the format and further parameters to "stdout" with the MongoDB log prefix, if debug mode is enabled
 */
static void mongodb_print_debug(const char *format, ...)
{
    if (MONGODB_BACKEND_DEBUG)
    {
        va_list ap;

        fprintf(stdout, MONGODB_LOG_PREFIX);
        va_start(ap, format);
        vfprintf(stdout, format, ap);
        va_end(ap);
    }
}


/**
 * Prints the format and further parameters to "stdout" with the MongoDB log prefix, if verbose mode is enabled
 */
static void mongodb_print_verbose(const char *format, ...)
{
    if (MONGODB_BACKEND_VERBOSE_FOR_TESTS)
    {
        va_list ap;

        fprintf(stdout, MONGODB_LOG_PREFIX);
        va_start(ap, format);
        vfprintf(stdout, format, ap);
        va_end(ap);
    }
}


/**
 * Prints the format and further parameters to "stderr" with the MongoDB log prefix
 */
static void mongodb_print_error(const char *format, ...)
{
    va_list ap;

    fprintf(stderr, MONGODB_LOG_PREFIX);
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
}


/** Prefix for logging in initialization methods */
#define LOGGING_METHOD_PREFIX_MONGODB_CONFIG ("Config")

/**
 * reads the MongoDB backend settings from key file
 * @param backend
 * @param filepath
 * @todo implement: actually use configuration settings instead of defined default values
 */
static bool read_from_group_mongodb_backend(mongodb_backend_t *backend, char *filepath)
{
    // GFile
    GKeyFile *keyfile = NULL;
    GError *error = NULL;

    // Read data
    char *host = NULL;
    int port = -1;
    char *database = NULL;
    char *user = NULL;
    char *seckey = NULL;

    char *uri = NULL;


    // Stop if no backend passed
    if (backend == NULL)
    {
        mongodb_print_error("[%s] Backend not initializd!\n", LOGGING_METHOD_PREFIX_MONGODB_CONFIG);
        return false;
    }

    keyfile = g_key_file_new();

    // Read from the keyfile
    if (g_key_file_load_from_file(keyfile, filepath, G_KEY_FILE_KEEP_COMMENTS, &error)
        && g_key_file_has_group(keyfile, GN_MONGODB_BACKEND) == TRUE)
    {
        host = read_string_from_file(keyfile, filepath, GN_MONGODB_BACKEND, KN_MONGODB_HOST,
                                     "Host not found in config!");
        port = read_int_from_file(keyfile, filepath, GN_MONGODB_BACKEND, KN_MONGODB_PORT, "Port not found in config!",
                                  -1);
        database = read_string_from_file(keyfile, filepath, GN_MONGODB_BACKEND, KN_MONGODB_DATABASE,
                                         "Database not found in config!");
        user = read_string_from_file(keyfile, filepath, GN_MONGODB_BACKEND, KN_MONGODB_USER,
                                     "User not found in config!");
        seckey = read_string_from_file(keyfile, filepath, GN_MONGODB_BACKEND, KN_MONGODB_KEY,
                                       "Key not found in config!");


    } else if (error != NULL)
    {
        mongodb_print_error("[%s] Failed to open '%s' configuration file: \n%s\n", LOGGING_METHOD_PREFIX_MONGODB_CONFIG,
                            filepath, error->message);
        free_error(error);
    }
    g_key_file_free(keyfile);


    // If following parameters not set, no default values are used and connection can not be initialized
    // User
    if (!user)
    {
        mongodb_print_error("[%s] Key [%s] not found in config!\n", LOGGING_METHOD_PREFIX_MONGODB_CONFIG,
                            KN_MONGODB_USER);
        return false;
    }

    // Secret key
    if (!seckey)
    {
        mongodb_print_error("[%s] Key [%s] not found in config!\n", LOGGING_METHOD_PREFIX_MONGODB_CONFIG,
                            KN_MONGODB_KEY);
        return false;
    }

    // Database
    if (!database)
    {
        mongodb_print_error("[%s] Key [%s] not found in config!\n", LOGGING_METHOD_PREFIX_MONGODB_CONFIG,
                            KN_MONGODB_DATABASE);
        return false;
    }

    // Following parameters are filled up with their default values if not specified
    // Host
    if (!host)
    {
        mongodb_print_debug("[%s] Set key value [%-8s] with default:\t'%s'\n", LOGGING_METHOD_PREFIX_MONGODB_CONFIG,
                            KN_MONGODB_HOST, MONGODB_DEFAULT_HOST);
        host = MONGODB_DEFAULT_HOST;
    }

    // Port
    if (port < 0)
    {
        mongodb_print_debug("[%s] Set key value [%-8s] with default:\t'%d'\n", LOGGING_METHOD_PREFIX_MONGODB_CONFIG,
                            KN_MONGODB_PORT, MONGODB_DEFAULT_PORT);
        port = MONGODB_DEFAULT_PORT;
    }

    // generate URI
    uri = g_strdup_printf("%s%s:%s@%s:%d", MONGODB_URI_PREFIX, user, seckey, host, port);


    // Add the values to the backend
    backend->connection_string = uri;
    backend->dbname = database;

    return true;
}

/**
 * Initializes the mongodb backend, including the mongoc-structure till client level
 * (so the collection to use can be different for each sent meta data as well as the cursor etc.)
 * @param server_struct is the main server structure the mongodb backend is stored into
 * @todo: use server_struct->backend_meta everywhere!
 * @todo: check if connection can be established (i.e. by "mongoc_client_get_database"?)
 */
void mongodb_init_backend(server_struct_t *server_struct)
{
    mongodb_print_info("Init MongoDB backend...\n");
    mongodb_backend_t *mongodb_backend;
    mongodb_backend = (mongodb_backend_t *) g_malloc0(sizeof(mongodb_backend_t));
    if (server_struct != NULL && server_struct->backend_data != NULL && server_struct->opt != NULL) // TODO: backend_meta!
    {
        if (read_from_group_mongodb_backend(mongodb_backend, server_struct->opt->configfile))
        {

            server_struct->backend_meta->user_data = mongodb_backend;

            mongoc_init();
            mongodb_backend->client = mongoc_client_new(mongodb_backend->connection_string);
            mongoc_client_set_appname(mongodb_backend->client, MONGODB_CLIENT_APPNAME);

            mongodb_print_info("Backend initialized.\n");
        } else
        {
            if (mongodb_backend != NULL)
                g_free(mongodb_backend);
            mongodb_print_error("An error occured while initialization, values could not be read from KeyFile!\n");
            return;
        }
    } else
    {
        if (mongodb_backend != NULL)
            g_free(mongodb_backend);

        mongodb_print_error("Server structure incomplete!\n");
        mongodb_print_error("Backend NOT initialized!\n");
    }
}


/**
 * Terminates the MongoDB backend (including destroying of mongoc client and mongoc_cleanup())
 */
void mongodb_terminate_backend(backend_t *backend)
{
    mongodb_print_verbose("Terminate MongoDB backend...\n");
    mongodb_backend_t *mongodb_backend;


    // Destroy opened mongoc client
    if (backend->user_data != NULL)
    {
        mongodb_backend = (mongodb_backend_t *) backend->user_data;
        if (mongodb_backend != NULL && mongodb_backend->client != NULL)
        {
            // Destroy client
            mongodb_print_verbose("Destroy client\n");
            mongoc_client_destroy(mongodb_backend->client);

            // Clean up mongoc structure
            mongodb_print_verbose("Cleanup MongoC\n");
            mongoc_cleanup();
        }
    }
    mongodb_print_info("MongoDB backend terminated.\n");
}


/**
 * Constructs the collection name, based on the passed host name
 * @param hostname
 * @return the (allocated) collection name for the host
 */
gchar *get_collection_name(char *hostname)
{
    gchar *collection_name = NULL;
    if (hostname != NULL)
    {
        collection_name = g_strdup_printf("%s_meta", hostname);
    }
    return collection_name;
}


/**
 * adds values of a list to the passed bson_t document under the passed key
 * @param document
 * @param bson_key
 * @param list
 * @return
 */
bool add_list_values_to_bson(bson_t *document, char *bson_key, GList *list)
{
    bson_t array;
    size_t keylen;
    GList *head;
    const char *key;
    char buf[16];
    hash_data_t *hash_data = NULL;

    if (document != NULL && bson_key != NULL & list != NULL)
    {
        head = list;
        BSON_APPEND_ARRAY_BEGIN(document, bson_key, &array);

        for (int i = 0; i < g_list_length(head); ++i)
        {
            // should never happen but..
            if (head == NULL)
                break;

            hash_data = head->data;
            guint8 *hash_insert = hash_data->hash;

            if (MONGODB_BACKEND_CONVERT_HASH_BASE64)
            {
                mongodb_print_verbose("Convert List value to Base64: %s\n", hash_insert);
                hash_insert = (unsigned char *) g_base64_encode(hash_data->hash, HASH_LEN);
            }

            mongodb_print_verbose("Writing list value: %s\n", hash_insert);

            keylen = bson_uint32_to_string(i, &key, buf, sizeof buf);
            bson_append_utf8(&array, key, (int) keylen, (const char *) hash_insert, -1);

            head = g_list_next(head);
        }

        bson_append_array_end(document, &array);

        return true;
    }
    return false;
}


/**
 * Creates a storeable bson_t document
 * @param server_struct
 * @param smeta
 * @return
 */
void init_meta_bson(bson_t *document, server_meta_data_t *smeta)
{
    bson_oid_t oid;
    meta_data_t *meta;

    if (document != NULL && smeta != NULL && smeta->meta != NULL)
    {
        // store meta_data in own simpler variable

        meta = smeta->meta;

        // add OID
        bson_oid_init(&oid, NULL);
        BSON_APPEND_OID(document, "_id", &oid);

        // add metadata
        if (meta->inode != INT_UNDEFINED)
            BSON_APPEND_INT64(document, LABEL_INODE, meta->inode);

        if (meta->file_type != INT_UNDEFINED)
            BSON_APPEND_INT32(document, LABEL_FILETYPE, (signed int) meta->file_type);

        if (meta->mode != INT_UNDEFINED)
            BSON_APPEND_INT32(document, LABEL_MODE, meta->mode);

        if (meta->atime != INT_UNDEFINED)
            BSON_APPEND_INT64(document, LABEL_ATIME, meta->atime);

        if (meta->ctime != INT_UNDEFINED)
            BSON_APPEND_INT64(document, LABEL_CTIME, meta->ctime);

        if (meta->mtime != INT_UNDEFINED)
            BSON_APPEND_INT64(document, LABEL_MTIME, meta->mtime);

        if (meta->size != INT_UNDEFINED)
            BSON_APPEND_INT64(document, LABEL_SIZE, meta->size);

        if (meta->owner != NULL)
            BSON_APPEND_UTF8(document, LABEL_OWNER, meta->owner);

        if (meta->group != NULL)
            BSON_APPEND_UTF8(document, LABEL_GROUP, meta->group);

        if (meta->uid != INT_UNDEFINED)
            BSON_APPEND_INT32(document, LABEL_UID, meta->uid);

        if (meta->gid != INT_UNDEFINED)
            BSON_APPEND_INT32(document, LABEL_GID, meta->gid);

        if (meta->name != NULL)
            BSON_APPEND_UTF8(document, LABEL_NAME, meta->name);

        if (meta->link != NULL)
            BSON_APPEND_UTF8(document, LABEL_LINK, meta->link);

        // if hash values existing, add them to BSON document
        if (meta->hash_data_list != NULL)
        {
            if (!add_list_values_to_bson(document, LABEL_HASHLIST, meta->hash_data_list))
            {
                mongodb_print_error("Could not write hash data list! Check data!");
            }
        }
    }
}


/** Prefix for logging in mongodb_store_smeta-Method */
#define LOGGING_METHOD_PREFIX_MONGODB_STORE_SMETA ("StoreSMeta")

/**
 * Inserts the passed data to the MongoDB server if found with passed credentials
 * @param server_struct is the main server structure
 * @param server_meta is the passed meta data to store into database
 */
void mongodb_store_smeta(server_struct_t *server_struct, server_meta_data_t *server_meta)
{
    bson_t *metadata;
    mongoc_collection_t *collection;
    bson_error_t error;
    bson_t *reply = g_malloc0(sizeof(bson_t));
    bson_oid_t oid;
    mongodb_backend_t *backend;
    char *collection_name;


    mongodb_print_debug("[%s] Insert data...\n", LOGGING_METHOD_PREFIX_MONGODB_STORE_SMETA);
    if (server_meta != NULL && server_meta->hostname != NULL && server_meta->meta != NULL)
    {
        collection_name = get_collection_name(server_meta->hostname);
        if (collection_name != NULL && server_struct != NULL && server_struct->backend_data != NULL &&
            server_struct->backend_meta->user_data != NULL)
        {
            backend = (mongodb_backend_t *) server_struct->backend_meta->user_data;
            if (backend->client != NULL && backend->dbname != NULL)
            {
                collection = mongoc_client_get_collection(backend->client, backend->dbname, collection_name);

                mongodb_print_verbose("[%s] Using collection:\t%s\n", collection_name,
                                      LOGGING_METHOD_PREFIX_MONGODB_STORE_SMETA);

                // Init meta
                metadata = bson_new();
                init_meta_bson(metadata, server_meta);

                if (!mongoc_collection_insert_one(collection, metadata, NULL, reply, &error))
                {
                    mongodb_print_error("[%s] Error at mongodb_store_smeta: %s\n",
                                        LOGGING_METHOD_PREFIX_MONGODB_STORE_SMETA,
                                        error.message);
                    if (reply != NULL)
                        mongodb_print_error("[%s] Reply: %s\n",
                                            LOGGING_METHOD_PREFIX_MONGODB_STORE_SMETA,
                                            bson_as_canonical_extended_json(reply, 0));
//                ((mongodb_backend_t *) server_struct->backend->user_data_meta)->corrupted = TRUE;
                }

                bson_destroy(metadata);
                bson_destroy(reply);
                mongoc_collection_destroy(collection);
            } else
            {
                mongodb_print_error("[%s] Backend corrupted! [%s,%s]\n",
                                    LOGGING_METHOD_PREFIX_MONGODB_STORE_SMETA,
                                    (backend->client ? "Client OK" : "Client empty"),
                                    (backend->dbname ? "DB OK" : "DB empty"));
            }
        }
        free(collection_name);
    } else
    {
        mongodb_print_error("[%s] Error: Server_meta or its hostname/meta NULL!\n",
                            LOGGING_METHOD_PREFIX_MONGODB_STORE_SMETA);
    }

    mongodb_print_debug("[%s] Insert done.\n\n", LOGGING_METHOD_PREFIX_MONGODB_STORE_SMETA);
}


// -------------------------------------------------------------------
// -------------------------------------------------------------------
// -------------------------------------------------------------------
// READ DATA

// TODO TEST: should beforedate_unix in following method be int64?  Would that work or create other issues?
/**
 * Initializes a passed bson_t with search properties of query
 * @param out
 * @param query
 */
static void init_search_property_bson(bson_t *out, query_t *query)
{
    if (query != NULL)
    {
        mongodb_print_verbose("Init search properties:\n");

        // filename
        if (query->filename != NULL)
        {
            BSON_APPEND_REGEX(out, LABEL_NAME, query->filename, "i");
            mongodb_print_verbose("- FILENAME: %s\n", query->filename);
        }

        // beforedate
        if (query->beforedate != INT_UNDEFINED)
        {
            GDateTime *beforedate_dt;
            gint32 beforedate_unix;
            bson_t *beforedate_bson;

            // Get unix time from passed query-beforedate
            beforedate_dt = convert_gchar_date_to_gdatetime(query->beforedate);
            beforedate_unix = g_date_time_to_unix(beforedate_dt);
            g_date_time_unref(beforedate_dt);

            beforedate_bson = g_malloc0(sizeof(bson_t));

            BSON_APPEND_DOCUMENT_BEGIN(out, LABEL_MTIME, beforedate_bson);
            BSON_APPEND_INT32(beforedate_bson, MONGOC_OWN_LESS_THAN_EQUAL, beforedate_unix);
            bson_append_document_end(out, beforedate_bson);

            mongodb_print_verbose("- BEFOREDATE: %s (UNIX: %d)\n", query->beforedate, beforedate_unix);
        }

        // afterdate
        if (query->afterdate != INT_UNDEFINED)
        {
            GDateTime *afterdate_dt;
            gint32 afterdate_unix;
            bson_t *afterdate_bson;

            // Get unix time from passed query-afterdate
            afterdate_dt = convert_gchar_date_to_gdatetime(query->afterdate);
            afterdate_unix = g_date_time_to_unix(afterdate_dt);
            g_date_time_unref(afterdate_dt);

            afterdate_bson = g_malloc0(sizeof(bson_t));

            BSON_APPEND_DOCUMENT_BEGIN(out, LABEL_MTIME, afterdate_bson);
            BSON_APPEND_INT32(afterdate_bson, MONGOC_OWN_GREATER_THAN_EQUAL, afterdate_unix);
            bson_append_document_end(out, afterdate_bson);

            mongodb_print_verbose("- AFTERDATE: %s (UNIX: %d)\n", query->afterdate, afterdate_unix);
        }

        // owner
        if (query->owner != NULL)
        {
            BSON_APPEND_REGEX(out, LABEL_OWNER, query->owner, "i");
            mongodb_print_verbose("- OWNER: %s\n", query->owner);
        }

        // group
        if (query->group != NULL)
        {
            BSON_APPEND_REGEX(out, LABEL_GROUP, query->group, "i");
            mongodb_print_verbose("- GROUP: %s\n", query->group);
        }
    }
}


/**
 * Initializes an allocated meta_data_t with data from bson document
 * @param meta_data
 * @param document
 */
static void init_meta_data_from_bson(meta_data_t *meta_data, const bson_t *document)
{
    bson_iter_t iter; // = malloc(sizeof(bson_iter_t));
    bson_iter_t child; // = malloc(sizeof(bson_iter_t));
    char *hash;
    gsize *len = NULL;
    hash_data_t *hash_data;


    if (meta_data != NULL && document != NULL)
    {

        // inode
        if (bson_iter_init_find(&iter, document, LABEL_INODE) &&
            BSON_ITER_HOLDS_INT64(&iter))
            meta_data->inode = bson_iter_int64(&iter);

        // filetype
        if (bson_iter_init_find(&iter, document, LABEL_FILETYPE) &&
            BSON_ITER_HOLDS_INT32(&iter))
            meta_data->file_type = bson_iter_int32(&iter);

        // mode
        if (bson_iter_init_find(&iter, document, LABEL_MODE) &&
            BSON_ITER_HOLDS_INT32(&iter))
            meta_data->mode = bson_iter_int32(&iter);

        // atime
        if (bson_iter_init_find(&iter, document, LABEL_ATIME) &&
            BSON_ITER_HOLDS_INT64(&iter))
            meta_data->atime = bson_iter_int64(&iter);

        // ctime
        if (bson_iter_init_find(&iter, document, LABEL_CTIME) &&
            BSON_ITER_HOLDS_INT64(&iter))
            meta_data->ctime = bson_iter_int64(&iter);

        // mtime
        if (bson_iter_init_find(&iter, document, LABEL_MTIME) &&
            BSON_ITER_HOLDS_INT64(&iter))
            meta_data->mtime = bson_iter_int64(&iter);

        // size
        if (bson_iter_init_find(&iter, document, LABEL_SIZE) &&
            BSON_ITER_HOLDS_INT64(&iter))
            meta_data->size = bson_iter_int64(&iter);

        // owner
        if (bson_iter_init_find(&iter, document, LABEL_OWNER) &&
            BSON_ITER_HOLDS_UTF8(&iter))
            meta_data->owner = bson_iter_dup_utf8(&iter, 0);

        // group
        if (bson_iter_init_find(&iter, document, LABEL_GROUP) &&
            BSON_ITER_HOLDS_UTF8(&iter))
            meta_data->group = bson_iter_dup_utf8(&iter, 0);

        // uid
        if (bson_iter_init_find(&iter, document, LABEL_UID) &&
            BSON_ITER_HOLDS_INT32(&iter))
            meta_data->uid = bson_iter_int32(&iter);

        // gid
        if (bson_iter_init_find(&iter, document, LABEL_GID) &&
            BSON_ITER_HOLDS_INT32(&iter))
            meta_data->gid = bson_iter_int32(&iter);

        // name
        if (bson_iter_init_find(&iter, document, LABEL_NAME) &&
            BSON_ITER_HOLDS_UTF8(&iter))
            meta_data->name = bson_iter_dup_utf8(&iter, 0);

        // link
        if (bson_iter_init_find(&iter, document, LABEL_LINK) &&
            BSON_ITER_HOLDS_UTF8(&iter))
            meta_data->link = bson_iter_dup_utf8(&iter, 0);


        // hashlist array
        if (bson_iter_init_find(&iter, document, LABEL_HASHLIST) &&
            BSON_ITER_HOLDS_ARRAY(&iter) &&
            bson_iter_recurse(&iter, &child))
        {
            // iterate over hashlist elements
            while (bson_iter_next(&child))
            {
                if (BSON_ITER_HOLDS_UTF8(&child))
                {
                    hash = bson_iter_dup_utf8(&child, 0);

                    // decode hash from base64 if base64-encoding is set
                    if (MONGODB_BACKEND_CONVERT_HASH_BASE64)
                    {
                        len = g_malloc0(sizeof(len));
                        g_base64_decode_inplace(hash, len);
                        g_free(len);
                    }

                    hash_data = new_hash_data_t_as_is(NULL, 0, (unsigned char *) hash, COMPRESS_NONE_TYPE, 0);
//                    hash_data = new_hash_data_t(NULL, 0, (unsigned char *) hash, meta_data->);
                    meta_data->hash_data_list = g_list_prepend(meta_data->hash_data_list, hash_data);
                }
            }

            meta_data->hash_data_list = g_list_reverse(meta_data->hash_data_list);
        }
    }
}

/**
 * Initializes and returns a new allocated meta_data_t
 * @param md
 */
static meta_data_t *init_meta_data()
{
    meta_data_t *meta_data = (meta_data_t *) g_malloc0(sizeof(meta_data_t));
    meta_data->hash_data_list = NULL;
    return meta_data;
}


/** Prefix for logging in mongodb_get_list_of_files-Method */
#define LOGGING_METHOD_PREFIX_MONGODB_GET_LIST_OF_FILES ("GetListOfFiles")

/**
 * Searches for files with properties, specified in query_t
 * @param server_struct contains all needed information about the main server structure
 * @param query the query specification, the data is searched with
 * @return The found hash_data_t, encoded in a JSON string
 */
gchar *mongodb_get_list_of_files(server_struct_t *server_struct, query_t *query)
{
    mongodb_backend_t *backend;
    char *collection_name;
    mongoc_collection_t *collection;
    mongoc_cursor_t *cursor;
    const bson_t *doc;

    bson_t *search_properties;
    meta_data_t *meta_data = NULL;
    GList *file_list = NULL;
    json_t *root = NULL;
    json_t *file_list_json = NULL;
    gchar *json_string = NULL;


    // Generate name of collection to use
    if (query != NULL)
    {
        // get collection name by client hostname
        if (query->hostname != NULL)
        {
            collection_name = get_collection_name(query->hostname);

            if (collection_name != NULL && server_struct != NULL && server_struct->backend_data != NULL &&
                server_struct->backend_meta->user_data != NULL)
            {
                backend = (mongodb_backend_t *) server_struct->backend_meta->user_data;

                if (backend->client != NULL && backend->dbname != NULL)
                {
                    collection = mongoc_client_get_collection(backend->client, backend->dbname, collection_name);
                    mongodb_print_verbose("Using collection:\t%s\n", collection_name);

                    // Init search_properties
                    search_properties = bson_new();
                    init_search_property_bson(search_properties, query);

                    cursor = mongoc_collection_find_with_opts(collection, search_properties, NULL, NULL);

                    // look at for more information: http://mongoc.org/libbson/current/parsing.html#recursing-into-sub-documents
                    // Iterate over found documents and add the meta_data_t to list
                    mongodb_print_verbose("\nFound Documents for search_properties:\n");
                    while (mongoc_cursor_next(cursor, &doc))
                    {
                        mongodb_print_verbose("%s\n", bson_as_canonical_extended_json(doc, NULL));

                        // Get meta data and add it to file list
                        meta_data = init_meta_data();
                        init_meta_data_from_bson(meta_data, doc);

                        file_list = g_list_prepend(file_list, meta_data);
                    }


                    // Sort found documents by mdate
                    file_list = g_list_sort(file_list, compare_meta_data_t);

                    /* Only keep latest if specified */
                    if (query->latest == TRUE)
                    {
                        file_list = keep_latests_meta_data_t_in_list(file_list);
                    }

                    /* Converting list into JSON array */
                    file_list_json = convert_meta_data_list_to_json_array(file_list, query->hostname, FALSE);

                    /* Freeing memory */
                    g_list_free_full(file_list, free_glist_meta_data_t);


                    root = json_object();
                    insert_json_value_into_json_root(root, "file_list", file_list_json);
                    json_string = json_dumps(root, 0);


                    // Clean up
                    json_decref(file_list_json);
                    json_decref(root);

                    bson_destroy(search_properties);
                    mongoc_cursor_destroy(cursor);
                    mongoc_collection_destroy(collection);
                } else
                {
                    mongodb_print_error("[%s] Backend corrupted! [%s,%s]\n",
                                        LOGGING_METHOD_PREFIX_MONGODB_GET_LIST_OF_FILES,
                                        (backend->client ? "Client OK" : "Client empty"),
                                        (backend->dbname ? "DB OK" : "DB empty"));
                }
            } else
            {
                mongodb_print_error("[%s] Server structure incomplete!\n",
                                    LOGGING_METHOD_PREFIX_MONGODB_GET_LIST_OF_FILES);
            }
        } else
        {
            mongodb_print_error("[%s] Hostname empty in query!\n", LOGGING_METHOD_PREFIX_MONGODB_GET_LIST_OF_FILES);
        }
    } else
    {
        mongodb_print_error("[%s] No query passed!\n", LOGGING_METHOD_PREFIX_MONGODB_GET_LIST_OF_FILES);
    }

    return json_string;
}

