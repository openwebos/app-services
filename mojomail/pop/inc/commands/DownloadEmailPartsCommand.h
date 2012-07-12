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

#ifndef DOWNLOADEMAILPARTSCOMMAND_H_
#define DOWNLOADEMAILPARTSCOMMAND_H_

#include "commands/PopMultiLineResponseCommand.h"
#include "data/PopEmail.h"
#include "request/Request.h"

class AsyncEmailParser;

class DownloadEmailPartsCommand : public PopMultiLineResponseCommand
{
public:
	static const char* const	COMMAND_STRING;
	static const int			DOWNLOAD_PERCENTAGE_TO_REPORT;

	DownloadEmailPartsCommand(PopSession& session, int msgNum, int size,
							PopEmail::PopEmailPtr email,
							boost::shared_ptr<FileCacheClient> fileCacheClient,
							MojRefCountedPtr<Request> request);
	virtual ~DownloadEmailPartsCommand();

	virtual void RunImpl();
	virtual MojErr 	HandleResponse(const std::string& line);
	bool GetIsParserFailed() {return m_parseFailed;}

	MojErr 			ParserReady();
	MojErr			ParserDone();

	void Status(MojObject& status) const;
protected:
	void			ParseFailed();
	void 			ParseResumeFailed();
	void 			UpdateDownloadProgress(int dlSize);
	void			CompleteDownloadListener();
	virtual void 	Cleanup();

	boost::shared_ptr<FileCacheClient>	m_fileCacheClient;
	int 								m_msgNum;
	PopEmail::PopEmailPtr 				m_email;
	int									m_msgSize;
	int									m_downloadedSize;	// The number of characters that have been downloaded.
	int									m_incrementalSize;	// We will report the download progress every 10% of the total message size are downloaded.  This value stored the value of 10% of the total message size.
	int									m_sizeInterval;		// The next downloaded size that we should report the download progress.

	MojRefCountedPtr<Request> 			m_request;
	boost::shared_ptr<DownloadListener> m_listener;
	std::string							m_partPath;
	std::string							m_partMimeType;

	bool								m_parseFailed;

	MojRefCountedPtr<AsyncEmailParser>	m_emailParser;

	bool m_pause;
	LineReaderPtr m_readerPtr;
	MojSignal<>::Slot<DownloadEmailPartsCommand>						m_parserReadySlot;
	MojSignal<>::Slot<DownloadEmailPartsCommand>						m_parserDoneSlot;
	//static gboolean RunCallback(gpointer data);

};

#endif /* DOWNLOADEMAILPARTSCOMMAND_H_ */
