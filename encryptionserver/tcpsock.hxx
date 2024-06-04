#ifndef TCPSOCK_HXX
#define TCPSOCK_HXX

#if !defined(_WINDOWS) && !defined(WIN32)
 # include <sys/types.h>
 # include <sys/socket.h>
 # include <sys/time.h>
 # include <sys/wait.h>
 # include <netinet/in.h>
 # include <arpa/inet.h>
 # include <netdb.h>
#else
 #ifdef USE_DECNET
  # include "prgpre.h"
 #else
  # include <winsock.h>
 #endif
#endif

#ifdef AIX
 # include <sys/select.h>
 # include <strings.h>
#endif


#define TCPSOCKERR	(-1)
#define TCPSOCKTIMEOUT	(-2)
#define TCPSOCKMAX	(-3)

#define R_FILLBUFF	(0x01)

#define A_MYSVC		(0)
#define A_ALLSVC	(1)

#define DECNET 1
#define CLIENT 1
#define SERVER 2

class tcpsock;
#define MAXTCPSVC 10

class tcpsock
	{
public:
	tcpsock(){};
	tcpsock(char *service);			// server master constructor
	tcpsock(int socket);			// server constructor
	tcpsock(char *address,char *service);	// client constructor
	~tcpsock();

	int Getservbyname(char *svc_name);
	char *Gethostbyname(char *host_name);
	int Connect(long timeout = 0);
	void Disconnect(void);
	int Accept(int timeout = 0,int svcs = A_MYSVC);
	int StatusChg(void);
	int Read(void *buffer,int len,int flags,int timeout = 0);
	int Read(char ch,char *buffer,int len);
	int Write(void *buffer,int len);
	void SetKeepAlive(int bool_val);
	struct sockaddr_in *Getclient(struct sockaddr_in *paddr);

	int	error;

protected:
	int open_skt_as_srv();
	int open_skt_as_cli(long timeout);
	int socket_startup();
	int get_svc_entry();

	int			skt;
	struct sockaddr_in	sock_addr;
	struct sockaddr_in	client_addr;

private:
	int			my_svc_entry;
	int			status_flag;
	char			host_addr[32];
	static int		ref_count;
	static int		svc_count;
	static tcpsock		*services[MAXTCPSVC];
	};

#endif

