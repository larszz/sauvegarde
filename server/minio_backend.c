//
// Created by work on 07.02.21.
//

// #include "s3_copy.c"


#include "server.h"

#define HOSTNAME_MAXLENGTH 256
#define KEY_MAXLENGTH 256

/**
 * MinIO default settings
 */
#define MINIO_DEFAULT_HOSTNAME "localhost:9000"
#define MINIO_BUCKET_DEFAULT_DATA "testbucket"
#define MINIO_BUCKET_DEFAULT_FILEMETA "testbucket-meta"

#define MINIO_FALLBACK_BUCKET "tmp-fallback"


// Read from config if available, only use as default if not configurated
#define CREATE_BUCKET_IF_MISSING 1     /** defines, if new bucket is created if it's missing */ \


// Logging helpers

/**
 * Prints the format and further parameters to "stdout" with the MinIO log prefix
 * @param format
 * @param ...
 */
static void minio_print_info(const char *format, ...)
{
    va_list ap;

    fprintf(stdout, "[%s] ", LOGGING_PREFIX_MINIO);
    va_start(ap, format);
    vfprintf(stdout, format, ap);
    va_end(ap);
}


/**
 * Prints the format and further parameters to "stdout" with the MinIO log prefix, if debug mode is enabled
 */
static void minio_print_debug(const char *format, ...)
{
    if (MINIO_BACKEND_DEBUG)
    {
        va_list ap;

        fprintf(stdout, "[%s] ", LOGGING_PREFIX_MINIO);
        va_start(ap, format);
        vfprintf(stdout, format, ap);
        va_end(ap);
    }
}


/**
 * Prints the format and further parameters to "stdout" with the MinIO log prefix, if verbose mode is enabled
 */
static void minio_print_verbose(const char *format, ...)
{
    if (MINIO_BACKEND_TRACE)
    {
        va_list ap;

        fprintf(stdout, "[%s-Verbose] ", LOGGING_PREFIX_MINIO);
        va_start(ap, format);
        vfprintf(stdout, format, ap);
        va_end(ap);
    }
}


/**
 * Prints the format and further parameters to "stderr" with the MinIO log prefix
 */
static void minio_print_error(const char *format, ...)
{
    va_list ap;

    fprintf(stderr, "[%s] ", LOGGING_PREFIX_MINIO);
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
}

/**
 * Prints the format and further parameters "stderr" with MinIO and Critical error log prefix
 */
static void minio_print_critical(const char *format, ...)
{
    va_list ap;
    fprintf(stderr, "[%s] [!!%s!!] ", LOGGING_PREFIX_MINIO, LOGGING_PREFIX_TAG_CRITICAL);
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
}





// Actual init
/** Prefix for logging in configuration methods */
#define LOGGING_METHOD_PREFIX_MINIO_CONFIG ("Config")

/**
 * Reads server connection params from the passed filepath and returns them into the passed pointers
 * (out-pointers are passed as NULL -> HAVE to be allocated first!)
 * @param out_hostname
 * @param out_access_key
 * @param out_secret_key
 * @param filepath
 * @return success of execution
 */
bool get_params_from_keyfile(minio_backend_t *backend, char **secret_key, char *filepath)
{
    // Keyfile
    GKeyFile *keyfile = NULL;
    GError *error = NULL;

    // Values
    char *hostname = NULL;
    char *access_key = NULL;
    char *bucketname_data = NULL;
    char *bucketname_filemeta = NULL;
    gboolean add_missing_bucket = 0;

    if (backend == NULL)
    {
        minio_print_error("[%s] Backend not initialized!\n", LOGGING_METHOD_PREFIX_MINIO_CONFIG);
        return false;
    }

    keyfile = g_key_file_new();

    // 1. Get data from config
    if (g_key_file_load_from_file(keyfile, filepath, G_KEY_FILE_KEEP_COMMENTS, &error) &&
        g_key_file_has_group(keyfile, GN_MINIO_BACKEND) == TRUE)
    {
        hostname = read_string_from_file(keyfile, filepath, GN_MINIO_BACKEND, KN_MINIO_HOSTNAME,
                                         "Hostname not found in config!");
        access_key = read_string_from_file(keyfile, filepath, GN_MINIO_BACKEND, KN_MINIO_ACCESSKEY,
                                           "Access key not found in config!");
        *secret_key = read_string_from_file(keyfile, filepath, GN_MINIO_BACKEND, KN_MINIO_SECRETKEY,
                                            "Secret key not found in config!");
        bucketname_data = read_string_from_file(keyfile, filepath, GN_MINIO_BACKEND, KN_MINIO_BUCKET_DATA,
                                                "Data bucket not found in config!");
        bucketname_filemeta = read_string_from_file(keyfile, filepath, GN_MINIO_BACKEND, KN_MINIO_BUCKET_FILEMETA,
                                                    "Filemeta bucket not found in config!");
        add_missing_bucket = read_boolean_from_file(keyfile, filepath, GN_MINIO_BACKEND, KN_MINIO_ADD_MISSING_BUCKET,
                                                    "'Add missing bucket' not found in config!");

    } else if (error != NULL)
    {
        minio_print_error("[%s] Failed to open '%s' configuration file: \n%s\n",
                          LOGGING_METHOD_PREFIX_MINIO_CONFIG,
                          filepath,
                          error->message);
        free_error(error);
    }
    g_key_file_free(keyfile);


    // Access key not specified, no initialization possible
    if (!access_key)
    {
        minio_print_error("[%s] Key [%s] not found in config!\n",
                          LOGGING_METHOD_PREFIX_MINIO_CONFIG,
                          KN_MINIO_ACCESSKEY);
        return false;
    }

    // Secret key not specified, no initialization possible neither
    if (!*secret_key)
    {
        minio_print_error("[%s] [Config] Key [%s] not found in config!\n",
                          LOGGING_METHOD_PREFIX_MINIO_CONFIG,
                          KN_MINIO_SECRETKEY);
        return false;
    }

    // Set default values for non critical properties if missing
    if (!hostname)
    {
        minio_print_debug("[%s] Set key value [%-12s] with default:\t'%d'\n",
                          LOGGING_METHOD_PREFIX_MINIO_CONFIG,
                          KN_MINIO_HOSTNAME,
                          MINIO_DEFAULT_HOSTNAME);
        hostname = MINIO_DEFAULT_HOSTNAME;
    }

    if (!bucketname_data)
    {
        minio_print_debug("[%s] Set key value [%-12s] with default:\t'%d'\n",
                          LOGGING_METHOD_PREFIX_MINIO_CONFIG,
                          KN_MINIO_BUCKET_DATA,
                          MINIO_BUCKET_DEFAULT_DATA);
        bucketname_data = MINIO_BUCKET_DEFAULT_DATA;
    }

    if (!bucketname_filemeta)
    {
        minio_print_debug("[%s] Set key value [%-12s] with default:\t'%d'\n",
                          LOGGING_METHOD_PREFIX_MINIO_CONFIG,
                          KN_MINIO_BUCKET_FILEMETA,
                          MINIO_BUCKET_DEFAULT_FILEMETA);
        bucketname_filemeta = MINIO_BUCKET_DEFAULT_FILEMETA;
    }

    if (add_missing_bucket < 0)
    {
        minio_print_debug("[%s] Set key value [%-12s] with default:\t'%d'\n",
                          LOGGING_METHOD_PREFIX_MINIO_CONFIG,
                          KN_MINIO_ADD_MISSING_BUCKET,
                          CREATE_BUCKET_IF_MISSING);
        add_missing_bucket = CREATE_BUCKET_IF_MISSING;
    }


    // 2. set the values in backend
    backend->hostname = hostname;
    backend->access_key = access_key;
    backend->bucketname_data = bucketname_data;
    backend->bucketname_filemeta = bucketname_filemeta;
    backend->add_missing_bucket = add_missing_bucket;

    return true;
}


/**
 * Adds a bucket with the passed bucketname_data to the MinIO server
 * @param bucketname bucketname_data to create a bucket with
 * @return success of bucket create, also false if bucketname_data not set
 */
static bool addBucket(const char *bucketname)
{
    if (bucketname != NULL)
    {
        minio_print_debug("Create bucket:\t%s\n", bucketname);
        return create_bucket(bucketname, false);
    }

    return false;
}


/** Prefix for checking bucket method */
#define LOGGING_METHOD_PREFIX_MINIO_CHECKBUCKET ("CheckBucket")


/**
 * Checks if a bucket with the passed bucket name can be accessed
 * @param bucketname is the name for the bucket to check for
 * @param add_if_missing: TRUE -> tries to add bucket if missing
 */
static bool checkBucketAccess(const char *bucketname, bool add_if_missing)
{
    bool access = false;
    if (bucketname != NULL)
    {
        access = s3_test_bucket(bucketname);

        if (access)
        {
            minio_print_debug("[%s] Bucket found:\t\t\t%s\n",
                              LOGGING_METHOD_PREFIX_MINIO_CHECKBUCKET,
                              bucketname);
        } else
        {
            minio_print_debug("[%s] Bucket not found:\t%s\n",
                              LOGGING_METHOD_PREFIX_MINIO_CHECKBUCKET,
                              bucketname);

            if (add_if_missing)
            {
                minio_print_debug("[%s] Try add missing bucket '%s'...\n",
                                  LOGGING_METHOD_PREFIX_MINIO_CHECKBUCKET,
                                  bucketname);
                access = addBucket(bucketname);

                if (access)
                {
                    minio_print_debug("[%s] Bucket created.\n",
                                      LOGGING_METHOD_PREFIX_MINIO_CHECKBUCKET);
                } else
                {
                    minio_print_error("[%s] Error while creating bucket (%s)!\n",
                                      LOGGING_METHOD_PREFIX_MINIO_CHECKBUCKET,
                                      bucketname);
                }
            }
        }
    } else
    {
        minio_print_error("[%s] Bucketname missing!\n",
                          LOGGING_METHOD_PREFIX_MINIO_CHECKBUCKET);
    }
    return access;
}


/**
 * Checks if the passed @param key already exists in the passed @param bucket
 * @param bucket
 * @param key
 * @return
 */
static bool checkKeyExistInBucket(const char *bucket, const char *key)
{
    if (bucket != NULL && key != NULL)
    {
        return head_object(bucket, key);
    }
    return false;
}



/** Prefix for logging in initialization methods */
#define LOGGING_METHOD_PREFIX_MINIO_INIT ("Init")


/**
 * Initializes the storage buckets (including a fallback bucket)
 * @param backend: the backend structure
 * @return
 */
static bool initBuckets(minio_backend_t *backend)
{
    minio_print_debug("[%s] Initialize buckets...\n", LOGGING_METHOD_PREFIX_MINIO_INIT);

    if (backend != NULL)
    {
        // init fallback bucket in every case
        if (!checkBucketAccess(MINIO_FALLBACK_BUCKET, backend->add_missing_bucket))
        {
            minio_print_error("[%s] Could not initialize the fallback bucket (%s)\n",
                              LOGGING_METHOD_PREFIX_MINIO_INIT,
                              MINIO_FALLBACK_BUCKET);
            return false;
        } else
        {
            minio_print_info("[%s] Using bucket for fallback cases:\t%s\n",
                             LOGGING_METHOD_PREFIX_MINIO_INIT,
                             MINIO_FALLBACK_BUCKET);
        }

        // init data bucket
        if (backend->bucketname_data != NULL)
        {
            if (!checkBucketAccess(backend->bucketname_data, backend->add_missing_bucket))
            {
                minio_print_error("[%s] Could not initialize data bucket (%s)\n",
                                  LOGGING_METHOD_PREFIX_MINIO_INIT,
                                  backend->bucketname_data);
                return false;
            } else
            {
                minio_print_info("[%s] Using bucket for data:\t\t%s\n",
                                 LOGGING_METHOD_PREFIX_MINIO_INIT,
                                 backend->bucketname_data);
            }
        } else
        {
            minio_print_error("[%s] No data bucket name configured\n",
                              LOGGING_METHOD_PREFIX_MINIO_INIT);
            return false;
        }

        // init meta bucket
        if (backend->bucketname_filemeta != NULL)
        {
            if (!checkBucketAccess(backend->bucketname_filemeta, backend->add_missing_bucket))
            {
                minio_print_error("[%s] No filemeta bucket name configured\n",
                                  LOGGING_METHOD_PREFIX_MINIO_INIT,
                                  backend->bucketname_filemeta);
                return false;
            } else
            {
                minio_print_info("[%s] Using bucket for filemeta:\t%s\n",
                                 LOGGING_METHOD_PREFIX_MINIO_INIT,
                                 backend->bucketname_filemeta);
            }
        } else
        {
            minio_print_error("[%s] No filemeta bucket name configured\n", LOGGING_METHOD_PREFIX_MINIO_INIT);
            return false;
        }

        // passed till here means every bucket already existed or has been created
        minio_print_info("[%s] Initialized buckets.\n", LOGGING_METHOD_PREFIX_MINIO_INIT);
        return true;
    } else
    {
        minio_print_error("[%s] No initialized backend passed!\n", LOGGING_METHOD_PREFIX_MINIO_INIT);
        return false;
    }
}


/**
 * Initializes the MinIO backend
 * @param server_struct: the main server structure which also stores (most of) the initialized server connection parameters
 * @todo: check before init, if necessary libraries are available; print Error and exit otherwise
 * @todo: add parameter "backend" to be able to use every backend as meta or data backend
 *        (see i.e. setting user_data below), maybe use if set, server_struct->[some backend which makes sense] else
 */
void minio_init_backend(server_struct_t *server_struct)
{
    minio_backend_t *minio_backend;
    char *sec_key;

    minio_print_info("Initializing MinIO backend...\n");


    if (server_struct != NULL && server_struct->backend_data != NULL)
    {
        // Memory alloc
        sec_key = g_malloc0(sizeof(char));
        minio_backend = g_malloc0(sizeof(minio_backend_t));


        // Init values
        if (get_params_from_keyfile(minio_backend, &sec_key, server_struct->opt->configfile))
        {
            // Init libs3
            if (S3_init_with_parameters(minio_backend->hostname, minio_backend->access_key, strdup(sec_key)))
            {
                // test buckets (or add them if configured)
                if (initBuckets(minio_backend))
                {
                    server_struct->backend_data->user_data = minio_backend;
                    minio_print_info("Backend initialized.\n");
                } else
                {
                    g_free(minio_backend);
                    minio_print_error("Buckets could not be initialized!\n");
                }
            } else
            {
                g_free(minio_backend);
                minio_print_debug("s3_interface could not be initialized with given configuration!)\n");
            }
        } else
        {
            g_free(minio_backend);
            minio_print_error("Error while initialization, values could not be read from KeyFile.\n");
        }
    } else
    {
        // Passed settings are uninitialized
        minio_print_error("MinIO Backend could not be initialized due to uninitialized server_struct->backend!");
    }

    // clean up
    g_free(sec_key);
}


/**
 * Terminates the MinIO backend
 * @param server_struct
 */
void minio_terminate_backend(server_struct_t *server_struct)
{
    S3_deinitialize();
    minio_print_debug("Backend deinitialized!\n");
}



/** Prefix for logging in data saving methods */
#define LOGGING_METHOD_PREFIX_MINIO_SAVEDATA ("SaveData")

/**
 * Generates the key of the corresponding filemeta object for the passed hash
 * (so the key is always created in the same pattern)
 * @param hash_string
 * @return new allocated filename
 */
static gchar *generate_filemeta_objectkey(const gchar *hash_string)
{
    gchar *objectkey;
    objectkey = g_strdup_printf("%s.%s", hash_string, MINIO_FILEMETA_SUFFIX);
    return objectkey;
}

/**
 * Sets cmptype and uncmplen in meta hash file and stores it to the passed MinIO bucket
 * @param hash_string is the hash_string of the hash. The meta file has the
 *        same name but ends with .meta
 * @param uncmplen the len of the uncompressed hash file.
 * @param cmptype the compression type used to store this hash file.
 * @TODO IMPORTANT: make return success bool, when put_object(..) is modified to return its actual success bool
 */
static bool save_filemeta_to_bucket(const gchar *bucketname, const gchar *hash_string, gssize uncmplen, gshort cmptype)
{
    a_clock_t *clock = new_clock_t();
    minio_print_debug("[%s] Saving filemeta...\n", LOGGING_METHOD_PREFIX_MINIO_SAVEDATA);

    if (bucketname != NULL && hash_string != NULL)
    {
        gchar *objectkey = NULL;
        GKeyFile *keyfile = NULL;
        GError *error = NULL;
        gchar *keyfile_content;

        objectkey = generate_filemeta_objectkey(hash_string);
        minio_print_verbose("Save file meta (%s)\n", objectkey);
        keyfile = g_key_file_new();

        if (is_compress_type_allowed(cmptype) == FALSE)
        {
            cmptype = COMPRESS_NONE_TYPE;
        }

        g_key_file_set_int64(keyfile, GN_META, KN_UNCMPLEN, uncmplen);
        g_key_file_set_integer(keyfile, GN_META, KN_CMPTYPE, cmptype);

        keyfile_content = g_key_file_to_data(keyfile, NULL, &error);

        // PUT the object
        put_object(bucketname, objectkey, keyfile_content);

        g_key_file_free(keyfile);
        free_variable(objectkey);
        end_clock(clock, "save filemeta");

        return true;
    } else
    {
        minio_print_error("[%s] Bucketname or Hashstring NULL!\n", LOGGING_METHOD_PREFIX_MINIO_SAVEDATA);
        end_clock(clock, "save filemeta");
        return false;
    }
}


/**
 * Stores the passed @param data to an object (named by @param hash_string)
 * @param bucketname
 * @param hash_string
 * @param data
 * @return
 * @todo: make return success bool, when put_object(..) is modified to return its actual success bool
 */
static bool save_data_to_bucket(const gchar *bucketname, const gchar *hash_string, const gchar *data)
{
    minio_print_debug("[%s] Saving data...\n", LOGGING_METHOD_PREFIX_MINIO_SAVEDATA);

    a_clock_t *clock;
    if (bucketname != NULL && hash_string != NULL && data != NULL)
    {
        clock = new_clock_t();
        minio_print_verbose("[%s] Save data (%s)\n", LOGGING_METHOD_PREFIX_MINIO_SAVEDATA, hash_string);
        put_object(bucketname, hash_string, data);
        end_clock(clock, "save data");

        minio_print_debug("[%s] Data saved.\n", LOGGING_METHOD_PREFIX_MINIO_SAVEDATA);
        return true;
    } else
    {
        minio_print_error("[%s] Saving properties corrupted! (%s,%s,%s)\n",
                          LOGGING_METHOD_PREFIX_MINIO_SAVEDATA,
                          (bucketname ? "Bucketname OK" : "Bucketname missing"),
                          (hash_string ? "Hashstring OK" : "Hashstring missing"),
                          (data ? "Data OK" : "Data missing")
        );
        return false;
    }
}


/**
 * Stores data into a flat file. The file is named by its hash in hex
 * representation (one should easily check that the sha256sum of such a
 * file gives its name !).
 * @param server_struct is the server's main structure where all
 *        informations needed by the program are stored.
 * @param hash_data is a hash_data_t * structure that contains the hash and
 *        the corresponding data in a binary form and a 'read' field that
 *        contains the number of bytes in 'data' field.
 */
void minio_store_data(server_struct_t *server_struct, hash_data_t *hash_data)
{
    minio_backend_t *backend;
    const char *bucket_data;    /* no free */
    const char *bucket_filemeta;    /* no free */
    a_clock_t *clock;

    gchar *hash_string;

    minio_print_debug("[%s] MinIO Store Data\n", LOGGING_METHOD_PREFIX_MINIO_SAVEDATA);


    if (server_struct != NULL && server_struct->backend_data != NULL)
    {
        backend = server_struct->backend_data->user_data;

        minio_print_debug("[%s] Check buckets.\n", LOGGING_METHOD_PREFIX_MINIO_SAVEDATA);

        // Get the working buckets for data and filemeta (Order for data and filemeta: {Configured Bucket} -> {Fallback bucket} -> Error and return)

        // Data bucket
        if (backend->bucketname_data != NULL && checkBucketAccess(backend->bucketname_data, CREATE_BUCKET_IF_MISSING))
        {
            // bucket available
            bucket_data = backend->bucketname_data;
        } else
        {
            // show error message
            if (backend->bucketname_data != NULL)
            {
                minio_print_error("[%s] Data bucket could not be accessed! (%s)\n",
                                  LOGGING_METHOD_PREFIX_MINIO_SAVEDATA,
                                  backend->bucketname_data);
            } else
            {
                minio_print_error("[%s] No data bucket configured!\n",
                                  LOGGING_METHOD_PREFIX_MINIO_SAVEDATA);
            }


            // try fallback bucket
            if (checkBucketAccess(MINIO_FALLBACK_BUCKET, TRUE))
            {
                bucket_data = MINIO_FALLBACK_BUCKET;
                minio_print_error("[%s] Using fallback bucket (%s)\n",
                                  LOGGING_METHOD_PREFIX_MINIO_SAVEDATA,
                                  MINIO_FALLBACK_BUCKET);
            } else
            {
                // If no possible saving method found, return from method
                minio_print_error("[%s] FALLBACK BUCKET COULD NOT BE ACCESSED, SO DATA COULD NOT BE STORED!\n",
                                  LOGGING_METHOD_PREFIX_MINIO_SAVEDATA);
                return;
            }
        }

        // Filemeta bucket
        if (backend->bucketname_filemeta != NULL &&
            checkBucketAccess(backend->bucketname_filemeta, CREATE_BUCKET_IF_MISSING))
        {
            bucket_filemeta = backend->bucketname_filemeta;

        } else
        {
            // show error message
            if (backend->bucketname_filemeta != NULL)
            {
                minio_print_error("[%s] File meta bucket could not be accessed! (%s)\n",
                                  LOGGING_METHOD_PREFIX_MINIO_SAVEDATA,
                                  backend->bucketname_filemeta);
            } else
            {
                minio_print_error("[%s] No filemeta bucket configured!\n",
                                  LOGGING_METHOD_PREFIX_MINIO_SAVEDATA);
            }

            // try fallback bucket
            if (checkBucketAccess(MINIO_FALLBACK_BUCKET, TRUE))
            {
                bucket_filemeta = MINIO_FALLBACK_BUCKET;
                minio_print_error("[%s] Using fallback bucket (%s)\n",
                                  LOGGING_METHOD_PREFIX_MINIO_SAVEDATA,
                                  MINIO_FALLBACK_BUCKET);
            } else
            {
                // If no possible saving method found, return from method
                minio_print_critical("[%s] FALLBACK BUCKET COULD NOT BE ACCESSED, SO DATA COULD NOT BE STORED!\n",
                                     LOGGING_METHOD_PREFIX_MINIO_SAVEDATA);
                return;
            }
        }



        /** generate and save DATA */
        if (hash_data != NULL && hash_data->hash != NULL && hash_data->data != NULL)
        {
            // get the hash as string
            hash_string = hash_to_string(hash_data->hash);
            if (hash_string != NULL)
            {

                // try save meta data
                if (save_filemeta_to_bucket(bucket_filemeta, hash_string, hash_data->uncmplen, hash_data->cmptype))
                {
                    minio_print_debug("[%s] Stored filemeta\n", LOGGING_METHOD_PREFIX_MINIO_SAVEDATA);
                } else
                {
                    minio_print_critical("[%s] Could not store file meta for hash '%s' to bucket '%s'!\n",
                                         LOGGING_METHOD_PREFIX_MINIO_SAVEDATA,
                                         hash_string,
                                         bucket_filemeta);
                }

                // save data
                if (save_data_to_bucket(bucket_data, hash_string, (const gchar *) hash_data->data))
                {
                    minio_print_debug("[%s] Stored data\n", LOGGING_METHOD_PREFIX_MINIO_SAVEDATA);
                } else
                {
                    minio_print_critical("[%s] Could not store data for hash '%s' to bucket '%s'!\n",
                                         LOGGING_METHOD_PREFIX_MINIO_SAVEDATA,
                                         hash_string,
                                         bucket_data);
                }

                free_variable(hash_string);
            } else
            {
                minio_print_critical("[%s] Hash string could not be converted to string!\n",
                                     LOGGING_METHOD_PREFIX_MINIO_SAVEDATA);
                minio_print_critical("[%s] DATA COULD NOT BE STORED!\n",
                                     LOGGING_METHOD_PREFIX_MINIO_SAVEDATA);
            }
        } else
        {
            // show error message
            if (hash_data != NULL)
            {
                minio_print_critical("[%s] Hash_data structure incomplete! (%s,%s)\n",
                                     LOGGING_METHOD_PREFIX_MINIO_SAVEDATA,
                                     (hash_data->hash ? "Hash OK" : "Hash missing"),
                                     (hash_data->data ? "Data OK" : "Data missing"));
                minio_print_critical("[%s] DATA COULD NOT BE STORED!\n",
                                     LOGGING_METHOD_PREFIX_MINIO_SAVEDATA);
            } else
            {
                minio_print_error("[%s] Hash_data missing!\n",
                                  LOGGING_METHOD_PREFIX_MINIO_SAVEDATA);
            }
        }
    } else
    {
        minio_print_error("[%s] Server struct or its backend NULL!\n",
                          LOGGING_METHOD_PREFIX_MINIO_SAVEDATA);
        minio_print_critical("[%s] DATA COULD NOT BE STORED!\n",
                             LOGGING_METHOD_PREFIX_MINIO_SAVEDATA);
    }

    minio_print_debug("[%s] Store Data done.\n\n", LOGGING_METHOD_PREFIX_MINIO_SAVEDATA);
}




/** Prefix for logging in building hash list methods */
#define LOGGING_METHOD_PREFIX_MINIO_BUILDHASHLIST ("BuildHashList")


/**
 * Builds a list of hashs that server's server needs.
 * @param server_struct is the server's main structure where all
 *        informations needed by the program are stored.
 * @param hash_list is the list of hashs that we have to check for.
 */
GList *minio_build_needed_hash_list(server_struct_t *server_struct, GList *hash_list)
{
    minio_backend_t *backend;
    const gchar *bucket;

    GList *head = NULL;
    GList *needed = NULL;
    hash_data_t *hash_data = NULL;
    hash_data_t *needed_hash_data = NULL;

    gchar *hash_string;
    head = hash_list;

    minio_print_debug("[%s] Start building needed hash list...\n", LOGGING_METHOD_PREFIX_MINIO_BUILDHASHLIST);

    if (server_struct != NULL && server_struct->backend_data != NULL && server_struct->backend_data->user_data != NULL)
    {
        backend = server_struct->backend_data->user_data;
        bucket = backend->bucketname_data;

        // check if bucket available
        if (!checkBucketAccess(bucket, false))
        {
            minio_print_error("[%s] Bucket not found to build needed hash list:\t%s\n",
                              LOGGING_METHOD_PREFIX_MINIO_BUILDHASHLIST,
                              (bucket != NULL ? bucket : "NULL"));
        } else
        {
            // iterate over list
            while (head != NULL)
            {
                hash_data = head->data;
                hash_string = hash_to_string(hash_data->hash);

                if (checkKeyExistInBucket(bucket, hash_string) == FALSE &&
                    hash_data_is_in_list(hash_data, needed) == FALSE)
                {
                    /*
                     * hash is neither name of an existing file in data bucket, nor is it already in needed list
                     * -> add it to needed list
                     */
                    minio_print_verbose("[%s] Adding hash to needed:\t'%s'\n", hash_string);

                    needed_hash_data = copy_only_hash(hash_data, NULL);
                    needed = g_list_prepend(needed, needed_hash_data);
                }

                free_variable(hash_string);

                // next element
                head = g_list_next(head);
            }

            // reverse order to "undo" the pre prepending of elements
            needed = g_list_reverse(needed);
        }
    } else
    {
        minio_print_error("[%s] Server structure incomplete, no bucket connection possible!\n",
                          LOGGING_METHOD_PREFIX_MINIO_BUILDHASHLIST);
    }

    minio_print_debug("[%s] Building hash list done.\n",
                      LOGGING_METHOD_PREFIX_MINIO_BUILDHASHLIST);
    return needed;
}


/**
 * Searches for the in Filemeta stored values in the Filemeta file in @param bucket with @param hash_string as key
 * @param bucket
 * @param hash_string
 * @param out_cmptype
 * @param out_uncmplen
 * @return
 */
static bool get_filemeta_values(const char *bucket, const char *hash_string, short *out_cmptype, gssize *out_uncmplen)
{
    bool success = 0;

    GKeyFile *keyfile = NULL;
    GError *error = NULL;
    size_t *read = NULL;
    char *filemeta_key = NULL;
    char *filemeta_content = NULL;

    // init out parameter pointers
    if (out_cmptype == NULL)
        out_cmptype = g_malloc0(sizeof(short));

    if (out_uncmplen == NULL)
        out_uncmplen = g_malloc0(sizeof(gssize));


    // get filemeta from server
    filemeta_key = generate_filemeta_objectkey(hash_string);
    read = g_malloc0(sizeof(size_t));
    filemeta_content = get_object(bucket, filemeta_key, read);
    g_free(read);

    // Set out param if content found
    if (filemeta_content != NULL && strlen(filemeta_content) > 0)
    {
        keyfile = g_key_file_new();

        if (g_key_file_load_from_data(keyfile, filemeta_content, (gsize) -1, G_KEY_FILE_KEEP_COMMENTS, &error))
        {
            *out_cmptype = (short) read_int_from_file(keyfile, filemeta_key, GN_META, KN_CMPTYPE, "Get CMPTYPE",
                                                      COMPRESS_NONE_TYPE);
            *out_uncmplen = read_int64_from_file(keyfile, filemeta_key, GN_META, KN_UNCMPLEN, "Get UNCMPLEN", -1);
            success = 1;
        }

        g_key_file_free(keyfile);
    }

    // Clean up
    free(filemeta_key);
    free(filemeta_content);
    free_error(error);

    return success;
}




/** Prefix for logging in retrieving data */
#define LOGGING_METHOD_PREFIX_MINIO_RETRIEVEDATA ("RetrieveData")


/**
 * Retrieves data from a file in MinIO. The file is named by its hash in hex
 * representation (one should easily check that the sha256sum of such a
 * file gives its name !).
 * @param server_struct is the server's main structure where all
 *        informations needed by the program are stored.
 * @param hash_string is a gchar * hash in hexadecimal format as retrieved
 *        from the url.
 */
hash_data_t *minio_retrieve_data(server_struct_t *server_struct, gchar *hash_string)
{
    a_clock_t *clock;
    clock = new_clock_t();

    minio_backend_t *backend = NULL;
    hash_data_t *hash_data = NULL;

    guchar *data = NULL;
    size_t *read;
    guint8 *hash;

    short *cmptype = g_malloc0(sizeof(short));
    gssize *uncmplen = g_malloc0(sizeof(gssize));

    minio_print_debug("[%s] Start retrieving data...\n", LOGGING_METHOD_PREFIX_MINIO_RETRIEVEDATA);

    if (server_struct != NULL && server_struct->backend_data != NULL && server_struct->backend_data->user_data != NULL)
    {
        backend = (minio_backend_t *) server_struct->backend_data->user_data;

        // get filemeta
        if (get_filemeta_values(backend->bucketname_filemeta, hash_string, cmptype, uncmplen))
        {
            read = g_malloc0(sizeof(size_t));
            data = (guchar *) get_object(backend->bucketname_data, hash_string, read);
            if (data != NULL)
            {
                hash = string_to_hash(hash_string);
                hash_data = new_hash_data_t_as_is(data, *read, hash, *cmptype, *uncmplen);
                minio_print_debug("[%s] Retrieving data finished.\n",
                                  LOGGING_METHOD_PREFIX_MINIO_RETRIEVEDATA);
            } else
            {
                minio_print_error("[%s] No data found in object! (Key: %s, Bucket: %s)\n",
                                  LOGGING_METHOD_PREFIX_MINIO_RETRIEVEDATA,
                                  hash_string,
                                  backend->bucketname_data);
            }

            g_free(read);
        } else
        {
            minio_print_error("[%s] No Filemeta found for hash '%s' (Bucket: %s)\n",
                              LOGGING_METHOD_PREFIX_MINIO_RETRIEVEDATA,
                              hash_string,
                              backend->bucketname_filemeta);
        }
    } else
    {
        minio_print_error("[%s] Server structure incomplete!\n", LOGGING_METHOD_PREFIX_MINIO_RETRIEVEDATA);
    }

    g_free(cmptype);
    g_free(uncmplen);

    end_clock(clock, "Retrieve data");
    return hash_data;
}