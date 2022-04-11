

#pragma once

#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(_MSC_VER)
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

const static uint64_t page_size = 4096;
const static uint64_t alloc_size = page_size * 8;
const static uint64_t partmask = 0x80000000000000;
const static uint32_t endian_test = 1;
static inline int is_little_endian() { return (*(char *)&endian_test == 1); }

#define next_page_multiple(s) ((s + (page_size - 1)) & ~(page_size - 1))
#define prev_page_multiple(s) (s & ~(page_size - 1))

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
    int32_t fd;
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
#if !defined(_MSC_VER)
    struct termios info;
    tcgetattr(0, &info); /* get current terminal attirbutes; 0 is the file descriptor for stdin */
    info.c_lflag &= ~ICANON; /* disable canonical mode */
    info.c_cc[VMIN] = 1; /* wait until at least one keystroke available */
    info.c_cc[VTIME] = 0; /* no timeout */
    tcsetattr(0, TCSANOW, &info); /* set immediately */
#endif
    va_list myargs;
    va_start(myargs, format);
    FILE *f = fopen(filename, "rb");
    char line[1024];
    char prebuff[1024];
    int32_t line_nr = 0;
    while (dbg_read_line(f, line)) {
        line_nr++;
        int32_t dist = abs(line_nr - linenr);
        if (dist <= 3) {
            if (dist == 0) {
                //
                //
                int c = 0;
                while (line[c] == ' ') {
                    prebuff[c++] = ' ';
                }
                prebuff[c++] = '/';
                prebuff[c++] = '/';
                prebuff[c++] = ' ';
                printf("%s", prebuff);
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

    switch (getchar()) {
    case 27:
        exit(1);
    default:
        break;
    }
}
#define log_dgb(fmt, ...) debug_step(__FILE__, __LINE__, fmt, __VA_ARGS__);

ssize_t stream_op(file_stream *fs, void *src, size_t sm)
{
    if (fs->mode == READ) {
        return read(fs->fd, src, sm);
    } else {
        return write(fs->fd, src, sm);
    }
}

size_t resize_buffer(file_stream *fs, size_t sm)
{
    size_t head = prev_page_multiple(fs->buffer_ptr);
    size_t tail = next_page_multiple(sm + fs->buffer_ptr);
    size_t span = tail - head;
    size_t start_offset = fs->buffer_ptr - head;
    size_t remaining_file_size = fs->file_size - fs->file_ptr;
    fs->buffer_ptr = start_offset;

    //   if we need more space for our buffer
    if (span > fs->buffer_size) {
        free(fs->buffer);
        fs->buffer = (uint8_t *)malloc(span);
        fs->buffer_size = span;
    }

    // do we need to rewind
    if (start_offset > 0) {
        // rewind to our last page multiple
        lseek(fs->fd, -fs->buffer_ptr, SEEK_CUR);
        fs->file_ptr = -fs->buffer_ptr;
    }

    size_t next_size = fs->buffer_size;
    if ((fs->file_ptr + next_size) > fs->file_size) {
        next_size = fs->file_size - fs->file_ptr;
    }

    return next_size;
}

size_t sync_stream(file_stream *fs, size_t sm)
{

    if ((sm + fs->file_ptr) >= fs->file_size) {
        return 0;
    }
    size_t next_size = fs->buffer_size;
    ssize_t opres = 0;
    if (fs->buffer_ptr == fs->buffer_size) {
        fs->buffer_ptr = 0;
        if (sm > fs->buffer_size) {
            next_size = resize_buffer(fs, sm);
        }
    } else {
        next_size = resize_buffer(fs, sm);
    }

    // read the next batch from file.
    if ((opres = stream_op(fs, fs->buffer, next_size)) == -1) {
        return 0;
    }
    fs->file_ptr += next_size;

    return 1;
}

file_stream *open_stream(const char *p, file_stream_mode mode)
{
    int32_t fd = open(p, mode == READ ? O_RDONLY, S_IREAD : O_RDWR | O_CREAT | O_TRUNC,
        S_IREAD | S_IWRITE);

    if (fd == -1) {
        return NULL;
    }
    struct stat stats;
    int32_t status = fstat(fd, &stats);
    if (status != 0) {
        close(fd);
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
    new_stream->file_size = stats.st_size;

    if (mode == READ) {
        new_stream->buffer_size = alloc_size;
        if (new_stream->file_size < new_stream->buffer_size) {
            new_stream->buffer_size = new_stream->file_size;
        }
        new_stream->buffer = (uint8_t *)malloc(new_stream->buffer_size);
        read(fd, new_stream->buffer, new_stream->buffer_size);
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
        close(stream->fd);
        // release our heap stores
        free(stream->file_path);
        free(stream->buffer);
        free(stream);
    }
}

void *read_stream(size_t sm, file_stream *fs)
{
    // are we requesting data beyond the buffer size?
    if ((sm + fs->buffer_ptr) > fs->buffer_size) {
        if (sync_stream(fs, sm) == 0) {
            return 0;
        }
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
    case 0x0:
    case 0xA:
    case 0xB:
    case 0xC:
    case EOF:
    case 0xD: {
        return 1;
    }

    default:
        return 0;
    }
}

size_t read_line_char(file_stream *fs, char **line_start)
{
    char c;
    size_t line_len = 0;
    do {
        // are we requesting data beyond the buffer size?
        if ((1 + fs->buffer_ptr) > fs->buffer_size) {
            if (sync_stream(fs, 1) == 0) {
                return 0;
            }
        }
        // otherwise, jsut grab the value from the buffer
        c = fs->buffer[fs->buffer_ptr++];
    } while (!is_eol(c));
    *line_start = (char *)&fs->buffer[fs->buffer_ptr];
    do {
        // are we requesting data beyond the buffer size?
        if ((1 + fs->buffer_ptr) > fs->buffer_size) {
            if (sync_stream(fs, 1) == 0) {
                return line_len;
            }
        }
        // otherwise, jsut grab the value from the buffer
        c = fs->buffer[fs->buffer_ptr++];
        line_len++;
    } while (!is_eol(c));
    // stick a null terminator to it.

    return line_len;
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
    if ((sm + fs->buffer_ptr) > fs->buffer_size) {
        if (sync_stream(fs, sm) == 0) {
            return 0;
        }
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