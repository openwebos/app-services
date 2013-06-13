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

#include "stream/QuotePrintableDecoderOutputStream.h"

QuotePrintableDecoderOutputStream::QuotePrintableDecoderOutputStream(const OutputStreamPtr& out)
: ChainedOutputStream(out), m_state(0), m_save(0), m_hi(0)
{

}

QuotePrintableDecoderOutputStream::~QuotePrintableDecoderOutputStream()
{

}

void QuotePrintableDecoderOutputStream::Write(const char* buf, size_t len)
{
	int offset = 0; 
	int cnt = 0;
	//fprintf(stdout, "Quoted Printable Output Stream %p before conversion [%s]\n", this, std::string(buf, len).c_str());
	//FIXME check size, do not reallocate if it is big enough.
	m_outbuf.reset( new char[len] );
	char *outbuf = m_outbuf.get();
	for (unsigned int i=0;i<len;i++) {
		char c = buf[offset+i];
		switch (m_state) {
		case 0: {
			if (c!='=') {
				outbuf[cnt++] = c;
			}
			else {
				m_state = 1;
			}
			break;
		}
		case 1: {
			m_hi = c;
			m_state = 2;
			break;
		}
		case 2: {
			int lo = c;
			m_state = 0;
			if (m_hi=='\r' && lo=='\n')
				break;
			m_hi = (m_hi>='A' ? m_hi-'A'+10 : m_hi-'0');
			lo = (lo>='A' ? lo-'A'+10 : lo-'0');
			outbuf[cnt++] = ((m_hi<<4)+lo);
			break;
		}
		}
	}

	//fprintf(stdout, "Quoted Printable Output Stream %p after conversion [%s]\n", this, std::string(outbuf, cnt).c_str());
	m_sink->Write(outbuf, cnt);
}
