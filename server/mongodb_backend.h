//
// Created by work on 14.04.21.
//

#ifndef SAUVEGARDE_MONGODB_BACKEND_H
#define SAUVEGARDE_MONGODB_BACKEND_H

#include <mongoc/mongoc.h>

#define INT_UNDEFINED 0

/**
 * MongoC BSON operators
 */
#define MONGOC_OWN_GREATER_THAN ("$gt")
#define MONGOC_OWN_LESS_THAN ("$lt")
#define MONGOC_OWN_GREATER_THAN_EQUAL ("$gte")
#define MONGOC_OWN_LESS_THAN_EQUAL ("$lte")


#define MONGODB_LOG_PREFIX ("[MongoDB Backend] ")

/**
 * Labels for storing the meta data in MongoDB document
 */
#define LABEL_INODE ("inode")
#define LABEL_FILETYPE ("filetype")
#define LABEL_MODE ("mode")
#define LABEL_ATIME ("atime")
#define LABEL_CTIME ("ctime")
#define LABEL_MTIME ("mtime")
#define LABEL_SIZE ("size")
#define LABEL_OWNER ("owner")
#define LABEL_GROUP ("group")
#define LABEL_UID ("uid")
#define LABEL_GID ("gid")
#define LABEL_NAME ("name")
#define LABEL_LINK ("link")
#define LABEL_HASHLIST ("hashlist")

/**
 * TODO PRODUCTIVE: add to configuration.h, delete here
 */
#define GN_MONGODB_BACKEND ("MongoDB_Backend")

// TODO DELETE AFTER TESTS
#define DEFAULT_URI ("mongodb://localhost:27017")
#define DEFAULT_DB ("testdb")
#define MONGODB_CLIENT_APPNAME ("sauvegarde-server")


/** The URI prefix for the MongoDB connection string */
#define MONGODB_URI_PREFIX ("mongodb://")


#define MONGODB_DEFAULT_HOST ("localhost")
#define MONGODB_DEFAULT_PORT (27017)

/**
 * Defines if Hashes will be encoded
 */
#define MONGODB_BACKEND_CONVERT_HASH_BASE64 0

typedef struct
{
    const char *connection_string;
    const char *dbname;
    mongoc_client_t *client;
//    gboolean initialized;
//    gboolean corrupted;
} mongodb_backend_t;



/**
 * Initializes the mongodb backend, including the mongoc-structure till client level
 * (so the collection to use can be different for each sent meta data as well as the cursor etc.)
 * @param server_struct is the main server structure the mongodb backend is stored into
 */
extern void mongodb_init_backend(server_struct_t *server_struct);


/**
 * Terminates the MongoDB backend (including destroying of mongoc client and mongoc_cleanup())
 */
void mongodb_terminate_backend(backend_t *backend)
;


/**
 * Inserts the passed data to the MongoDB server if found with passed credentials
 * @param server_struct is the main server structure
 * @param server_meta is the passed meta data to store into database
 */
extern void mongodb_store_smeta(server_struct_t *server_struct, server_meta_data_t *server_meta);


/**
 * Searches for files with properties, specified in query_t
 * @param server_struct contains all needed information about the main server structure
 * @param query the query specification, the data is searched with
 * @return The found hash_data_t, encoded in a JSON string
 */
extern gchar *mongodb_get_list_of_files(server_struct_t *server_struct, query_t *query);




#endif //SAUVEGARDE_MONGODB_BACKEND_H
