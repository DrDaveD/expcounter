#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

extern int vflag;

int
sock_init(char *host, int port, int family, int type, struct sockaddr_in *addr, int client)
{
	int	sd;
	struct sockaddr_in *in_name;
	struct hostent *hptr;
	
	sd = socket(family, type, 0 );
	if ( sd < 0 )  {
		perror("sock_init");
		exit(1);
	}
		
	in_name = (struct sockaddr_in *)addr;
	in_name->sin_family = family;
	in_name->sin_port = htons(port);
	
	if ( host != NULL ) {

		hptr = gethostbyname( host );
		if ( hptr == NULL ) {
			fprintf(stderr,"Unable to find host %s\n",host);
			exit(1);
		}
		memcpy( &in_name->sin_addr.s_addr,hptr->h_addr,hptr->h_length);
	}
	else {
		in_name->sin_addr.s_addr = INADDR_ANY;
	}
	return(sd);

}

int
sock_init_a(char *host, int port, int family, int type, struct sockaddr_in *addr, int client)
{
	int	sd;
	struct sockaddr_in *in_name;
	struct hostent *hptr;
	u_long ipaddr;

	sd = socket(family, type, 0 );
	if ( sd < 0 )  {
		perror("sock_init");
		exit(1);
	}
		
	in_name = (struct sockaddr_in *)addr;
	in_name->sin_family = family;
	in_name->sin_port = htons(port);
	
	if ( host != NULL ) {
		ipaddr = inet_addr(host);
		if ( ipaddr == -1 ) {
			fprintf(stderr,"Invalid IP address: %s\n",host);
			exit(1);
		}
		in_name->sin_addr.s_addr = ipaddr;
 	}
	else {
		in_name->sin_addr.s_addr = INADDR_ANY;
	}
	return(sd);

}
