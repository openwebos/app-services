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

#ifndef CAPABILITIES_H_
#define CAPABILITIES_H_

#include <string>
#include <set>

/**
 * Keeps track of the capabilities provided by the a server.
 */
class Capabilities {
public:
	Capabilities();
	virtual ~Capabilities();

	static const std::string IDLE;
	static const std::string XLIST;
	static const std::string UIDPLUS;
	static const std::string NAMESPACE;
	static const std::string STARTTLS;
	static const std::string LOGINDISABLED;
	static const std::string COMPRESS_DEFLATE;

	void SetCapability(const std::string& cap);
	void RemoveCapability(const std::string& cap);
	bool HasCapability(const std::string& cap) const { return m_caps.find(cap) != m_caps.end(); }

	// Is the capability list up-to-date?
	bool IsValid() const { return m_valid; }

	// Mark as invalid
	void SetValid(bool valid) { m_valid = valid; }

	// Clear all capabilities and mark as invalid
	void Clear();

protected:
	std::set<std::string>	m_caps;
	bool					m_valid;
};

#endif /* CAPABILITIES_H_ */
