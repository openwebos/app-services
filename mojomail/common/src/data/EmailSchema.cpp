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

#include "data/EmailSchema.h"

namespace EmailSchema {
	const char *const KIND			= "_kind";
	
	namespace Kind {
		const char *const EMAIL		= "com.palm.email:1";
	}

	const char *const PURGEABLE			= "purgeable";

	const char *const FOLDER_ID			= "folderId";
	const char *const FOLDER_SERVER_ID 	= "folderServerId";
	const char *const SUBJECT			= "subject";
	const char *const SUMMARY			= "summary";
	const char *const TIMESTAMP			= "timestamp";
	const char *const FROM				= "from";
	const char *const REPLY_TO			= "replyTo";
	const char *const RECIPIENTS		= "to";
	const char *const PARTS				= "parts";
	const char *const FLAGS				= "flags";
	const char *const SEND_STATUS		= "sendStatus";
	const char *const ORIGINAL_MSG_ID 	= "originalMsgId";
	const char *const DRAFT_TYPE		= "draftType";
	const char *const PRIORITY			= "priority";
	const char *const MESSAGE_ID		= "messageId";
	const char *const IN_REPLY_TO		= "inReplyTo";
	const char *const DATERCVDTZFMT		= "%Y-%m-%dT%H:%M:%S%Z";

	namespace Address {
		const char *const ADDR		= "addr";
		const char *const NAME		= "name";
		const char *const TYPE		= "type";
		
		namespace Type {
			const char *const TO	= "to";
			const char *const CC	= "cc";
			const char *const BCC	= "bcc";
			const char *const FROM	= "from";
			const char *const REPLY_TO	= "replyTo";
		}
	}
	
	namespace Part {
		const char *const TYPE				= "type";
		const char *const MIME_TYPE			= "mimeType";
		const char *const PATH				= "path";
		const char *const NAME				= "name";
		const char *const CONTENT_ID		= "contentId";
		const char *const ESTIMATED_SIZE	= "estimatedSize";
		const char *const ENCODED_SIZE		= "encodedSize";
		const char *const ENCODING			= "encoding";
		const char *const CHARSET			= "charset";
		const char *const SECTION			= "section";
		
		namespace Type {
			const char *const BODY			= "body";
			const char *const ATTACHMENT	= "attachment";
			const char *const INLINE		= "inline";
			const char *const SMART_TEXT	= "smartText";
			const char *const ALT_TEXT		= "altText";
		}
	}
	
	namespace Flags {
		const char *const READ				= "read";
		const char *const REPLIED			= "replied";
		const char *const FLAGGED			= "flagged";
		const char *const EDITEDORIGINAL	= "editedOriginal";
		const char *const EDITEDDRAFT 		= "editedDraft";
		const char *const VISIBLE			= "visible";
	}
	
	namespace SendStatus {
		const char *const ERROR			= "error";
		const char *const LAST_ERROR	= "lastError";
		const char *const FATAL_ERROR	= "fatalError";
		const char *const SENT			= "sent";
		const char *const RETRY_COUNT	= "retryCount";
	}

	namespace SendStatusError {
		const char *const ERROR_CODE	= "errorCode";
		const char *const ERROR_TEXT	= "errorText";
	}
}
