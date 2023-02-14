#include <stdio.h>
#include <string.h>

char *fpath;				/* Path to .expdata file */
#define EXPDATA "/.expdata"
#define NODENAME "/.nodename"

extern char *getenv();

extern int vflag;

getexpinfo(ipaddr,port,nodename)
char **ipaddr;
int *port;
char **nodename;
{
	char *tools;	/* Path to exptools */
	FILE *f;	/* .expdata file */
	char buf[1000];

	/* Find the exptools base directory.  Try $TOOLS first, then /opt/exp */
	if ((tools = getenv("TOOLS")) == NULL) {
		tools = "/opt/exp";
		if (access(tools, 1) == -1) {
			fprintf(stderr,
			"expcounter: $TOOLS not set and can't access %s\n",tools);
			exit(1);
		}
	}

	
	/* Find path to data file */
	fpath = (char *)malloc(strlen(tools)+strlen(EXPDATA)+1);
	if ( fpath == NULL ) {
		fprintf(stderr,"expcounter: malloc failure\n");
		exit(1);
	}
	strcpy(fpath,tools);
	strcat(fpath,EXPDATA);

	f = fopen(fpath,"r");
	if ( f == NULL ) {
		if ( vflag ) {
			fprintf(stderr,"expcounter: failure to open %s\n",fpath);
		}	
		exit(1);
	}

 	/* Parse the file for the IP address */
	while ( fgets(buf,sizeof(buf),f) != NULL ) {
		char *s;
		if ( strncmp(buf,"expCounterIP",strlen("expCounterIP")) == 0 ) {
			s = buf+strlen("expCounterIP");
			while ( *s && isspace(*s) ) s++;
			*ipaddr=strdup(s);
			if ( (s=strchr(*ipaddr,':')) != NULL ) {
				*s = '\0';
				*port=atoi(s+1);
			}
		}
	}

	/* Find path to nodename file */
	fpath = (char *)malloc(strlen(tools)+strlen(NODENAME)+1);
	if ( fpath == NULL ) {
		fprintf(stderr,"expcounter: malloc failure\n");
		exit(1);
	}
	strcpy(fpath,tools);
	strcat(fpath,NODENAME);

	f = fopen(fpath,"r");
	if ( f == NULL ) {
		if ( vflag ) {
			fprintf(stderr,"expcounter: failure to open %s\n",fpath);
		}	
		exit(1);
	}

 	/* Read nodename from the file */
	if ( fgets(buf,sizeof(buf),f) != NULL ) {
		*nodename = strdup(buf);
	} else {
		*nodename = "";
	}
}
