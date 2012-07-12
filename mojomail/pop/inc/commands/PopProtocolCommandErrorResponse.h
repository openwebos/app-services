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

#ifndef POPPROTOCOLCOMMANDERRORRESPONSE_H_
#define POPPROTOCOLCOMMANDERRORRESPONSE_H_

class PopProtocolCommandErrorResponse {
public:
	/*** Hotmail/Live accounts related errors ***/
	// Account login errors
	static const char* const HOTMAIL_LOGIN_TOO_FREQUENT;
	static const char* const HOTMAIL_LOGIN_LIMIT_EXCEED;
	static const char* const HOTMAIL_MAILBOX_NOT_OPENED;
	static const char* const HOTMAIL_WEB_LOGIN_REQUIRED;
};

#endif /* POPPROTOCOLCOMMANDERRORRESPONSE_H_ */
