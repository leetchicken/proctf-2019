# The Polyfill Service

The name of service comes from the web development. A [polyfill](https://en.wikipedia.org/wiki/Polyfill_(programming)) is a code that implements a feature on web browsers that do not support the feature. The Polyfill service uses polyfill technique to draw filled polygons in ASCII.

The service is written on C language and compiled to WebAssembly platform. It consists of two parts: a static web server which runs the service code in the browser and a TCP-service, where the same code launched using Wasmtime, the standalone JIT-style runtime for WebAssembly.

There are three types of objects: **frames**, **polygons** and **points**. Users can add *polygons* to the *frame*, copy them from one *frame* to another and delete them from the *frame*. Also it is possible to add, print and del *points* from the *polygon* and render the result.

The full commands summary:
```
set_frame <frame_idx>
info

new_poly
del_poly <poly_idx>
copy_poly <poly_idx> <dest_frame_idx>

clear_frame
copy_frame <dest_frame_idx>

add_point <poly_idx> <pos> <x> <y>
get_point <poly_idx> <pos>
del_point <poly_idx> <pos>

render
```

Flags are stored as files. Only users who know password can read the flags. The user list is available to any user and printed in the beginning.

The example of renders:
```        
           AA BBBBBBBBBBBBBB                                                    
           AA BBBBBBBBBBBB                                                      
           AA BBBBBBBBBBB                                                       
           AA BBBBBBBBBBBB                                                      
           AA BBBBBBBBBBBBBB                                                    
           AA                                                                   
           AA                                                                   
           AA                                                                   
           AA                                                                   
           AA                                                                   
           AA                                                                   
           AA                                                                   
           AA                                                                   
           AA                           
```

```
                     BBBB                                                       
       AAAAAAAAAACAABBBBBBBBBBBBBAAAA                                           
        AAAAAAAAACCCCBBBBBBBBBBBBBBBBBBBB                            DDD        
         AAAAAAAACCCCCCCAAAAAAAAA           BBBBB              DDDDDD           
           AAAAAAACCCCCCCCAAAAA            BBBBBBBBBBBBBBDDDDDDDDDD             
            AAAAACAAAACCCCCCC             BBBBBBBBBDDDDDDDDDDDDD                
             AAAACCCCCAAAACCCCCC         BBBBDDDDDDDDDDDDDDDDD                  
               AACCCCCCCCC   CCCCCC    DDDDDDDDDDDDDDDDDDDD                     
         DD     ACCCCCCCCCCCCAAAADDDDDDDDDDDDDDDDDDDDDDDDAAAAAAAAAA             
         DDDDDDD CCCCCCCCCCDDDDDDDDDDDDDDDDDDDDDDDDDDDAAAAAAAAAAA               
        DDDDDDD  CCCCCCDDDDDDDDDDDDDCBDDDDDDDDDDDDDAAAAAAAAAAA                  
        D         CCCCDDDDDDDDDDDCCAADDDDDDDDDDDDCAAAAAAAAAA                    
                  CCDDDDDDDDDDCCCAAADDDDDDDDDDACAAAAAAAAAA                      
                AADDDDDDDDCCCCCBBBBDDDDDDDDDAAAAAAAAAAAA                        
              AAADDDDDDCCCCCCBBBBBDDDDDDDAAAAAAAAAAAAAAAAA                      
            AAADDDDDCCCCCCCBBBBBBBDDDDDAAAAAAAAAAAAAAAAAAAAAAA                  
             DDDD CCCCCCC   ABBBBDDDAAAAAAAAAAAAAAAAAAAAAAAAAAAAA               
           DDD    CCCCC         DBBB                      AAAAAAAAAA            
                  CCC             BBB                                           
                  C                 B                                        
```

This is a binary service, where the attacker should attack the heap to steal the flags. The C source is not given.

## Memory Layout ##

The memory is organized this way:

- the name of the current user (100 bytes)
- the array of polygon pointers (10 frames * 25 polygons per frame * 4 bytes per pointer = 1000 bytes)
- the first polygon (50 points * 2 coordinates * 1 byte per coordinate = 100 bytes)
- the second polygon (100 bytes)
- ...

The polygons can be dynamicaly created or deleted. Several frames can use one polygon. When the polygon
is not used by any frame it is deleted.

To allocate the memory the default Wasmtime's allocator, dlmalloc, is used. This is a popular allocator that 
used in some Android OSes. The glibc's allocator is derived from dlmalloc.


# Known Vulns

There are two known ways how to cause use after free. Both of them are connected with checking if the polygon is not used anymore.

## Off-By-One Error While Testing if the Frame is Empty

In the **frame_is_empty** function the last polygon is not checked:
```c
int frame_is_empty(polyline* f) {
    for(unsigned int i = 0; i < MAXFRAMEPOLYS-1; i += 1) {
        if(f[i] != 0) {
            return 0;
        }
    }
    return 1;
}
```

This function is used when checking if the polygon is unused:
```c
void del_poly_if_unused(frame *frames, polyline f) {
    for (unsigned int frame_idx=0; frame_idx < MAXFRAMES; frame_idx += 1) {
        if(frame_is_empty(frames[frame_idx])) {
            continue;
        }
        // ... more checks ...
    }
    printf("ok, poly is not on needed anymore, deleting it\n");
    free(f);
    polys_allocated -= 1;
}
```

This allows to create a polygon, then create its copy on the last position and delete the original polygon. The freed memory  can still be accessed using the second polygon.

**The fix**: change the condition from `i < MAXFRAMEPOLYS-1` to `i < MAXFRAMEPOLYS`.

## Insufficient Check if the Polygon is not Used

In the **del_poly_if_unused** function there is a *"... more checks ..."* part:
```c
for (unsigned int poly_idx=0; 
     poly_idx < MAXFRAMEPOLYS && frames[frame_idx][poly_idx] != 0; 
     poly_idx += 1) {
    if (frames[frame_idx][poly_idx] == f) {
        return; // do not delete
    }
}
```
The code tries to find the same polygon on the other frames, but stops when the polygon is absent. If the middle polygon is absent, the rest are not checked.

**The fix**: remove the `&& frames[frame_idx][poly_idx] != 0` part of condition.

# The Exploitation #

To successfully exploit the heap corruption an attacker should know how the allocator works. There is no ALSR in WebAssembly, so the addresses are the same during several launchs.

The memory layout after user login with the allocator structures:
```
[S | LOGIN] [S | POLYGON_PTRS] [P | S | FREE_SPACE]

S - size of the chunk (with the two lowest bits having special meaning)
P - size of the previous chunk
```

The one way to exploit the use after free is to create four polygons:

```
[S | LOGIN] [S | POLYGON_PTRS] [S | POLY0] [S | POLY1] [S | POLY2] [S | POLY3] [P | S | FREE_SPACE]
```

Then free *POLY1*, in such way that allows to access its memory (*Vuln 1* or *Vuln 2*):

```
[S | LOGIN] [S | POLYGON_PTRS] [S | POLY0] [P | S | FD | BK | FREE] [S | POLY2] [S | POLY3] [P | S | FREE_SPACE]

FD - pointer to the next chunk of free memory of such size. The free chunks of short sizes are organized as double-linked lists
BK - pointer to the previous chunk of free memory of such size
```

The **FD** should be overwritten to point to the second half of its chunk. That will allow to gain the memory that overlaps with **POLY2** chunk. This is needed because we are not able to rewrite **P** or **S** of the current chunk since they are behind the accessable memory.

The **BK** should be also overwritten to point back to the previous chunk to pass consistency checks of dlmalloc.

After adding one fake free chunk, the **P1** should be allocated again (it will be put on the same place), and then P5 should be allocated, which will be placed on the fake free chunk:

```
[S | LOGIN] [S | POLYGON_PTRS] [S | POLY0] [S | POLY1] [S | POLY2] [S | POLY3] [P | S | FREE_SPACE]
                                                [ S | POLY5]
```
How the POLY5 can be used to overwrite the headers of **POLY2**. They can be overwritten to mark it a free chunk of some big size, e.g. 300 bytes. The big chunks are organized as trees, keyed by size, where the node has pointers to parent node and two child nodes:

```
[S | LOGIN] [S | POLYGON_PTRS] [S | POLY0] [S | POLY1] [P | S | FD | BK | PP | C1 | C2 | IDX ] [S | POLY3] [P | S | FREE_SPACE]
                                                [ S | POLY5]

PP - pointer to the parent node
C1 - pointer to the first child node
C2 - pointer to the second child node
IDX - tree index. There are 31 trees for various allocation sizes
```

When the *dlmalloc* frees some memory area, if the previous or the next chunk is free, they are merged. So the dlmalloc keeps the invariant that there is no two adjacent free chunks. 

Now delete **POLY1**, causing merging the just freed space with the another fake free chunk we created:
```
[S | LOGIN] [S | POLYGON_PTRS] [S | POLY0] [SOME_HEADERS | MERGED_FREE_SPACE ] [S | POLY3] [P | S | FREE_SPACE]
                                                   [ S | POLY5]
```

To merge the chunks, if the one chunk is a part of the free chunks tree, the allocator should delete the node from this tree, by reparenting **C1** and **C2** to **PP**. 

The **PP** is a pointer which is controlled by attacker, so during reparenting the **C1** and **C2** will be written by this pointer. This gives the memory write primitive. This primitive can be used to modify some value in **POLYGON_PTRS** array, making it pointing to the LOGIN chunk.

Now by creating a polygon points, the login of the current user can be overwritten by victims login and *the attacker is able to see flags of other users*.

The sploit can be found at [/sploits/polyfill/polyfill_sploit.py](../../sploits/polyfill/polyfill_sploit.py).





