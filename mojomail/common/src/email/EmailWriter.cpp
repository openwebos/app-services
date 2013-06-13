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

#include "email/EmailWriter.h"

#include <cassert>
#include <algorithm>
#include <cstring>
#include <ctime>
#include <sstream>
#include <boost/algorithm/string/predicate.hpp>

#include "data/Email.h"
#include "data/EmailPart.h"
#include "data/EmailAdapter.h"
#include "email/MimeHeaderWriter.h"
#include "stream/ByteBufferOutputStream.h"
#include "stream/Base64OutputStream.h"
#include "stream/QuotedPrintableEncoderOutputStream.h"

using namespace std;

EmailWriter::EmailWriter(Email& email)
: m_email(email),
  m_includeBcc(true)
{
	// FIXME: needs a more unique boundary id
	time_t now = time(NULL);
	
	// FIXME workaround
	if(email.GetDateReceived()) {
		email.SetDateReceived((MojInt64)(now) * 1000L);
	}
	
	std::stringstream stream;
	stream << now;

	m_boundary = "Multipart_=_Boundary_=_" + stream.str();
	m_altBoundary = "Alternative_=_Boundary_=_" + stream.str();
}

EmailWriter::~EmailWriter()
{
}

void EmailWriter::SetOutputStream(const OutputStreamPtr& outputStream)
{
	m_outputStream = outputStream;
}

void EmailWriter::SetBccIncluded(bool includeBcc)
{
	m_includeBcc = includeBcc;
}

OutputStreamPtr EmailWriter::GetPartOutputStream(const EmailPart& part)
{
	assert(m_outputStream.get());
	
	OutputStreamPtr out;
	
	if(part.IsBodyPart() || part.GetCharset() == "UTF-8") {
		// Bodies should be quoted-printable encoded
		out.reset( new QuotedPrintableEncoderOutputStream(m_outputStream) );
		return out;
		//return m_outputStream;
	} else {
		// Attachments are always base64-encoded
		out.reset( new Base64EncoderOutputStream(m_outputStream) );
		return out;
		//return m_outputStream;
	}
}

void EmailWriter::WriteEmailHeader(const std::string& emailContentType, const std::string& boundary)
{
	assert(m_outputStream.get());
	
	// Write email header
	std::string header;
	MimeHeaderWriter headerWriter(header);
	
	// NOTE: EAS apparently requires the "From" header before the subject.
	// It seems to fail with a 500 error if the order is reversed.
	
	// Date header
	// FIXME should be a cleaner way to format the date
	const size_t MAX_DATE_LENGTH = 255;
	char buf[MAX_DATE_LENGTH];
	//time_t timestamp = (time_t) (m_email.GetDateReceived() / 1000);
	time_t timestamp = time(NULL);
	size_t len = strftime(buf, MAX_DATE_LENGTH, "%a, %d %b %Y %H:%M:%S %z", localtime(&timestamp));
	headerWriter.WriteHeader("Date", std::string(buf, len));
	
	// From header
	if(m_email.GetFrom().get()) {
		headerWriter.WriteAddressHeader("From", m_email.GetFrom());
	}

	// Reply-To header
	if(m_email.GetReplyTo().get()) {
		headerWriter.WriteAddressHeader("Reply-To", m_email.GetReplyTo());
	}
	
	// Recipient headers
	if(m_email.GetTo().get() && !m_email.GetTo()->empty())
		headerWriter.WriteAddressHeader("To", m_email.GetTo());
	if(m_email.GetCc().get() && !m_email.GetCc()->empty())
		headerWriter.WriteAddressHeader("Cc", m_email.GetCc());
	
	// Bcc headers are included in EAS, but not SMTP
	if(m_includeBcc && m_email.GetBcc().get() && !m_email.GetBcc()->empty())
		headerWriter.WriteAddressHeader("Bcc", m_email.GetBcc());
	
	// Subject
	headerWriter.WriteHeader("Subject", m_email.GetSubject(), true);
	
	// TODO: Accept-Language
	
	// Priority / Importance
	Email::Priority priority = m_email.GetPriority();
	if (priority != Email::Priority_Normal) {
		headerWriter.WriteHeader("X-Priority", EmailAdapter::GetHeaderPriority(priority));
		headerWriter.WriteHeader("Importance", EmailAdapter::GetHeaderImportance(priority));
	}

	// In-Reply-To header
	if(!m_email.GetInReplyTo().empty()) {
		headerWriter.WriteHeader("In-Reply-To", m_email.GetInReplyTo());
	}

	headerWriter.WriteHeader("X-Mailer", "Palm webOS");
	headerWriter.WriteHeader("MIME-Version", "1.0");
	
	// FIXME: use multipart/alternative if there's no attachments
	headerWriter.WriteHeader("Content-Type", emailContentType + (boundary.empty() ? "" : ("; boundary=\"" + boundary + "\"")));
	
	header.append("\r\n");
	m_outputStream->Write(header.data(), header.size());
}

void EmailWriter::WriteAlternativeHeader(const std::string& boundary, const std::string& altBoundary)
{
	std::string header("--" + boundary + "\r\n");
	MimeHeaderWriter headerWriter(header);
	headerWriter.WriteHeader("Content-Type", "multipart/alternative; boundary=\"" + altBoundary + "\"");

	header.append("\r\n");
	m_outputStream->Write(header.data(), header.size());
}

void EmailWriter::WritePartHeader(const EmailPart& part, const std::string& boundary)
{
	std::string partHeader("--" + boundary + "\r\n");
	
	MimeHeaderWriter partHeaderWriter(partHeader);
	
	std::string mimeType = part.GetMimeType();
	
	// Pick a reasonable default mime type if the mime type is missing
	if(mimeType.empty()) {
		mimeType = part.IsBodyPart() ? "text/plain" : "application/octet-stream";
	}
	
	// Content-Type and Content-Transfer-Encoding
	if(part.IsBodyPart() || boost::iequals(part.GetCharset(), "UTF-8")) {
		partHeaderWriter.WriteParameterHeader("Content-Type", mimeType, "charset", "UTF-8");
		partHeaderWriter.WriteHeader("Content-Transfer-Encoding", "quoted-printable");
	} else {
		partHeaderWriter.WriteHeader("Content-Type", mimeType);
		partHeaderWriter.WriteHeader("Content-Transfer-Encoding", "base64");
	}
	
	// Write Content-Disposition for attachment/inline
	if(!part.IsBodyPart()) {
		std::string disposition = part.IsAttachment() ? "attachment" : "inline";

		if(!part.GetContentId().empty()) {
			partHeaderWriter.WriteHeader("Content-Id", "<" + part.GetContentId() + ">");
		}

		if(!part.GetDisplayName().empty()) {
			// Include filename
			partHeaderWriter.WriteParameterHeader("Content-Disposition", disposition, "filename", part.GetDisplayName());
		} else {
			// No filename
			partHeaderWriter.WriteHeader("Content-Disposition", disposition);
		}
	}
	
	partHeader.append("\r\n");
	m_outputStream->Write(partHeader.data(), partHeader.size());
}

void EmailWriter::WritePartFooter()
{
	m_outputStream->Write("\r\n", 2);
}

void EmailWriter::WriteBoundary(const std::string& boundary, bool end)
{
	if(!boundary.empty()) {
		if(!end) {
			m_outputStream->Write("--" + boundary + "\r\n");
		} else {
			m_outputStream->Write("--" + boundary + "--\r\n");
		}
	}
}

void EmailWriter::WriteEmailFooter()
{
	std::string footer("--" + m_boundary + "--\r\n");
	m_outputStream->Write(footer.data(), footer.size());
}
