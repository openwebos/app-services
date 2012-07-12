#include "client/MockFileCacheService.h"
#include "CommonPrivate.h"
#include <sstream>

using namespace std;

MockFileCacheService::MockFileCacheService()
: MockRemoteService("com.palm.filecache"),
  m_fileCount(0)
{
}

MockFileCacheService::~MockFileCacheService()
{
}

std::string MockFileCacheService::GetPathFor(int fileNum)
{
	stringstream ss;
	ss << "/var/file-cache/MOCK-FILE-" << fileNum;
	return ss.str();
}

void MockFileCacheService::HandleRequestImpl(MockRequestPtr req)
{
	MojErr err;
	string method = req->GetMethod();

	if (method == "InsertCacheObject") {
		int fileNum = m_fileCount++;

		MojObject response;
		err = response.putString("pathName", GetPathFor(fileNum).c_str());
		ErrorToException(err);

		req->ReplySuccess(response);
	} else if (method == "ResizeCacheObject") {
		MojInt64 newSize;
		err = req->GetPayload().getRequired("newSize", newSize);

		MojObject response;
		err = response.put("newSize", newSize);
		ErrorToException(err);

		req->ReplySuccess(response);
	}
}
