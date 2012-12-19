asgn5
=====

This was actually an assignment to test some things about mapped memory. 
Used to learn `mmap` and catching signals. Extra credit was to determine 
the size of a page by intentionally causing a segfault and catching the 
signal to determine the size of a page experimentally. I used `sbrk`.

#Build
`gcc mmaptest.c` or `gcc pagesize.c` for the extra credit

