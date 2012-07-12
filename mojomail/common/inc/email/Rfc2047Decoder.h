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

#ifndef RFC2047DECODER_H_
#define RFC2047DECODER_H_

#include <string>

namespace Rfc2047Decoder {
	// Encode a UTF-8 string using RFC 2047 Q-encoding or B-encoding
	void Decode(const std::string& text, std::string& output);

	// Decode a block of text, which may contain a mix of ASCII and encoded text
	// The text will be assigned to the out parameter
	void DecodeText(const std::string& text, std::string& out);

	// Decodes a block of text. Ensures no exceptions will be thrown.
	std::string SafeDecodeText(const std::string& text);
};

#endif /* RFC2047DECODER_H_ */
