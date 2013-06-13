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

#ifndef EMAILSCHEMA_H_
#define EMAILSCHEMA_H_

namespace EmailSchema {
	extern const char *const KIND;
	
	namespace Kind {
		extern const char *const EMAIL;
	}

	extern const char *const PURGEABLE;

	extern const char *const FOLDER_ID;
	extern const char *const FOLDER_SERVER_ID;
	extern const char *const SUBJECT;
	extern const char *const SUMMARY;
	extern const char *const TIMESTAMP;
	extern const char *const FROM;
	extern const char *const REPLY_TO;
	extern const char *const RECIPIENTS;
	extern const char *const PARTS;
	extern const char *const FLAGS;
	extern const char *const SEND_STATUS;
	extern const char *const ORIGINAL_MSG_ID;
	extern const char *const DRAFT_TYPE;
	extern const char *const PRIORITY;
	extern const char *const MESSAGE_ID;
	extern const char *const IN_REPLY_TO;
	extern const char *const DATERCVDTZFMT;
	
	namespace Address {
		extern const char *const ADDR;
		extern const char *const NAME;
		extern const char *const TYPE;
		
		namespace Type {
			extern const char *const TO;
			extern const char *const CC;
			extern const char *const BCC;
			extern const char *const FROM;
			extern const char *const REPLY_TO;
		}
	}
	
	namespace Part {
		extern const char *const TYPE;
		extern const char *const MIME_TYPE;
		extern const char *const PATH;
		extern const char *const NAME;
		extern const char *const CONTENT_ID;
		extern const char *const ESTIMATED_SIZE;
		extern const char *const ENCODED_SIZE;
		extern const char *const ENCODING;
		extern const char *const CHARSET;
		extern const char *const SECTION;
		
		namespace Type {
			extern const char *const BODY;
			extern const char *const ATTACHMENT;
			extern const char *const INLINE;
			extern const char *const SMART_TEXT;
			extern const char *const ALT_TEXT;
		}
	}
	
	namespace Flags {
		extern const char *const READ;
		extern const char *const REPLIED;
		extern const char *const FLAGGED;
		extern const char *const EDITEDORIGINAL;
		extern const char *const EDITEDDRAFT;
		extern const char *const VISIBLE;
	}
	
	namespace SendStatus {
		extern const char *const ERROR;
		extern const char *const LAST_ERROR;
		extern const char *const FATAL_ERROR;
		extern const char *const SENT;
		extern const char *const RETRY_COUNT;
	}

	namespace SendStatusError {
		extern const char *const ERROR_CODE;
		extern const char *const ERROR_TEXT;
	}
}


#endif /*EMAILSCHEMA_H_*/
