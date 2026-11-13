#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

void die(const char *msg)
{
    fprintf(stderr, "csvstat: %s\n", msg);
    exit(1);
}

void *xmalloc(size_t size)
{
    void *ptr = malloc(size);

    if (ptr == NULL)
        die("out of memory");
    return ptr;
}

void *xrealloc(void *ptr, size_t size)
{
    ptr = realloc(ptr, size);
    if (ptr == NULL)
        die("out of memory");
    return ptr;
}

Type value_type(const char *text)
{
    int i = 0;

    if (text[0] == '\0')
        return TYPE_NULL;
    if (text[0] == '+' || text[0] == '-')
        i = 1;
    if (text[i] == '\0')
        return TYPE_STRING;                                     
    for (; text[i] != '\0'; i++) {
        if (text[i] < '0' || text[i] > '9')
            return TYPE_STRING;
    }
    errno = 0;
    strtoll(text, NULL, 10);
    if (errno == ERANGE)
        return TYPE_STRING;                                                  
    return TYPE_INT;
}

const char *type_name(Type type)
{
    if (type == TYPE_INT)
        return "INT";
    if (type == TYPE_STRING)
        return "STRING";
    return "NULL";
}

static char *copy_text(const char *text)
{
    char *copy = xmalloc(strlen(text) + 1);

    strcpy(copy, text);
    return copy;
}

Schema schema_read(const char *path)
{
    CsvReader *reader = csv_open(path);
    Schema schema;
    char **fields;
    int count;
    int i, j;
    Type type;

    if (!csv_next(reader, &fields, &count))
        die("file is empty");
    schema.ncols = count;
    schema.rows = 0;
    schema.names = xmalloc(count * sizeof(char *));
    schema.types = xmalloc(count * sizeof(Type));
    for (i = 0; i < count; i++) {
        if (fields[i][0] == '\0')
            die("bad column name");
        for (j = 0; j < i; j++) {
            if (strcmp(schema.names[j], fields[i]) == 0)
                die("duplicate column name");
        }
        schema.names[i] = copy_text(fields[i]);
        schema.types[i] = TYPE_NULL;
    }

    while (csv_next(reader, &fields, &count)) {
        if (count != schema.ncols)
            die("wrong number of fields");
        for (i = 0; i < count; i++) {
            type = value_type(fields[i]);
            if (type > schema.types[i])
                schema.types[i] = type;
        }
        schema.rows++;
    }
    csv_close(reader);
    return schema;
}

void schema_free(Schema *schema)
{
    int i;

    for (i = 0; i < schema->ncols; i++)
        free(schema->names[i]);
    free(schema->names);
    free(schema->types);
}

void print_schema(const Schema *schema)
{
    int i;

    printf("rows %lld\ncolumns %d\n", schema->rows, schema->ncols);
    for (i = 0; i < schema->ncols; i++)
        printf("%d \"%s\" %s\n", i, schema->names[i], type_name(schema->types[i]));
}

static void print_usage(FILE *out)
{
    fprintf(out, "Usage:\n  csvstat --help\n  csvstat schema <csv-file>\n  csvstat stats <csv-file>\n");
}

int main(int argc, char **argv)
{
    Schema schema;
    IntStats *stats;

    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        print_usage(stdout);
        return 0;
    }
    if (argc != 3 || argv[2][0] == '-') {
        print_usage(stderr);
        return 2;
    }

    if (strcmp(argv[1], "schema") == 0) {
        schema = schema_read(argv[2]);
        print_schema(&schema);
    } else if (strcmp(argv[1], "stats") == 0) {
        schema = schema_read(argv[2]);
        stats = stats_compute(argv[2], &schema);
        print_stats(&schema, stats);
        free(stats);
    } else {
        print_usage(stderr);
        return 2;
    }
    schema_free(&schema);

    if (fflush(stdout) != 0 || ferror(stdout))                                
        return 1;
    return 0;
}
