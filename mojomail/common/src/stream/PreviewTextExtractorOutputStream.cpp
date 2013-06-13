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

#include "stream/PreviewTextExtractorOutputStream.h"
#include "email/PreviewTextGenerator.h"

PreviewTextExtractorOutputStream::PreviewTextExtractorOutputStream(const OutputStreamPtr& sink, size_t previewLength)
: ChainedOutputStream(sink), m_previewLength(previewLength)
{
}

PreviewTextExtractorOutputStream::~PreviewTextExtractorOutputStream()
{
}

void PreviewTextExtractorOutputStream::Write(const char* src, size_t length)
{
	if(m_previewBuf.length() >= m_previewLength) {
		// already have enough text; do nothing
	} else {
		size_t space = m_previewLength - m_previewBuf.length();
		size_t bytesToCopy = std::min(space, length);
		m_previewBuf.append(src, bytesToCopy);
	}

	ChainedOutputStream::Write(src, length);
}

const std::string& PreviewTextExtractorOutputStream::GetPreviewText()
{
	// Chop off any partial UTF-8 characters

	//fprintf(stdout, "m_previewBuf:%d and m_previewLength:%d", m_previewBuf.length(), m_previewLength);
	PreviewTextGenerator::TruncateUTF8(m_previewBuf, m_previewLength);
	return m_previewBuf;
}
