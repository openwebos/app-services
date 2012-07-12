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

#ifndef POPEMAIL_H_
#define POPEMAIL_H_

#include <string>
#include <vector>
#include "data/Email.h"

class PopEmail : public Email
{
public:
	typedef boost::shared_ptr<PopEmail> 					PopEmailPtr;
	typedef std::vector<PopEmailPtr> 						PopEmailPtrVector;
	typedef boost::shared_ptr<PopEmail::PopEmailPtrVector>	PopEmailPtrVectorPtr;

	PopEmail();
	PopEmail(const Email& email);
	virtual ~PopEmail();

	// Setters
	void SetServerUID(const std::string& uid) 				{ m_serverUid = uid; }
	void SetContentType(const std::string& contentType)		{ m_contentType = contentType; }
	void SetDownloaded(bool downloaded)						{ m_downloaded = downloaded; }

	// Getters
	const std::string&		GetServerUID() const			{ return m_serverUid; }
	const std::string&		GetContentType() const			{ return m_contentType; }
	const bool				IsDownloaded() const			{ return m_downloaded; }

	void SetContentSubType(std::string& contentSubType)		{ m_contentSubType = contentSubType; }
	void SetEncodingMechanism(const std::string& encodingMechanism)		{ m_encodingMechanism = encodingMechanism; }
private:
	// properties that are specific to POP transport
	std::string		m_serverUid;
	std::string		m_contentType;
	std::string		m_contentSubType;
	std::string		m_encodingMechanism;
	bool			m_downloaded;
};

#endif /* POPEMAIL_H_ */
