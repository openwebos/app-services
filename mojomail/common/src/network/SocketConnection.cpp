// @@@LICENSE
//
//      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// LICENSE@@@

#include "network/SocketConnection.h"
#include "palmsocket.h"
#include "CommonPrivate.h"
#include "exceptions/GErrorException.h"
#include <sstream>
#include "exceptions/ExceptionUtils.h"

#ifdef MOJ_LINUX
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

using namespace std;

SocketConnection::SocketConnection(GIOChannel* channel, bool useSsl)
: GIOChannelWrapper(channel),
  m_useSsl(useSsl),
  m_bindPort(0),
  m_connectTimeout(0),
  m_connectedSignal(this),
  m_tlsReadySignal(this)
{
}

SocketConnection::~SocketConnection()
{
}

MojRefCountedPtr<SocketConnection> SocketConnection::CreateSocket(const char* hostname, unsigned int port, bool useSsl, const string& bindAddress, int bindPort)
{
	GError* gerr = NULL; // must be initialized

	// Create palm socket channel. Bind address is NULL if there's no bind address.
	GIOChannel* channel = PmNewIoChannel(hostname, port,
			!bindAddress.empty() ? bindAddress.c_str() : NULL, bindPort, &gerr);
	GErrorToException(gerr);
	
	MojRefCountedPtr<SocketConnection> aioChannel( new SocketConnection(channel, useSsl) );
	
	gerr = NULL; // must be initialized

	// Set null encoding
	g_io_channel_set_encoding(channel, NULL, &gerr);
	GErrorToException(gerr);
	
	// Disable buffering, since this interferes with STARTTLS and the like
	g_io_channel_set_buffered(channel, false);

	// Make sure the channel gets closed on unref
	g_io_channel_set_close_on_unref(channel, true);

	aioChannel->m_bindAddress = bindAddress;

	return aioChannel;
}

void SocketConnection::SetConnectTimeout(int seconds)
{
	m_connectTimeout = seconds;
}

void SocketConnection::Connect(ConnectedSignal::SlotRef connectedSlot)
{
	GError* gerr = NULL; // must be initialized
	
	m_connectedSignal.connect(connectedSlot);
	
	if(m_connectTimeout > 0) {
		m_connectTimer.SetTimeout(m_connectTimeout, this, &SocketConnection::ConnectTimeout);
	}

	try {

		if (m_useSsl)
			PmSslSocketConnect(m_channel, gpointer(this), (PmSocketConnectCb) &SocketConnection::SocketConnectCb, &gerr);
		else
			PmSocketConnect(m_channel, gpointer(this), (PmSocketConnectCb) &SocketConnection::SocketConnectCb, &gerr);

		GErrorToException(gerr);
	} catch(...) {
		m_connectTimer.Cancel();

		throw;
	}
}

gboolean SocketConnection::SocketConnectCb(GIOChannel* gio, gpointer data, const GError* error)
{
	SocketConnection *socketConnection = reinterpret_cast<SocketConnection *>( data );
	assert(data);
	
	// Temp reference to keep SocketConnection reference alive while we're executing it
	MojRefCountedPtr<SocketConnection> temp(socketConnection);

	// Note: error should not be freed

	socketConnection->m_connectTimer.Cancel();

	if(error == NULL)
		socketConnection->Connected();
	else
		socketConnection->ConnectError(error);
	
	return true;
}

void SocketConnection::ConnectTimeout()
{
	MojRefCountedPtr<SocketConnection> temp(this); // temp reference to keep alive

	MailNetworkTimeoutException exc("connect timed out", __FILE__, __LINE__);

	Shutdown();

	m_connectedSignal.fire(&exc);
}

void SocketConnection::NegotiateTLS(TLSReadySignal::SlotRef tlsReadySlot)
{
	GError* gerr = NULL; // must be initialized
	m_tlsReadySignal.connect(tlsReadySlot);
	PmSetSocketEncryption(m_channel, true, gpointer(this), (PmSecureSocketSwitchCb) &SocketConnection::NegotiateTLSCb, &gerr);
	GErrorToException(gerr);
}

gboolean SocketConnection::NegotiateTLSCb(GIOChannel* gio, gboolean isSocketEncrypted, gpointer data, const GError* error)
{
	SocketConnection *socketConnection = reinterpret_cast<SocketConnection *>( data );

	// Temp reference to keep SocketConnection reference alive while we're executing it
	MojRefCountedPtr<SocketConnection> temp(socketConnection);

	// Note: error should not be freed

	if(error == NULL && isSocketEncrypted)
		socketConnection->TLSReady();
	else
		socketConnection->TLSError(error);

	return true;
}

void SocketConnection::TLSReady()
{
	m_tlsReadySignal.fire(NULL);
}

// Check if the error is a SSL error, and update the exception if needed
void SocketConnection::UpdatePslVerifyResult(PslErrorException& exc)
{
	if(exc.GetPslError() == PSL_ERR_SSL_CERT_VERIFY) {
		PmSockPeerCertVerifyErrorInfo result;
		PslError getErrorError = PmSockGetPeerCertVerifyError( (PmSockIOChannel*) m_channel, &result );
		if(!getErrorError) {
			exc.SetSSLVerifyResult(result.opensslx509_v_err);
		}
	}
}

void SocketConnection::TLSError(const GError* error)
{
	if(error == NULL){
		MailException exc("TLS not encrypted", __FILE__, __LINE__);
		m_tlsReadySignal.fire(&exc);
	} else {
		PslError pslError = PmSockGetLastError( (PmSockIOChannel*) m_channel );
		PslErrorException exc(pslError, __FILE__, __LINE__);

		UpdatePslVerifyResult(exc);

		m_tlsReadySignal.fire(&exc);
	}
}

void SocketConnection::Connected()
{
	m_connectedSignal.fire(NULL);
}

void SocketConnection::ConnectError(const GError* error)
{
	PslError pslError = PmSockGetLastError( (PmSockIOChannel*) m_channel );
	PslErrorException exc(pslError, __FILE__, __LINE__);

	UpdatePslVerifyResult(exc);

	m_connectedSignal.fire(&exc);
}

void SocketConnection::SetException(const std::exception& e)
{
	// Note: this should only get called after a failed read/write
	PslError pslError = PmSockGetLastError( (PmSockIOChannel*) m_channel );

	PslErrorException ex(pslError, __FILE__, __LINE__);
	m_exception = boost::copy_exception(ex);

	m_errorInfo = ex.GetErrorInfo();
}

string SocketConnection::DescribeLocalSocket() const
{
#ifdef MOJ_LINUX
	int fd = GetFD();
	if(fd > 0) {
		struct sockaddr addr;
		socklen_t addrSize = sizeof(addr);

		if(getsockname(fd, &addr, &addrSize) == 0) {
			sa_family_t family = addr.sa_family;
			if(family == AF_INET) {
				struct sockaddr_in *inet_addr = (struct sockaddr_in*) &addr;

				stringstream ss;
				ss << inet_ntoa(inet_addr->sin_addr) << ":" << htons(inet_addr->sin_port);

				return ss.str();
			} else if(family == AF_INET6) {
				struct sockaddr_in6 *inet_addr = (struct sockaddr_in6*) &addr;
				char buf[PATH_MAX];

				stringstream ss;
				ss << inet_ntop(AF_INET6, &inet_addr->sin6_addr, buf, PATH_MAX) << ":"
				   << htons(inet_addr->sin6_port);

				return ss.str();
			}
		}
	}
#endif

	return "";
}

void SocketConnection::Status(MojObject& status) const
{
	GIOChannelWrapper::Status(status);

	string localSocket = DescribeLocalSocket();
	if(!localSocket.empty()) {
		MojErr err = status.putString("localSocket", localSocket.c_str());
		ErrorToException(err);
	}
}
