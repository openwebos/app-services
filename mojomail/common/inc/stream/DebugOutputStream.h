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

#ifndef DEBUGOUTPUTSTREAM_H_
#define DEBUGOUTPUTSTREAM_H_

#include "stream/BaseOutputStream.h"

class DebugOutputStream : public ChainedOutputStream
{
public:
	DebugOutputStream(const OutputStreamPtr& sink, const char* filename = "/dev/fd/2");
	virtual ~DebugOutputStream();

	virtual void Write(const char* src, size_t bytes);
	virtual void Flush();
	virtual void Close();

protected:
	virtual void Init(const char* filename);

	FILE*	m_file;
};

class HexOutputStream : public DebugOutputStream
{
public:
	HexOutputStream(const OutputStreamPtr& sink, const char* filename = "/dev/fd/2");
	virtual ~HexOutputStream();

	virtual void Write(const char* src, size_t bytes);
};

#endif /* DEBUGOUTPUTSTREAM_H_ */
