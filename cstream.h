
#pragma once

#include <fcntl.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <wchar.h>

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
typedef int (*char_delim)(wchar_t *);
#define next_page_multiple(s) ((s + (page_size - 1)) & ~(page_size - 1))
#define prev_page_multiple(s) (s & ~(page_size - 1))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

static inline int is_eol_8(wchar_t *c)
{
    //
    // null terminator C0 80 in some odd java unicode world
    //
    uint32_t c_value = (uint8_t)*c;

    switch (c_value) {
    case 0x00: /* term */
    case 0x05: /*eof*/
    case 0x0A: /*\n*/
    case 0x0B:
    case 0x0C:
    case 0x0D: /*\r*/ {
        return 1;
    }

    default:
        return 0;
    }
}

static inline int is_eol_16(wchar_t *c)
{
    //
    // null terminator C0 80 in some odd java unicode world
    //
    uint32_t c_value = (uint16_t)*c;

    switch (c_value) {
    case 0x0000: /* term */
    case 0xC080: /* modified utf8 */
    case 0x80C0: /* modified utf8 */
    case 0x0005: /*eof*/
    case 0x000A: /*\n*/
    case 0x000B: /*vertical tab */
    case 0x000C: /*form feed */
    case 0x000D: /*\r*/
    case 0x0500: // big endian EOF ??
    case 0x0A00:
    case 0x0B00:
    case 0x0C00:
    case 0x0D00:
        return 1;

    default:
        return 0;
    };
}

static inline int is_eol_32(wchar_t *c)
{
    //
    // null terminator C0 80 in some odd java unicode world
    //
    uint32_t c_value = (uint32_t)*c;

    switch (c_value) {
    case 0x00000000: /* term */
    case 0x0000C080: /* modified utf8 */
    case 0xC0800000: /* modified utf8 */
    case 0x00000005: /*eof*/
    case 0x0000000A: /*\n*/
    case 0x0000000B: /*vertical tab */
    case 0x0000000C: /*form feed */
    case 0x0000000D: /*\r*/
    case 0x05000000: // big endian EOF ??
    case 0x0A000000:
    case 0x0B000000:
    case 0x0C000000:
    case 0x0D000000:
        return 1;

    default:
        return 0;
    };
}

static inline int from_char_8(wchar_t *c)
{
    return (uint8_t)*c;
}

static inline int from_char_16(wchar_t *c)
{
    return (uint16_t)*c;
}

static inline int from_char_32(wchar_t *c)
{
    return (uint32_t)*c;
}

static inline int swap_16(wchar_t *c)
{
    uint32_t res = (uint16_t)*c;
    return (0xff & res << 8) | (0xff & res >> 8);
}

static inline int swap_32(wchar_t *c)
{
    uint32_t res = (uint32_t)*c;
    return (0xff & (res << 24)) | (0xff00 & (res << 8)) | (0xff0000 & (res >> 8)) | (0xff000000 & (res >> 24));
}

typedef enum file_stream_mode_e {
    READ = 1,
    WRITE = 2,
    APPEND = 4
} file_stream_mode;

typedef enum file_stream_type_e {
    ASCII = 1,
    UNICODE_16 = 2,
    UNICODE_32 = 4,
} file_stream_type;

typedef struct file_stream_t
{
    // file data
    int32_t fd;
    uint8_t *file_path;
    size_t file_size;
    size_t file_ptr;
    // internal buffer data
    uint8_t *buffer;
    size_t buffer_ptr;
    size_t buffer_size;
    file_stream_mode mode;

} file_stream;

typedef struct bit_stream_t
{
    // file data
    union
    {
        file_stream animal;
    } base;
    uint64_t mask;
    uint64_t part;
} bit_stream;

#define declare_delim(name, char_size, delim_cb)                                                                  \
    size_t static name(file_stream *fs, uint8_t **line_start, int32_t delim_val)                                  \
    {                                                                                                             \
        size_t line_len = 0;                                                                                      \
    n_entry:                                                                                                      \
        for (size_t i = (fs->buffer_ptr + fs->buffer_size) - fs->file_ptr; i < fs->buffer_size; i += char_size) { \
            *line_start = &fs->buffer[i];                                                                         \
            if (delim_cb((wchar_t *)&fs->buffer[i]) == delim_val) {                                               \
                goto c_entry;                                                                                     \
            }                                                                                                     \
            fs->buffer_ptr += char_size;                                                                          \
        }                                                                                                         \
        if ((char_size + fs->buffer_ptr) > fs->file_ptr) {                                                        \
            if (sync_stream_read(fs, char_size) == 0) {                                                           \
                return 0;                                                                                         \
            }                                                                                                     \
        }                                                                                                         \
        goto n_entry;                                                                                             \
    c_entry:                                                                                                      \
        for (size_t i = (fs->buffer_ptr + fs->buffer_size) - fs->file_ptr; i < fs->buffer_size; i += char_size) { \
            if (delim_cb((wchar_t *)&fs->buffer[i]) != delim_val) {                                               \
                return line_len;                                                                                  \
            }                                                                                                     \
            fs->buffer_ptr += char_size;                                                                          \
            line_len++;                                                                                           \
        }                                                                                                         \
        if ((char_size + fs->buffer_ptr) > fs->file_ptr) {                                                        \
            if (sync_stream_read(fs, char_size) == 0) {                                                           \
                return line_len;                                                                                  \
            }                                                                                                     \
        }                                                                                                         \
        goto c_entry;                                                                                             \
    }

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

static void resize_buffer(file_stream *fs, size_t sm)
{
    /*
        we compute the span of the buffers range in
        os page sizes.

        When a size request reaches passed the current end of
        the buffer, we need to see if the size requested and
        the current page we are in will fit into our current buffer.

        Because to avoid needless copies, we just re-read
        the current page, but set the start of our buffer
        as that last page. Then we read in the remaining request,
        rounded up to the next os page size.

    */
    size_t head = prev_page_multiple(fs->buffer_ptr);
    size_t tail = next_page_multiple(sm + fs->buffer_ptr);
    size_t span = tail - head;
    size_t start_offset = fs->buffer_ptr - head;
    fs->buffer_ptr = head;

    // if we need more space for our buffer
    if (span > fs->buffer_size) {
        free(fs->buffer);
        fs->buffer = (uint8_t *)malloc(span);
        fs->buffer_size = span;
    }

    // do we need to rewind, because we are streaming
    // passed the end.
    if (start_offset > 0) {
        // rewind to our last page multiple
        lseek(fs->fd, -page_size, SEEK_CUR);
        fs->file_ptr -= page_size;
    }
}

static size_t sync_stream_read(file_stream *fs, size_t sm)
{
    /*
        Sync the stream and the internal buffer.
    */

    if (fs->file_ptr == fs->file_size) {
        return 0;
    }
    size_t next_size = fs->buffer_size;
    ssize_t opres = 0;

    if ((fs->buffer_ptr % fs->buffer_size) == 0) {
        /*
            The fast path when you are streaming fixed sizes
        */
        if (sm > fs->buffer_size) {
            /*
                A rare case where you are at the very border
                but the size is larger then the buffer.
            */
            resize_buffer(fs, sm);
        }
    } else {
        /*
            We still have data left in our current buffer.
            And want to stream the next part.
        */
        resize_buffer(fs, sm);
    }
    if ((fs->file_ptr + next_size) > fs->file_size) {
        next_size = fs->file_size - fs->file_ptr;
        fs->buffer_size = next_size;
    }
    // stream the next batch
    if ((opres = read(fs->fd, fs->buffer, next_size)) == -1) {
        return 0;
    }

    fs->file_ptr += opres;

    return opres;
}

static size_t sync_stream_write(file_stream *fs, size_t sm)
{
    /*
        Sync the stream and the internal buffer.
    */

    ssize_t opres = 0;
    // flush our buffer
    if ((opres = write(fs->fd, fs->buffer, fs->buffer_ptr - fs->file_ptr)) == -1) {
        return 0;
    }
    fs->file_ptr += opres;
    if ((fs->buffer_ptr % fs->buffer_size) == 0) {
        /*
            The fast path when you are streaming fixed sizes
        */
        if (sm > fs->buffer_size) {
            /*
                A rare case where you are at the very border
                but the size is larger then the buffer.
            */
            resize_buffer(fs, sm);
            fs->file_ptr -= page_size;
            if ((opres = read(fs->fd, fs->buffer, page_size)) == -1) {
                return 0;
            }
        }
    } else {
        /*
            We still have data left in our current buffer.
            And want to stream the next part.
        */
        resize_buffer(fs, sm);
        fs->file_ptr -= page_size;
        if ((opres = read(fs->fd, fs->buffer, page_size)) == -1) {
            return 0;
        }
    }

    return opres;
}
static file_stream *create_stream(uint8_t stream_size, const char *p, file_stream_mode mode)
{
    /*
        we use the buffer-less file IO. Because we are managing our own buffer.
    */
    int32_t fd = open(p, mode == READ ? O_RDONLY, S_IREAD : O_RDWR | O_CREAT | O_TRUNC,
        S_IREAD | S_IWRITE);

    if (fd == -1) {
        // Well that did not work out so well.
        return NULL;
    }

    // copy our path string into the stream
    uint32_t path_len = strlen(p) + 1;
    uint8_t *path_string = (uint8_t *)malloc(path_len);
    memcpy(path_string, p, path_len);
    path_string[path_len - 1] = 0;

    // create our stream
    file_stream *new_stream = (file_stream *)malloc(stream_size);
    new_stream->file_path = path_string;
    new_stream->fd = fd;
    new_stream->mode = mode;
    new_stream->buffer_size = alloc_size;
    new_stream->file_size = page_size;
    new_stream->buffer = (uint8_t *)malloc(new_stream->buffer_size);
    new_stream->buffer_ptr = 0;
    new_stream->file_ptr = 0;

    struct stat stats;
    if (fstat(fd, &stats) == 0) {
        new_stream->file_size = stats.st_size;
        if ((new_stream->file_size > 0) && (new_stream->file_size < new_stream->buffer_size)) {
            new_stream->buffer_size = new_stream->file_size;
        }
    }

    return new_stream;
}

file_stream *fs_open(const char *p, file_stream_mode mode)
{
    /*
        Create a file streaming object
    */
    return create_stream(sizeof(file_stream), p, mode);
}

void fs_flush(file_stream *fs)
{
    if (fs->mode & READ) {
        return;
    }

    ssize_t opres = 0;
    if ((opres = write(fs->fd, fs->buffer, fs->buffer_ptr - fs->file_ptr)) == -1) {
        return;
    }
    fs->file_ptr += opres;
}

void close_stream(file_stream *stream)
{
    // release our buffer and file descriptor
    if (stream) {
        fs_flush(stream);
        close(stream->fd);
        // release our heap stores
        free(stream->file_path);
        free(stream->buffer);
        free(stream);
    }
}

uint8_t *fs_read(file_stream *fs, size_t desired, size_t *result)
{
    /*
        This acts more as an allocator then a copy based data stream.
        Hand you a pointer to the start of your data.
    */

    *result = desired;
    if ((desired + fs->buffer_ptr) > fs->file_ptr) {
        /*
            Our buffer has reached its very end.
        */
        if (fs->file_ptr == fs->file_size) {

            // We have nothing yet to read from the actual file on disk.
            size_t rem_size = fs->file_size - fs->buffer_ptr;
            if (rem_size < desired) {
                // if there is something left to be fetched form the buffer
                // we correct the desired size.
                *result = rem_size;
                if (rem_size == 0) {
                    // well, we are completely empty
                    return 0;
                }
            }
        } else {
            if (sync_stream_read(fs, desired) == 0) {
                return 0;
            }
        }
    }

    // otherwise, jsut grab the value from the buffer
    uint8_t *res = &fs->buffer[(fs->buffer_ptr + fs->buffer_size) - fs->file_ptr];
    fs->buffer_ptr += *result;
    return res;
}

uint8_t *fs_write(file_stream *fs, size_t sm)
{
    /*
        This acts more as an allocator then a copy based data stream.
        Hand you a pointer to the start of your data.
    */
    if ((sm + fs->buffer_ptr) > (fs->file_ptr + fs->buffer_size)) {
        /*
            Our buffer has reached its very end.
        */
        if (sync_stream_write(fs, sm) == 0) {
            return 0;
        }
    }

    // otherwise, jsut grab the value from the buffer
    uint8_t *res = &fs->buffer[fs->buffer_ptr - fs->file_ptr];
    fs->buffer_ptr += sm;
    return res;
}

declare_delim(fs_read_line_8, 1, is_eol_8);
declare_delim(fs_read_line_16, 2, is_eol_16);
declare_delim(fs_read_line_32, 4, is_eol_32);
size_t fs_read_line(file_stream *fs, uint8_t **line_start, file_stream_type st)
{
    switch (st) {
    case ASCII:
        return fs_read_line_8(fs, line_start, 1);
    case UNICODE_16:
        return fs_read_line_16(fs, line_start, 1);
    default:
        return fs_read_line_32(fs, line_start, 1);
    }
}

declare_delim(fs_get_delim_8, 1, from_char_8);
declare_delim(fs_get_delim_16, 2, from_char_16);
declare_delim(fs_get_delim_32, 4, from_char_32);
size_t fs_get_delim(file_stream *fs, uint8_t **line_start, int32_t delim, file_stream_type st)
{
    switch (st) {
    case ASCII:
        return fs_get_delim_8(fs, line_start, delim);
    case UNICODE_16:
        return fs_get_delim_16(fs, line_start, delim);
    default:
        return fs_get_delim_32(fs, line_start, delim);
    }
}

int64_t fs_tell(file_stream *fs)
{
    return fs->buffer_ptr;
}

int64_t fs_seek(file_stream *fs, int32_t offset, int32_t whence)
{
    fs_flush(fs);
    ssize_t opres = 0;
    if ((opres = lseek(fs->fd, offset, whence)) == -1) {
        return -1;
    }
    // if we jumped back to the beginning
    if (opres == 0) {
        // just reset and return
        fs->file_ptr = 0;
        fs->buffer_ptr = 0;
        return 0;
    }
    size_t idx_offset = 0;
    if (!(fs->mode & WRITE)) {
        // if we jumped to the end
        if (opres == (ssize_t)fs->file_size) {
            // we have a fixed size if we are reading.
            fs->file_ptr = opres;
            fs->buffer_ptr = opres;
            return opres;
        }
        offset = fs->buffer_size;
    }
    ssize_t next_buffer_pos = (opres + idx_offset) - fs->file_ptr;

    fs->buffer_ptr = opres;
    if (next_buffer_pos >= 0 && next_buffer_pos < (ssize_t)fs->buffer_size) {
        // we have moved but are still within our current buffer.
        return opres;
    }
    if ((opres % page_size) == 0) {
        fs->file_ptr = opres;
        return opres;
    }

    //
    size_t head = prev_page_multiple(opres);
    if ((opres = lseek(fs->fd, head, SEEK_SET)) == -1) {
        return -1;
    }
    fs->file_ptr = opres;
    if ((opres = read(fs->fd, fs->buffer, fs->buffer_size)) == -1) {
        return -1;
    }
    fs->buffer_size = opres;
    if (fs->mode & READ) {
        // move the file head
        if ((opres = lseek(fs->fd, fs->buffer_size, SEEK_CUR)) == -1) {
            return -1;
        }
        fs->file_ptr = opres;
    }
    return fs->buffer_ptr;
}

bit_stream *bs_open(const char *p, file_stream_mode mode)
{
    bit_stream *bs = (bit_stream *)create_stream(sizeof(bit_stream), p, mode);
    bs->mask = 0;
    bs->part = 0;
    return bs;
}

uint8_t bs_write(bit_stream *bs, int bit)
{
    if (bit)
        bs->part |= bs->mask;
    bs->mask = bs->mask >> 1;
    if (bs->mask == 0) {
        *(uint64_t *)fs_write((file_stream *)bs, sizeof(uint64_t)) = bs->part;
        bs->part = 0;
        bs->mask = partmask;
        return 1;
    }
    return 0;
}

uint32_t bs_read(bit_stream *bs)
{
    int result = 0;
    if (bs->mask == partmask) {
        size_t expected = 0;
        bs->part = *(uint64_t *)fs_read((file_stream *)bs, sizeof(uint64_t), &expected);
    }
    result = bs->part & bs->mask;
    bs->mask = bs->mask >> 1;
    if (bs->mask == 0) {
        bs->mask = partmask;
    }
    return result;
}
