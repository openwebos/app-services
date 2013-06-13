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

#include "email/MimeHeaderWriter.h"
#include "data/EmailAddress.h"
#include "email/Rfc2047Encoder.h"

const char *MimeHeaderWriter::NEWLINE = "\r\n";

MimeHeaderWriter::MimeHeaderWriter(std::string& output)
: out(output)
{
}

MimeHeaderWriter::~MimeHeaderWriter()
{
}

bool MimeHeaderWriter::ShouldEncode(const std::string& text)
{
	for(std::string::const_iterator it = text.begin(); it != text.end(); ++it) {
		char c = *it;
		
		// Needs encoding if there's any non-printable characters or newlines
		if(!isprint(c) || c == '\r' || c == '\n')
			return true;
	}
	return false;
}

void MimeHeaderWriter::EscapeQuotes(const std::string& text, std::string &out)
{
	for(std::string::const_iterator it = text.begin(); it != text.end(); ++it) {
		char c = *it;
		
		if(c == '"' || c == '\\')
			out.push_back('\\');

		out.push_back(c);
	}
}

void MimeHeaderWriter::WriteHeader(const char* header, const std::string& value, bool encode)
{
	out.append(header);
	out.append(": ");
	if(encode && ShouldEncode(value)) {
		Rfc2047Encoder encoder;
		encoder.QEncode(value, out);
	} else {
		out.append(value);
	}
	out.append(NEWLINE);
}

void MimeHeaderWriter::WriteAddressHeader(const char* header, EmailAddressPtr emailAddress)
{
	EmailAddressListPtr addresses(new EmailAddressList);
	addresses->push_back(emailAddress);
	WriteAddressHeader(header, addresses);
}

void MimeHeaderWriter::WriteAddressHeader(const char* header, const EmailAddressListPtr& addresses)
{
	out.append(header);
	out.append(": ");
	
	EmailAddressList::const_iterator it;
	
	for(it = addresses->begin(); it != addresses->end(); it++) {
		EmailAddressPtr address = *it;
		if(it != addresses->begin())
			out.append(", ");
		
		if(address->HasDisplayName()) {
			const std::string& displayName = address->GetDisplayName();
			
			// TODO handle wrapping
			out.append("\"");
			if(ShouldEncode(displayName)) {
				Rfc2047Encoder encoder;
				encoder.QEncode(displayName, out);
			} else {
				EscapeQuotes(displayName, out);
			}
			out.append("\" ");
		}
		
		out.append("<");
		out.append(address->GetAddress());
		out.append(">");
	}
	
	out.append(NEWLINE);
}

void MimeHeaderWriter::WriteParameterHeader(const char* header, const std::string& value,
		const std::string& parameter, const std::string& parameterValue)
{
	out.append(header);
	out.append(": ");
	out.append(value);
	out.append("; ");
	out.append(parameter);
	out.append("=\"");
	
	if(ShouldEncode(parameterValue)) {
		Rfc2047Encoder encoder;
		encoder.QEncode(parameterValue, out);
	} else {
		out.append(parameterValue);
	}

	out.append("\"");
	out.append(NEWLINE);
}
