// @@@LICENSE
//
//      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

#ifndef REQUESTMANAGER_H_
#define REQUESTMANAGER_H_

#include "client/Command.h"
#include "request/Request.h"

class HandleRequestCommand;

/**
 * Request manager contains a request handler to handle different kinds of
 * on-demand requests.
 *
 * There will be at most one request handler in request manager to handle requests.
 * This design is created to handler servers (such as Hotmail) that only allow one
 * active POP session at any time.  So we cannot create multiple connections to
 * handle different types of requests.
 *
 * Also, within the same connection scope, we can only issue one POP command at
 * a time.  Therefore, we can only allow one request handle in request manager
 * at any time.
 */
class RequestManager
{
public:
	typedef MojRefCountedPtr<HandleRequestCommand> CommandPtr;

	RequestManager();
	~RequestManager();

	/**
	 * Register a request handler to handle one kind of a request.  If there
	 * is already a handler exists in request manager for this request kind, the
	 * new handler will not be registered into request manager. Otherwise, the new
	 * handler will be added to request manager.
	 */
	void RegisterCommand(CommandPtr command);

	/**
	 * Unregister a request handler from request manager if it is previously
	 * registered successfully.
	 */
	void UnregisterCommand(Command* command);

	bool HasCommandHandler();

	/**
	 * Add a request for a request handler to handle.
	 */
	void AddRequest(Request::RequestPtr request);
private:
	typedef CommandPtr	CommandContainer;

	CommandContainer	m_commandContainer;
};

#endif /* REQUESTMANAGER_H_ */
