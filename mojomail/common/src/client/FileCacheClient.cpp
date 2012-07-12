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

#include "client/FileCacheClient.h"
#include "CommonPrivate.h"

const char* const FileCacheClient::FILECACHE_SERVICE = "com.palm.filecache";

const char* const FileCacheClient::EMAIL = "email";
const char* const FileCacheClient::ATTACHMENT = "attachment";

FileCacheClient::FileCacheClient(BusClient& busClient)
: m_busClient(busClient)
{
}

FileCacheClient::~FileCacheClient()
{
}

void FileCacheClient::InsertCacheObject(ReplySignal::SlotRef slot, const char* typeName, const char* fileName, MojInt64 size, MojInt64 cost, MojInt64 lifetime)
{
	MojErr err;
	MojObject payload;

	err = payload.putString("typeName", typeName);
	ErrorToException(err);

	err = payload.putString("fileName", fileName);
	ErrorToException(err);

	// Size must be >0 and larger than the actual content.
	// If the content is empty, we'll use 1 so FileCache doesn't complain.
	err = payload.put("size", size > 0 ? size : 1);
	ErrorToException(err);

	if(cost >= 0) {
		err = payload.put("cost", cost);
		ErrorToException(err);
	}

	if(lifetime >= 0) {
		err = payload.put("lifetime", lifetime);
		ErrorToException(err);
	}

	err = payload.put("subscribe", true);
	ErrorToException(err);

	m_busClient.SendRequest(slot, FILECACHE_SERVICE, "InsertCacheObject", payload, MojServiceRequest::Unlimited);
}

void FileCacheClient::ResizeCacheObject(ReplySignal::SlotRef slot, const char* pathName, MojInt64 newSize)
{
	MojErr err;
	MojObject payload;

	err = payload.putString("pathName", pathName);
	ErrorToException(err);

	err = payload.put("newSize", newSize);
	ErrorToException(err);

	m_busClient.SendRequest(slot, FILECACHE_SERVICE, "ResizeCacheObject", payload);
}

void FileCacheClient::ExpireCacheObject(ReplySignal::SlotRef slot, const char* pathName)
{
	MojErr err;
	MojObject payload;

	err = payload.putString("pathName", pathName);
	ErrorToException(err);

	m_busClient.SendRequest(slot, FILECACHE_SERVICE, "ExpireCacheObject", payload);
}

void FileCacheClient::SubscribeCacheObject(ReplySignal::SlotRef slot, const char* pathName)
{
	MojErr err;
	MojObject payload;

	err = payload.putString("pathName", pathName);
	ErrorToException(err);

	err = payload.put("subscribe", true);
	ErrorToException(err);

	m_busClient.SendRequest(slot, FILECACHE_SERVICE, "ResizeCacheObject", payload);
}
