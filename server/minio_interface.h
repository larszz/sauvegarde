//
// Created by work on 09.02.21.
//

#ifndef MINIOTEST_MINIO_INTERFACE_H
#define MINIOTEST_MINIO_INTERFACE_H


#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include "libs3.h"
#include "misc.h"


extern bool S3_init_with_parameters(const char *hostname, const char *access_key, const char *secret_key);

extern bool s3_test_bucket(const char *bucketName);
extern bool create_bucket(const char *bucketName, bool validateBucketName);

extern bool head_object(const char *bucket, const char *key);
extern void put_object(const char *targetBucket, const char *targetKey, const char *dataToWrite);
extern char *get_object(const char *targetBucket, const char *sourceKey, size_t *read_size);

#endif //MINIOTEST_MINIO_INTERFACE_H
