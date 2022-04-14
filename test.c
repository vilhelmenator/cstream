
#include <signal.h>
#include <execinfo.h>
#include "cstream.h"

#define CTEST_ENABLED
#include "ctest/ctest.h"


int file_read_line(FILE *fd, char *buff, size_t s)
{
    char c;
    int offset = 0;
    do { // read one line
        c = fgetc(fd);
        if (c != EOF) {
            if (offset < s) {
                buff[offset++] = c;
            }
        }
    } while (c != EOF && c != '\n');
    buff[offset - 1] = 0;
    if (c == EOF) {
        return 0;
    }
    return 1;
}

void handler(int sig)
{
    void* array[100];
    size_t size;

    size = backtrace(array, 100);

    fprintf(stderr, "error %d\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}
int main()
{
    //
    //
    //
    //
    
    //[x] resizing for input
    //  need to seek, because the file ptr is in front.
    //

    //
    //[ ] resizing for output
    //  need to write the buffer, read the last page into the first page.
    //  flush()
    //

    //
    //  seek.
    //      - read_mode
    //      - write_mode
    //

    /*
    fs_write()
    fs_read()
    fs_close()
    fs_open()
    fs_getc()
    fs_putc()
    fs_tell()
    fs_seek()
    fs_stat()
*/
    START_TEST(stream, {});

    signal(SIGSEGV, handler);
    
    int fd = open("//Users/vilhelmsaevarsson/Documents/Thingi10K/raw_meshes/994785.obj", O_RDONLY, S_IREAD);
    struct stat stats;
    int32_t status = fstat(fd, &stats);
    printf("file size %lli\n", stats.st_size);
    char* bu = (char*)malloc(stats.st_size);
    MEASURE_MS(stream, file_read_whole, {
        
        read(fd, bu, stats.st_size);
        
    });
    free(bu);
    close(fd);

    
    file_stream *fs = open_stream("//Users/vilhelmsaevarsson/Documents/Thingi10K/raw_meshes/994785.obj", READ);
    /*
    MEASURE_MS(stream, file_stream_128bytes, {
        int prev_loc = 0;
        size_t expected = 128;
        while(read_stream(fs, 128, &expected) != NULL){
            //printf("location %zu", fs->file_ptr);
            
        }
        
    });
    close_stream(fs);

    fs = open_stream("//Users/vilhelmsaevarsson/Documents/Thingi10K/raw_meshes/994785.obj", READ);
    
    MEASURE_MS(stream, file_stream_8bytes, {
        int prev_loc = 0;
        size_t expected = 8;
        while(read_stream(fs, 8, &expected) != NULL){
            //printf("location %zu", fs->file_ptr);
            
        }
        
    });

    close_stream(fs);
    char buff[8];
    FILE* f = fopen("//Users/vilhelmsaevarsson/Documents/Thingi10K/raw_meshes/994785.obj", "rb");
    MEASURE_MS(stream, file_read_8bytes, {
    while(fread(&buff, 8, 1, f))
    {

    }
    });
    fclose(f);

    fs = open_stream("//Users/vilhelmsaevarsson/Documents/Thingi10K/raw_meshes/994785.obj", READ);
    
    MEASURE_MS(stream, file_stream_1byte, {
        int prev_loc = 0;
        size_t expected = 1;
        while(read_stream(fs, 1, &expected) != NULL){
            //printf("location %zu", fs->file_ptr);
            
        }
        
    });

    
    close_stream(fs);
    f = fopen("//Users/vilhelmsaevarsson/Documents/Thingi10K/raw_meshes/994785.obj", "rb");
    MEASURE_MS(stream, file_read_1byte, {
    while(fgetc(f) != EOF)
    {

    }
    });
    fclose(f);
    
    
    fs = open_stream("//Users/vilhelmsaevarsson/Documents/Thingi10K/raw_meshes/994785.obj", READ);
    */
    char* line = 0;
    MEASURE_MS(stream, file_stream_read_line, {
    while(read_line(fs, (void*)&line, ASCII))
    {

    }
    });
    close_stream(fs);
    /*
    char lbuff[1024];
    f = fopen("//Users/vilhelmsaevarsson/Documents/Thingi10K/raw_meshes/994785.obj", "rb");
    MEASURE_MS(stream, file_read_line, {
    while(file_read_line(f, lbuff, 1024))
    {

    }
    });
    fclose(f);
    
    fs = open_stream("//Users/vilhelmsaevarsson/Documents/Thingi10K/raw_meshes/994785.obj", READ);
    file_stream *ofs = open_stream("out.obj", WRITE);
    
    MEASURE_MS(stream, file_stream_write, {
        size_t expected = 8;
        uint8_t* buff = read_stream(fs, 8, &expected);
        while(buff){
            uint64_t* b = (uint64_t*)write_stream(ofs, expected);
            *b = *(uint64_t*)buff;
            buff = read_stream(fs, 8, &expected);
        }
        
    });
    close_stream(ofs);
    close_stream(fs);
    */
    END_TEST(stream, {});

    return 0;
}