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

#include "stream/UTF8DecoderOutputStream.h"
#include "exceptions/MailException.h"
#include "email/CharsetSupport.h"

using namespace std;

// Number of outbox bytes to convert at a time
const size_t UTF8DecoderOutputStream::BLOCK_SIZE = 4096;

// Size of pivot buffer
const size_t UTF8DecoderOutputStream::PIVOT_BUF_SIZE = 10;

UConverter *UTF8DecoderOutputStream::s_targetCnv;

UTF8DecoderOutputStream::UTF8DecoderOutputStream(const OutputStreamPtr& sink, const char* converterName)
: ChainedOutputStream(sink),
  m_initialized(false),
  m_sourceCnv(NULL),
  m_pivotSource(NULL),
  m_pivotTarget(NULL)
{
	UErrorCode err = U_ZERO_ERROR;

	m_sourceCnv = GetCacheConverter(converterName, &err);

	if(err) {
		string errorMsg = "unable to get converter for charset '" + string(converterName) + "'";
		throw MailException(errorMsg.c_str(), __FILE__, __LINE__);
	}

	if(s_targetCnv == NULL) {
		s_targetCnv = GetCacheConverter("UTF-8", &err);

		if(err)
		{
			throw MailException("unable to get UTF-8 converter", __FILE__, __LINE__);
		}
	}

	// Setup pivots
	m_pivotBuf.reset(new UChar[PIVOT_BUF_SIZE]);
	m_pivotSource = m_pivotBuf.get();
	m_pivotTarget = m_pivotBuf.get();
}

UTF8DecoderOutputStream::~UTF8DecoderOutputStream()
{
	if(m_sourceCnv)
		ReleaseCacheConverter(m_sourceCnv);
}

void UTF8DecoderOutputStream::Write(const char* src, size_t length, bool eof)
{
	UErrorCode ErrorCode;

	char outbuf[BLOCK_SIZE];

	const char* start = src;
	const char* end = src + length;

	do {
		char* target = outbuf;

		// Must be initialized to success before calling convert
		ErrorCode = U_ZERO_ERROR;

		ucnv_convertEx(s_targetCnv, m_sourceCnv,
				               &target, outbuf + BLOCK_SIZE,
				               &start, end,
				               m_pivotBuf.get(), &m_pivotSource,
				               &m_pivotTarget, m_pivotBuf.get() + PIVOT_BUF_SIZE,
				               !m_initialized, eof,
				               &ErrorCode);

		m_initialized = true;

		m_sink->Write(outbuf, target - outbuf);
	} while(ErrorCode == U_BUFFER_OVERFLOW_ERROR);

	if(!U_SUCCESS(ErrorCode)) {
		throw MailException("conversion error", __FILE__, __LINE__);
	}
}

void UTF8DecoderOutputStream::Write(const char* src, size_t length)
{
	Write(src, length, false);
}

void UTF8DecoderOutputStream::Close()
{
	Write("", 0, true);
	m_sink->Close();
}

//FIXME cache the converters
// NOTE: need to be careful about state
UConverter * UTF8DecoderOutputStream::GetCacheConverter(const char* converterName, UErrorCode *err)
{
	converterName = CharsetSupport::SelectCharset(converterName);

	//int32_t length;
	//UErrorCode status;
	//std::map<const MojObject, boost::shared_ptr<UConverter> >::iterator it;
	//MojObject(converterName);
	//it=m_cacheConverterMap.find();
	//if (it == m_cacheConverterMap.end()) {
	//	MojLogInfo(m_log, "creating converter for account %s", converterName);
	//return retCnv = ucnv_open(encodeName.data(), err);
	//
	//return retCnv = NULL;
	//	m_cacheConverterMap[accountId] = retCnv;
	//	client->SetAccount(accountId, false);
	//} else {
	//	client = it->second;
	//}

	return ucnv_open(converterName, err);
}

void UTF8DecoderOutputStream::ReleaseCacheConverter(UConverter* conv)
{
	ucnv_close(conv);
}
