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

#ifndef FOLDER_H_
#define FOLDER_H_

#include <string>
#include "core/MojObject.h"

class Folder
{
public:

	Folder();
	virtual ~Folder();

	// Setters
	void SetId(const MojObject& id)						{ m_id = id; }
	void SetAccountId(const MojObject& id) 				{ m_accountId = id; }
	void SetParentId(const MojObject& id)				{ m_parentId = id; }
	void SetDisplayName(const std::string& name)		{ m_displayName = name; }
	void SetFavorite(bool favorite)						{ m_favorite = favorite; }

	// Getters
	const MojObject&	GetId() const					{ return m_id; }
	const MojObject& 	GetAccountId() const			{ return m_accountId; }
	const MojObject& 	GetParentId() const				{ return m_parentId; }
	const std::string&	GetDisplayName() const			{ return m_displayName; }
	bool				GetFavorite() const				{ return m_favorite; }

protected:
	MojObject	m_id;
	MojObject 	m_accountId;
	MojObject	m_parentId;
	std::string	m_displayName;
	bool		m_favorite;
};
#endif /* FOLDER_H_ */
