// @@@LICENSE
//
//      Copyright (c) 2009-2013 LG Electronics, Inc.
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

#ifndef POPACCOUNTADAPTER_H_
#define POPACCOUNTADAPTER_H_

#include <string>

#include "PopDefs.h"
#include "core/MojObject.h"
#include "data/PopAccount.h"
#include "data/EmailAccountAdapter.h"
#include "data/EmailSchema.h"

class PopAccountAdapter : public EmailAccountAdapter
{
public:
	static const char* const 	POP_ACCOUNT_KIND;
	
	static const char* const	ID;
	static const char* const	KIND;
	static const char* const	CONFIG;
	static const char* const	ACCOUNT_ID;
	static const char* const 	HOST_NAME;
	static const char* const	PORT;
	static const char* const	USERNAME;
	static const char* const	PASSWORD;
	static const char* const	ENCRYPTION;
	static const char* const	INITIAL_SYNC;
	static const char* const	DELETE_FROM_SERVER;
	static const char* const	DELETE_ON_DEVICE;

	// Get data from MojoDB and turn them into PopAccount
	// static void	ParseDatabasePopObject(const MojObject& obj, PopAccount& accnt);
	// Convert PopAccount data into MojoDB.
	static void	SerializeToDatabasePopObject(const PopAccount& accnt, MojObject& obj);
	static void	GetPopAccountFromPayload(MojLogger& log, const MojObject& payload, MojObject& popAccount);

	// get PopAccount from accounts fetched from the database
	static void GetPopAccountFromTransportObject(const MojObject& transportIn, PopAccount& out);
	static void GetPopAccount(const MojObject& in, const MojObject& transportIn, PopAccount& out);
};

#endif /* POPACCOUNTADAPTER_H_ */
