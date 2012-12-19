#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_PATHSTR_LEN 1024
#define MAX_TIMESTR_LEN 16

int mtime, user, xflag, lflag;
dev_t link_dev;
ino_t link_ino;

void listDir(char * s_path);
void modeToStr(mode_t mode, char * str); 

main(int argc, char *argv[]) {
	char c;
	int i;
	struct passwd *pwd;
	struct stat st;

	errno = 0, mtime = 0, xflag = 0, lflag = 0, user = -1;
	link_dev = 0; link_ino = 0;
	
	while ((c = getopt(argc, argv, "m:u:l:x")) != -1) {
		switch(c) {
			case 'm':
				mtime = atoi(optarg);
				break;
			case 'u':
				pwd = getpwnam(optarg);
				if (pwd == NULL) {
					if (errno == 0)
						fprintf(stderr,"Error: User %s not found.\n",optarg);
					else 
						fprintf(stderr,"Error: User %s not found: %s\n",optarg,strerror(errno));
					return -1;
				}
				else
					user = pwd->pw_uid;
				break;
			case 'l':
				if (lstat(optarg,&st) != 0) {
					fprintf(stderr,"Error: Could not get file status of path \"%s\" for option \"-l\": %s\n",
							optarg,strerror(errno));
					exit(-1);
				}
				lflag = 1;
				link_dev = st.st_dev;
				link_ino = st.st_ino;
				break;
			case 'x':
				xflag = 1;
				break;
			case '?':
				return -1;
				break;
		}
	}
	
	if (optind == argc)
		listDir(".");
	
	for (i = optind; i < argc; i++)
		listDir(argv[i]);
	return 0;
}

void listDir(char * s_path) {
	DIR *dirp;
	struct dirent *de;
	
	struct stat st, lst;
	struct passwd *pwd;
	struct group *grp;
	
	char path[MAX_PATHSTR_LEN], lpath[MAX_PATHSTR_LEN];
	char time_str[MAX_TIMESTR_LEN];
	char permissions[11];
	
	int lpathlen;
	
	dirp = opendir(s_path);
	if (dirp == NULL) {
		fprintf(stderr, "Error: Could not access directory \"%s\": %s\n", s_path, strerror(errno));
		if (errno == EACCES)
			return;
		exit(-1);
	}
	while (de = readdir(dirp)) {
		if (!strcmp(".",de->d_name) || !strcmp("..",de->d_name))
			continue; 	// Ignore . and .. nodes
		
		snprintf(path,MAX_PATHSTR_LEN,"%s/%s",s_path,de->d_name);
		if (lstat(path,&st) != 0) {
			fprintf(stderr,"Error: Could not get file status of path \"%s\": %s\n",
					path,strerror(errno));
			exit(-1);
		}
		
		if (xflag > 0 && st.st_ino == 2) { // If option -x is set
			if (xflag != 1) {
				fprintf(stderr,"Note: Not crossing mount point at \"%s\"",path);
				continue;
			}
			else // Don't skip root node of filesystem in which the traversal began
				xflag++;
		}
		
		if (user >= 0 && user != st.st_uid || 
			lflag > 0 && !S_ISLNK(st.st_mode) || 
			mtime > 0 && difftime(time(NULL),st.st_mtime) <  mtime || 
			mtime < 0 && difftime(time(NULL),st.st_mtime) > -mtime )
			continue;
		
		if (S_ISLNK(st.st_mode)) {
			if ((lpathlen = readlink(path,lpath,MAX_PATHSTR_LEN)) < 0) {
				fprintf(stderr,"Error: Could not resolve target of symlink \"%s\": %s\n",
						path,strerror(errno));
				strcpy(lpath,"?");
			}
			else
				lpath[lpathlen] = '\0';
			
			if (lflag > 0) {
				if (stat(lpath,&lst) != 0) {
					fprintf(stderr,"Error: Could not get file status of symlink target \"%s\": %s\n",
							lpath,strerror(errno));
					exit(-1);
				}
				if (link_dev != lst.st_dev || link_ino != lst.st_ino)
					continue;	// Target of link does not match argument of "-l" option
			}
		}
		
		// Device name (in hex) and inode number
		printf("%04x/%-6ld ",(unsigned int)st.st_dev,(long)st.st_ino);
		
		// Type of node, permissions, number of links to node
		modeToStr(st.st_mode, permissions);
		printf("%s %3d ",permissions, (int)st.st_nlink);
		
		// Print owner of node as a name, or integer if name unavailable
		if ((pwd = getpwuid(st.st_uid)) != NULL)
			printf("%-8s ", pwd->pw_name);
		else
			printf("%-8d ", st.st_uid);
		
		// Print group owner of node as a name, or integer if name unavailable
		if ((grp = getgrgid(st.st_gid)) != NULL)
			printf("%-8s ", grp->gr_name);
		else
			printf("%-8d ", st.st_gid);
		
		// Size of node
		if (S_ISBLK(st.st_mode) || S_ISCHR(st.st_mode))
			printf("0x%6lx ",(unsigned long)st.st_rdev);
		else
			printf("%8ld ",(long)st.st_size);
		
		// Last modified date of node, and path of node.
		strftime(time_str,MAX_TIMESTR_LEN,"%b %e %H:%M",localtime(&st.st_mtime));
		printf("%s %s",time_str,path);

		if (S_ISLNK(st.st_mode))
			printf(" -> %s",lpath);
			
		printf("\n");
		
		if (S_ISDIR(st.st_mode))
			listDir(path);
	}
}

// Based on strmode(3). Create string representation of mode_t.
void modeToStr(mode_t mode, char * str) {
	static char ftypes[] = "?pc?d?b?-?l?s???";
	
	strcpy(str,"----------");
	str[0] = ftypes[mode>>12];
	
	// Owner permissions
	if (mode & S_IRUSR)
		str[1] = 'r';
	if (mode & S_IWUSR)
		str[2] = 'w';
	switch (mode & (S_IXUSR|S_ISUID)) {
		case S_IXUSR:
			str[3] = 'x';
			break;
		case S_ISUID:
			str[3] = 'S';
			break;
		case S_IXUSR|S_ISUID:
			str[3] = 's';
			break;
	}
	
	// Group permissions
	if (mode & S_IRGRP)
		str[4] = 'r';
	if (mode & S_IWGRP)
		str[5] = 'w';
	switch (mode & (S_IXGRP|S_ISGID)) {
		case S_IXGRP:
			str[6] = 'x';
			break;
		case S_ISGID:
			str[6] = 'S';
			break;
		case S_IXGRP|S_ISGID:
			str[6] = 's';
			break;
	}
	
	// Other permissions
	if (mode & S_IROTH)
		str[7] = 'r';
	if (mode & S_IWOTH)
		str[8] = 'w';
	switch (mode & (S_IXOTH | S_ISVTX)) {
		case S_IXOTH:
			str[9] = 'x';
			break;
		case S_ISVTX:
			str[9] = 'T';
			break;
		case S_IXOTH | S_ISVTX:
			str[9] = 't';
			break;
	}
}