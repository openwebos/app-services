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

#ifndef EMAILPART_H_
#define EMAILPART_H_

#include <string>
#include "data/CommonData.h"
#include "core/MojObject.h"

class EmailPart
{
public:
	enum PartType {
		BODY, INLINE, ATTACHMENT, SMART_TEXT, ALT_TEXT, MULTIPART
	};
	
	EmailPart(PartType type);
	virtual ~EmailPart();
	
	// Setters
	void SetId(const MojObject& id)						{ m_id = id; }
	void SetType(PartType type)							{ m_type = type; }
	void SetMimeType(const std::string& mimeType)		{ m_mimeType = mimeType; }
	void SetLocalFilePath(const std::string& path)		{ m_localFilePath = path; }
	void SetDisplayName(const std::string& name)		{ m_displayName = name; }
	void SetContentId(const std::string& contentId)		{ m_contentId = contentId; }
	void SetEncoding(const std::string& encoding)		{ m_encoding = encoding; }
	void SetCharset(const std::string& charset)			{ m_charset = charset; }

	void SetEstimatedSize(MojInt64 size)				{ m_estimatedSize = size; }
	void SetEncodedSize(MojInt64 size)					{ m_encodedSize = size; }
	void SetSection(const std::string& section)			{ m_section = section; }

	// Getters
	const MojObject&		GetId() const				{ return m_id; }
	PartType				GetType() const				{ return m_type; }
	const std::string&		GetMimeType() const			{ return m_mimeType; }
	const std::string&		GetLocalFilePath() const	{ return m_localFilePath; }
	const std::string&		GetDisplayName() const		{ return m_displayName; }
	const std::string&		GetContentId() const		{ return m_contentId; }
	const std::string&		GetEncoding() const			{ return m_encoding; }
	const std::string&		GetCharset() const			{ return m_charset; }
	
	MojInt64				GetEstimatedSize()			{ return m_estimatedSize; }
	MojInt64				GetEncodedSize()			{ return m_encodedSize; }
	const std::string&		GetSection()				{ return m_section; }

	// Types
	bool					IsBodyPart() const			{ return m_type == EmailPart::BODY || m_type == EmailPart::SMART_TEXT || m_type == EmailPart::ALT_TEXT; }
	bool					IsAttachment() const		{ return m_type == EmailPart::ATTACHMENT; }
	bool					IsInline() const			{ return m_type == EmailPart::INLINE; }
	
	// Returns whether this part has a text/* mime type
	bool					IsTextType() const;
	
	// Returns whether this part has an image/* mime type
	bool					IsImageType() const;
	
	// Returns whether this part has an multipart/* mime type
	bool					IsMultipartType() const;

	// Calculate the maximum size of a part
	MojInt64 EstimateMaxSize() const;

	// Calculate estimated size
	MojInt64 EstimateSize() const;

	// Guess a file extension from the MIME type
	std::string GuessFileExtension() const;

	// Not persisted -- used for internal use only
	std::string GetPreviewText() const { return m_previewText; }
	void SetPreviewText(const std::string& previewText) { m_previewText = previewText; }

	// Manage subparts (only for multipart parts)
	const EmailPartList& GetSubparts() const { return m_subparts; }
	void AddSubpart(const EmailPartPtr& part) { m_subparts.push_back(part); }
	static void GetFlattenedParts(const EmailPartPtr& rootPart, EmailPartList& outPartList, int depth = 0);

protected:
	// The part type; either BODY, INLINE, or ATTACHMENT
	PartType m_type;
	
	// Database id of the part
	MojObject m_id;

	// The mime type of the part, formatted as: type/subtype
	std::string m_mimeType;
	
	// Path to the content of this part stored on the file system
	std::string m_localFilePath;
	
	// The name of this part, e.g. a file name for an attachment
	std::string m_displayName;
	
	// The original encoding of this part, e.g. "base64" or "7bit"
	// Note that the locally stored copy may already be decoded
	std::string m_encoding;
	
	// The original charset of this part, e.g. "UTF-8", "ISO-2022-JP2"
	// Note that the locally stored copy may already be decoded
	std::string m_charset;

	// Unique identifier for this part among other parts associated with the same message
	// Mainly used for cid: URIs
	std::string m_contentId;

	// IMAP-specific field containing the section (e.g. 1.2) of the part
	// OPTIONAL but IMAP may report an error if it's missing
	std::string m_section;

	// Estimated size (after decoding)
	MojInt64 m_estimatedSize;

	// Encoded size on server
	MojInt64 m_encodedSize;

	// Preview text (not saved)
	std::string m_previewText;

	// Subparts (not saved)
	EmailPartList m_subparts;
};

#endif /*EMAILPART_H_*/
