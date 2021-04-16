//
// Created by work on 26.02.21.
//

#ifndef MINIOTEST_MINIO_BACKEND_H
#define MINIOTEST_MINIO_BACKEND_H

#include <stdlib.h>
#include <glib.h>
#include <stdbool.h>


#include "minio_interface.h"


/**
 * Logging
 */
#define LOGGING_PREFIX_MINIO ("MinIO")
#define LOGGING_PREFIX_TAG_CRITICAL ("CRITICAL")

/**
 * To store meta data of the hash file into a GKeyFile structure.
 */
#define GN_META ("Meta")
#define KN_UNCMPLEN ("uncmplen")
#define KN_CMPTYPE ("cmptype")


/**
 * Object prefixes and suffixes
 */
#define MINIO_FILEMETA_SUFFIX "meta"


/**
 * Stores the properties of the data backend
 * @todo: ggf. die Möglichkeit zum speichern einer externen FileMeta-Speichermethode (READ & WRITE)?
 * (Wäre eigentlich in MongoDB besser aufgehoben)
 */
typedef struct minio_backend_t
{
    const char *hostname;
    const char *access_key;
    const char *bucketname_data;
    const char *bucketname_filemeta;
    bool add_missing_bucket;
//    bool initialized;
//    bool corrupted;
} minio_backend_t;

/**
 * Initializes the MinIO backend
 * @param server_struct
 */
void minio_init_backend(server_struct_t *server_struct);


/**
 * Terminates the MinIO backend
 * @param server_struct
 */
void minio_terminate_backend(server_struct_t *server_struct);


/**
 * Builds a list of hashs that server's server needs.
 * @param server_struct is the server's main structure where all
 *        informations needed by the program are stored.
 * @param hash_list is the list of hashs that we have to check for.
 * @todo implement (HINT: vllt geht das recht easy je key über S3_head_object)
 */
extern GList *minio_build_needed_hash_list(server_struct_t *server_struct, GList *hash_list);


/**
 * Stores data into a flat file. The file is named by its hash in hex
 * representation (one should easily check that the sha256sum of such a
 * file gives its name !).
 * @param server_struct is the server's main structure where all
 *        informations needed by the program are stored.
 * @param hash_data is a hash_data_t * structure that contains the hash and
 *        the corresponding data in a binary form and a 'read' field that
 *        contains the number of bytes in 'data' field.
 *
 * @TODO: falls bucket für client nicht verfügbar, bucket erstellen!
 */
extern void minio_store_data(server_struct_t *server_struct, hash_data_t *hash_data);


/**
 * Retrieves data from a file in MinIO. The file is named by its hash in hex
 * representation (one should easily check that the sha256sum of such a
 * file gives its name !).
 * @param server_struct is the server's main structure where all
 *        informations needed by the program are stored.
 * @param hash_string is a gchar * hash in hexadecimal format as retrieved
 *        from the url.
 */
extern hash_data_t *minio_retrieve_data(server_struct_t *server_struct, gchar *hash_string);



// TODO DELETE
void putTest();

void getTest();


#endif //MINIOTEST_MINIO_BACKEND_H
