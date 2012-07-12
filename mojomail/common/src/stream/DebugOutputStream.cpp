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

#include "stream/DebugOutputStream.h"
#include "util/Hexdump.h"

DebugOutputStream::DebugOutputStream(const OutputStreamPtr& sink, const char* filename)
: ChainedOutputStream(sink),
  m_file(NULL)
{
	Init(filename);
}

DebugOutputStream::~DebugOutputStream()
{
	if(m_file) {
		fclose(m_file);
		m_file = NULL;
	}
}

void DebugOutputStream::Init(const char* filename)
{
	m_file = fopen(filename, "w");
	if(!m_file) {
		fprintf(stderr, "error opening debug log file '%s'\n", filename);
	}
}

void DebugOutputStream::Write(const char* src, size_t bytes)
{
	if(m_file) {
		fwrite(src, 1, bytes, m_file);
		fflush(m_file);
	}

	if(m_sink.get()) {
		m_sink->Write(src, bytes);
	}
}

void DebugOutputStream::Flush()
{
	if(m_file) {
		fflush(m_file);
	}

	if(m_sink.get()) {
		m_sink->Flush();
	}
}

void DebugOutputStream::Close()
{
	if(m_file) {
		fclose(m_file);
		m_file = NULL;
	}

	if(m_sink.get()) {
		m_sink->Close();
	}
}

HexOutputStream::HexOutputStream(const OutputStreamPtr& sink, const char* filename)
: DebugOutputStream(sink, filename)
{
}

HexOutputStream::~HexOutputStream()
{
}

void HexOutputStream::Write(const char* src, size_t bytes)
{
	if(m_file) {
		Hexdump::Dump(m_file, "", src, bytes);
		fflush(m_file);
	}

	if(m_sink.get()) {
		m_sink->Write(src, bytes);
	}
}

