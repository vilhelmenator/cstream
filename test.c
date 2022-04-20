
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

    //  seek.
    //      - read_mode
    //      - write_mode
    //      - 
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

    
    file_stream *fs = fs_open("//Users/vilhelmsaevarsson/Documents/Thingi10K/raw_meshes/994785.obj", READ);
    MEASURE_MS(stream, file_stream_128bytes, {
        size_t expected = 128;
        while(fs_read(fs, 128, &expected) != NULL){
            //printf("location %zu", fs->file_ptr);
            
        }
    });
    close_stream(fs);

    fs = fs_open("//Users/vilhelmsaevarsson/Documents/Thingi10K/raw_meshes/994785.obj", READ);
    size_t expected = 8;
    MEASURE_MS(stream, file_stream_8bytes, {        
        while(fs_read(fs, 8, &expected) != NULL){
            //printf("location %zu", fs->file_ptr);
            
        }
        
    });
    close_stream(fs);

    fs = fs_open("//Users/vilhelmsaevarsson/Documents/Thingi10K/raw_meshes/994785.obj", READ);
    expected = 6;
    MEASURE_MS(stream, file_stream_6bytes, {        
        while(fs_read(fs, 6, &expected) != NULL){
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

    fs = fs_open("//Users/vilhelmsaevarsson/Documents/Thingi10K/raw_meshes/994785.obj", READ);
    MEASURE_MS(stream, file_stream_1byte, {
        size_t expected = 64;
        uint8_t *res = fs_read(fs, 64, &expected);
        char c = 0;
        while(res != NULL){
            for(int i = 0; i < expected; i++)
            {
                c = *((char*)res + i);
            }
            res = fs_read(fs, 64, &expected);
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
    
    
    fs = fs_open("//Users/vilhelmsaevarsson/Documents/Thingi10K/raw_meshes/994785.obj", READ);
    char* line = 0;
    MEASURE_MS(stream, file_stream_read_line, {
    while(fs_read_line(fs, (uint8_t**)&line, ASCII))
    {

    }
    });
    close_stream(fs);
    
    char lbuff[1024];
    f = fopen("//Users/vilhelmsaevarsson/Documents/Thingi10K/raw_meshes/994785.obj", "rb");
    MEASURE_MS(stream, file_read_line, {
    while(file_read_line(f, lbuff, 1024))
    {

    }
    });
    fclose(f);
    
    
    fs = fs_open("//Users/vilhelmsaevarsson/Documents/Thingi10K/raw_meshes/994785.obj", READ);
    file_stream *ofs = fs_open("out.obj", WRITE);
    
    MEASURE_MS(stream, file_stream_read_write, {
        size_t expected = 8;
        uint8_t* buff = fs_read(fs, 8, &expected);
        while(buff){
            *(uint64_t*)fs_write(ofs, expected) = *(uint64_t*)buff;
            buff = fs_read(fs, 8, &expected);
        }
        
    });
    close_stream(ofs);
    close_stream(fs);
    
    fd = open("//Users/vilhelmsaevarsson/Documents/Thingi10K/raw_meshes/994785.obj", O_RDONLY, S_IREAD);
    status = fstat(fd, &stats);
    int32_t num_bytes = stats.st_size;
    bu = (char*)malloc(num_bytes);
    read(fd, bu, num_bytes);
    close(fd);

    int ofd = open("out.obj",  O_RDWR | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
    MEASURE_MS(stream, file_write, {
        write(ofd, bu, num_bytes);
        close(ofd);
    });
    
    
    

    ofs = fs_open("out.obj", WRITE);
    MEASURE_MS(stream, file_stream_write8, {
        for(int i = 0; i < num_bytes; i+= 8){
            (*(uint64_t*)fs_write(ofs, 8)) = *(uint64_t*)(char*)(bu + i);
        } 
        close_stream(ofs);
    });

    ofs = fs_open("out.obj", WRITE);
    MEASURE_MS(stream, file_stream_write6, {
        for(int i = 0; i < num_bytes; i+= 6){
            (*(uint64_t*)fs_write(ofs, 6)) = *(uint64_t*)(char*)(bu + i);
        } 
        close_stream(ofs);
    });

    free(bu);
    //free(bu);
    //close(fd);

    //fd = open("//Users/vilhelmsaevarsson/Documents/Thingi10K/raw_meshes/994785.obj", O_RDONLY, S_IREAD);
    //status = fstat(fd, &stats);
    //num_bytes = stats.st_size;
    //bu = (char*)malloc(num_bytes);
    //read(fd, bu, num_bytes);
    

    

    fs = fs_open("utf8.txt", READ);
    wchar_t* wline = 0;
    int line_len = 0;
    
    MEASURE_MS(stream, file_stream_read_line, {
        do
        {
            line_len = fs_read_line(fs, (uint8_t**)&wline, UNICODE_16);
        }while(line_len != 0);
        

    });
    close_stream(fs);
    END_TEST(stream, {});


    //
    // read/write odd sizes, forcing the buffer to resize.
    // read zero size files.
    //  write into read modes
    //  read from write modes.
    //  append modes.
    //  seek and tell.
    //  jump between file positions.
    //      - reset
    // read/write lines.
    // read/write small files.
    // read curropted text files.
    // compare input and output files. do they match.

    //
    // putc and string without null terminator
    // convert values to strings.
    // print string as raw values.
    // outputs bytes to be formatted.
    // print("name ", thing, "yeah",  )
    //
    // print(id, "biifb", {ptr,size}, int, int, float, {ptr, size})
    // print("Ssiifs", string, string, int, int, float, string)
    //
    // int to string. float to string. const string.0, 
    //
    return 0;
}