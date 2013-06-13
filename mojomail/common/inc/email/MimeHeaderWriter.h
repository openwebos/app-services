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

#ifndef MIMEHEADERWRITER_H_
#define MIMEHEADERWRITER_H_

#include <string>
#include "data/CommonData.h"

class MimeHeaderWriter
{
	static const char* NEWLINE;
public:
	// Create a MimeHeaderWriter that writes to an output string
	MimeHeaderWriter(std::string& output);
	virtual ~MimeHeaderWriter();
	
	void WriteHeader(const char* header, const std::string& value, bool encode = false);
	void WriteAddressHeader(const char* header, EmailAddressPtr emailAddress);
	void WriteAddressHeader(const char* header, const EmailAddressListPtr& addresses);
	
	void WriteParameterHeader(const char* header, const std::string& value,
			const std::string& parameter, const std::string& parameterValue);
	
protected:
	bool ShouldEncode(const std::string& text);
	void EscapeQuotes(const std::string& text, std::string& out);
	
	std::string&	out;
};

#endif /*MIMEHEADERWRITER_H_*/
