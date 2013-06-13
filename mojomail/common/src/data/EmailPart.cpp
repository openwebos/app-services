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

#include "data/EmailPart.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/foreach.hpp>

using namespace std;

EmailPart::EmailPart(PartType type)
: m_type(type), m_estimatedSize(-1), m_encodedSize(-1)
{
}

EmailPart::~EmailPart()
{
}

bool EmailPart::IsTextType() const
{
	// If there's no mime type available, treat bodies as text, otherwise false
	if(m_mimeType.empty()) return m_type == BODY;
	
	return boost::istarts_with(m_mimeType, "text/");
}

bool EmailPart::IsImageType() const
{
	if(m_mimeType.empty()) return false;
	
	return boost::istarts_with(m_mimeType, "image/");
}

bool EmailPart::IsMultipartType() const
{
	if(m_mimeType.empty()) return false;

	return boost::istarts_with(m_mimeType, "multipart/");
}

MojInt64 EmailPart::EstimateMaxSize() const
{
	MojInt64 size = m_encodedSize;

	if(size <= 0) return size;

	if(m_encoding == "base64") {
		size = (size * 3) / 4;
	}

	if (IsBodyPart()) {
		if (!m_charset.empty() && !boost::iequals(m_charset, "utf-8") && !boost::iequals(m_charset, "us-ascii")) {
			// Decoding from some charset to UTF-8 could be up to 4x as large as the native encoding
			// Must be a conservative (large) guess so we don't under-allocate
			size *= 4;
		} else {
			size *= 2; // just in case we missed something
		}
	}

	size += 1024; // add some padding (for undecodable characters, etc)

	return size;
}

MojInt64 EmailPart::EstimateSize() const
{
	MojInt64 size = m_encodedSize;

	if(size <= 0) return size;

	if(m_encoding == "base64") {
		size = (size * 3) / 4;
	}

	if(m_charset.empty() || m_charset == "utf-8" || m_charset == "us-ascii") {
		// Leave size alone; no charset decoding needed
	} else {
		// Arbitrary guess for non-ASCII charsets
		size *= 2;
	}

	return size;
}

std::string EmailPart::GuessFileExtension() const
{
	if( boost::iequals(m_mimeType, "text/plain") ) {
		return ".txt";
	} else if( boost::iequals(m_mimeType, "text/html") ) {
		return ".html";
	} else if( boost::iequals(m_mimeType, "image/jpeg") || boost::iequals(m_mimeType, "image/jpg") ) {
		return ".jpg";
	} else if( boost::iequals(m_mimeType, "image/png") ) {
		return ".png";
	} else if( boost::iequals(m_mimeType, "image/gif") ) {
		return ".gif";
	} else {
		return "";
	}
}

void EmailPart::GetFlattenedParts(const EmailPartPtr& rootPart, EmailPartList& outPartList, int depth)
{
	// Prevent unbounded recursion
	if (depth > 100) {
		return;
	}

	if (rootPart->IsMultipartType()) {
		string mimeType = rootPart->GetMimeType();
		const EmailPartList& subparts = rootPart->GetSubparts();

		if (mimeType == "multipart/alternative") {
			// Iterate over parts *backwards*
			const EmailPartList& alternatives = subparts;

			bool seenBodyAlternative = false;
			BOOST_FOREACH (const EmailPartPtr& altPart, make_pair(alternatives.rbegin(), alternatives.rend())) {
				EmailPartList altFlattened;

				// Get flattened list of parts
				GetFlattenedParts(altPart, altFlattened, depth + 1);

				bool hasBodyPart = false;

				if (!seenBodyAlternative) {
					// Check for body parts
					BOOST_FOREACH (const EmailPartPtr& oneFlattenedPart, altFlattened) {
						if (oneFlattenedPart->IsBodyPart()) {
							hasBodyPart = true;
							break;
						}
					}
				}

				// Copy parts and fix up types
				BOOST_FOREACH (const EmailPartPtr& oneFlattenedPart, altFlattened) {
					if (oneFlattenedPart->IsBodyPart() && !hasBodyPart) {
						// turn into an alt part
						oneFlattenedPart->SetType(EmailPart::ALT_TEXT);
					}

					outPartList.push_back(oneFlattenedPart);
				}

				// Other alternatives will not be treated as bodies
				if (hasBodyPart) {
					seenBodyAlternative = true;
				}
			}

		} else {
			// Recursively add subparts
			BOOST_FOREACH (const EmailPartPtr& part, subparts) {
				GetFlattenedParts(part, outPartList, depth + 1);
			}
		}
	} else {
		if(rootPart->GetType() == EmailPart::INLINE) {
			string mimeType = rootPart->GetMimeType();

			if(boost::iequals(mimeType, "text/html")
			|| boost::iequals(mimeType, "text/plain")
			|| boost::iequals(mimeType, "text/enriched")) {
				rootPart->SetType(EmailPart::BODY); // may be adjusted to ALT_BODY later
			} else if(rootPart->GetContentId().empty() || !rootPart->IsImageType()) {
				// not inline if it doesn't have a contentId, or isn't an image
				rootPart->SetType(EmailPart::ATTACHMENT);
			}
		}

		outPartList.push_back(rootPart);
	}
}
