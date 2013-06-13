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

#ifndef UTF8DECODEROUTPUTSTREAM_H_
#define UTF8DECODEROUTPUTSTREAM_H_
#include <glib.h>
#include "stream/BaseOutputStream.h"
#include <boost/scoped_array.hpp>
#include <unicode/ucnv.h>
#include <unicode/uenum.h>
#include "core/MojString.h"

class UTF8DecoderOutputStream : public ChainedOutputStream
{
	static const size_t BLOCK_SIZE;
	static const size_t PIVOT_BUF_SIZE;
	static UConverter *s_targetCnv;

public:
	UTF8DecoderOutputStream(const OutputStreamPtr& sink, const char* converterName);
	virtual ~UTF8DecoderOutputStream();

	// Write some data to the stream
	void Write(const char* src, size_t length);

	// Close the stream
	void Close();

protected:
	void Write(const char* src, size_t length, bool eof);

	bool		m_initialized;
	UConverter*	m_sourceCnv;
	boost::scoped_array<UChar>	m_pivotBuf;
	UChar*		m_pivotSource;
	UChar*		m_pivotTarget;

private:
	//std::map< const MojString, boost::shared_ptr<UConverter> > m_cacheConverterMap;
	static UConverter* GetCacheConverter(const char* converterName, UErrorCode *err);
	static void ReleaseCacheConverter(UConverter* conv);
};
#endif /* UTF8DECODEROUTPUTSTREAM_H_ */
