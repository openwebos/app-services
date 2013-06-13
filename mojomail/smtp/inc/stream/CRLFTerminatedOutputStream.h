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

#ifndef CRLFTERMINATEDOUTPUTSTREAM_H_
#define CRLFTERMINATEDOUTPUTSTREAM_H_

#include <glib.h>
#include "stream/BaseOutputStream.h"
#include <boost/scoped_array.hpp>
#include <unicode/ucnv.h>
#include <unicode/uenum.h>
#include "core/MojString.h"

class CRLFTerminatedOutputStream : public ChainedOutputStream
{
public:
	CRLFTerminatedOutputStream(const OutputStreamPtr& sink);
	virtual ~CRLFTerminatedOutputStream();

	// Write some data to the stream
	virtual void Write(const char* src, size_t length);
	
	// Flush the stream at the end
	virtual void Flush();

private:

	// Remember last two bytes seen
	char m_penultimateChar;
	char m_ultimateChar;

};
#endif /* CRLFTERMINATEDOUTPUTSTREAM_H_ */
