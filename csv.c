#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

CsvReader *csv_open(const char *path)
{
    CsvReader *r = xmalloc(sizeof(CsvReader));

    r->file = fopen(path, "rb");
    if (r->file == NULL)
        die("cannot open file");
    r->buf_size = 64;
    r->buf = xmalloc(r->buf_size);
    r->length = 0;
    r->fields = NULL;
    r->fields_size = 0;
    return r;
}

void csv_close(CsvReader *r)
{
    fclose(r->file);
    free(r->buf);
    free(r->fields);
    free(r);
}

static void add_char(CsvReader *r, int ch)
{
    if (r->length + 1 >= r->buf_size) {
        r->buf_size *= 2;
        r->buf = xrealloc(r->buf, r->buf_size);
    }
    r->buf[r->length] = (char)ch;
    r->length++;
}

static int read_record(CsvReader *r)
{
    int ch;
    int quoted = 0;                                  

    r->length = 0;
    for (;;) {
        ch = fgetc(r->file);
        if (ch == EOF) {
            if (ferror(r->file) || quoted)
                die("read error");
            if (r->length == 0)
                return 0;                                
            break;                                                            
        }
        if (ch == '\0')
            die("read error");
        if (ch == '\n' && !quoted)
            break;
        if (ch == '"')
            quoted = !quoted;                                                    
        add_char(r, ch);
    }
    if (r->length > 0 && r->buf[r->length - 1] == '\r')
        r->length--;                                      
    add_char(r, '\0');
    return 1;
}

static int split_record(CsvReader *r)
{
    int quoted = 0;
    int nfields = 1;
    size_t i;
    char *p;

    for (i = 0; i < r->length; i++) {
        if (r->buf[i] == '"')
            quoted = !quoted;
        else if (r->buf[i] == ',' && !quoted) {
            r->buf[i] = '\0';
            nfields++;
        }
    }
    if (nfields > r->fields_size) {
        r->fields = xrealloc(r->fields, nfields * sizeof(char *));
        r->fields_size = nfields;
    }
    p = r->buf;
    for (i = 0; i < (size_t)nfields; i++) {
        r->fields[i] = p;
        p += strlen(p) + 1;                                                
    }
    return nfields;
}

static void clean_field(char *field)
{
    char *start = field;
    char *end;
    char *in, *out;

    while (*start == ' ' || *start == '\t')
        start++;
    end = start + strlen(start);
    while (end > start && (end[-1] == ' ' || end[-1] == '\t'))
        end--;
    *end = '\0';

    if (*start != '"') {
        if (strchr(start, '"') != NULL)
            die("read error");                                        
        memmove(field, start, end - start + 1);
        return;
    }

    if (end - start < 2 || end[-1] != '"')
        die("read error");                                                    
    end--;                                                                    
    out = field;
    for (in = start + 1; in < end; in++) {
        if (*in == '"') {
            if (in + 1 < end && in[1] == '"')
                in++;                                          
            else
                die("read error");                                
        }
        *out = *in;
        out++;
    }
    *out = '\0';
}

int csv_next(CsvReader *r, char ***fields, int *count)
{
    int i;

    if (!read_record(r))
        return 0;
    *count = split_record(r);
    for (i = 0; i < *count; i++)
        clean_field(r->fields[i]);
    *fields = r->fields;
    return 1;
}
