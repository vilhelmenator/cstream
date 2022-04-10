
#include "cstream.h"

#define CTEST_ENABLED
#include "ctest/ctest.h"

int main()
{
    //
    // fwrite, fread, fopen, fclose, getc, putc, ftell, rewind, fseek, fstat
    //
    //
    // perf stats reading.
    // perf stats writing.
    // perf stats reading lines.
    // perf stats writing lines.
    //
    // perf stats compare streamers.
    //  ifstream, iostream, C++ stream whatever
    //

    //
    // test bitstream writing
    // test bitstream reading
    //
    
    START_TEST(stream, {});
    /*
    int fd = open("//Users/vilhelmsaevarsson/Documents/Thingi10K/raw_meshes/994785.obj", O_RDONLY, S_IREAD);
    struct stat stats;
    int32_t status = fstat(fd, &stats);
    printf("file size %zu\n", stats.st_size);
    char* bu = (char*)malloc(stats.st_size);
    MEASURE_MS(stream, file_read_whole, {
        
        read(fd, bu, stats.st_size);
        
    });
    free(bu);
    close(fd);

    file_stream *fs = open_stream("//Users/vilhelmsaevarsson/Documents/Thingi10K/raw_meshes/994785.obj", READ);
    
    MEASURE_MS(stream, file_stream_8bytes, {
        int prev_loc = 0;
        while(read_stream(8, fs) != NULL){
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
        while(read_stream(1, fs) != NULL){
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
    
    */
    file_stream *fs = open_stream("//Users/vilhelmsaevarsson/Documents/Thingi10K/raw_meshes/994785.obj", READ);
    char lbuff[1024];
    MEASURE_MS(stream, file_read_line, {
    while(read_line_char(fs, &lbuff[0], 1024))
    {

    }
    });
    close_stream(fs);

    END_TEST(stream, {});
    return 0;
}