// @@@LICENSE
//
//      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
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

#include "exceptions/MojErrException.h"
#include "core/MojString.h"
#include "core/MojObject.h"

#include <sstream>

using namespace std;

MojErrException::MojErrException(MojErr err, const char* file, int line)
: MailException(file, line),
  m_err(err)
{
	MojString errorString;
	(void) MojErrToString(m_err, errorString);
	stringstream msg;
	msg << errorString.data() << " (MojErr " << m_err << ")";
	SetMessage(msg.str().c_str());
}

MojErrException::MojErrException(MojErr err, const char* errorMessage, const char* file, int line)
: MailException(file, line),
  m_err(err)
{
	stringstream msg;
	msg << errorMessage << " (MojErr " << m_err << ")";
	SetMessage(msg.str().c_str());
}

void MojErrException::CheckErr(MojErr err, const char* filename, int line) {
	if(err)
		throw MojErrException(err, filename, line);
}

void MojErrException::CheckErr(MojObject& response, MojErr err, const char* filename, int line) {
	if(err) {
		bool hasErrorText = false;
		MojString errorText;
		if(response.get("errorText", errorText, hasErrorText) == MojErrNone && hasErrorText) {
			throw MojErrException(err, errorText.data(), filename, line);
		} else {
			throw MojErrException(err, filename, line);
		}
	}
}
