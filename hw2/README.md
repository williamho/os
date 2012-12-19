asgn2
=====

`simplifind` -- a simplified `find`

#Description from code
Perform recursive walk of filesystem, printing information about nodes.

##Options
* `-u user`: Only list nodes belonging to `user`
* `-m mtime`: 
	* (mtime < 0) List nodes not older than at least -`mtime` seconds 
	* (mtime > 0) List nodes older than at least `mtime` seconds
* `-l target`: Only list nodes that are symlinks pointing to `target`
* `-x`: Do not cross mount points

#My description, a full semester later
A simplified version of UNIX `find`. Recursively explores all nodes in the 
filesystem below a specified point. Used to learn `readdir` and `stat`.

##Extra
Two extra credit options were provided (one or the other):

* Extra credit #1 was: don't cross mount points.
* Extra credit #2 was: given an 'evil' filesystem (with hardlinks to directories 
enabled), detect hardlink loops. Didn't do this one.

#Build
`gcc simplifind.c`

