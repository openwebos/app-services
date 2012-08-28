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

#ifndef EMAILWRITER_H_
#define EMAILWRITER_H_

#include <sys/types.h>
#include <string>
#include "data/CommonData.h"
#include "stream/BaseOutputStream.h"

class BaseOutputStream;
class ByteBufferOutputStream;

class EmailWriter
{
public:
	EmailWriter(Email& email);
	virtual ~EmailWriter();
	
	// Sets the output stream. Caller retains ownership of outputStream and is responsible
	// for freeing the output stream.
	void SetOutputStream(const OutputStreamPtr& outputStream);
	
	// Sets whether Bcc fields should be generated.
	void SetBccIncluded(bool includeBcc);

	// Creates an output stream for writing out a part using the proper encodings.
	// The caller is responsible for freeing the output stream.
	OutputStreamPtr GetPartOutputStream(const EmailPart& part);

	void WriteEmailHeader(const std::string& emailContentType, const std::string& boundary);
	void WriteAlternativeHeader(const std::string& boundary, const std::string& altBoundary);
	void WritePartHeader(const EmailPart& part, const std::string& boundary);
	void WritePartFooter();
	void WriteBoundary(const std::string& boundary, bool end);
	void WriteEmailFooter();
	
protected:
	Email&				m_email;
	
	OutputStreamPtr		m_outputStream;

	std::string			m_boundary;
	std::string			m_altBoundary;

	bool				m_includeBcc;
};

#endif /*EMAILWRITER_H_*/
