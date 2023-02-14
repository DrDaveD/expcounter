/* $Id: expcounterd.c,v 1.5 2000/09/06 18:52:27 psfales Exp psfales $ */

/* #define DEBUG 1 */
#ifdef DEBUG
#define ROOTDIR "/usr/people/psfales/exp-debug"
#else
#define ROOTDIR "/usr/expcounters" 
/* #define ROOTDIR "/home/psfales/exp" */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>
#include <time.h>

#include "expcounter.h"

char 	tool[MAXCOUNTLEN];
char	hostname[MAXCOUNTLEN];
char	package[MAXCOUNTLEN];
char 	type[MAXCOUNTLEN];
char 	userinfo[MAXCOUNTLEN];
char	nodename[MAXCOUNTLEN];

char	uid[16];
char	pid[16];
int	vflag=1;	/* Not used as argument, but needed by sock_init*/

struct package_info {
	char package[MAXCOUNTLEN];
#ifdef __linux__
	time_t mod_time;
#else
	timestruc_t mod_time;
#endif
	char counters[MAXCOUNTERS][MAXCOUNTLEN];
	struct package_info *next;
} *package_head;

struct package_info *find_package();
struct group *group_info;

void process_buf(char *, int, struct sockaddr_in *);
void convert_hostname(char *h,struct sockaddr_in  *from_ptr);

/* ARGSUSED */
int
main(int argc, char *argv[])
{
	int	port = COUNTERPORT;
	int	ld;
	int 	nbytes;
	struct sockaddr_in  addr, from;
	
	int 	fromlen = sizeof(from);
	char	msgbuf[1001];		/* UDP Message buffer */
	char	*t_ipaddr;		/* Required for getexpinfo, but not used */
	char 	*t_nodename;		/* Ditto */	
#ifndef DEBUG
	if ( argc == 1 ) {
		if ( fork() != 0 ) {
			exit(0);
		}
	}
#endif
	group_info = getgrnam("exptools");
	if ( group_info == NULL ) {
		perror("Unable to find exptools group");
		exit(1);
	}
	
	umask(002);

	/* Get IP address and port from .expdata file */
	getexpinfo(&t_ipaddr,&port,&t_nodename);

#ifdef DEBUG
	port = COUNTERPORT-1;
#endif

	ld = sock_init(NULL, port, AF_INET, SOCK_DGRAM, &addr, 1);

	if ( bind(ld, (struct sockaddr *)&addr, sizeof(addr)) < 0 ) {
		/* 
		 * If the bind() fails, we'll assume that the daemon is
		 * already running and exit quietly (except for returning)
		 * a non-zero exit code).
		 */
		/* perror("bind"); */
		exit(1);
	}
	do {
		nbytes = recvfrom(ld,msgbuf,sizeof(msgbuf),0,(struct sockaddr *)&from,&fromlen);
		if ( nbytes < 0 ) {
			perror("read error");
		}

		/*
		 * Buffer should have been null terminated by sender,
		 * but just in case (for example, if are being spoofed)
		 * make sure that we have a null terminator.
		 */
		msgbuf[sizeof(msgbuf)-1] = '\0';
		process_buf(msgbuf,nbytes,&from);

	} while ( nbytes != 0 );
}

void
process_buf(key,n,from_ptr)
char *key;
int n;
struct sockaddr_in  *from_ptr;
{
	int i;
	char *tmp,*arg;
	char fname[256];
	FILE *f;
	time_t curtime = time(NULL);
	struct tm curtime_s;
	struct stat statbuf;
	struct package_info *package_info;

	curtime_s = *gmtime(&curtime);

#ifdef DEBUG
	printf("Got message:\n%s",key);
#endif
	tool[0] = hostname[0] = pid[0] = uid[0] = package[0] = '\0';
	type[0] = userinfo[0] = nodename[0] = '\0';

	while ( (tmp=strchr(key,'=')) != NULL ) {
		*tmp = '\0';		

		arg = tmp+1;
		if ( (tmp=strchr(arg,'\n')) == NULL ) {
			break;		
		}
		*tmp = '\0';

		if ( strcmp(key,"tool") == 0 ) {
			strncpy(tool,arg,sizeof(tool));
			tool[sizeof(tool)-1]='\0';
		}
		else if ( strcmp(key,"hostname") == 0 ) {
			strncpy(hostname,arg,sizeof(hostname));
			hostname[sizeof(hostname)-1]='\0';
			convert_hostname(hostname,from_ptr);
		}
		else if ( strcmp(key,"uid") == 0 ) {
			strncpy(uid,arg,sizeof(uid));
			uid[sizeof(uid)-1]='\0';
		}
		else if ( strcmp(key,"pid") == 0 ) {
			strncpy(pid,arg,sizeof(pid));
			pid[sizeof(pid)-1]='\0';
		}
		else if ( strcmp(key,"package") == 0 ) {
			strncpy(package,arg,sizeof(package));
			package[sizeof(package)-1]='\0';
		}
		else if ( strcmp(key,"type") == 0 ) {
			strncpy(type,arg,sizeof(type));
			type[sizeof(type)-1]='\0';
		}
		else if ( strcmp(key,"userinfo") == 0 ) {
			strncpy(userinfo,arg,sizeof(userinfo));
			userinfo[sizeof(userinfo)-1]='\0';
		}
		else if ( strcmp(key,"nodename") == 0 ) {
			strncpy(nodename,arg,sizeof(nodename));
			nodename[sizeof(nodename)-1]='\0';
		}

		key = tmp+1;

		/* Handle multiple line termination chars */
		while (*key == '\n') key++;

	}

	/*
	 * Start by making sure that the directory exists 
	 */
	sprintf(fname,"%s/%s",ROOTDIR,package);

	if ( stat(fname,&statbuf) != 0 ) {
		/* Package directory does not exist */
		return;
	}
	if ( (statbuf.st_mode & S_IFDIR) == 0 ) {
		/* File exists, but is not directory */
		return;
	}
	
	package_info = find_package(package); 

	if ( package_info == NULL ) {
		/* Something unexpected went wrong */
		return;
	}

	for ( i=0 ; i < MAXCOUNTERS ; i++ ) {
#ifdef DEBUG
printf("Validating counter %s with %s\n",tool,package_info->counters[i]);
#endif
		if ( strcmp(tool,package_info->counters[i]) == 0 ) {
			break;
		}
	}
	if ( i == MAXCOUNTERS ) {
		strncat(tool,"-unapproved!",sizeof(tool));
		tool[sizeof(tool)-1] = '\0';
	}

	/* Check for year directory */
	sprintf(fname,"%s/%s/log%04d",ROOTDIR,package,
		curtime_s.tm_year+1900);
	if ( stat(fname,&statbuf) != 0 ) {
		mkdir(fname,0775);
		chown(fname,-1,group_info->gr_gid);
		chmod(fname,02775);
	}

	/* Then check for month directory */
	sprintf(fname,"%s/%s/log%04d/%02d",ROOTDIR,package,
		curtime_s.tm_year+1900,curtime_s.tm_mon+1);
	if ( stat(fname,&statbuf) != 0 ) {
		mkdir(fname,0775);
		chown(fname,-1,group_info->gr_gid);
		chmod(fname,02775);
	}

	sprintf(fname,"%s/%s/log%04d/%02d/%02d",ROOTDIR,package,
		curtime_s.tm_year+1900,curtime_s.tm_mon+1,curtime_s.tm_mday);

	f = fopen(fname,"a");
	if ( f == NULL ) {
		perror(fname);
		exit(1);
	} else {

		if ( strstr(hostname,".de.lucent.com") ) {
			/* Don't save uid info from de.lucent.com - We 
			 * believe this is a requirement of German privacy
			 * laws
			 */
			
			uid[0] = '\0';
		}

		fprintf(f,"%d:%s:%s:%s:%s:%s:%s:%s:%s\n",
			curtime,package,tool,hostname,uid,pid,type,userinfo,nodename);
		fclose(f);
	}
}

struct package_info *
find_package(char *package)
{
	char fname[256],buf[MAXCOUNTLEN],*s;
	struct stat statbuf;
	struct package_info *curpackage = package_head;
	FILE *f;
	int i;

#ifdef DEBUG
printf("find_package: Looking for info on package %s\n",package);
#endif
	while ( curpackage ) {
#ifdef DEBUG
printf("Comparing against %s\n",curpackage->package);
#endif
		if ( strcmp(curpackage->package,package) == 0 ) {
			break;
		}
		curpackage = curpackage->next;
	}

	if ( curpackage == NULL ) {
#ifdef DEBUG
printf("Not found in current list\n");
#endif
		curpackage = calloc(1, sizeof(struct package_info));
		if (curpackage == NULL) {
			return(NULL);
		}
		/* Link to head of list */
		curpackage->next = package_head;
		package_head = curpackage;
		strcpy(curpackage->package,package);
	}

	/*
	 * Get the timestamp on the counternames file
	 */
	sprintf(fname,"%s/%s/counternames",ROOTDIR,package);

	if ( stat(fname,&statbuf) != 0 ) {
		/* counternames file does not exist */
		return(NULL);
	}
#ifdef DEBUG
printf("Comparing %x and %x\n",statbuf.st_mtim.tv_sec,curpackage->mod_time.tv_sec);
#endif
#ifdef __linux__
	if ( statbuf.st_mtime > curpackage->mod_time )
#else
	if ( statbuf.st_mtim.tv_sec > curpackage->mod_time.tv_sec ||
	 ((statbuf.st_mtim.tv_sec == curpackage->mod_time.tv_sec) &&
	 (statbuf.st_mtim.tv_nsec > curpackage->mod_time.tv_nsec)) ) 
#endif
	{
#ifdef DEBUG
printf("New time is greater\n");
#endif
#ifdef __linux__
		curpackage->mod_time = statbuf.st_mtime;
#else
		curpackage->mod_time = statbuf.st_mtim;
#endif
		f = fopen(fname,"r");
		if ( f == NULL ) {
			/* Couldn't open file, even though stat() was OK */
			return(NULL);
		}

		i=0;
		while ( fgets(buf,sizeof(buf),f) != NULL && i < MAXCOUNTERS ) {
			if ( (s=strchr(buf,'\n')) != NULL ) {
				*s = '\0';
			}
			strncpy(curpackage->counters[i],buf,MAXCOUNTLEN);
			curpackage->counters[i][MAXCOUNTLEN-1] = '\0';
#ifdef DEBUG
printf("Added count to list: %s\n",curpackage->counters[i]);
#endif
			i++;
		}
		fclose(f);
	}
	return curpackage;
}

/* 
 * convert_hostname() - If hostname was sent as a numeric dotted quad, see 
 * if we can look it up here 
 */
void
convert_hostname(char *h,struct sockaddr_in  *from_ptr)
{
	char *p;
	int n;
	struct sockaddr_in sin;
	struct hostent *host;
	
  /* As a (possibly permanent) experiment, we're going to try looking up
   * every host name that comes in.  This will allow the stored data
   * to use hostnames that are consistent with the log files
   */
  strcpy(h,inet_ntoa(from_ptr->sin_addr));

	if ( ! isdigit(h[0]) ) return;
	
	for ( n=0, p=h ; *p ; p++ ) {
		if ( *p == '.' ) n++;
	}

	if ( n != 3 ) return;

	sin.sin_addr.s_addr = inet_addr(h);
	sin.sin_family = AF_INET;
	host = gethostbyaddr((const char *) &sin.sin_addr, sizeof (sin.sin_addr), sin.sin_family);

	if ( host == NULL ) return;

	if (strchr(host->h_name, '.')) {
		/*
		 * printf("%s->%s\n",h,host->h_name);
		 */
		strncpy(h, host->h_name, MAXCOUNTLEN);
		h[MAXCOUNTLEN] = '\0';
	}
}
