/*
* File Name :	tcpsock.cxx
*	Author	:	Paul Franklin
*
* History	:
*
*	02/07/97 PJF	Added the Getclient() method to retrieve client address info.
*
*	01/24/97 JTK	Added an overload of the Read method that takes a character as a parameter
*				and waits for this character on the recv or until the buffer is full.  All
*				excess characters in the buffer after the passed delimiter are replaced with
*				NULL's
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if !defined(_WINDOWS) && !defined(WIN32)
 # include <unistd.h>
 # include <fcntl.h>
 # define SockErr	errno
 # define HostErr	h_errno
 extern int h_errno;
#else
 # include <windows.h>

 # if defined(_WINDOWS) && ! defined MAKEWORD
  #   define MAKEWORD(lo,hi)    ((WORD)(((BYTE)(lo))|(((WORD)((BYTE)(hi)))<<8)))
 # endif

 # define SockErr	WSAGetLastError()
 # define HostErr	WSAGetLastError()
 # define endservent()
 # define endhostent()
 # define close(p1)	closesocket(p1)
 # define EWOULDBLOCK	WSAEWOULDBLOCK
 # define EINPROGRESS	WSAEINPROGRESS
#endif

#include <tcpsock.hxx>

#ifndef SOMAXCONN
 # define SOMAXCONN 128
#endif

int		tcpsock::ref_count = 0;
int		tcpsock::svc_count = 0;
tcpsock	*tcpsock::services[MAXTCPSVC];

///////////////////////////////////////////////////////////////////////////////
int tcpsock::get_svc_entry(void)
 {
 int i;

 if(svc_count < MAXTCPSVC)
	return svc_count++;

 for(i = 0;i < MAXTCPSVC;++i)
	{
	if(services[i] == NULL)
		return i;
	}

 return TCPSOCKMAX;
 }


///////////////////////////////////////////////////////////////////////////////
int tcpsock::socket_startup(void)
 {
#if defined(_WINDOWS) || defined(WIN32)
 WORD		wVer = MAKEWORD( 1, 1 );    // minor, major
 WSADATA	wsaData;
 int		stat;

 stat = WSAStartup(wVer,&wsaData);
 if( stat != 0 )
	{
	error = stat;
	return TCPSOCKERR;
	}
#endif

 return 0;
 }


///////////////////////////////////////////////////////////////////////////////
static int is_numeric(char *s)
 {
 if(!s)
	return 0;

 while(*s)
	{
	if(((*s < '0') || (*s > '9')) && (*s != '.'))
		return 0;

	++s;
	}

 return 1;
 }


///////////////////////////////////////////////////////////////////////////////
//
// server master constructor
//
///////////////////////////////////////////////////////////////////////////////
tcpsock::tcpsock(char *service)
 {
 int	port;

 error = 0;
 skt = 0;
 status_flag = 0;

 if(socket_startup() == TCPSOCKERR)
	return;

 memset(&sock_addr,0,sizeof(sock_addr));

 if(is_numeric(service))
	sock_addr.sin_port = htons(atoi(service));
 else
	{
	if((port = Getservbyname(service)) == TCPSOCKERR)
		return;

	sock_addr.sin_port = port;
	}

 if((my_svc_entry = get_svc_entry()) == TCPSOCKMAX)
	{
	error = TCPSOCKMAX;
	return;
	}

 if(open_skt_as_srv() >= 0)
	services[my_svc_entry] = this;
 
 ++ref_count;
 }


///////////////////////////////////////////////////////////////////////////////
//
// server constructor
//
///////////////////////////////////////////////////////////////////////////////
tcpsock::tcpsock(int socket)
 {
 error = 0;
 skt = socket;
 my_svc_entry = -1;
 status_flag = 0;
 
 ++ref_count;
 }



///////////////////////////////////////////////////////////////////////////////
//
// client constructor
//
///////////////////////////////////////////////////////////////////////////////
tcpsock::tcpsock(char *address,char *service)
 {
 unsigned long	addr;
 int		port;

 error = 0;
 skt = 0;
 my_svc_entry = -1;
 status_flag = 0;

 if(socket_startup() == TCPSOCKERR)
	return;

 memset(&sock_addr,0,sizeof(sock_addr));

 if(is_numeric(service))
	sock_addr.sin_port = htons(atoi(service));
 else
	{
	if((port = Getservbyname(service)) == TCPSOCKERR)
		return;

	sock_addr.sin_port = port;
	}

 if(is_numeric(address))
	{
	addr = (unsigned long)inet_addr(address);
	memcpy(&sock_addr.sin_addr,&addr,sizeof(struct in_addr));
	}
 else
	{
	char *tmp;

	if((tmp = Gethostbyname(address)))
		{
		addr = (unsigned long)inet_addr(tmp);
		memcpy(&sock_addr.sin_addr,&addr,sizeof(struct in_addr));
		}
	else
		return;
	}
 }



///////////////////////////////////////////////////////////////////////////////
//
// destructor
//
///////////////////////////////////////////////////////////////////////////////
tcpsock::~tcpsock()
 {
 Disconnect();

 if(my_svc_entry >= 0)
	services[my_svc_entry] = NULL;

 --ref_count;

#if defined(_WINDOWS) || defined(WIN32)
 if(!ref_count)
	WSACleanup();
#endif
 }



///////////////////////////////////////////////////////////////////////////////
//
// looks up port number for specified service name, returns in network order
//
///////////////////////////////////////////////////////////////////////////////
int tcpsock::Getservbyname(char *svc_name)
 {
 struct servent	*psrv;
 int		rc;

 if((psrv = getservbyname(svc_name,(const char *)NULL)))
	rc = psrv->s_port;
 else
	{
	rc = TCPSOCKERR;
	error = HostErr;
	}

 endservent();

 return rc;
 }


///////////////////////////////////////////////////////////////////////////////
//
// looks up IP address from host name
//
///////////////////////////////////////////////////////////////////////////////
char *tcpsock::Gethostbyname(char *host_name)
 {
 struct hostent		*phost;
 char			*rc = NULL;
 struct in_addr		net_addr;

 if((phost = gethostbyname(host_name)))
	{
	memcpy(&net_addr,phost->h_addr,phost->h_length);
	strncpy(host_addr,inet_ntoa(net_addr),sizeof(host_addr));
	host_addr[sizeof(host_addr) - 1] = '\0';
	rc = host_addr;
	}
 else
	error = HostErr;

 endhostent();

 return rc;
 }


///////////////////////////////////////////////////////////////////////////////
//
// Connects a client socket
//
// INPUTS:
//  timeout - An optional timeout value given in milliseconds. The
//            default value for this parm is 0, indicating that the
//            default timeout for the TCP/IP stack will be used. In
//            this case, the connect will block until it completes or
//            fails. Most TCP/IP stacks will timeout in 30-60 seconds
//            if the connection does not succeed. If timeout > 0, the
//            function will return after timeout milliseconds have passed
//            without successfully connecting. 
//            In this case, SockErr will be set to TCPSOCKTIMEOUT.
//
// OUTPUTS:
//  none
//
// RETURNS:
//  zero - connect was successful.
//  TCPSOCKERR - error, see SockErr
//
///////////////////////////////////////////////////////////////////////////////
int tcpsock::Connect(long timeout)
 {
 int	rc;

 rc = open_skt_as_cli(timeout);
 return (rc == TCPSOCKERR) ? rc : 0;
 }


///////////////////////////////////////////////////////////////////////////////
//
// Disconnects a client socket
//
///////////////////////////////////////////////////////////////////////////////
void tcpsock::Disconnect()
 {
 shutdown(skt,2);
 close(skt);
 }


///////////////////////////////////////////////////////////////////////////////
//
// Accepts a connection on a passive (server) socket
//
// INPUTS:
//  timeout - An optional timeout value given in seconds. The
//            default value for this parm is 0, indicating no
//            accept timeout - the accept will block until a client
//            connects. if > 0, the function will return after
//            timeout seconds have passed with no connection.
//            In this case, SockErr will be set to TCPSOCKTIMEOUT.
//
// OUTPUTS: None
// RETURNS:
//  positive integer - success. a valid file descriptor is returned.
//  zero - timeout.
//  TCPSOCKERR - error, see SockErr
//  
///////////////////////////////////////////////////////////////////////////////
int tcpsock::Accept(int timeout,int svcs)
 {
 int		i,rc,fd,nfds;
 socklen_t      addrlen;
 fd_set		readfds, exceptfds;
 struct timeval	tvs,*ptvs;
 tcpsock	*ps;

 for(;;)
	{
	if(timeout)
		{
		tvs.tv_sec  = 1;
		tvs.tv_usec = 0;
		ptvs = &tvs;
		}
	else
		ptvs = NULL;

	// set read fd to monitor
	FD_ZERO(&readfds);

	// set exception fd to monitor
	FD_ZERO(&exceptfds);

	if(svcs == A_ALLSVC)
		{
		nfds = 0;
		for(i = 0;i < svc_count;++i)
			{
			if(services[i])
				{
				FD_SET(services[i]->skt,&readfds);
				FD_SET(services[i]->skt,&exceptfds);

				if(services[i]->skt > nfds)
					nfds = services[i]->skt;
				}
			}
		++nfds;
		}
	else
		{
		FD_SET(skt,&readfds);
		FD_SET(skt,&exceptfds);
		nfds = skt + 1;
		}

#	ifndef HP_UX
	 rc = select(nfds,&readfds,(fd_set *)NULL,&exceptfds,ptvs );
#	else
	 rc = select(nfds,(int *)&readfds,(int *)NULL,(int *)&exceptfds,ptvs );
#	endif

	// check for select error
	if(rc < 0)
		{
		error = SockErr;
		status_flag = 1;
		return TCPSOCKERR;
		}

	// check for timeout
	if(rc == 0)
		{
		if(--timeout <= 0)
			{
			error = TCPSOCKTIMEOUT;
			return 0;
			}

		continue;
		}

	// check for readable, exception, on all ports
	for(i = 0;i < svc_count;++i)
		{
		if(ps = services[i])
			{
			if(FD_ISSET(ps->skt,&exceptfds))
				{
				ps->status_flag = 1;
				ps->error = SockErr;
				return TCPSOCKERR;
				}

			if(FD_ISSET(ps->skt,&readfds))
				{
				ps->status_flag = 1;
				memset(&ps->client_addr,0,sizeof(client_addr));
				addrlen = sizeof(client_addr);
				if((fd = accept(ps->skt,(struct sockaddr *)&ps->client_addr,&addrlen)) < 0)
					{
					ps->error = SockErr;
					return TCPSOCKERR;
					}

				return fd;
				}
			}
		}
	}
 }


///////////////////////////////////////////////////////////////////////////////
int tcpsock::StatusChg(void)
 {
 if(status_flag)
	{
	status_flag = 0;
	return 1;
	}

 return 0;
 }


///////////////////////////////////////////////////////////////////////////////
//
// Reads from socket
//
// INPUTS:
//  buffer  - pointer to buffer to place read data
//  len     - maximum number of bytes to read into buffer
//  flags   - if 0, returns after one successful read. The amount
//            of data returned wil be <= len.
//            if the R_FILLBUFF bit is set, this function will
//            attempt to read the number of bytes specified by len.
//            This is useful when the amount of data to read is
//            known in advance.
//  timeout - An optional timeout value given in seconds. The
//            default value for this parm is 0, indicating no
//            read timeout - the read will block until data is
//            available. if > 0, the function will return after
//            timeout seconds have passed with no data to read.
//            In this case, SockErr will be set to TCPSOCKTIMEOUT.
//
// OUTPUTS:
//  buffer  - received data will be in area pointed to by buffer.
//
// RETURNS:
//  positive integer - number of bytes read.
//  zero - no data read. see SockErr for reason.
//  TCPSOCKERR - error, see SockErr
//
///////////////////////////////////////////////////////////////////////////////
int tcpsock::Read(void *buffer,int len,int flags,int timeout)
 {
 int			rc,index = 0;
 fd_set			readfds, exceptfds;
 struct timeval	tvs,*ptvs = (struct timeval *)NULL;

 while(len > 0)
	{
	if(timeout)
		{
		tvs.tv_sec  = 1;
		tvs.tv_usec = 0;
		ptvs = &tvs;
		}

	// set read fd to monitor
	FD_ZERO(&readfds);
	FD_SET(skt,&readfds);

	// set exception fd to monitor
	FD_ZERO(&exceptfds);
	FD_SET(skt,&exceptfds);

#	ifndef HP_UX
	 rc = select(skt + 1,&readfds,(fd_set *)NULL,&exceptfds,ptvs );
#	else
	 rc = select(skt + 1,(int *)&readfds,(int *)NULL,(int *)&exceptfds,ptvs );
#	endif

// printf("select() = %d, errno = %d, E = %x, R = %x\n",rc,errno,FD_ISSET(skt,&exceptfds),FD_ISSET(skt,&readfds));
// fflush(stdout);

	// check for select error
	if(rc < 0)
		{
		error = SockErr;
		return TCPSOCKERR;
		}

	// check for exception
	if((rc > 0) && (FD_ISSET(skt,&exceptfds) != 0))
		{
		error = SockErr;
		return TCPSOCKERR;
		}

	// check for timeout
	if(rc == 0)
		{
		if(--timeout == 0)
			{
			error = TCPSOCKTIMEOUT;
			break;
			}

		continue;
		}
	
	// got here, must be OK!
	if((rc = recv(skt,&((char *)buffer)[index],len,0)) <= 0)
		{
		error = SockErr;
		return TCPSOCKERR;
		}

	len -= rc;			// reduce the amount of buffer available
	index += rc;		// move pointer in buffer over

	if(!(flags & R_FILLBUFF))	// Do you want to wait for a full buffer
		break;
	}

 return index;
 }

///////////////////////////////////////////////////////////////////////////////
//
// Reads from socket waiting for passed character or until buffer is full
// Any character in buffer after delimiter will be wiped out with NULLS
//
///////////////////////////////////////////////////////////////////////////////
int tcpsock::Read(char ch,char *buffer,int len)
{
 
	int	rc,index = 0;

	char	*target = NULL;

	while(len > 0)
	{
		if((rc = recv(skt,&((char *)buffer)[index],len,0)) <= 0)
		{
			error = SockErr;
			break;
		}

		len -= rc;		// reduce the amount of buffer available
		index += rc;	// move pointer in buffer over

		if(len == 0)
			break;

		if(target = strchr(buffer,ch))	// If we have a positive return value
		{
			if(target != (buffer + index - 1))	// If the target character is not the last one
			{
				memset(target+1,'\0',buffer-target);	// trim the end
				index = buffer - target;				// reset return value
			}

			break;
		}
	}

	return index;
 }

///////////////////////////////////////////////////////////////////////////////
//
// Writes to socket
//
///////////////////////////////////////////////////////////////////////////////
int tcpsock::Write(void *buffer,int len)
 {
 int	rc,index = 0;

 while(len > 0)
	{
	if((rc = send(skt,&((char *)buffer)[index],len,0)) <= 0)
		{
		error = SockErr;
		break;
		}

	len -= rc;
	index += rc;
	}

 return index;
 }

///////////////////////////////////////////////////////////////////////////////
// Enable/disable keep-alive
///////////////////////////////////////////////////////////////////////////////
void tcpsock::SetKeepAlive(int bool_val)
 {
 if(skt)
	setsockopt(skt,SOL_SOCKET,SO_KEEPALIVE,(const char *)&bool_val,sizeof(bool_val));
 }

///////////////////////////////////////////////////////////////////////////////
//
// Opens a stream socket for passive connection on specified port
//
///////////////////////////////////////////////////////////////////////////////
int tcpsock::open_skt_as_srv()
 {
 int option = 1;

 skt = socket( AF_INET, SOCK_STREAM, 0 );
 if ( skt == -1  )
	{
	error = SockErr;
	return TCPSOCKERR;
	}

 sock_addr.sin_family = AF_INET;			// what else ?
 sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);		// listen on all interfaces

 setsockopt(skt,SOL_SOCKET,SO_REUSEADDR,(const char *)&option,sizeof(option));

 if(bind(skt,(struct sockaddr *)&sock_addr,sizeof(sock_addr)) < 0)
	{
	error = SockErr;
	close(skt);
	return TCPSOCKERR;
	}

 if(listen(skt,SOMAXCONN) < 0)		// make it passive
	{
	error = SockErr;
	close(skt);
	return TCPSOCKERR;
	}

 return skt;
 }


///////////////////////////////////////////////////////////////////////////////
//
// creates a stream socket connection to the specified host & service
//
///////////////////////////////////////////////////////////////////////////////
int tcpsock::open_skt_as_cli(long timeout)
 {
 struct timeval	tvs;
 fd_set			readfds, writefds, exceptfds;
 int rc;
#if !defined(_WINDOWS) && !defined(WIN32)
 int			flags;
#else
 unsigned		long flags;
 struct sockaddr_in	bsock_addr;
#endif

 skt = socket( AF_INET, SOCK_STREAM, 0 );
 if ( skt == -1 )
	{
	error = SockErr;
	return TCPSOCKERR;
	}

 if(timeout > 0)
	{
#if !defined(_WINDOWS) && !defined(WIN32)
	flags = fcntl(skt,F_GETFL,0);
	if(flags >= 0)
		fcntl(skt,F_SETFL,flags | O_NONBLOCK);
#else
	flags = 1; // enable non-blocking
	ioctlsocket(skt,FIONBIO,&flags);
#endif
	}

#if defined(_WINDOWS) || defined(WIN32)
 // 2/1/99 try setting local port for connect, trying to fix a stupid NT problem
 memset(&bsock_addr,0,sizeof(bsock_addr));
 // bsock_addr.sin_port = htons(atoi(service));

 if(bind(skt,(struct sockaddr *)&bsock_addr,sizeof(bsock_addr)) < 0)
	{
	error = SockErr;
	close(skt);
	return TCPSOCKERR;
	}
#endif

 sock_addr.sin_family = AF_INET;

 // connect to the server
 if(connect(skt,(struct sockaddr *)&sock_addr,sizeof(sock_addr)) < 0)
	{
	error = SockErr;
	if(timeout <= 0)
		{
		close(skt);
		return TCPSOCKERR;
		}
	else
		{
		if((error != EWOULDBLOCK) && (error != EINPROGRESS))
			{
			close(skt);
			return TCPSOCKERR;
			}

		// timeout is in milliseconds
		tvs.tv_sec  = timeout / 1000;
		tvs.tv_usec = (timeout % 1000) * 1000;

		// set write fd to monitor
		FD_ZERO(&writefds);
		FD_SET(skt,&writefds);

		// set exception fd to monitor
		FD_ZERO(&exceptfds);
		FD_SET(skt,&exceptfds);

		// set read fd to monitor
		FD_ZERO(&readfds);
		FD_SET(skt,&readfds);

#		ifndef HP_UX
		 // rc = select(skt + 1,(fd_set *)NULL,&writefds,&exceptfds,&tvs );
		 rc = select(skt + 1,&readfds,&writefds,&exceptfds,&tvs );
#		else
		 rc = select(skt + 1,(int *)&readfds,(int *)&writefds,(int *)&exceptfds,&tvs );
#		endif

	// printf("rc = %d, read = %x, write = %x, except = %x, errno = %d\n",rc,FD_ISSET(skt,&readfds),FD_ISSET(skt,&writefds),FD_ISSET(skt,&exceptfds),errno);
	// fflush(stdout);

		// check for select error
		if(rc < 0)
			{
			error = SockErr;
			close(skt);
			return TCPSOCKERR;
			}

		// check for refused connection
		if((rc > 0) && (FD_ISSET(skt,&readfds) != 0) && (FD_ISSET(skt,&writefds) != 0))
			{
			error = SockErr;
			close(skt);
			return TCPSOCKERR;
			}

		// check for timeout
		if(rc == 0)
			{
			error = TCPSOCKTIMEOUT;
			close(skt);
			return TCPSOCKERR;
			}
		}
	}

 if(timeout > 0)
	{
#if !defined(_WINDOWS) && !defined(WIN32)
	fcntl(skt,F_SETFL,flags);
#else
	flags = 0; // enable blocking
	ioctlsocket(skt,FIONBIO,&flags);
#endif
	}

 return skt;
 }

///////////////////////////////////////////////////////////////////////////////
//
// Copies the sockaddr structure from last Accept to structure supplied by caller
//
///////////////////////////////////////////////////////////////////////////////
struct sockaddr_in *tcpsock::Getclient(struct sockaddr_in *paddr)
 {
 if(paddr)
 	memcpy(paddr,&client_addr,sizeof(*paddr));
 
 return paddr;
 }
