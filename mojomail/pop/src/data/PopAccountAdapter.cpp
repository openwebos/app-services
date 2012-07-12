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

#include "data/PopAccountAdapter.h"

const char* const PopAccountAdapter::POP_ACCOUNT_KIND		= "com.palm.pop.account:1";

// constants for POP specific properties
const char* const PopAccountAdapter::ID						= "_id";
const char* const PopAccountAdapter::KIND		 			= "_kind";
const char* const PopAccountAdapter::CONFIG					= "config";
const char* const PopAccountAdapter::ACCOUNT_ID 			= "accountId";
const char* const PopAccountAdapter::HOST_NAME 				= "server";
const char* const PopAccountAdapter::PORT		 			= "port";
const char* const PopAccountAdapter::ENCRYPTION 			= "encryption";
const char* const PopAccountAdapter::USERNAME 				= "username";
const char* const PopAccountAdapter::PASSWORD 				= "password";
const char* const PopAccountAdapter::INITIAL_SYNC			= "initialSync";
const char* const PopAccountAdapter::DELETE_FROM_SERVER		= "deleteFromServer";
const char* const PopAccountAdapter::DELETE_ON_DEVICE		= "deleteOnDevice";

void PopAccountAdapter::SerializeToDatabasePopObject(const PopAccount& accnt, MojObject& obj)
{
	MojErr err;
	// Set the object kind
	err = obj.putString(KIND, POP_ACCOUNT_KIND);
	ErrorToException(err);

	// Set ID
	err = obj.put(ID, accnt.GetId());
	ErrorToException(err);

	// Set account ID
	err = obj.put(ACCOUNT_ID, accnt.GetAccountId());
	ErrorToException(err);
	
	// Set host name
	err = obj.putString(HOST_NAME, accnt.GetHostName().c_str());
	ErrorToException(err);

	// Set port
	err = obj.putInt(PORT, accnt.GetPort());
	ErrorToException(err);

	// Set encryption
	err = obj.putString(ENCRYPTION, accnt.GetEncryption().c_str());
	ErrorToException(err);

	// Set user name
	err = obj.putString(USERNAME, accnt.GetUsername().c_str());
	ErrorToException(err);

	// Set 'initialSynced' flag
	err = obj.putBool(INITIAL_SYNC, accnt.IsInitialSync());
	ErrorToException(err);

	// Set 'deleteFromServer' flag
	err = obj.putBool(DELETE_FROM_SERVER, accnt.IsDeleteFromServer());
	ErrorToException(err);

	// Set 'deleteOnDevice' flag
	err = obj.putBool(DELETE_ON_DEVICE, accnt.IsDeleteOnDevice());
	ErrorToException(err);
	
	// Set sync window
	err = obj.putInt(EmailAccountAdapter::SYNC_WINDOW_DAYS, accnt.GetSyncWindowDays());
	ErrorToException(err);

	// Set sync frequency
	err = obj.putInt(SYNC_FREQUENCY_MINS, accnt.GetSyncFrequencyMins());
	ErrorToException(err);
	
	if (accnt.IsError()) {
		MojObject mojErr;
		err = mojErr.putInt(EmailAccountAdapter::ERROR_CODE, (int)accnt.GetError().errorCode);
		ErrorToException(err);

		err = mojErr.putString(EmailAccountAdapter::ERROR_TEXT, accnt.GetError().errorText.c_str());
		ErrorToException(err);

		err = obj.put(EmailAccountAdapter::ERROR, mojErr);
		ErrorToException(err);
	} else {
		err = obj.put(EmailAccountAdapter::ERROR, MojObject::Null);
	}

	SerializeSpecialFolders(accnt, obj);
}

void PopAccountAdapter::GetPopAccountFromPayload(MojLogger& log, const MojObject& payload, MojObject& popAccount)
{
	MojErr err = popAccount.putString(KIND, POP_ACCOUNT_KIND);
	ErrorToException(err);

	MojObject accountId;
	err = payload.getRequired(ACCOUNT_ID, accountId);
	ErrorToException(err);
	err = popAccount.put(ACCOUNT_ID, accountId);
	ErrorToException(err);

	MojObject config;
	err = payload.getRequired(CONFIG, config);
	ErrorToException(err);

	MojObject hostname;
	err = config.getRequired(HOST_NAME, hostname);
	ErrorToException(err);
	err = popAccount.put(HOST_NAME, hostname);
	ErrorToException(err);

	MojObject port;
	err = config.getRequired(PORT, port);
	ErrorToException(err);
	err = popAccount.put(PORT, port);
	ErrorToException(err);

	MojObject encryption;
	err = config.getRequired(ENCRYPTION, encryption);
	ErrorToException(err);
	err = popAccount.put(ENCRYPTION, encryption);
	ErrorToException(err);

	MojObject username;
	err = config.getRequired(USERNAME, username);
	ErrorToException(err);
	err = popAccount.put(USERNAME, username);
	ErrorToException(err);

	// Set 'deleteFromServer' flag
	bool deleteFromServer = true;
	if (!config.get(DELETE_FROM_SERVER, deleteFromServer)) {
		// default value is true if this field doesn't exist
		deleteFromServer = true;
	}
	err = popAccount.putBool(DELETE_FROM_SERVER, deleteFromServer);
	ErrorToException(err);

	// Set 'deleteOnDevice' flag
	bool deleteOnDevice = false;
	if (!config.get(DELETE_ON_DEVICE, deleteOnDevice)) {
		// defaul value is false if this field doesn't exist
		deleteOnDevice = false;
	}
	err = popAccount.putBool(DELETE_ON_DEVICE, deleteOnDevice);
	ErrorToException(err);

	// Set sync window
	MojInt64 syncWindow;
	if (!config.get(EmailAccountAdapter::SYNC_WINDOW_DAYS, syncWindow)) {
		// set default value if this field doesn't exist
		syncWindow = PopAccount::DEFAULT_SYNC_WINDOW;
	}
	err = popAccount.putInt(EmailAccountAdapter::SYNC_WINDOW_DAYS, syncWindow);
	ErrorToException(err);

	// Set sync frequency
	MojInt64 syncFreq;
	if (!config.get(SYNC_FREQUENCY_MINS, syncFreq)) {
		syncFreq = PopAccount::DEFAULT_SYNC_FREQUENCY_MINS;
	}
	err = popAccount.putInt(SYNC_FREQUENCY_MINS, syncFreq);
	ErrorToException(err);
}

void PopAccountAdapter::GetPopAccountFromTransportObject(const MojObject& transportIn, PopAccount& out)
{
	MojObject id;
	MojErr err = transportIn.getRequired(ID, id);
	ErrorToException(err);
	out.SetId(id);

	MojObject accntId;
	err = transportIn.getRequired(ACCOUNT_ID, accntId);
	ErrorToException(err);
	out.SetAccountId(accntId);

	MojString username;
	err = transportIn.getRequired(USERNAME, username);
	ErrorToException(err);
	out.SetUsername(username.data());

	MojString hostname;
	err = transportIn.getRequired(HOST_NAME, hostname);
	ErrorToException(err);
	out.SetHostName(hostname.data());

	int port;
	err = transportIn.getRequired(PORT, port);
	ErrorToException(err);
	out.SetPort(port);

	MojString encryptionStr;
	err = transportIn.getRequired(ENCRYPTION, encryptionStr);
	ErrorToException(err);
	out.SetEncryption(encryptionStr.data());

	bool deleteFromServer;
	err = transportIn.getRequired(DELETE_FROM_SERVER, deleteFromServer);
	ErrorToException(err);
	out.SetDeleteFromServer(deleteFromServer);

	bool deleteOnDevice;
	err = transportIn.getRequired(DELETE_ON_DEVICE, deleteOnDevice);
	ErrorToException(err);
	out.SetDeleteOnDevice(deleteOnDevice);

	MojInt64 syncWindow;
	if (!transportIn.get(EmailAccountAdapter::SYNC_WINDOW_DAYS, syncWindow)) {
		// set default value if this field doesn't exist
		syncWindow = PopAccount::DEFAULT_SYNC_WINDOW;
	}
	out.SetSyncWindowDays(syncWindow);
	ErrorToException(err);

	// Set sync frequency
	MojInt64 syncFreq;
	if (!transportIn.get(SYNC_FREQUENCY_MINS, syncFreq)) {
		syncFreq = PopAccount::DEFAULT_SYNC_FREQUENCY_MINS;
	}
	out.SetSyncFrequencyMins(syncFreq);
	ErrorToException(err);

	MojObject error;
	if(transportIn.get(ERROR, error))
	{
		EmailAccount::AccountError accErr;
		MojInt64 errorCode;
		if(error.get(ERROR_CODE, errorCode))
		{
			accErr.errorCode = GetErrorCode(errorCode);
		}

		bool found = false;
		MojString errorText;
		err = error.get(ERROR_TEXT, errorText, found);
		ErrorToException(err);
		if(found)
		{
			accErr.errorText = errorText.data();
		}

		out.SetError(accErr);
	}

	MojString inboxId;
	bool exists;
	err = transportIn.get(INBOX_FOLDERID, inboxId, exists);
	ErrorToException(err);
	if (exists)
		out.SetInboxFolderId(inboxId);

	MojString outboxId;
	err = transportIn.get(OUTBOX_FOLDERID, outboxId, exists);
	ErrorToException(err);
	if (exists)
		out.SetOutboxFolderId(outboxId);

	MojString draftId;
	err = transportIn.get(DRAFTS_FOLDERID, draftId, exists);
	ErrorToException(err);
	if (exists)
		out.SetDraftsFolderId(draftId);

	MojString sentId;
	err = transportIn.get(SENT_FOLDERID, sentId, exists);
	ErrorToException(err);
	if (exists)
		out.SetSentFolderId(sentId);

	MojString trashId;
	err = transportIn.get(TRASH_FOLDERID, trashId, exists);
	ErrorToException(err);
	if (exists)
		out.SetTrashFolderId(trashId);
}

void PopAccountAdapter::GetPopAccount(const MojObject& in, const MojObject& transportIn, PopAccount& out)
{
	MojObject id;
	MojErr err = transportIn.getRequired(ID, id);
	ErrorToException(err);
	out.SetId(id);

	MojObject accntId;
	err = transportIn.getRequired(ACCOUNT_ID, accntId);
	ErrorToException(err);
	out.SetAccountId(accntId);

	MojString username;
	err = transportIn.getRequired(USERNAME, username);
	ErrorToException(err);
	out.SetUsername(username.data());

	MojString hostname;
	err = transportIn.getRequired(HOST_NAME, hostname);
	ErrorToException(err);
	out.SetHostName(hostname.data());

	int port;
	err = transportIn.getRequired(PORT, port);
	ErrorToException(err);
	out.SetPort(port);

	MojString encryptionStr;
	err = transportIn.getRequired(ENCRYPTION, encryptionStr);
	ErrorToException(err);
	out.SetEncryption(encryptionStr.data());

	bool initialSync;
	if (!transportIn.get(INITIAL_SYNC, initialSync)) {
		// if 'initialSynced' flag doesn't exist, it may come from an account
		// that has been created without this flag.  So set this flag to be
		// true to indicate that the account has passed the initial sync stage.
		initialSync = false;
	}
	out.SetInitialSync(initialSync);

	bool deleteFromServer;
	err = transportIn.getRequired(DELETE_FROM_SERVER, deleteFromServer);
	ErrorToException(err);
	out.SetDeleteFromServer(deleteFromServer);

	bool deleteOnDevice;
	err = transportIn.getRequired(DELETE_ON_DEVICE, deleteOnDevice);
	ErrorToException(err);
	out.SetDeleteOnDevice(deleteOnDevice);

	MojInt64 syncWindow;
	if (!transportIn.get(EmailAccountAdapter::SYNC_WINDOW_DAYS, syncWindow)) {
		// set default value if this field doesn't exist
		syncWindow = PopAccount::DEFAULT_SYNC_WINDOW;
	}
	out.SetSyncWindowDays(syncWindow);
	ErrorToException(err);

	// Set sync frequency
	MojInt64 syncFreq;
	if (!transportIn.get(SYNC_FREQUENCY_MINS, syncFreq)) {
		syncFreq = PopAccount::DEFAULT_SYNC_FREQUENCY_MINS;
	}
	out.SetSyncFrequencyMins(syncFreq);
	ErrorToException(err);

	MojObject error;
	if(transportIn.get(ERROR, error))
	{
		EmailAccount::AccountError accErr;
		MojInt64 errorCode;
		if(error.get(ERROR_CODE, errorCode))
		{
			accErr.errorCode = GetErrorCode(errorCode);
		}

		bool found = false;
		MojString errorText;
		err = error.get(ERROR_TEXT, errorText, found);
		ErrorToException(err);
		if(found)
		{
			accErr.errorText = errorText.data();
		}

		out.SetError(accErr);
	}

	MojString inboxId;
	bool exists;
	err = transportIn.get(INBOX_FOLDERID, inboxId, exists);
	ErrorToException(err);
	if (exists)
		out.SetInboxFolderId(inboxId);

	MojString outboxId;
	err = transportIn.get(OUTBOX_FOLDERID, outboxId, exists);
	ErrorToException(err);
	if (exists)
		out.SetOutboxFolderId(outboxId);

	MojString draftId;
	err = transportIn.get(DRAFTS_FOLDERID, draftId, exists);
	ErrorToException(err);
	if (exists)
		out.SetDraftsFolderId(draftId);

	MojString sentId;
	err = transportIn.get(SENT_FOLDERID, sentId, exists);
	ErrorToException(err);
	if (exists)
		out.SetSentFolderId(sentId);

	MojString trashId;
	err = transportIn.get(TRASH_FOLDERID, trashId, exists);
	ErrorToException(err);
	if (exists)
		out.SetTrashFolderId(trashId);
}
