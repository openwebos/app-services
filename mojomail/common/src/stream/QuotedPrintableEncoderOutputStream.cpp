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

#include "stream/QuotedPrintableEncoderOutputStream.h"

QuotedPrintableEncoderOutputStream::QuotedPrintableEncoderOutputStream(const OutputStreamPtr& out)
: ChainedOutputStream(out), m_state(0), m_col(0){

    m_col = 0;
}

QuotedPrintableEncoderOutputStream::~QuotedPrintableEncoderOutputStream() {

}

#define output_char(c, force) do {					\
	bool encode = force;							\
	if (!((c == 32) || (c == 9) ||					\
		(c >= 33 && c <= 60) ||						\
		(c >= 62 && c <= 126)))						\
		encode = true;								\
													\
	/* Enforce line wrapping */						\
	if ((m_col + (encode ? 3 : 1)) >= 76) {			\
		outbuf[cnt++] = '=';						\
		outbuf[cnt++] = '\r';						\
		outbuf[cnt++] = '\n';						\
		m_col = 0;									\
	}												\
													\
    /* For SMTP transparency, over-encode dots at beginning of line. */ \
	if (c == '.' && m_col == 0)						\
		encode = true;								\
													\
	if (encode) {									\
		const char HEX[] = "0123456789ABCDEF";		\
		unsigned int hi = ((unsigned char)c)>>4;	\
		unsigned int lo = ((unsigned char)c)&0x0f;	\
		outbuf[cnt++] = '=';						\
		outbuf[cnt++] = HEX[hi];					\
		outbuf[cnt++] = HEX[lo];					\
		m_col += 3;									\
	} else {										\
		outbuf[cnt++] = c;							\
		m_col ++;									\
	}												\
} while (0)

void QuotedPrintableEncoderOutputStream::Write(const char* buf, size_t len) {

	int offset = 0; 
	int cnt = 0;
	
	//FIXME check size, do not reallocate if it is big enough.
    // worst case, repeated "\xFF" stream expands to "=FF" with line breaks every 25 1/3 input chars, of "=\r\n"
    // characters. (((25.3*3)+4) / (25*1). That's a bit less than 3.2, so multiply by 3.2 over 1.
    
    m_outbuf.reset( new char[((len + 1) * 32 / 10)] );
	
	char *outbuf = m_outbuf.get();
	for (unsigned int i=0;i<len;i++) {
		char c = buf[offset+i];
		
		switch (m_state) {
		restart_state_0:
			m_state = 0;
			// FALLTHROUGH
		case 0: {	// Normal characters
			if (c == '\r') {
				m_state = 1;
				break;
			} else if (c == ' ') {
				m_state = 11;
				break;
			} else if (c == '\t') {
				m_state = 21;
				break;
			} else {
				output_char(c, false);
				break;
			}
		}
		case 1: { // Possible start of CRLF sequence, seen CR
			if (c == '\n') {
				outbuf[cnt++] = '\r';
				outbuf[cnt++] = '\n';
				m_col = 0;
				m_state = 0;
				break;
			} else {
				output_char('\r', false);
				goto restart_state_0;
			}
		}
		case 11: { // Possible start of SPCRLF sequence, seen SP
			if (c == '\r') {
				m_state = 12;
				break;
			} else {
				output_char(' ', false);
				goto restart_state_0;
			}
		}
		case 12: { // Possible start of SPCRLF sequence, seen SPCR
			if (c == '\n') {
				output_char(' ', true); // force hex encoding, so there is not unguarded whitespace
										// at the end of the line.
				outbuf[cnt++] = '\r';
				outbuf[cnt++] = '\n';
				m_col = 0;
				m_state = 0;
				break;
			} else {
				output_char(' ', false);
				output_char('\r', false);
				goto restart_state_0;
			}
		}
		case 21: { // Possible start of HTCRLF sequence, seen HT
			if (c == '\r') {
				m_state = 22;
				break;
			} else {
				output_char('\t', false);
				goto restart_state_0;
			}
		}
		case 22: { // Possible start of HTCRLF sequence, seen HTCR
			if (c == '\n') {
				output_char('\t', true); // force hex encoding, so there is not unguarded whitespace
										// at the end of the line.
				outbuf[cnt++] = '\r';
				outbuf[cnt++] = '\n';
				m_col = 0;
				m_state = 0;
				break;
			} else {
				output_char('\t', false);
				output_char('\r', false);
				goto restart_state_0;
			}
		}
		}
	}

	m_sink->Write(outbuf, cnt);
}

void QuotedPrintableEncoderOutputStream::Flush(FlushType flushType)
{
	int cnt = 0;
    
	m_outbuf.reset( new char[10] );
	char *outbuf = m_outbuf.get();

	// Assume that the end of the stream is a significant barrier, so
	// that any partial CRLF sequences should be encoded as they are
	// to not be considered proper line endings, and that whitespace
	// should also be encoded, in case there might be forced line-ending
	// after the stream.
	
	switch (m_state) {
	case 1: { // Possible start of CRLF sequence, seen CR
		output_char('\r', false);
		break;
	}
	case 11: { // Possible start of SPCRLF sequence, seen SP
		output_char(' ', true);
		break;
	}
	case 12: { // Possible start of SPCRLF sequence, seen SPCR
		output_char(' ', true);
		output_char('\r', false);
		break;
	}
	case 21: { // Possible start of HTCRLF sequence, seen HT
		output_char('\t', true);
		break;
	}
	case 22: { // Possible start of HTCRLF sequence, seen HTCR
		output_char('\t', true);
		output_char('\r', false);
		break;
	}
	}

	if (cnt > 0)
		m_sink->Write(outbuf, cnt);

	m_sink->Flush(flushType);
}
