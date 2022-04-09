
#include "cstream.h"

#define CTEST_ENABLED
#include "ctest/ctest.h"

int main()
{
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
    FILE *f = fopen("//Users/vilhelmsaevarsson/Documents/Thingi10K/raw_meshes/994785.obj", "rb");
    fseek(f, 0L, SEEK_END);
    size_t s = ftell(f);
    rewind(f);
    printf("file size %zu\n", s);
    char* bu = (char*)malloc(s);
    MEASURE_MS(stream, file_read_whole, {
        
        fread(bu, s, 1, f);
        
    });
    free(bu);
    fclose(f);

    file_stream *fs = open_stream("//Users/vilhelmsaevarsson/Documents/Thingi10K/raw_meshes/994785.obj", READ);
    
    MEASURE_MS(stream, file_stream_8bytes, {
        int prev_loc = 0;
        while(read_stream(8, fs) != NULL){
            //printf("location %zu", fs->file_ptr);
            
        }
        
    });

    
    close_stream(fs);
    char buff[8];
    f = fopen("//Users/vilhelmsaevarsson/Documents/Thingi10K/raw_meshes/994785.obj", "rb");
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
    END_TEST(stream, {});
    return 0;
}