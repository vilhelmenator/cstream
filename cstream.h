

#pragma once

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_SIZE 4096 * 8
const static uint64_t partmask = 0x80000000000000;
const static uint32_t endian_test = 1;
static inline int is_little_endian() { return (*(char *)&endian_test == 1); }

static inline uintptr_t round_mul(uintptr_t v, size_t mul)
{
    uintptr_t mask = mul - 1;
    if ((mul & mask) == 0) {
        return (v & ~mask);
    } else {
        return ((v / mul) * mul);
    }
}

static inline uintptr_t round_up(uintptr_t v, size_t mul)
{
    return round_mul(v + mul - 1, mul);
}

static inline uintptr_t round_down(uintptr_t v, size_t mul)
{
    return round_mul(v, mul);
}

typedef enum file_stream_mode_e {
    READ = 0,
    WRITE
} file_stream_mode;

typedef struct file_stream_t
{
    // file data
    FILE *fd;
    uint8_t *file_path;
    size_t file_size;
    size_t file_ptr;
    // internal buffer data
    uint8_t *buffer;
    uint32_t buffer_ptr;
    uint32_t buffer_size;
    file_stream_mode mode;
} file_stream;

int dbg_read_line(FILE *fd, char *buff)
{
    char c;
    int offset = 0;
    do { // read one line
        c = fgetc(fd);
        if (c != EOF) {
            if (offset < 1024) {
                buff[offset++] = c;
            }
        }
    } while (c != EOF && c != '\n');
    if (c == EOF) {
        return 0;
    }
    buff[offset - 1] = 0;
    return 1;
}

void debug_step(const char *filename, int32_t linenr, const char *format, ...)
{
    va_list myargs;
    va_start(myargs, format);
    FILE *f = fopen(filename, "rb");
    char line[1024];
    int32_t line_nr = 0;
    while (dbg_read_line(f, line)) {
        line_nr++;
        int32_t dist = abs(line_nr - linenr);
        if (dist <= 3) {
            if (dist == 0) {
                vprintf(format, myargs);
            } else {
                printf("%s\n", line);
            }
        }
        if (line_nr > (linenr + 3)) {
            break;
        }
    }
    va_end(myargs);
    getchar();
}
#define log_dgb(fmt, ...) debug_step(__FILE__, __LINE__, fmt, __VA_ARGS__);

size_t stream_op(file_stream *fs, void *src, size_t sm)
{
    if (fs->mode == READ) {
        return fread(src, 1, sm, fs->fd);
    } else {
        return fwrite(src, 1, sm, fs->fd);
    }
}

void resize_buffer(file_stream *fs, size_t sm)
{
    // resize the buffer to include the next size rounded up to the next os page
    size_t next_size = round_up(sm + fs->buffer_size, PAGE_SIZE);
    if ((fs->file_ptr + next_size) > fs->file_size) {
        next_size = fs->file_size - fs->file_ptr;
    }

    fs->buffer = (uint8_t *)realloc(fs->buffer, next_size);
    fs->buffer_size = next_size;
}

size_t sync_stream(file_stream *fs, size_t sm)
{
    // are we requesting data beyond the buffer size?
    if ((sm + fs->buffer_ptr) > fs->buffer_size) {
        // Are we requesting data beyond the file size?
        if ((sm + fs->file_ptr) >= fs->file_size) {
            return 0;
        }
        size_t next_size = PAGE_SIZE;
        size_t opres = 0;
        if (fs->buffer_ptr == fs->buffer_size) {
            // we can reuse the previous buffer

            // don't try to read beyond the file size.
            if ((fs->file_ptr + next_size) > fs->file_size) {
                next_size = fs->file_size - fs->file_ptr;
            }

            // read the next batch from file.
            size_t opres;
            if ((opres = stream_op(fs, fs->buffer, next_size)) != next_size) {
                return 0;
            }
        } else {
            // this needs to try and avoid resizing the buffer
            size_t prev_buffer_size = fs->buffer_size;
            resize_buffer(fs, sm);
            size_t step_size = fs->buffer_size - prev_buffer_size;
            if ((opres = stream_op(fs, &fs->buffer[prev_buffer_size], step_size)) != step_size) {
                return 0;
            }
        }
        fs->file_ptr += next_size;
    }
    return 1;
}

file_stream *open_stream(const char *p, file_stream_mode mode)
{
    FILE *fd = fopen(p, mode == READ ? "rb" : "wb");
    if (fd == NULL) {
        return NULL;
    }
    // copy our path string into the stream
    uint32_t path_len = strlen(p) + 1;
    uint8_t *path_string = (uint8_t *)malloc(path_len);
    memcpy(path_string, p, path_len);
    path_string[path_len - 1] = 0;

    // create our stream
    file_stream *new_stream = (file_stream *)malloc(sizeof(file_stream));
    new_stream->file_path = path_string;
    new_stream->fd = fd;
    new_stream->mode = mode;

    // get file size
    fseek(fd, 0L, SEEK_END);
    new_stream->file_size = ftell(fd);
    rewind(fd);

    if (mode == READ) {
        new_stream->buffer_size = PAGE_SIZE;
        if (new_stream->file_size < new_stream->buffer_size) {
            new_stream->buffer_size = new_stream->file_size;
        }
        new_stream->buffer = (uint8_t *)malloc(new_stream->buffer_size);
        fread(new_stream->buffer, new_stream->buffer_size, 1, fd);
        new_stream->file_ptr = new_stream->buffer_size;
    } else {
        new_stream->buffer_size = 0;
        new_stream->buffer = NULL;
        new_stream->file_ptr = 0;
    }
    new_stream->buffer_ptr = 0;
    return new_stream;
}

void close_stream(file_stream *stream)
{
    // release our buffer and file descriptor
    if (stream) {
        fclose(stream->fd);
        // release our heap stores
        free(stream->file_path);
        free(stream->buffer);
        free(stream);
    }
}

void *read_stream(size_t sm, file_stream *fs)
{
    if (sync_stream(fs, sm) == 0) {
        return NULL;
    }
    // otherwise, jsut grab the value from the buffer
    uint8_t *res = &fs->buffer[fs->buffer_ptr];
    fs->buffer_ptr += sm;
    return res;
}

int is_eol(char c)
{
    //
    // null terminator C0 80 in some odd java unicode world
    //
    switch (c) {
    case 0xA:
    case 0xB:
    case 0xC:
    case 0xD: {
        return 1;
    }

    default:
        return 0;
    }
}

int read_line_char(file_stream *fs, char *buff, size_t buff_size)
{

    size_t buff_idx = 0;
    void *cr = NULL;
    char c = 0;
    if ((cr = read_stream(1, fs)) == NULL) {
        return 0;
    }
    c = *(char *)cr;
    while (is_eol(c)) {
        if ((cr = read_stream(1, fs)) == NULL) {
            buff[0] = 0;
            return 0;
        }
        c = *(char *)cr;
    }
    do {
        if (buff_idx >= buff_size) {
            break;
        }
        buff[buff_idx++] = c;
        if ((cr = read_stream(1, fs)) == NULL) {
            buff[buff_idx] = 0;
            return 0;
        }
        c = *(char *)cr;
    } while (!is_eol(c));
    buff[buff_idx] = 0;
    return 1;
}

int read_line_unicode(file_stream *fs, short *buff, size_t buff_size)
{
    size_t buff_idx = 0;
    void *cr = NULL;
    short c = 0;
    if ((cr = read_stream(2, fs)) == NULL) {
        return 0;
    }
    c = *(short *)cr;
    while (is_eol(c)) {
        if ((cr = read_stream(2, fs)) == NULL) {
            buff[0] = 0;
            return 0;
        }
        c = *(short *)cr;
    }
    do {
        if (buff_idx >= buff_size) {
            break;
        }
        buff[buff_idx++] = c;
        if ((cr = read_stream(2, fs)) == NULL) {
            buff[buff_idx] = 0;
            return 0;
        }
        c = *(short *)cr;
    } while (!is_eol(c));
    buff[buff_idx] = 0;
    return 1;
}

int32_t write_stream(void *src, size_t sm, file_stream *fs)
{
    if (sync_stream(fs, sm) == 0) {
        return 0;
    }
    memcpy(&fs->buffer[fs->buffer_ptr], src, sm);
    fs->buffer_ptr += sm;
    return sm;
}

typedef struct bitstream_t
{
    file_stream *fs;
    uint64_t mask;
    uint64_t part;
} bitstream;

inline static bitstream new_bistream(const char *path, file_stream_mode mode)
{
    file_stream *fs = open_stream(path, mode);
    return (bitstream) { fs, 0, 0 };
}

inline static void close_bistream(bitstream *bs)
{
    close_stream(bs->fs);
}

uint8_t write_bitstream(bitstream *bs, int bit)
{
    if (bit)
        bs->part |= bs->mask;
    bs->mask = bs->mask >> 1;
    if (bs->mask == 0) {
        write_stream(&bs->part, sizeof(uint64_t), bs->fs);
        bs->part = 0;
        bs->mask = partmask;
        return 1;
    }
    return 0;
}

uint32_t read_bistream(bitstream *bs)
{
    int result = 0;
    if (bs->mask == partmask) {
        bs->part = *(uint64_t *)read_stream(sizeof(uint64_t), bs->fs);
    }
    result = bs->part & bs->mask;
    bs->mask = bs->mask >> 1;
    if (bs->mask == 0) {
        bs->mask = partmask;
    }
    return result;
}
