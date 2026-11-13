#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>

typedef enum {
    TYPE_NULL,
    TYPE_INT,
    TYPE_STRING
} Type;

typedef struct {
    int ncols;                              
    long long rows;                                    
    char **names;                           
    Type *types;                           
} Schema;

typedef struct {
    long long count;                                                
    long long nulls;                                              
    long long min, max;
    long long positive, negative, even;
    long long quotient, remainder;                                         
    int mean_negative;                                           
    unsigned long long mean_whole;                                      
    unsigned long long mean_fraction;                            
} IntStats;

typedef struct {
    FILE *file;
    char *buf;                                                            
    size_t length;                                         
    size_t buf_size;                                        
    char **fields;                                            
    int fields_size;                                      
} CsvReader;

void die(const char *message);
void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);

CsvReader *csv_open(const char *path);
void csv_close(CsvReader *reader);
int csv_next(CsvReader *reader, char ***fields, int *count);

Type value_type(const char *text);
const char *type_name(Type type);
Schema schema_read(const char *path);
void schema_free(Schema *schema);
void print_schema(const Schema *schema);

IntStats *stats_compute(const char *path, const Schema *schema);
void print_stats(const Schema *schema, const IntStats *stats);

#endif
