asgn3
=====

`wish` -- William's shell

#Description
Simple shell that accepts commands until `EOF`. 

Arguments are of the form: 
`command {argument {argument...} } {redirection_operation {redirection_operation...}}`.

Lines beginning with `#` are ignored.

##I/O redirection
`<filename` Open filename and redirect `stdin`
`>filename` Open/Create/Truncate filename and redirect `stdout`
`2>filename` Open/Create/Truncate filename and redirect `stderr`
`>>filename` Open/Create/Append filename and redirect `stdout`
`2>>filename` Open/Create/Append filename and redirect `stderr`

#My description, a full semester later
It's a shell. It does things shells can do but not as well. 
Used to learn `fork` and `exec` and redirection operations (haven't learned 
about pipes at this point though). Has things like `ls`; can also be invoked 
as a script interpreter. `wish scriptname`

#Build
`gcc wish.c`

