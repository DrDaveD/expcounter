/* General exptools counter. */
/* Peter Fales 1/6/99 */
/* $Id: expcounter.c,v 1.3 2000/09/06 20:37:32 psfales Exp psfales $ */

/*
 * todo:
 *	Parse expdata file to get IP address
 *	Get hostname the way apache does 
 */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#include "expcounter.h"
#include "proctype.h"

char toolname[MAXCOUNTLEN];		/* "Name" of tool for counting usage */
char package[MAXCOUNTLEN];
char userinfo[MAXCOUNTLEN];		/* Optional "user info" */

char buf[1000];				/* Buffer for UDP message */

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif
char myname[MAXHOSTNAMELEN+1];		/* Fully Qualified (if possible) hostname */
char *nodename="";			/* Value from .nodename */

char *ipaddr = "127.0.0.1";		/* IP Address to send UDP message to */
int  port = 0;				/* Port to send UDP message to */

extern int opterr;      		/* for getopt() */
extern int optind;      		/* for getopt() */
extern char *optarg;    		/* for getopt() */

int vflag=0;				/* verbose flag */
int Hflag=0;				/* Hostname supplied */

void alarm_trap()
{
	if ( vflag ) {
		printf("Alarm trap!\n");
	}
	exit(1);
}

void
usage(void)
{
	fprintf(stderr,"expcounter: -n toolname -p package [ -u userinfo ] \n");
	return;
}

main(argc,argv)
char **argv;
{
	int c;		/* Arg from getopt() */
	int sd;		/* Socket descriptor */
        struct sockaddr_in  addr;	
	struct hostent *p;
	char *tmp;
	
	/* Parse the command line arguments */
	c = 0;
	while (optind < argc && c != EOF ) {
		c = getopt(argc, argv, "n:i:p:P:H:vu:");
		switch ( c ) {

		case 'n':	/* Specify tool "name" (required) */

			strncpy(toolname,optarg,sizeof(toolname));
			toolname[sizeof(toolname)-1] = '\0';

			/* If someone is using argv[0] as the tool name,
			 * and it has .exe in it, strip it off 
			 */	
			tmp = strchr(toolname,'.');
			if ( tmp && strcmp(tmp, ".exe" ) == 0 ) {
				*tmp = '\0';
			}
			break;

		case 'i':	/* Specify IP address */
	
			ipaddr = strdup(optarg);
			break;

		case 'p':	/* Specify package name (required) */
			
			strncpy(package,optarg,sizeof(package));
			package[sizeof(package)-1] = '\0';
			break;

		case 'P':	/* Specify port */

			port = atoi(optarg);
			break;

		case 'v':	/* Verbose errors */
			vflag++;
			break;

		case 'u':	/* User optional info */
			strncpy(userinfo,optarg,sizeof(userinfo));
			userinfo[sizeof(userinfo)-1] = '\0';
			break;

		case 'H':	/* Hostname[:port] */
			ipaddr = strdup(optarg);
			tmp = strchr(ipaddr,':');
			if ( tmp ) {
				*tmp = '\0';
				port = atoi(tmp+1);
			}
			Hflag = 1;
			break;

		case EOF:

			break;

		default:
			usage();
			exit(1);
		}
	}

	/* If anything below takes too long (e.g. DNS lookups on a machine
	 * that is disconnected from the network) just give up.
	 */
	signal(SIGALRM, alarm_trap);
	alarm(2);

	/* Get our hostame for lookup */
	if (gethostname(myname, sizeof(myname) - 1) != 0) {
		if ( vflag ) {
			perror("expcounter: Unable to gethostname");
		}
		exit(1);
	}
	myname[MAXHOSTNAMELEN] = '\0';


	/* Now that host names are being converted on the server, it's
	 * no longer necessary to do gethostbyname here.   That can be
	 * a particularly severe performance penalty if DNS lookups are
	 * not working for some reason
	 */

#ifdef OLD_REMOTE_LOOKUP
	p = gethostbyname(myname);
	if ( p == NULL ) {
		if ( vflag ) {
			perror("expcounter: Unable get local IP address\n");
		}
		exit(1);
	} else {		
		/* 
		 * If the host name contains a dot, we're done.  Otherwise,
		 * search the alias list 
		 */
		if (strchr(p->h_name, '.')) {
			strncpy(myname, p->h_name, MAXHOSTNAMELEN);
			myname[MAXHOSTNAMELEN] = '\0';
		} else {
			int x;

			for (x = 0; p->h_aliases[x]; ++x) {
				if (strchr(p->h_aliases[x], '.') &&
				 (!strncasecmp(p->h_aliases[x], p->h_name, strlen(p->h_name)))) {
					strncpy(myname, p->h_aliases[x], MAXHOSTNAMELEN);
					myname[MAXHOSTNAMELEN] = '\0';
					break;
				}
			}
		}
	}

	/* If we still couldn't find a fully qualified host name, then
	 * just send the IP address 
	 */
	if (strchr(myname, '.')  ==  NULL ) {
		strncpy(myname, inet_ntoa(*(struct in_addr *)p->h_addr), MAXHOSTNAMELEN);
		myname[MAXHOSTNAMELEN] = '\0';
	}
#endif

	
	/* 
	 * Get IP address and port from .expdata file, unless they 
	 * have already been specified on the command line
	 */
	if ( strcmp(ipaddr,"127.0.0.1") == 0 &&  port == 0 ) {
		getexpinfo(&ipaddr,&port,&nodename);
	}

	if  ( port == 0 ) {
		port = COUNTERPORT;
	}

	/* Die if we have extra arguments or no toolname specified */
	if ( optind != argc || argc < 2 || 
	  toolname[0] == '\0' || package[0] == '\0' ) {
		usage();
		exit(2);
	}

	snprintf(buf,sizeof(buf),"package=%s\ntool=%s\nhostname=%s\nuid=%d\npid=%d\ntype=%s\nnodename=%s\n",
		package,
		toolname,
		myname,
		getuid(),
		getppid(),
		PROCTYPE,
		nodename 
	);
	if ( userinfo[0] ) {
		sprintf(&buf[strlen(buf)],"userinfo=%s\n",userinfo);
	}	

	if ( Hflag ) {
		sd = sock_init(ipaddr, port, AF_INET, SOCK_DGRAM, &addr, 0);
	}
	else {
		sd = sock_init_a(ipaddr, port, AF_INET, SOCK_DGRAM, &addr, 0);
	}

	if ( sendto(sd, buf, strlen(buf)+1, 0, (struct sockaddr *)&addr,sizeof(addr)) < 0 ) {
		if ( vflag ) {
			perror("expcounter: client sendto failed");
		}
		exit(1);
        }
	exit(0);
}

