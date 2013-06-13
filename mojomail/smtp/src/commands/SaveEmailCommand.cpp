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

#include "commands/SaveEmailCommand.h"
#include "db/MojDbQuery.h"
#include "data/EmailSchema.h"
#include "data/DatabaseAdapter.h"
#include "data/EmailAccountAdapter.h"
#include "email/PreviewTextGenerator.h"

const int SaveEmailCommand::EMAIL_FILECACHE_COST = 100;
const int SaveEmailCommand::EMAIL_NO_LIFE_TIME = -1;
const int SaveEmailCommand::MAX_SUMMARY_LENGTH = 128;

SaveEmailCommand::SaveEmailCommand(SmtpClient& client,
								   const MojRefCountedPtr<MojServiceMessage> msg,
								   MojObject email,
								   MojObject accountId,
								   bool isDraft)
: SmtpCommand(client),
  m_client(client),
  m_message(msg),
  m_email(email),
  m_accountId(accountId),
  m_isDraft(isDraft),
  m_dbFolderSlot(this, &SaveEmailCommand::GetAccountResponse),
  m_dbPutSlot(this, &SaveEmailCommand::PutDone),
  m_fileCacheInsertedSlot(this, &SaveEmailCommand::FileCacheObjectInserted)
{
}

SaveEmailCommand::~SaveEmailCommand()
{
}

void SaveEmailCommand::RunImpl()
{
	try {
		GetAccount();
	} catch(const std::exception& e) {
		Error(e);
	} catch(...) {
		Error( MailException("unknown", __FILE__, __LINE__) );
	}
	
	return;
}

void SaveEmailCommand::GetAccount()
{
	m_client.GetDatabaseInterface().GetAccount(m_dbFolderSlot, m_accountId);
}

MojErr SaveEmailCommand::GetAccountResponse(MojObject &response, MojErr err)
{
	try {
		ResponseToException(response, err);

		MojObject accountObj;
		DatabaseAdapter::GetOneResult(response, accountObj);

		const char* folderPropertyName = m_isDraft ? EmailAccountAdapter::DRAFTS_FOLDERID : EmailAccountAdapter::OUTBOX_FOLDERID;
		err = accountObj.getRequired(folderPropertyName, m_folderId);
		ErrorToException(err);

		MojLogDebug(m_log, "preparing to save email to folderId %s", AsJsonString(m_folderId).c_str());

		PrepareParts();
	} catch(const std::exception& e) {
		Error(e);
	} catch(...) {
		Error( MailException("unknown", __FILE__, __LINE__) );
	}
	
	return MojErrNone;
}

void SaveEmailCommand::PrepareParts()
{
	MojErr err;

	err = m_email.getRequired(EmailSchema::PARTS, m_partsArray);
	ErrorToException(err);

	// Basic preflight to make sure the parts array is valid
	MojObject::ArrayIterator it;
	err = m_partsArray.arrayBegin(it);
	ErrorToException(err);

	for (; it != m_partsArray.arrayEnd(); ++it) {
		MojObject& part = *it;

		bool hasPath;
		MojString path;
		err = part.get(EmailSchema::Part::PATH, path, hasPath);
		ErrorToException(err);

		MojString type;
		err = part.getRequired(EmailSchema::Part::TYPE, type);
		ErrorToException(err);

		bool hasContent;
		MojString content;
		err = part.get("content", content, hasContent);
		ErrorToException(err);
		
		if(hasPath && hasContent) {
			throw MailException("part should have \"content\" or \"path\", not both", __FILE__, __LINE__);
		}
		
		if(!hasPath && !hasContent) {
			throw MailException("part has neither \"content\" nor \"path\"", __FILE__, __LINE__);
		}
	}

	// Point to the first part in the array
	err = m_partsArray.arrayBegin(m_partsIt);
	ErrorToException(err);

	CreateNextCacheObject();
}

void SaveEmailCommand::CreateNextCacheObject()
{
	MojErr err;
	
	for(; m_partsIt != m_partsArray.arrayEnd(); ++m_partsIt) {
		MojObject& part = *m_partsIt;
		
		bool hasContent;
		MojString content;
		err = part.get("content", content, hasContent);
		ErrorToException(err);
		
		if(hasContent) {
			MojString cacheType;
			cacheType.assign("email");
			
			bool hasDisplayName;
			MojString displayName;
			err = part.get("displayName", displayName, hasDisplayName);
			ErrorToException(err);
			
			MojString cacheFileName;
			
			if(hasDisplayName && displayName.length() > 0) {
				cacheFileName.assign(displayName);
			} else {
				// FIXME only for HTML parts
				cacheFileName.assign("body.html");
			}
			
			MojLogDebug(m_log, "creating a file cache entry");
			m_client.GetFileCacheClient().InsertCacheObject(m_fileCacheInsertedSlot, cacheType, cacheFileName, content.length(), EMAIL_FILECACHE_COST, EMAIL_NO_LIFE_TIME);
			
			// Exit method and wait for response
			return;
		}
	}
	
	// If this is the last part with content, persist to database now
	PersistToDatabase();
}

MojErr SaveEmailCommand::FileCacheObjectInserted(MojObject &response, MojErr err)
{
	try {
		ResponseToException(response, err);

		MojLogDebug(m_log, "successfully created file cache entry");

		MojString pathName;
		err = response.getRequired("pathName", pathName);
		ErrorToException(err);

		MojObject& part = *m_partsIt++;
		part.putString(EmailSchema::Part::PATH, pathName);

		WriteToFileCache(part, pathName);
	} catch(const std::exception& e) {
		Error(e);
	} catch(...) {
		Error( MailException("unknown", __FILE__, __LINE__) );
	}
	
	return MojErrNone;
}

void SaveEmailCommand::WriteToFileCache(MojObject &part, const MojString &pathName)
{
	MojErr err;
	MojString content;
	err = part.getRequired("content", content);
	ErrorToException(err);
	
	// FIXME: make this async
	FILE *fp = fopen(pathName.data(), "w");
	if(fp) {
		fwrite(content.data(), sizeof(MojChar), content.length(), fp);
		fclose(fp);
	} else {
		throw MailException("error opening file cache path", __FILE__, __LINE__);
	}
	
	// Cancel subscription to signal that we're done writing
	m_fileCacheInsertedSlot.cancel();

	MojString type;
	err = part.getRequired(EmailSchema::Part::TYPE, type);
	ErrorToException(err);

	if (type == EmailSchema::Part::Type::BODY) {
		std::string previewText = PreviewTextGenerator::GeneratePreviewText(content.data(), MAX_SUMMARY_LENGTH, true);
		err = m_email.putString(EmailSchema::SUMMARY, previewText.c_str());
		ErrorToException(err);
	}

	// Delete content field so it doesn't get written to the database
	bool wasDeleted;
	part.del("content", wasDeleted);
	
	// Next step
	if(m_partsIt == m_partsArray.arrayEnd())
		PersistToDatabase(); // done
	else
		CreateNextCacheObject(); // more parts remaining
	
	return;
}

void SaveEmailCommand::PersistToDatabase()
{
	m_client.GetDatabaseInterface().PersistDraftToDatabase(m_dbPutSlot, m_email, m_folderId, m_partsArray);
}

MojErr SaveEmailCommand::PutDone(MojObject &response, MojErr err)
{
	try {
		ResponseToException(response, err);

		MojObject results;
		err = response.getRequired("results", results);
		ErrorToException(err);

		MojObject put;
		results.at(0, put);

		MojObject emailId;
		err = put.getRequired("id", emailId);
		ErrorToException(err);

		// create response that contains emailId
		MojObject response;
		
		err = response.putBool(MojServiceMessage::ReturnValueKey, true);
		ErrorToException(err);
		        
		err = response.put(DatabaseAdapter::ID, emailId);
		ErrorToException(err);

		err = m_message->reply(response);
		ErrorToException(err);

		m_client.CommandComplete(this);
	} catch(const std::exception& e) {
		Error(e);
	} catch(...) {
		Error( MailException("unknown", __FILE__, __LINE__) );
	}

	return MojErrNone;
}

void SaveEmailCommand::Error(const std::exception& e)
{
	MojLogError(m_log, "Error: %s", e.what());
	
	const MojErrException *mojErrException = dynamic_cast<const MojErrException *>(&e);
	
	// TODO: better error codes and reporting
	// TODO: cleanup file cache entries?
	
	if(mojErrException) {
		m_message->replyError(mojErrException->GetMojErr());
	} else {
		m_message->replyError(MojErrInternal, e.what());
	}
	
	m_client.CommandFailure(this);
}

void SaveEmailCommand::Cancel()
{

}
