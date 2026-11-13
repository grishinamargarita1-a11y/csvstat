#include <stdio.h>
#include <stdlib.h>

#include "common.h"

static CsvReader *open_data(const char *path)
{
    CsvReader *r = csv_open(path);
    char **fields;
    int count;

    csv_next(r, &fields, &count);
    return r;
}

static void add_value(IntStats *s, long long value)
{
    if (s->count == 0 || value < s->min)
        s->min = value;
    if (s->count == 0 || value > s->max)
        s->max = value;
    s->count++;
    if (value > 0)
        s->positive++;
    if (value < 0)
        s->negative++;
    if (value % 2 == 0)
        s->even++;
}

static void first_pass(const char *path, const Schema *schema, IntStats *stats)
{
    CsvReader *r = open_data(path);
    char **fields;
    int count;
    int i;

    while (csv_next(r, &fields, &count)) {
        for (i = 0; i < schema->ncols; i++) {
            if (schema->types[i] != TYPE_INT)
                continue;
            if (fields[i][0] == '\0')
                stats[i].nulls++;
            else
                add_value(&stats[i], strtoll(fields[i], NULL, 10));
        }
    }
    csv_close(r);
}

static void second_pass(const char *path, const Schema *schema, IntStats *stats)
{
    CsvReader *r = open_data(path);
    char **fields;
    int count;
    int i;
    long long value;

    while (csv_next(r, &fields, &count)) {
        for (i = 0; i < schema->ncols; i++) {
            if (schema->types[i] != TYPE_INT || fields[i][0] == '\0')
                continue;
            value = strtoll(fields[i], NULL, 10);
            stats[i].quotient += value / stats[i].count;
            stats[i].remainder += value % stats[i].count;
        }
    }
    csv_close(r);
}

static void finish_mean(IntStats *s)
{
    long long n = s->count;
    unsigned long long divisor = (unsigned long long)n;
    long long whole = s->quotient + s->remainder / n;
    long long rest = s->remainder % n;                      
    unsigned long long scaled, fraction, left;

    if (whole > 0 && rest < 0) {
        whole--;
        rest += n;
    }
    if (whole < 0 && rest > 0) {
        whole++;
        rest -= n;
    }

    s->mean_negative = whole < 0 || rest < 0;
    if (rest < 0)
        rest = -rest;
    if (whole < 0)
        s->mean_whole = -(unsigned long long)whole;
    else
        s->mean_whole = (unsigned long long)whole;

    scaled = (unsigned long long)rest * 1000000;
    fraction = scaled / divisor;
    left = scaled % divisor;
    if (2 * left > divisor || (2 * left == divisor && fraction % 2 == 1))
        fraction++;
    if (fraction == 1000000) {                                         
        fraction = 0;
        s->mean_whole++;
    }
    if (s->mean_whole == 0 && fraction == 0)
        s->mean_negative = 0;                                            
    s->mean_fraction = fraction;
}

IntStats *stats_compute(const char *path, const Schema *schema)
{
    IntStats *stats = xmalloc(schema->ncols * sizeof(IntStats));
    IntStats empty = {0};
    int i;

    for (i = 0; i < schema->ncols; i++)
        stats[i] = empty;
    first_pass(path, schema, stats);
    second_pass(path, schema, stats);
    for (i = 0; i < schema->ncols; i++) {
        if (schema->types[i] == TYPE_INT)
            finish_mean(&stats[i]);
    }
    return stats;
}

void print_stats(const Schema *schema, const IntStats *stats)
{
    const IntStats *s;
    int i;

    printf("rows %lld\ncolumns %d\n", schema->rows, schema->ncols);
    for (i = 0; i < schema->ncols; i++) {
        printf("\ncolumn %d\nname \"%s\"\ntype %s\n",
               i, schema->names[i], type_name(schema->types[i]));
        if (schema->types[i] != TYPE_INT) {
            printf("rows %lld\n", schema->rows);
            continue;
        }
        s = &stats[i];
        printf("nulls %lld\nmin %lld\nmax %lld\n", s->nulls, s->min, s->max);
        printf("mean %s%llu.%06llu\n",
               s->mean_negative ? "-" : "", s->mean_whole, s->mean_fraction);
        printf("positive %lld\nnegative %lld\neven %lld\n",
               s->positive, s->negative, s->even);
    }
}
