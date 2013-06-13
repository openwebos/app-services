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

#ifndef FILECACHECLIENT_H_
#define FILECACHECLIENT_H_

#include "core/MojSignal.h"
#include "core/MojServiceRequest.h"
#include "client/BusClient.h"

class FileCacheClient
{
public:
	typedef MojServiceRequest::ReplySignal ReplySignal;

	static const char* const FILECACHE_SERVICE;

	static const char* const EMAIL;
	static const char* const ATTACHMENT;

	FileCacheClient(BusClient& busClient);
	virtual ~FileCacheClient();

	/**
	 * Create a file cache entry using the file cache service.
	 * @param typeName usually FileCacheClient::EMAIL or FileCacheClient::ATTACHMENT
	 * @param fileName
	 * @param size maximum size that will be written (may be adjusted using ResizeCacheObject)
	 * @param cost 0-100, or -1 for default
	 * @param lifetime in seconds, or -1 for default
	 */
	virtual void InsertCacheObject(ReplySignal::SlotRef slot, const char* typeName, const char* fileName, MojInt64 size, MojInt64 cost, MojInt64 lifetime);

	/**
	 * Resizes the maximum size of a file cache entry in the FileCache.
	 * @param pathName
	 * @param newSize
	 */
	virtual void ResizeCacheObject(ReplySignal::SlotRef slot, const char* pathName, MojInt64 newSize);

	/**
	 * Expires an entry from the FileCache. The file will be deleted by the FileCache when there are no subscriptions.
	 * @param pathName
	 */
	virtual void ExpireCacheObject(ReplySignal::SlotRef slot, const char* pathName);

	/**
	 * Subscribes to a cache object for reading. Prevents the entry from being deleted until the subscription is cancelled.
	 * @param pathName
	 */
	virtual void SubscribeCacheObject(ReplySignal::SlotRef slot, const char* pathName);

protected:
	BusClient&	m_busClient;
};

#endif /* FILECACHECLIENT_H_ */
