// @@@LICENSE
//
//      Copyright (c) 2010-2013 LG Electronics, Inc.
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

#ifndef SOCKETCONNECTION_H_
#define SOCKETCONNECTION_H_

#include "async/GIOChannelWrapper.h"
#include "stream/BaseInputStream.h"
#include "stream/BaseOutputStream.h"
#include <string>
#include "util/Timer.h"

class PslErrorException;

class SocketConnection : public GIOChannelWrapper
{
public:
	virtual ~SocketConnection();
	
	typedef MojSignal<const std::exception*> ConnectedSignal;
	typedef MojSignal<const std::exception*> TLSReadySignal;

	/**
	 * Set connect timeout.
	 */
	virtual void SetConnectTimeout(int seconds);

	/**
	 * Connect to the server. Slot will be called after successfully connecting, or on error.
	 */
	virtual void Connect(ConnectedSignal::SlotRef connectedSlot);
	
	/**
	 * Negotiate TLS with the server.  Slot will be called after negotiation completed.
	 */
	virtual void NegotiateTLS(TLSReadySignal::SlotRef tlsReadySlot);

	/**
	 * Create a socket connection.
	 *
	 * @param hostname
	 * @param port
	 * @param ssl
	 * @param bindAddress	leave empty for default interface
	 * @param bindPort
	 */
	static MojRefCountedPtr<SocketConnection> CreateSocket(const char* hostname, unsigned int port, bool useSsl, const std::string& bindAddress = "", int bindPort = 0);
	
	/**
	 * Get the bind address used to create the socket.
	 * Returns an empty string if no bind address was specified.
	 *
	 * TODO: add method to get the currently bound IP, after connecting
	 */
	virtual const std::string& GetBindAddress() const { return m_bindAddress; }

	virtual void Status(MojObject& status) const;

protected:
	SocketConnection(GIOChannel* channel, bool useSsl);
	
	static gboolean SocketConnectCb(GIOChannel* channel, gpointer data, const GError* error);
	virtual void Connected();
	virtual void ConnectError(const GError* error);
	virtual void ConnectTimeout();
	
	static gboolean NegotiateTLSCb(GIOChannel* gio, gboolean isSocketEncrypted, gpointer data, const GError* error);
	virtual void TLSReady();
	virtual void TLSError(const GError* error);

	virtual std::string DescribeLocalSocket() const;

	virtual void SetException(const std::exception& e);

	void UpdatePslVerifyResult(PslErrorException& exc);

	bool			m_useSsl;
	std::string		m_bindAddress;
	int				m_bindPort;
	
	int							m_connectTimeout;
	Timer<SocketConnection>		m_connectTimer;

	InputStreamPtr		m_inputStream;
	OutputStreamPtr		m_outputStream;
	
	ConnectedSignal		m_connectedSignal;
	TLSReadySignal		m_tlsReadySignal;
};

#endif /*SOCKETCONNECTION_H_*/
