/***************************************************************************

  CUdpSocket.c

  (c) 2003-2004 Daniel Campos Fernández <dcamposf@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
  MA 02110-1301, USA.

***************************************************************************/

#define __CUDPSOCKET_C

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include "main.h"
#include "tools.h"

#include "CUdpSocket.h"

DECLARE_EVENT (EVENT_Read);
DECLARE_EVENT (EVENT_SocketError);

GB_STREAM_DESC UdpSocketStream = {
	.open = CUdpSocket_stream_open,
	.close = CUdpSocket_stream_close,
	.read = CUdpSocket_stream_read,
	.write = CUdpSocket_stream_write,
	.seek = CUdpSocket_stream_seek,
	.tell = CUdpSocket_stream_tell,
	.flush = CUdpSocket_stream_flush,
	.eof = CUdpSocket_stream_eof,
	.lof = CUdpSocket_stream_lof,
	.handle = CUdpSocket_stream_handle
};

static bool fill_in_addr(struct in_addr *addr, const char *str)
{
	if (!str || !*str)
		addr->s_addr = htonl(INADDR_ANY);
	else if (inet_aton(str, addr) == 0)
	{
		GB.Error("Incorrect address");
		return TRUE;
	}
	
	return FALSE;
}

void CUdpSocket_post_data(intptr_t Param)
{
	CUDPSOCKET *t_obj;
	t_obj=(CUDPSOCKET*)Param;
	GB.Raise(t_obj,EVENT_Read,0);
	GB.Unref(POINTER(&t_obj));
}

void CUdpSocket_post_error(intptr_t Param)
{
	CUDPSOCKET *t_obj;
	t_obj=(CUDPSOCKET*)Param;
	GB.Raise(t_obj,EVENT_SocketError,0);
	GB.Unref(POINTER(&t_obj));
}

static void clear_buffer(CUDPSOCKET *_object)
{
	if (THIS->buffer)
	{
		GB.Free(POINTER(&THIS->buffer));
		THIS->buffer_pos = 0;
		THIS->buffer_len = 0;
	}
}

static void fill_buffer(CUDPSOCKET *_object)
{
	socklen_t host_len;
	int ret, block;
	char buffer[1];
	
	//fprintf(stderr, "fill_buffer\n");
	
	clear_buffer(THIS);
	
	host_len = sizeof(THIS->addr);

	block = GB.Stream.Block(&SOCKET->stream, TRUE);
	USE_MSG_NOSIGNAL(ret = recvfrom(SOCKET->socket, (void*)buffer, sizeof(char), MSG_PEEK | MSG_NOSIGNAL, (struct sockaddr*)&THIS->addr, &host_len));
	GB.Stream.Block(&SOCKET->stream, block);
	
	if (ioctl(SOCKET->socket, FIONREAD, &THIS->buffer_len))
		return;
	
	//fprintf(stderr, "buffer_len = %d\n", THIS->buffer_len);
	
	if (THIS->buffer_len)
		GB.Alloc(POINTER(&THIS->buffer), THIS->buffer_len);
	
	USE_MSG_NOSIGNAL(ret = recvfrom(SOCKET->socket, (void *)THIS->buffer, THIS->buffer_len, MSG_NOSIGNAL, (struct sockaddr*)&THIS->addr, &host_len));

	//fprintf(stderr, "recvfrom() -> %d\n", ret);
	
	if (ret < 0)
	{
		CUdpSocket_stream_close(&SOCKET->stream);
		SOCKET->status = NET_CANNOT_READ;
		return;
	}

// 	THIS->sport = ntohs(host.sin_port);
// 	GB.FreeString(&THIS->shost);
// 	GB.NewString (&THIS->shost , inet_ntoa(host.sin_addr) ,0);
}

void CUdpSocket_CallBack(int t_sock,int type, intptr_t param)
{
	//struct sockaddr_in t_test;
	//unsigned int t_test_len;
	struct timespec mywait;
	CUDPSOCKET *_object = (CUDPSOCKET *)param;

	/*	Just sleeping a little to reduce CPU waste	*/
	mywait.tv_sec=0;
	mywait.tv_nsec=100000;
	nanosleep(&mywait,NULL);

	if (SOCKET->status <= NET_INACTIVE) return;

	//t_test.sin_port=0;
	//t_test_len=sizeof(struct sockaddr);

	//USE_MSG_NOSIGNAL(numpoll=recvfrom(t_sock,POINTER(buf), sizeof(char), MSG_PEEK | MSG_NOSIGNAL, (struct sockaddr*)&t_test, &t_test_len));
	fill_buffer(THIS);
	
	if (THIS->buffer)
	{
		GB.Ref((void*)THIS);
		GB.Post(CUdpSocket_post_data, (intptr_t)THIS);
	}
}
/* not allowed methods */
int CUdpSocket_stream_open(GB_STREAM *stream, const char *path, int mode, void *data){return -1;}
int CUdpSocket_stream_seek(GB_STREAM *stream, int64_t pos, int whence){return -1;}
int CUdpSocket_stream_tell(GB_STREAM *stream, int64_t *pos)
{
	*pos=0;
	return -1; /* not allowed */
}

int CUdpSocket_stream_flush(GB_STREAM *stream)
{
	return 0; /* OK */
}

int CUdpSocket_stream_handle(GB_STREAM *stream)
{
	void *_object = stream->tag;
	return SOCKET->socket;
}

int CUdpSocket_stream_close(GB_STREAM *stream)
{
	void *_object = stream->tag;

	if ( !_object ) return -1;
	stream->desc=NULL;
	if (SOCKET->status > NET_INACTIVE)
	{
		GB.Watch (SOCKET->socket,GB_WATCH_NONE,(void *)CUdpSocket_CallBack,(intptr_t)THIS);
		close(SOCKET->socket);
		SOCKET->status = NET_INACTIVE;
	}

	GB.FreeString(&THIS->thost);
	GB.FreeString(&THIS->tpath);
	
	if (THIS->path)
	{
		unlink(THIS->path);
		GB.FreeString(&THIS->path);
	}
	
	THIS->tport=0;
	SOCKET->status = NET_INACTIVE;
	clear_buffer(THIS);
	return 0;
}

int CUdpSocket_stream_lof(GB_STREAM *stream, int64_t *len)
{
	void *_object = stream->tag;
	*len = THIS->buffer_len - THIS->buffer_pos;
	return 0;
}

int CUdpSocket_stream_eof(GB_STREAM *stream)
{
	void *_object = stream->tag;
	return THIS->buffer_pos >= THIS->buffer_len;
}

int CUdpSocket_stream_read(GB_STREAM *stream, char *buffer, int len)
{
	void *_object = stream->tag;
	int len_max;

	if ( !_object ) return TRUE;
	
	len_max = THIS->buffer_len - THIS->buffer_pos;
	
	if (len_max <= 0)
		return TRUE;
	
	if (len > len_max)
		len = len_max;
		
	memcpy(buffer, &THIS->buffer[THIS->buffer_pos], len);
	THIS->buffer_pos += len;
	
	return len;
}

int CUdpSocket_stream_write(GB_STREAM *stream, char *buffer, int len)
{
	void *_object = stream->tag;
	int retval;
	//struct in_addr dest_ip;
	NET_ADDRESS dest;
	size_t size;
	struct sockaddr *addr;

	if (!THIS) return -1;
	
	CLEAR(&dest);

	if (THIS->tpath && *THIS->tpath)
	{
		dest.un.sun_family = PF_UNIX;
		strcpy(dest.un.sun_path, THIS->tpath);
		size = sizeof(struct sockaddr_un);
		addr = (struct sockaddr *)&dest.un;
	}
	else
	{
		if (fill_in_addr(&dest.in.sin_addr, THIS->thost))
			return -1;
		//dest.in.sin_addr.s_addr = dest_ip.s_addr;
		dest.in.sin_family = PF_INET;
		dest.in.sin_port = htons(THIS->tport);
		size = sizeof(struct sockaddr);
		addr = (struct sockaddr *)&dest.in;
	}

	//fprintf(stderr, "write: %s %d %d '%.*s'\n", THIS->thost, THIS->tport, len, len, buffer);

	USE_MSG_NOSIGNAL(retval = sendto(SOCKET->socket, (void*)buffer, len, MSG_NOSIGNAL, addr, size));

	if (retval < 0) 
	{
		CUdpSocket_stream_close(stream);
		SOCKET->status= NET_CANNOT_WRITE;
	}
	
	return retval;
}

/************************************************************************************************
################################################################################################
--------------------UDPSOCKET CLASS GAMBAS INTERFACE IMPLEMENTATION------------------------------
################################################################################################
***********************************************************************************************/

static bool return_error(int err, const char *msg)
{
	if (err != 0)
	{
		GB.Error(msg, strerror(errno));
		return TRUE;
	}
	else
		return FALSE;
}

static bool update_broadcast(CUDPSOCKET *_object)
{
	int broadcast;
	
	if (SOCKET->socket < 0)
		return FALSE;

	broadcast = THIS->broadcast;
	return return_error(setsockopt(SOCKET->socket, SOL_SOCKET, SO_BROADCAST, (char *)&broadcast, sizeof(int)),
		"Cannot set broadcast socket option: &1");
}

static bool update_multicast_loop(CUDPSOCKET *_object)
{
	if (SOCKET->socket < 0)
		return FALSE;

	return return_error(setsockopt(SOCKET->socket, IPPROTO_IP, IP_MULTICAST_LOOP, &THIS->mc_loop, sizeof(unsigned char)),
		"Cannot set multicast loop socket option: &1");
}

static bool update_multicast_ttl(CUDPSOCKET *_object)
{
	if (SOCKET->socket < 0)
		return FALSE;

	return return_error(setsockopt(SOCKET->socket, IPPROTO_IP, IP_MULTICAST_TTL, &THIS->mc_ttl, sizeof(unsigned char)),
		"Cannot set multicast ttl socket option: &1");
}

static void dgram_start(CUDPSOCKET *_object)
{
	sa_family_t domain;
	size_t size;
	struct stat info;
	struct sockaddr *addr;
	struct in_addr mc_interface_addr;

	if (SOCKET->status > NET_INACTIVE)
	{
		GB.Error("Socket is active");
		return;
	}
	
	if (THIS->path && *THIS->path)
	{
		domain = PF_UNIX;
		if (strlen(THIS->path) >= NET_UNIX_PATH_MAX)
		{
			GB.Error("Socket path is too long");
			return;
		}
	}
	else
	{
		domain = PF_INET;
	}

	if ((SOCKET->socket = socket(domain, SOCK_DGRAM, 0)) < 0)
		goto CANNOT_CREATE_SOCKET;

	if (update_broadcast(THIS) || SOCKET_update_timeout(SOCKET))
		goto CANNOT_CREATE_SOCKET;

	CLEAR(&THIS->addr);

	if (domain == PF_UNIX)
	{
		if (stat(THIS->path, &info) >= 0 && S_ISSOCK(info.st_mode))
			unlink(THIS->path);
		THIS->addr.un.sun_family = domain;
		strcpy(THIS->addr.un.sun_path, THIS->path);
		size = sizeof(struct sockaddr_un);
		addr = (struct sockaddr *)&THIS->addr.un;
	}
	else
	{
		THIS->addr.in.sin_family = domain;
		if (fill_in_addr(&THIS->addr.in.sin_addr, THIS->host))
			goto CANNOT_CREATE_SOCKET;

		THIS->addr.in.sin_port = htons(THIS->port);
		size = sizeof(struct sockaddr_in);
		addr = (struct sockaddr *)&THIS->addr.in;
		
		if (update_multicast_loop(THIS) || update_multicast_ttl(THIS))
			goto CANNOT_CREATE_SOCKET;
		
		if (fill_in_addr(&mc_interface_addr, THIS->mc_interface))
			goto CANNOT_CREATE_SOCKET;
		
		setsockopt(SOCKET->socket, IPPROTO_IP, IP_MULTICAST_IF, &mc_interface_addr, sizeof(mc_interface_addr));	
	}
	
	if (bind(SOCKET->socket, addr, size) < 0)
	{
		close(SOCKET->socket);
		SOCKET->status = NET_CANNOT_BIND_SOCKET;
		GB.Ref(THIS);
		GB.Post(CUdpSocket_post_error, (intptr_t)THIS);
		return;
	}

	SOCKET->status = NET_ACTIVE;
	SOCKET->stream.desc = &UdpSocketStream;
	GB.Stream.SetSwapping(&SOCKET->stream, htons(0x1234) != 0x1234);
	
	GB.Watch(SOCKET->socket, GB_WATCH_READ, (void *)CUdpSocket_CallBack, (intptr_t)THIS);
	return;
	
CANNOT_CREATE_SOCKET:

	SOCKET->status = NET_CANNOT_CREATE_SOCKET;
	GB.Ref(THIS);
	GB.Post(CUdpSocket_post_error, (intptr_t)THIS);
	return;
}


BEGIN_PROPERTY(UdpSocket_Status)

	GB.ReturnInteger(SOCKET->status);

END_PROPERTY


BEGIN_PROPERTY(UdpSocket_SourceHost)

	if (THIS->addr.a.sa_family == PF_INET)
		GB.ReturnNewZeroString(inet_ntoa(THIS->addr.in.sin_addr));
	else
		GB.ReturnVoidString();

END_PROPERTY

BEGIN_PROPERTY(UdpSocket_SourcePort)

	if (THIS->addr.a.sa_family == PF_INET)
		GB.ReturnInteger(ntohs(THIS->addr.in.sin_port));
	else
		GB.ReturnInteger(0);

END_PROPERTY

BEGIN_PROPERTY(UdpSocket_SourcePath)

	if (THIS->addr.a.sa_family == PF_UNIX)
		GB.ReturnNewZeroString(THIS->addr.un.sun_path);
	else
		GB.ReturnVoidString();

END_PROPERTY

static void handle_address_property(void *_object, void *_param, char **store, bool allow_any)
{
	char *addr;
	struct in_addr rem_ip;
	
	if (READ_PROPERTY)
	{
		GB.ReturnString(*store);
	}
	else
	{
		addr = GB.ToZeroString(PROP(GB_STRING));
		
		if ((*addr == 0 && !allow_any) || !inet_aton(addr, &rem_ip))
		{
			GB.Error("Invalid IP address");
			return;
		}
		
		GB.StoreString(PROP(GB_STRING), store);
	}
}

BEGIN_PROPERTY(UdpSocket_TargetHost)

	handle_address_property(_object, _param, &THIS->thost, FALSE);

END_PROPERTY

BEGIN_PROPERTY(UdpSocket_TargetPort)

	if (READ_PROPERTY)
		GB.ReturnInteger(THIS->tport);
	else
	{
		int port = VPROP(GB_INTEGER);
		
		if (port < 1 || port > 65535)
		{
			GB.Error("Invalid port number");
			return;
		}
	
		THIS->tport = port;
	}

END_PROPERTY

BEGIN_PROPERTY(UdpSocket_TargetPath)

	if (READ_PROPERTY)
		GB.ReturnString(THIS->tpath);
	else
	{
		if (PLENGTH() >= NET_UNIX_PATH_MAX)
		{
			GB.Error("Socket path is too long");
			return;
		}
		GB.StoreString(PROP(GB_STRING), &THIS->tpath);
	}

END_PROPERTY

/*************************************************
Gambas object "Constructor"
*************************************************/

BEGIN_METHOD_VOID(UdpSocket_new)

	SOCKET->stream.tag = _object;
	SOCKET->socket = -1;
	THIS->mc_loop = 1;
	THIS->mc_ttl = 1;

END_METHOD

/*************************************************
Gambas object "Destructor"
*************************************************/

BEGIN_METHOD_VOID(UdpSocket_free)

	CUdpSocket_stream_close(&SOCKET->stream);
	GB.FreeString(&THIS->host);

END_METHOD


BEGIN_METHOD_VOID (UdpSocket_Peek)

	char *sData=NULL;
	socklen_t host_len;
	int retval=0;
	//int NoBlock=0;
	int peeking;
	int bytes=0;
	if (SOCKET->status <= NET_INACTIVE)
	{
		GB.Error("Socket is inactive");
		return;
	}

	peeking = MSG_NOSIGNAL | MSG_PEEK;

	ioctl(SOCKET->socket,FIONREAD,&bytes);
	if (bytes)
	{
		GB.Alloc( POINTER(&sData),bytes*sizeof(char) );
		host_len = sizeof(THIS->addr);
		//ioctl(SOCKET->socket,FIONBIO,&NoBlock);
		USE_MSG_NOSIGNAL(retval=recvfrom(SOCKET->socket, (void*)sData, 1024 * sizeof(char), peeking, (struct sockaddr*)&THIS->addr, &host_len));
		if (retval<0)
		{
			GB.Free(POINTER(&sData));
			CUdpSocket_stream_close(&SOCKET->stream);
			SOCKET->status = NET_CANNOT_READ;
			GB.Raise(THIS,EVENT_SocketError,0);
			GB.ReturnVoidString();
			return;
		}
		//NoBlock++;
		//ioctl(SOCKET->socket,FIONBIO,&NoBlock);
		if (retval>0)
			GB.ReturnNewString(sData,retval);
		else
			GB.ReturnVoidString();
		GB.Free(POINTER(&sData));
	}
	else
	{
		GB.ReturnVoidString();
	}

END_METHOD

BEGIN_METHOD_VOID(UdpSocket_Bind)

	dgram_start(THIS);

END_METHOD


BEGIN_PROPERTY(UdpSocket_Broadcast)

	if (READ_PROPERTY)
	{
		GB.ReturnBoolean(THIS->broadcast);
	}
	else
	{
		THIS->broadcast = VPROP(GB_BOOLEAN);
		update_broadcast(THIS);
	}

END_PROPERTY

BEGIN_PROPERTY(UdpSocket_Port)

	if (READ_PROPERTY)
		GB.ReturnInteger(THIS->port);
	else
	{
		int port = VPROP(GB_INTEGER);
		if (port < 0 || port > 65535)
		{
			GB.Error("Invalid port value");
			return;
		}
		if (SOCKET->status > NET_INACTIVE)
		{
			GB.Error("Socket is active");
			return;
		}
		THIS->port = port;
	}

END_PROPERTY

BEGIN_PROPERTY(UdpSocket_Path)

	if (READ_PROPERTY)
		GB.ReturnString(THIS->path);
	else
	{
		if (SOCKET->status > NET_INACTIVE)
		{
			GB.Error("Socket is active");
			return;
		}
		GB.StoreString(PROP(GB_STRING), &THIS->path);
	}

END_PROPERTY

BEGIN_PROPERTY(UdpSocket_Host)

	handle_address_property(_object, _param, &THIS->host, FALSE);

END_PROPERTY


//-------------------------------------------------------------------------

BEGIN_PROPERTY(UdpSocketMulticast_Loop)

	if (READ_PROPERTY)
		GB.ReturnBoolean(THIS->mc_loop);
	else
	{
		THIS->mc_loop = VPROP(GB_BOOLEAN);
		update_multicast_loop(THIS);
	}

END_PROPERTY

BEGIN_PROPERTY(UdpSocketMulticast_Ttl)

	if (READ_PROPERTY)
		GB.ReturnInteger(THIS->mc_ttl);
	else
	{
		int ttl = VPROP(GB_INTEGER);
		if (ttl < 0 || ttl > 255)
		{
			GB.Error(GB_ERR_ARG);
			return;
		}
		THIS->mc_ttl = ttl;
		update_multicast_ttl(THIS);
	}

END_PROPERTY

BEGIN_PROPERTY(UdpSocketMulticast_Interface)

	handle_address_property(_object, _param, &THIS->mc_interface, TRUE);

END_PROPERTY

static void handle_multicast_membership(CUDPSOCKET *_object, bool add, GB_STRING *group, GB_STRING *interface)
{
	struct ip_mreq membership;
	
	if (SOCKET->socket < 0)
	{
		GB.Error("UDP socket is not binded");
		return;
	}
	
	if (fill_in_addr(&membership.imr_multiaddr, GB.ToZeroString(group)))
		return;
	
	if (fill_in_addr(&membership.imr_interface, interface ? GB.ToZeroString(interface) : NULL))
		return;
	
	if (setsockopt(SOCKET->socket, IPPROTO_IP, add ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP, &membership, sizeof(membership)))
		GB.Error(add ? "Cannot join multicast group: &1" : "Cannot leave multicast group: &1", strerror(errno));
}

BEGIN_METHOD(UdpSocketMulticast_Join, GB_STRING group; GB_STRING interface)

	handle_multicast_membership(THIS, TRUE, ARG(group), MISSING(interface) ? NULL : ARG(interface));

END_METHOD

BEGIN_METHOD(UdpSocketMulticast_Leave, GB_STRING group; GB_STRING interface)

	handle_multicast_membership(THIS, FALSE, ARG(group), MISSING(interface) ? NULL : ARG(interface));

END_METHOD


/***************************************************************
Here we declare the public interface of UdpSocket class
***************************************************************/

GB_DESC CUdpSocketMulticastDesc[] = 
{
	GB_DECLARE_VIRTUAL(".UdpSocket.Multicast"),
	
	GB_PROPERTY("Loop", "b", UdpSocketMulticast_Loop),
	GB_PROPERTY("Ttl", "i", UdpSocketMulticast_Ttl),
	GB_PROPERTY("Interface", "s", UdpSocketMulticast_Interface),

	GB_METHOD("Join", NULL, UdpSocketMulticast_Join, "(Group)s[(Interface)s]"),
	GB_METHOD("Leave", NULL, UdpSocketMulticast_Leave, "(Group)s[(Interface)s]"),
	
	GB_END_DECLARE
};

GB_DESC CUdpSocketDesc[] =
{
	GB_DECLARE("UdpSocket", sizeof(CUDPSOCKET)),

	GB_INHERITS("Stream"),

	GB_EVENT("Error", NULL, NULL, &EVENT_SocketError),
	GB_EVENT("Read", NULL, NULL, &EVENT_Read),

	GB_METHOD("_new", NULL, UdpSocket_new, NULL),
	GB_METHOD("_free", NULL, UdpSocket_free, NULL),
	GB_METHOD("Bind", NULL, UdpSocket_Bind, NULL),
	GB_METHOD("Peek", "s", UdpSocket_Peek,NULL),

	GB_PROPERTY_READ("Status", "i", UdpSocket_Status),
	GB_PROPERTY_READ("SourceHost", "s", UdpSocket_SourceHost),
	GB_PROPERTY_READ("SourcePort", "i", UdpSocket_SourcePort),
	GB_PROPERTY_READ("SourcePath", "s", UdpSocket_SourcePath),
	GB_PROPERTY("TargetHost", "s", UdpSocket_TargetHost),
	GB_PROPERTY("TargetPort", "i", UdpSocket_TargetPort),
	GB_PROPERTY("TargetPath", "s", UdpSocket_TargetPath),

	GB_PROPERTY("Host", "s", UdpSocket_Host),
	GB_PROPERTY("Port", "i", UdpSocket_Port),
	GB_PROPERTY("Path", "s", UdpSocket_Path),
	
	GB_PROPERTY("Broadcast", "b", UdpSocket_Broadcast),
	GB_PROPERTY("Timeout", "i", Socket_Timeout),
	
	GB_PROPERTY_SELF("Multicast", ".UdpSocket.Multicast"),

  GB_CONSTANT("_IsControl", "b", TRUE),
  GB_CONSTANT("_IsVirtual", "b", TRUE),
  GB_CONSTANT("_Group", "s", "Network"),
	GB_CONSTANT("_Properties", "s", "Port{Range:0;65535},Path,TargetHost,TargetPort,Broadcast"),
	GB_CONSTANT("_DefaultEvent", "s", "Read"),

	GB_END_DECLARE
};


