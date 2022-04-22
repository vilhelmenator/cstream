#ifndef _CSTREAM_H
#define _CSTREAM_H

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

#include "../ctest/ctest.h"
CLOGGER(_CSTREAM_H, 4096)

const static uint64_t page_size = 4096;
const static uint64_t alloc_size = page_size * 8;
const static uint64_t partmask = 0x80000000000000;

#define next_page_multiple(s) ((s + (page_size - 1)) & ~(page_size - 1))
#define prev_page_multiple(s) (s & ~(page_size - 1))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

static inline int is_eol_8(uint8_t *c)
{
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

static inline int is_eol_16(uint8_t *c)
{
    uint32_t c_value = (uint16_t)*c;

    switch (c_value) {
    case 0x0000: /* term */
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
    case 0xC080: /* modified utf8 */
    case 0x80C0: /* modified utf8 */
        return 1;

    default:
        return 0;
    };
}

static inline int is_eol_32(uint8_t *c)
{
    //
    // null terminator C0 80 in some odd java unicode world
    //
    uint32_t c_value = (uint32_t)*c;

    switch (c_value) {
    case 0x00000000: /* term */
    case 0x00000005: /*eof*/
    case 0x0000000A: /*\n*/
    case 0x0000000B: /*vertical tab */
    case 0x0000000C: /*form feed */
    case 0x0000000D: /*\r*/
    case 0x0000C080: /* modified utf8 */
    case 0xC0800000: /* modified utf8 */
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

static inline int from_char_8(uint8_t *c)
{
    return (uint8_t)*c;
}

static inline int from_char_16(uint8_t *c)
{
    return (uint16_t)*c;
}

static inline int from_char_32(uint8_t *c)
{
    return (uint32_t)*c;
}

static inline int swap_16(uint8_t *c)
{
    uint32_t res = (uint16_t)*c;
    return (0xff & res << 8) | (0xff & res >> 8);
}

static inline int swap_32(uint8_t *c)
{
    uint32_t res = (uint32_t)*c;
    return (0xff & (res << 24)) | (0xff00 & (res << 8)) | (0xff0000 & (res >> 8)) | (0xff000000 & (res >> 24));
}

typedef enum file_stream_mode_e {
    READ = 1,
    WRITE = 2
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
    union // C inheritance trick
    {
        file_stream animal;
    } base;
    uint64_t mask;
    uint64_t part;
} bit_stream;

// test declarations
#define declare_delim_found_nl_at_start __LINE__
#define declare_delim_failed_sync_at_start __LINE__
#define declare_delim_success_sync_at_start __LINE__
#define declare_delim_found_nl_at_end __LINE__
#define declare_delim_failed_sync_at_end __LINE__
#define declare_delim_success_sync_at_end __LINE__
#define declare_delim(name, char_size, delim_cb)                                                                  \
    size_t static name(file_stream *fs, uint8_t **line_start, int32_t delim_val)                                  \
    {                                                                                                             \
        size_t line_len = 0;                                                                                      \
    n_entry:                                                                                                      \
        for (size_t i = (fs->buffer_ptr + fs->buffer_size) - fs->file_ptr; i < fs->buffer_size; i += char_size) { \
            *line_start = &fs->buffer[i];                                                                         \
            if (delim_cb(&fs->buffer[i]) == delim_val) {                                                          \
                log_define(_CSTREAM_H, declare_delim_found_nl_at_start);                                          \
                goto c_entry;                                                                                     \
            }                                                                                                     \
            fs->buffer_ptr += char_size;                                                                          \
        }                                                                                                         \
        if ((char_size + fs->buffer_ptr) > fs->file_ptr) {                                                        \
            if (sync_stream_read(fs, char_size) == 0) {                                                           \
                log_define(_CSTREAM_H, declare_delim_failed_sync_at_start);                                       \
                return 0;                                                                                         \
            }                                                                                                     \
            log_define(_CSTREAM_H, declare_delim_success_sync_at_start);                                          \
        }                                                                                                         \
        goto n_entry;                                                                                             \
    c_entry:                                                                                                      \
        for (size_t i = (fs->buffer_ptr + fs->buffer_size) - fs->file_ptr; i < fs->buffer_size; i += char_size) { \
            if (delim_cb(&fs->buffer[i]) != delim_val) {                                                          \
                log_define(_CSTREAM_H, declare_delim_found_nl_at_end);                                            \
                return line_len;                                                                                  \
            }                                                                                                     \
            fs->buffer_ptr += char_size;                                                                          \
            line_len++;                                                                                           \
        }                                                                                                         \
        if ((char_size + fs->buffer_ptr) > fs->file_ptr) {                                                        \
            if (sync_stream_read(fs, char_size) == 0) {                                                           \
                log_define(_CSTREAM_H, declare_delim_failed_sync_at_end);                                         \
                return line_len;                                                                                  \
            }                                                                                                     \
            log_define(_CSTREAM_H, declare_delim_success_sync_at_end);                                            \
        }                                                                                                         \
        goto c_entry;                                                                                             \
    }

#define fs_flush_wrong_file_mode __LINE__
#define fs_flush_failed_to_write __LINE__
#define fs_flush_success __LINE__
void fs_flush(file_stream *fs)
{
    if (fs->mode & READ) {
        log_define(_CSTREAM_H, fs_flush_wrong_file_mode);
        return;
    }

    ssize_t opres = 0;
    if ((opres = write(fs->fd, fs->buffer, fs->buffer_ptr - fs->file_ptr)) == -1) {
        log_define(_CSTREAM_H, fs_flush_failed_to_write);
        return;
    }
    fs->file_ptr += opres;
    log_define(_CSTREAM_H, fs_flush_success);
}

#define fs_seek_lseek_failed __LINE__
#define fs_seek_to_start __LINE__
#define fs_seek_to_end __LINE__
#define fs_seek_buffer_offset_on_read __LINE__
#define fs_seek_buffer_still_in_view __LINE__
#define fs_seek_buffer_pos_aligned_to_os_page __LINE__
#define fs_seek_buffer_seek_to_prev_head_failed __LINE__
#define fs_seek_failed_to_read_at_new_head __LINE__
#define fs_seek_failed_to_reset_on_read __LINE__
#define fs_seek_success_to_reset_on_read __LINE__
int64_t fs_seek(file_stream *fs, int32_t offset, int32_t whence)
{
    fs_flush(fs);
    ssize_t opres = 0;
    if ((opres = lseek(fs->fd, offset, whence)) == -1) {
        // seek failed
        log_define(_CSTREAM_H, fs_seek_lseek_failed);
        return -1;
    }
    // if we jumped back to the beginning
    if (opres == 0) {
        // just reset and return
        log_define(_CSTREAM_H, fs_seek_to_start);
        fs->file_ptr = 0;
        fs->buffer_ptr = 0;
        return 0;
    }
    size_t idx_offset = 0;
    if (!(fs->mode & WRITE)) {
        // if we jumped to the end
        if (opres == (ssize_t)fs->file_size) {
            log_define(_CSTREAM_H, fs_seek_to_end);
            // we have a fixed size if we are reading.
            fs->file_ptr = opres;
            fs->buffer_ptr = opres;
            return opres;
        }
        offset = fs->buffer_size;
        log_define(_CSTREAM_H, fs_seek_buffer_offset_on_read);
    }
    ssize_t next_buffer_pos = (opres + idx_offset) - fs->file_ptr;

    fs->buffer_ptr = opres;
    if (next_buffer_pos >= 0 && next_buffer_pos < (ssize_t)fs->buffer_size) {
        // we have moved but are still within our current buffer.
        log_define(_CSTREAM_H, fs_seek_buffer_still_in_view);
        return opres;
    }
    if ((opres % page_size) == 0) {
        fs->file_ptr = opres;
        log_define(_CSTREAM_H, fs_seek_buffer_pos_aligned_to_os_page);
        return opres;
    }

    //
    size_t head = prev_page_multiple(opres);
    if ((opres = lseek(fs->fd, head, SEEK_SET)) == -1) {
        log_define(_CSTREAM_H, fs_seek_buffer_seek_to_prev_head_failed);
        return -1;
    }
    fs->file_ptr = opres;
    if ((opres = read(fs->fd, fs->buffer, fs->buffer_size)) == -1) {
        log_define(_CSTREAM_H, fs_seek_failed_to_read_at_new_head);
        return -1;
    }
    fs->buffer_size = opres;
    if (fs->mode & READ) {
        // move the file head
        if ((opres = lseek(fs->fd, fs->buffer_size, SEEK_CUR)) == -1) {
            log_define(_CSTREAM_H, fs_seek_failed_to_reset_on_read);
            return -1;
        }
        fs->file_ptr = opres;
        log_define(_CSTREAM_H, fs_seek_success_to_reset_on_read);
    }
    return fs->buffer_ptr;
}

#define create_stream_open_failed __LINE__
#define create_stream_buffer_resized __LINE__
static file_stream *create_stream(uint8_t stream_size, const char *p, file_stream_mode mode)
{
    /*
        we use the buffer-less file IO. Because we are managing our own buffer.
    */
    int32_t fd = open(p, mode == READ ? O_RDONLY, S_IREAD : O_RDWR | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
    if (fd == -1) {
        // Well that did not work out so well.
        log_define(_CSTREAM_H, create_stream_open_failed);
        return NULL;
    }

    // create our stream
    file_stream *new_stream = (file_stream *)malloc(stream_size);
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
            //
            log_define(_CSTREAM_H, create_stream_buffer_resized);
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

#define close_stream_valid_stream __LINE__
void close_stream(file_stream *stream)
{
    // release our buffer and file descriptor
    if (stream) {
        log_define(_CSTREAM_H, close_stream_valid_stream);
        fs_flush(stream);
        close(stream->fd);
        // release our heap stores
        free(stream->buffer);
        free(stream);
    }
}

#define resize_buffer_needed_resize __LINE__
#define resize_buffer_needed_seek __LINE__
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
        log_define(_CSTREAM_H, resize_buffer_needed_resize);
        free(fs->buffer);
        fs->buffer = (uint8_t *)malloc(span);
        fs->buffer_size = span;
    }

    // do we need to rewind, because we are streaming
    // passed the end.
    if (start_offset > 0) {
        log_define(_CSTREAM_H, resize_buffer_needed_seek);
        // rewind to our last page multiple
        lseek(fs->fd, -page_size, SEEK_CUR);
        fs->file_ptr -= page_size;
    }
}

#define sync_stream_read_file_done __LINE__
#define sync_stream_read_size_larger_than_buffer_size __LINE__
#define sync_stream_read_aligned __LINE__
#define sync_stream_read_misaligned __LINE__
#define sync_stream_read_remaining_size_adjusted __LINE__
#define sync_stream_read_failed_to_read __LINE__
static size_t sync_stream_read(file_stream *fs, size_t sm)
{
    /*
        Sync the stream and the internal buffer.
    */

    if (fs->file_ptr == fs->file_size) {
        log_define(_CSTREAM_H, sync_stream_read_file_done);
        return 0;
    }
    size_t next_size = fs->buffer_size;
    ssize_t opres = 0;

    if ((fs->buffer_ptr % fs->buffer_size) == 0) {
        /*
            The fast path when you are streaming fixed sizes
        */
        log_define(_CSTREAM_H, sync_stream_read_aligned);
        if (sm > fs->buffer_size) {
            /*
                A rare case where you are at the very border
                but the size is larger then the buffer.
            */
            log_define(_CSTREAM_H, sync_stream_read_size_larger_than_buffer_size);
            resize_buffer(fs, sm);
        }
    } else {
        /*
            We still have data left in our current buffer.
            And want to stream the next part.
        */
        log_define(_CSTREAM_H, sync_stream_read_misaligned);
        resize_buffer(fs, sm);
    }
    if ((fs->file_ptr + next_size) > fs->file_size) {
        log_define(_CSTREAM_H, sync_stream_read_remaining_size_adjusted);
        next_size = fs->file_size - fs->file_ptr;
        fs->buffer_size = next_size;
    }
    // stream the next batch
    if ((opres = read(fs->fd, fs->buffer, next_size)) == -1) {
        log_define(_CSTREAM_H, sync_stream_read_failed_to_read);
        return 0;
    }

    fs->file_ptr += opres;

    return opres;
}

#define sync_stream_write_failed_to_write __LINE__
#define sync_stream_write_aligned __LINE__
#define sync_stream_write_misaligned __LINE__
#define sync_stream_write_failed_to_read __LINE__
static size_t sync_stream_write(file_stream *fs, size_t sm)
{
    /*
        Sync the stream and the internal buffer.
    */

    ssize_t opres = 0;
    // flush our buffer
    if ((opres = write(fs->fd, fs->buffer, fs->buffer_ptr - fs->file_ptr)) == -1) {
        log_define(_CSTREAM_H, sync_stream_write_failed_to_write);
        return 0;
    }
    fs->file_ptr += opres;
    if ((fs->buffer_ptr % fs->buffer_size) == 0) {
        /*
            The fast path when you are streaming fixed sizes
        */
        log_define(_CSTREAM_H, sync_stream_write_aligned);
        if (sm > fs->buffer_size) {
            /*
                A rare case where you are at the very border
                but the size is larger then the buffer.
            */
            resize_buffer(fs, sm);
            fs->file_ptr -= page_size;
            if ((opres = read(fs->fd, fs->buffer, page_size)) == -1) {
                log_define(_CSTREAM_H, sync_stream_write_failed_to_read);
                return 0;
            }
        }
    } else {
        /*
            We still have data left in our current buffer.
            And want to stream the next part.
        */
        log_define(_CSTREAM_H, sync_stream_write_misaligned);
        resize_buffer(fs, sm);
        fs->file_ptr -= page_size;
        if ((opres = read(fs->fd, fs->buffer, page_size)) == -1) {
            log_define(_CSTREAM_H, sync_stream_write_failed_to_read);
            return 0;
        }
    }

    return opres;
}

#define fs_read_fill_buffer __LINE__
#define fs_read_failed_to_fill_buffer __LINE__
#define fs_read_file_exhausted __LINE__
#define fs_read_trunc_to_remaining_size __LINE__
#define fs_read_remaining_size_zero __LINE__
uint8_t *fs_read(file_stream *fs, size_t desired, size_t *result)
{
    /*
        This acts more as an allocator then a copy based data stream.
        Hand you a pointer to the start of your data.
    */

    *result = desired;
    if ((desired + fs->buffer_ptr) > fs->file_ptr) {
        log_define(_CSTREAM_H, fs_read_fill_buffer);
        /*
            Our buffer has reached its very end.
        */
        if (fs->file_ptr == fs->file_size) {
            log_define(_CSTREAM_H, fs_read_file_exhausted);
            // We have nothing yet to read from the actual file on disk.
            size_t rem_size = fs->file_size - fs->buffer_ptr;
            if (rem_size < desired) {
                log_define(_CSTREAM_H, fs_read_trunc_to_remaining_size);
                // if there is something left to be fetched form the buffer
                // we correct the desired size.
                *result = rem_size;
                if (rem_size == 0) {
                    log_define(_CSTREAM_H, fs_read_remaining_size_zero);
                    // well, we are completely empty
                    return 0;
                }
            }
        } else {
            if (sync_stream_read(fs, desired) == 0) {
                log_define(_CSTREAM_H, fs_read_failed_to_fill_buffer);
                return 0;
            }
        }
    }

    // otherwise, jsut grab the value from the buffer
    uint8_t *res = &fs->buffer[(fs->buffer_ptr + fs->buffer_size) - fs->file_ptr];
    fs->buffer_ptr += *result;
    return res;
}

#define fs_write_flush_buffer __LINE__
#define fs_write_failed_to_flush_buffer __LINE__
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
        log_define(_CSTREAM_H, fs_write_flush_buffer);
        if (sync_stream_write(fs, sm) == 0) {
            log_define(_CSTREAM_H, fs_write_failed_to_flush_buffer);
            return 0;
        }
    }

    // otherwise, jsut grab the value from the buffer
    uint8_t *res = &fs->buffer[fs->buffer_ptr - fs->file_ptr];
    fs->buffer_ptr += sm;
    return res;
}

#define fs_read_line_ASCII __LINE__
#define fs_read_line_UNICODE_16 __LINE__
#define fs_read_line_UNICODE_32 __LINE__
declare_delim(fs_read_line_8, 1, is_eol_8);
declare_delim(fs_read_line_16, 2, is_eol_16);
declare_delim(fs_read_line_32, 4, is_eol_32);
size_t fs_read_line(file_stream *fs, uint8_t **line_start, file_stream_type st)
{
    switch (st) {
    case ASCII: {
        log_define(_CSTREAM_H, fs_read_line_ASCII);
        return fs_read_line_8(fs, line_start, 1);
    }
    case UNICODE_16: {
        log_define(_CSTREAM_H, fs_read_line_UNICODE_16);
        return fs_read_line_16(fs, line_start, 1);
    }
    default: {
        log_define(_CSTREAM_H, fs_read_line_UNICODE_32);
        return fs_read_line_32(fs, line_start, 1);
    }
    }
}

#define fs_get_delim_ASCII __LINE__
#define fs_get_delim_UNICODE_16 __LINE__
#define fs_get_delim_UNICODE_32 __LINE__
declare_delim(fs_get_delim_8, 1, from_char_8);
declare_delim(fs_get_delim_16, 2, from_char_16);
declare_delim(fs_get_delim_32, 4, from_char_32);
size_t fs_get_delim(file_stream *fs, uint8_t **line_start, int32_t delim, file_stream_type st)
{
    switch (st) {
    case ASCII: {
        log_define(_CSTREAM_H, fs_get_delim_ASCII);
        return fs_get_delim_8(fs, line_start, delim);
    }
    case UNICODE_16: {
        log_define(_CSTREAM_H, fs_get_delim_UNICODE_16);
        return fs_get_delim_16(fs, line_start, delim);
    }
    default: {
        log_define(_CSTREAM_H, fs_get_delim_UNICODE_32);
        return fs_get_delim_32(fs, line_start, delim);
    }
    }
}

int64_t fs_tell(file_stream *fs)
{
    return fs->buffer_ptr;
}

bit_stream *bs_open(const char *p, file_stream_mode mode)
{
    bit_stream *bs = (bit_stream *)create_stream(sizeof(bit_stream), p, mode);
    bs->mask = 0;
    bs->part = 0;
    return bs;
}

#define bs_write_flush __LINE__
#define bs_write_bit_set __LINE__
uint8_t bs_write(bit_stream *bs, int bit)
{
    if (bit) {
        log_define(_CSTREAM_H, bs_write_bit_set);
        bs->part |= bs->mask;
    }

    bs->mask = bs->mask >> 1;
    if (bs->mask == 0) {
        log_define(_CSTREAM_H, bs_write_flush);
        *(uint64_t *)fs_write((file_stream *)bs, sizeof(uint64_t)) = bs->part;
        bs->part = 0;
        bs->mask = partmask;
        return 1;
    }
    return 0;
}

#define bs_read_fill __LINE__
#define bs_read_mask_reset __LINE__
uint32_t bs_read(bit_stream *bs)
{
    int result = 0;
    if (bs->mask == partmask) {
        log_define(_CSTREAM_H, bs_read_fill);
        size_t expected = 0;
        bs->part = *(uint64_t *)fs_read((file_stream *)bs, sizeof(uint64_t), &expected);
    }
    result = bs->part & bs->mask;
    bs->mask = bs->mask >> 1;
    if (bs->mask == 0) {
        log_define(_CSTREAM_H, bs_read_mask_reset);
        bs->mask = partmask;
    }
    return result;
}

#endif // _CSTREAM_H