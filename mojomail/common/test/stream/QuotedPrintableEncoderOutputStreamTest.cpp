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

#include "stream/QuotedPrintableEncoderOutputStream.h"
#include "stream/QuotePrintableDecoderOutputStream.h"
#include "stream/ByteBufferOutputStream.h"
#include <gtest/gtest.h>

/* Compare an input string filtered through the QP encoder to the expected output.
   We iterate over all possible split points, testing interruption of the write
   between all input characters, with zero-to-three one-character runs at the split,
   so we should single-step through all states.
   
   We then feed it back through the decoder to make sure it survives a round-trip.
 */
void MATCH(const char * in, const char * expect)
{
	int BUF_SIZE = 4096;
	char inbuf[BUF_SIZE];
	char xform[BUF_SIZE];
	char decoded[BUF_SIZE];
	int nread = 0;
	
	memset(inbuf, '\0', BUF_SIZE);
	memset(xform, '\0', BUF_SIZE);
	memset(decoded, '\0', BUF_SIZE);
	
//	printf("Testing encoding of {%s}\n", in);
	
	for (unsigned int junction = 0; junction < 4 && junction < strlen(in); junction++) {
		for (unsigned int split = 0; split < strlen(in) - junction; split++) {
		
			MojRefCountedPtr<ByteBufferOutputStream> bbos( new ByteBufferOutputStream() );
			MojRefCountedPtr<QuotedPrintableEncoderOutputStream> qpdos( new QuotedPrintableEncoderOutputStream(bbos) ) ;
	
			if (split > 0)
				qpdos->Write(in, split);
			if (junction > 0)
				qpdos->Write(in+split, 1);
			if (junction > 1)
				qpdos->Write(in+split+1, 1);
			if (junction > 2)
				qpdos->Write(in+split+2, 1);
			if (junction > 3)
				qpdos->Write(in+split+3, 1);
			if (split < strlen(in)-junction) 
				qpdos->Write(in + split + junction, strlen(in) - split - junction);
				
			qpdos->Flush();
		
			nread = bbos->ReadFromBuffer(xform, BUF_SIZE);
		
			ASSERT_EQ((int)strlen(expect), (int)nread);
			
			xform[nread] = '\0';
			
			ASSERT_STREQ(expect, std::string(xform, nread).c_str());
		}
	}
	
//	printf("Got {%s}\n", xform);

	// Take the last encoding, and try to decode it.
	MojRefCountedPtr<ByteBufferOutputStream> bbos2( new ByteBufferOutputStream() );
	MojRefCountedPtr<QuotePrintableDecoderOutputStream> qpeos( new QuotePrintableDecoderOutputStream(bbos2) ) ;
	
	qpeos->Write(xform, nread);
	qpeos->Flush();
			
	nread = bbos2->ReadFromBuffer(decoded, BUF_SIZE);

	ASSERT_EQ((int)strlen(in), (int)nread);
	ASSERT_STREQ(in, std::string(decoded, nread).c_str());
}

TEST(QuotedPrintableEncoderOutputStreamTest, TestQuotedPrintableEncoder)
{
	// Trivial tests, make sure the stream works
	MATCH("", "");
	MATCH("Test", "Test");
	MATCH("abcdefghijklmnopqrstuvwxyz", "abcdefghijklmnopqrstuvwxyz");
	MATCH("\r\n", "\r\n");

	// Test hex encoding
	MATCH("\x01", "=01");
	MATCH("\x01\xB5", "=01=B5");
	MATCH("\x01\xB5\xFE", "=01=B5=FE");
	MATCH("Hello\x01World", "Hello=01World");
	MATCH("Hello\x01\xB5World", "Hello=01=B5World");
	MATCH("Hello\x01\xB5\xFEWorld", "Hello=01=B5=FEWorld");
	
	// Test encoding of dots at beginning of lines, without encoding other dots needlessly
	MATCH(".", "=2E");
	MATCH(".\r\n", "=2E\r\n");
	MATCH("..\r\n", "=2E.\r\n");
	MATCH(" .\r\n", " .\r\n");
	MATCH(".\r\n.\r\n", "=2E\r\n=2E\r\n");
	MATCH("x.\r\n..\r\n", "x.\r\n=2E.\r\n");
	
	// Test line wrapping. No output characters should go past column 76.
	//     0         1         2         3         4         5         6         7
	//     01234567890123456789012345678901234567890123456789012345678901234567890123456789

	// 75 characters, easy
	MATCH("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
	      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
	MATCH("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\r\n",
	      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\r\n");

	// 76 characters, we currently wrap this, due to not having lookahead for every character.
	MATCH("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
	      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa=\r\n"
	      "a");
	MATCH("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\r\n",
	      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa=\r\n"
	      "a\r\n");
	MATCH("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n",
	      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa=\r\n"
	      "a=0A");

	// Very long line
	MATCH("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
	      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
	      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
	      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa=\r\n"
	      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa=\r\n"
	      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa=\r\n"
	      "aaa");
	      
	// More line wrapping: escaped characters showing up at end of line should be handled
	// properly, not causing line to be too long.
	//     0         1         2         3         4         5         6         7
	//     01234567890123456789012345678901234567890123456789012345678901234567890123456789
	MATCH("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\xFF""aaaaa",
	      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa=\r\n"
	      "=FFaaaaa");
	MATCH("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\xFF""aaaaa",
	      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa=\r\n"
	      "=FFaaaaa");
	MATCH("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\xFF""aaaaaa",
	      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa=\r\n"
	      "=FFaaaaaa");
	MATCH("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\xFF""aaaaaaa",
	      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa=\r\n"
	      "=FFaaaaaaa");
	MATCH("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\xFF""aaaaaaaa",
	      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa=FF=\r\n"
	      "aaaaaaaa");

	// Test maximum expansion: 26 characters that all have to be encoded, and which will need to be wrapped
	// to limit the line length.
	MATCH("\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF",
	"=FF=FF=FF=FF=FF=FF=FF=FF=FF=FF=FF=FF=FF=FF=FF=FF=FF=FF=FF=FF=FF=FF=FF=FF=FF=\r\n=FF");

	// Test encoding of CR and LF characters that aren't part of a proper line ending
	MATCH("abc\r", "abc=0D");
	MATCH("abc\n", "abc=0A");
	MATCH("abc\rdef", "abc=0Ddef");
	MATCH("abc\ndef", "abc=0Adef");
	MATCH("abc\n\r\ndef", "abc=0A\r\ndef");
	MATCH("abc\r\n\rdef", "abc\r\n=0Ddef");

        // Test encoding of whitespace at end of lines
	MATCH(" \r\n", "=20\r\n");
	MATCH("\t\r\n", "=09\r\n");
	MATCH("  \r\n", " =20\r\n");
	MATCH("\t\t\r\n", "\t=09\r\n");
	MATCH(" .\r\n", " .\r\n");
	MATCH("\t.\r\n", "\t.\r\n");
	// A lone LF is not part of a line-ending, so white-space encoding is not necessary.
	MATCH(" \n", " =0A");
	MATCH("\t\n", "\t=0A");
	// Make sure uninteresting whitespace doesn't trigger encoding
	MATCH(" x", " x");
	MATCH("\tx", "\tx");
	MATCH(" \rx", " =0Dx");
	MATCH("\t\rx", "\t=0Dx");
	// Make sure whitespace at end of buffer is encoded for safety.
	MATCH(" ", "=20");
	MATCH("\t", "=09");
	MATCH(" \r", "=20=0D");
	MATCH("\t\r", "=09=0D");
}
