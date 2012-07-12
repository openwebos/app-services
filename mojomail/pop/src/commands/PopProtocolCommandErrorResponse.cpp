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

#include "commands/PopProtocolCommandErrorResponse.h"

const char* const PopProtocolCommandErrorResponse::HOTMAIL_LOGIN_TOO_FREQUENT = " login allowed only every";
const char* const PopProtocolCommandErrorResponse::HOTMAIL_LOGIN_LIMIT_EXCEED = "Exceeded the login limit";  // The whole server message is "Exceeded the login limit for a 15 minute period. Reduce the frequency of requests to the POP3 server."
const char* const PopProtocolCommandErrorResponse::HOTMAIL_MAILBOX_NOT_OPENED = " Mailbox could not be opened";
const char* const PopProtocolCommandErrorResponse::HOTMAIL_WEB_LOGIN_REQUIRED = " Login to your account via a web browser";
