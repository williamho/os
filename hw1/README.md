asgn1
=====

`copycat` -- concatenate and copy files 

#Description from code
Copies contents of user-specified file(s) into output stream.

##Options
* option `-o` changes the output stream to the specified output file
* option `-b` specifies the size of the read/write buffer, in bytes
* input files specified as "-" (dash/hyphen/minus) are instead taken from standard input
* if no input files specified, input is taken from standard input

#My description, a full semester later
It's `cat`. Concatenates input files into one output stream or file.
The point of this was to learn to use `open`, `close`, `read`, and `write`
system calls. Also `getopt()`.

#Build
`gcc copycat2.c`

