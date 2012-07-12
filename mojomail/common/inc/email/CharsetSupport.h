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

#ifndef CHARSETSUPPORT_H_
#define CHARSETSUPPORT_H_

namespace CharsetSupport {
	/**
	 * Given a charset name, returns a charset to use for decoding.
	 * This returns a different charset name for certain charsets
	 * that are often improperly labeled by email clients.
	 */
	const char* SelectCharset(const char* charsetName);
}

#endif /* CHARSETSUPPORT_H_ */
