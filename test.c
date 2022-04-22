#define CTEST_ENABLED
#include "cstream.h"



int file_read_line(FILE *fd, char *buff, size_t s)
{
    return getline(&buff, &s, fd) != -1;
}



int main()
{
    START_TEST(stream, {});
    
    
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
        }
    });
    close_stream(fs);

    fs = fs_open("//Users/vilhelmsaevarsson/Documents/Thingi10K/raw_meshes/994785.obj", READ);
    size_t expected = 8;
    MEASURE_MS(stream, file_stream_8bytes, {        
        while(fs_read(fs, 8, &expected) != NULL){   
        }
        
    });
    close_stream(fs);

    fs = fs_open("//Users/vilhelmsaevarsson/Documents/Thingi10K/raw_meshes/994785.obj", READ);
    expected = 6;
    MEASURE_MS(stream, file_stream_6bytes, {        
        while(fs_read(fs, 6, &expected) != NULL){
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