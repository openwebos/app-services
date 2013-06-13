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

#include "data/PopEmailAdapter.h"

using namespace EmailSchema;

const char* const PopEmailAdapter::POP_EMAIL_KIND				= "com.palm.pop.email:1";
const char* const PopEmailAdapter::POP_TRANSPORT_EMAIL_KIND		= "com.palm.email.pop:1";

// constants for POP specific properties
const char* const PopEmailAdapter::ID		 					= "_id";
const char* const PopEmailAdapter::SERVER_UID		 			= "serverUid";
const char* const PopEmailAdapter::CONTENT_TYPE 				= "contentType";
const char* const PopEmailAdapter::CONTENT_ENCODING				= "contentEncoding";
const char* const PopEmailAdapter::DOWNLOADED					= "downloaded";

void PopEmailAdapter::ParseDatabasePopObject(const MojObject& obj, PopEmail& email)
{
	// Get the command properties from MojObject
	ParseDatabaseObject(obj, email);

	MojErr err;
	MojObject id;
	err = obj.getRequired(ID, id);
	ErrorToException(err);
	email.SetId(id);

	// Get POP email specific properties
	MojString serverUid;
	err = obj.getRequired(SERVER_UID, serverUid);
	ErrorToException(err);
	email.SetServerUID(std::string(serverUid));

	MojString contentType;
	err = obj.getRequired(CONTENT_TYPE, contentType);
	ErrorToException(err);
	email.SetContentType(std::string(contentType));;

	MojString encoding;
	bool exist = false;
	err = obj.get(CONTENT_ENCODING, encoding, exist);
	ErrorToException(err);
	if (exist) {
		email.SetEncodingMechanism(std::string(encoding));
	}

	bool downloaded = false;
	err = obj.getRequired(DOWNLOADED, downloaded);
	email.SetDownloaded(downloaded);
}

void PopEmailAdapter::SerializeToDatabasePopObject(const PopEmail& email, MojObject& obj)
{
	SerializeToDatabaseObject(email, obj);

	MojErr err;
	// Set the object kind
	err = obj.putString(KIND, POP_EMAIL_KIND);
	ErrorToException(err);

	if (!email.GetId().undefined() && !email.GetId().null()) {
		err = obj.put(ID, email.GetId());
		ErrorToException(err);
	}

	// Set server UID
	err = obj.putString(SERVER_UID, email.GetServerUID().c_str());
	ErrorToException(err);

	// Set content-type
	err = obj.putString(CONTENT_TYPE, email.GetContentType().c_str());
	ErrorToException(err);

	err = obj.putBool(DOWNLOADED, email.IsDownloaded());
	ErrorToException(err);
}
