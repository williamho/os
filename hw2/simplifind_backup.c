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

int mtime, user;

void listDir(char * s_path);

main(int argc, char *argv[]) {
	char c;
	int i;
	struct passwd *pwd;
	
	mtime = 0;
	user = -1;
	errno = 0;
	
	while ((c = getopt(argc, argv, "m:u:")) != -1) {
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
			case '?':
				return -1;
				break;
		}
	}
	
	if (optind == argc) {
		listDir(".");
		return 0;
	}
	
	for (i = optind; i < argc; i++)
		listDir(argv[i]);
	return 0;
}

void listDir(char * s_path) {
	static char ftypes[] = "-pc-d-b---l-s---";
	static char *permissions[] = {"---","--x","-w-","-wx","r--","r-x","rw-","rwx"};

	DIR *dirp;
	struct dirent *de;
	
	struct stat st, lst;
	struct passwd *pwd;
	struct group *grp;
	
	int pathlen = strlen(s_path);
	char path[MAX_PATHSTR_LEN], lpath[MAX_PATHSTR_LEN];
	char time_str[MAX_TIMESTR_LEN];
	
	dirp = opendir(s_path);
	if (dirp == NULL) {
		fprintf(stderr, "Error: Could not access \"%s\": %s\n", s_path, strerror(errno));
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
		
		if (user >= 0 && user != st.st_uid || 
			mtime > 0 && time(NULL)-st.st_mtime <  mtime || 
			mtime < 0 && time(NULL)-st.st_mtime > -mtime )
			continue;
			
		// Device name (in hex) and inode number
		printf("%04x/%-6ld ",(unsigned int)st.st_dev,(long)st.st_ino);
		
		// Type of node and associated permissions
		printf("%c%s%s%s ",ftypes[st.st_mode>>12],permissions[(st.st_mode>>6)&7],
							permissions[(st.st_mode>>3)&7],permissions[st.st_mode&7]);
		
		// Number of links to node
		printf("%3d ", (int)st.st_nlink, getgrgid(st.st_gid)->gr_name);
		
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
		
		// Print size of node
		if (S_ISBLK(st.st_mode) || S_ISCHR(st.st_mode))
			printf("0x%6lx ",(unsigned long)st.st_rdev);
		else
			printf("%8ld ",(long)st.st_size);
		
		// Last modified date of node, and path of node.
		strftime(time_str,MAX_TIMESTR_LEN,"%b %e %H:%M",localtime(&st.st_mtime));
		printf("%s %s ",time_str,path);

		if (S_ISLNK(st.st_mode)) {
			if (stat(path,&lst) != 0) {
				fprintf(stderr,"Error: Could not get file status of symlink \"%s\": %s\n",
						path,strerror(errno));
				printf("-> ?");
			}
			else if (readlink(path,lpath,lst.st_size) < 0) {
				fprintf(stderr,"Error: Could not resolve target of symlink \"%s\": %s\n",
						path,strerror(errno));
				printf("-> ?");
			}
			else {
				lpath[lst.st_size] = '\0';
				printf("-> %s",lpath);
			}
		}
		printf("\n");
		
		if (S_ISDIR(st.st_mode))
			listDir(path);
	}
}