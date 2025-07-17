#define CTEST_ENABLED
#include "cstream.h"

#if defined(WINDOWS)
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#else
#include <execinfo.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

void clearScreen()
{
    write(STDOUT_FILENO, "\x1b[?25l", 6);
    write(STDOUT_FILENO, "\x1b[H", 3);
}

int getCursorPosition(int *rows, int *cols)
{
    char buf[32];
    unsigned int i = 0;
    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
        return -1;
    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1)
            break;
        if (buf[i] == 'R')
            break;
        i++;
    }
    buf[i] = '\0';
    if (buf[0] != '\x1b' || buf[1] != '[')
        return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2)
        return -1;
    return 0;
}

int getWindowSize(int *rows, int *cols)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
            return -1;
        return getCursorPosition(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

int32_t debug_step(const char *filename, int32_t linenr, char *format, ...)
{
#if !defined(_MSC_VER)
    struct termios orig_termios;
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw_term = orig_termios;
    raw_term.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw_term.c_cflag |= (CS8);
    raw_term.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw_term.c_cc[VMIN] = 1;  // wait until at least one keystroke available
    raw_term.c_cc[VTIME] = 0; // no timeout
    tcsetattr(0, TCSANOW, &raw_term); // set immediately
#endif

    va_list myargs;
    va_start(myargs, format);
    FILE *f = fopen(filename, "r");

    size_t line_size = 1024;
    char line[1024];
    char prebuff[1024];
    char *line_ptr = &line[0];
    int32_t line_nr = 0;
    // clearScreen();
    printf("\n\x1b[48;5;239m");
    printf("%s : %d\n", filename, linenr);
    vprintf(format, myargs);
    printf("\x1b[48;5;0m\n");

    while (getline(&line_ptr, &line_size, f) != -1) {
        line_nr++;
        int32_t dist = abs(line_nr - linenr);
        if (dist <= 8) {
            if (dist == 0) {
                printf("\x1b[48;5;239m");
                printf("->");
                printf("\x1b[48;5;0m");
            }
            printf("%s", line);
        }
        if (line_nr > (linenr + 3)) {
            break;
        }
    }
    va_end(myargs);
    char c = getchar();
#if !defined(_MSC_VER)
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
#endif
    switch (c) {
    case 27: {
        exit(1);
    }
    default:
        break;
    }
    return 0;
}

void signal_handler(int sig)
{
    void *array[100];
    size_t size;

    size = backtrace(array, 100);

    fprintf(stderr, "error %d\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}
#define DEBUG_INIT()                                                           \
    do {                                                                       \
        signal(SIGSEGV, signal_handler);                                       \
        CLOGGER_SET_CALLBACK(_CSTREAM_H, debug_step);                          \
    } while (0)

int file_read_line(FILE *fd, char *buff, size_t s)
{
    return getline(&buff, &s, fd) != -1;
}

void gen_test_file(char *filename, ssize_t size)
{
    int32_t nr_lines = size / 128;
    int32_t rem = size % 128;
    // 128 bytes per line.
    char buff[128];
    FILE *f = fopen(filename, "w");
    for (int i = 0; i < nr_lines; i++) {
        fprintf(f, "%08d %0118d\n", i + 1, 0);
    }
    fclose(f);
}
void gen_test_files()
{
    gen_test_file("test_1k.txt", 1024);
    gen_test_file("test_4k.txt", 1024 * 4);
    gen_test_file("test_8k.txt", 1024 * 8);
    gen_test_file("test_32k.txt", 1024 * 32);
    gen_test_file("test_64k.txt", 1024 * 64);
    gen_test_file("test_128k.txt", 1024 * 128);
    gen_test_file("test_256k.txt", 1024 * 256);
    gen_test_file("test_512k.txt", 1024 * 512);
    gen_test_file("test_1m.txt", 1024 * 1024);
    gen_test_file("test_10m.txt", 1024 * 1024 * 10);
    gen_test_file("test_50m.txt", 1024 * 1024 * 50);
    gen_test_file("test_100m.txt", 1024 * 1024 * 100);
}
int main()
{
    gen_test_file("test_10m.txt", 1024 * 1024 * 10);
    const char *test_file_path = "test_10m.txt"; // some 100 megabyte text file

    // DEBUG_INIT();
    // [x] Gen test files
    // [ ] test lseek
    //   - lseek read. lseek write.
    // [ ] high level functions. loadall, writeall
    // [ ] test writing larger than buffer size.
    // [ ] test reading larger than buffer size.
    // [ ] test various file sizes.
    // [ ] test bit stream.
    // [ ] run on windows
    // [ ] compare against streamer implementations.
    // -   - standard C++ and C streaming objects
    // [ ] done!
    START_TEST(stream, {});

    struct stat stats;
    int fd = open(test_file_path, O_RDONLY, S_IREAD);
    int32_t status = fstat(fd, &stats);
    printf("file size %lli\n", stats.st_size);
    char *bu = (char *)malloc(stats.st_size);

    read(fd, bu, stats.st_size);
    close(fd);

    fd = open(test_file_path, O_RDONLY, S_IREAD);
    MEASURE_TIME(stream, file_read_whole, { read(fd, bu, stats.st_size); });
    free(bu);
    close(fd);

    file_stream *fs = fs_open(test_file_path, "r");
    MEASURE_TIME(stream, file_stream_128bytes, {
        size_t expected = 128;
        while (fs_read(fs, 128, &expected) != NULL) {
        }
    });
    close_stream(fs);

    fs = fs_open(test_file_path, "r");
    size_t expected = 8;
    MEASURE_TIME(stream, file_stream_8bytes, {
        while (fs_read(fs, 8, &expected) != NULL) {
        }
    });
    close_stream(fs);

    fs = fs_open(test_file_path, "r");
    expected = 6;
    MEASURE_TIME(stream, file_stream_6bytes, {
        while (fs_read(fs, 6, &expected) != NULL) {
        }
    });
    close_stream(fs);

    char buff[8];
    FILE *f = fopen(test_file_path, "rb");
    MEASURE_TIME(stream, file_read_8bytes, {
        while (fread(&buff, 8, 1, f)) {
        }
    });
    fclose(f);

    fs = fs_open(test_file_path, "r");
    MEASURE_TIME(stream, file_stream_1byte, {
        size_t expected = 64;
        uint8_t *res = fs_read(fs, 64, &expected);
        char c = 0;
        while (res != NULL) {
            for (int i = 0; i < expected; i++) {
                c = *((char *)res + i);
            }
            res = fs_read(fs, 64, &expected);
        }
    });
    close_stream(fs);

    f = fopen(test_file_path, "rb");
    MEASURE_TIME(stream, file_read_1byte, {
        while (fgetc(f) != EOF) {
        }
    });
    fclose(f);

    fs = fs_open(test_file_path, "r");
    char *line = 0;
    MEASURE_TIME(stream, file_stream_read_line, {
        while (fs_read_line(fs, (uint8_t **)&line, ASCII)) {
        }
    });
    close_stream(fs);

    char lbuff[1024];
    f = fopen(test_file_path, "rb");
    MEASURE_TIME(stream, file_read_line, {
        while (file_read_line(f, lbuff, 1024)) {
        }
    });
    fclose(f);

    fs = fs_open(test_file_path, "r");
    file_stream *ofs = fs_open("out.txt", "w");
    if(ofs == NULL)
    {

    }
    MEASURE_TIME(stream, file_stream_read_write, {
        size_t expected = 8;
        uint8_t *buff = fs_read(fs, 8, &expected);
        while (buff) {
            *(uint64_t *)fs_write(ofs, expected) = *(uint64_t *)buff;
            buff = fs_read(fs, 8, &expected);
        }
    });
    close_stream(ofs);
    close_stream(fs);

    fd = open(test_file_path, O_RDONLY, S_IREAD);
    status = fstat(fd, &stats);
    int32_t num_bytes = stats.st_size;
    bu = (char *)malloc(num_bytes);
    read(fd, bu, num_bytes);
    close(fd);

    int ofd = open("out.txt", O_RDWR | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
    MEASURE_TIME(stream, file_write, { write(ofd, bu, num_bytes); });
    close(ofd);
    ofs = fs_open("out.txt", "w");
    MEASURE_TIME(stream, file_stream_write8, {
        for (int i = 0; i < num_bytes; i += 8) {
            (*(uint64_t *)fs_write(ofs, 8)) = *(uint64_t *)(char *)(bu + i);
        }
    });
    close_stream(ofs);
    ofs = fs_open("out.txt", "w");
    MEASURE_TIME(stream, file_stream_write6, {
        for (int i = 0; i < num_bytes; i += 6) {
            (*(uint64_t *)fs_write(ofs, 6)) = *(uint64_t *)(char *)(bu + i);
        }
    });
    close_stream(ofs);
    free(bu);

    fs = fs_open("utf8.txt", "r");
    wchar_t *wline = 0;
    int line_len = 0;

    MEASURE_TIME(stream, file_stream_read_line, {
        do {
            line_len = fs_read_line(fs, (uint8_t **)&wline, UNICODE_16);
        } while (line_len != 0);
    });
    close_stream(fs);

    END_TEST(stream, {});

    //
    // read 6 bytes at a time.
    // see that the internals are being hit.
    //
    // [ ] read write odd sizes.
    // [ ] read zero size files.
    // [ ] reading writing sizes larger than buffer.
    // [ ] writing zero sizes.
    // [ ] writing lines.
    // [ ] seek while reading.
    // [ ] seek while writing.
    // [ ] test currupt text files.
    // [ ] compare output files.
    // [ ] reading small files.
    // [ ] writing small files.
    // [ ] compare perf of various streamers.
    // [ ] character delimeter test.
    // [ ] bit stream test.
    // [ ] high level operations.
    //      - read the whole file into a single large buffer.
    //      - write a single large buffer into a file.
    // [ ] run on windows
    // done!

    return 0;
}