//
#include <stdint.h>
#include <stdlib.h>

typedef enum t_desc_type {
    dt_none = -1, //  null
    dt_builtin = 0, //  local type
    dt_struct, //  local struct
    dt_array, //  local array
    dt_struct_ref, //  heap struct
    dt_builtin_ref, //  heap builtin
    dt_array_ref, //  heap array
} desc_type;

//
typedef struct t_desc_define
{
    const char *name; // who
    desc_type dtype; // what
    uint32_t size; //
    uint32_t offset; // where
    const t_desc_define *schema; // how
} desc_define;

// terminal placement.
const desc_define end_desc = { NULL, dt_none, 0, 0, NULL };

// define struct
const desc_define sub_type[] { { "thing", dt_none, 4, 0, NULL }, end_desc };

const desc_define my_type[] { { "thing", dt_none, 4, 0, sub_type }, end_desc };
//
// the allocator can remove a ptr network.
//      - plucks free memory into a deferred list for each owner thread.
//      - can more effectively release memory.
//      - can pluck items into a deferred list per thread.
// the allocator can move a ptr network.
//      - copy any item of memory that is not local to the current thread.
// the allocator can copy a ptr network.
//      - make a new copy of a network in a single contiguous buffer.
//
void test_a(void *src, const desc_define *t)
{
    // free a network of objects.
    // does the type have desc objects.
    // are they all pointing to the same one.
    //  - then it is just collecting the offsets and traversing those.
    //  - for every type of equal size. if the ptrs are all just within the same
    //  thread area.
    //      we can simply string the ptrs together like a
    int i = 0;
    do {
        //
        //
        //
    } while (t[i++].name != NULL);
}

void test() { test_a(NULL, my_type); }
