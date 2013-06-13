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

#ifndef EMAIL_H_
#define EMAIL_H_

#include <string>
#include "data/CommonData.h"
#include "core/MojObject.h"
#include "CommonErrors.h"

class Email
{
public:
	Email();
	virtual ~Email();
	
	typedef enum {
		DraftTypeNew,
		DraftTypeReply,
		DraftTypeForward
	} DraftType;

	typedef enum {
		Priority_Highest	= 1,
		Priority_High		= 2,
		Priority_Normal		= 3,
		Priority_Low		= 4,
		Priority_Lowest		= 5
	} Priority;

	// Setters
	void SetId(const MojObject& id)						{ m_id = id; }
	void SetOriginalMsgId(const MojObject& id)			{ m_originalMsgId = id; }
	void SetFolderId(const MojObject& folderId)			{ m_folderId = folderId; }
	void SetSubject(const std::string& subject)			{ m_subject = subject; }
	void SetPreviewText(const std::string& previewText)	{ m_previewText = previewText; }
	void SetDateReceived(MojInt64 dateReceived)			{ m_dateReceived = dateReceived; }
	void SetDraftType(DraftType draftType)				{ m_draftType = draftType; }

	void SetFrom(const EmailAddressPtr& fromAddress)	{ m_from = fromAddress; }
	void SetReplyTo(const EmailAddressPtr& replyTo)		{ m_replyTo = replyTo; }
	void SetTo(const EmailAddressListPtr& toList)		{ m_to = toList; }
	void SetCc(const EmailAddressListPtr& ccList)		{ m_cc = ccList; }
	void SetBcc(const EmailAddressListPtr& bccList)		{ m_bcc = bccList; }
	
	void SetPartList(const EmailPartList& partList)		{ m_partList = partList; }
	
	void SetRead(bool read)								{ m_read = read; }
	void SetReplied(bool replied)						{ m_replied = replied; }
	void SetFlagged(bool flagged)						{ m_flagged = flagged; }
	void SetHasAttachments(bool hasAttachments)			{ m_hasAttachments = hasAttachments; }
	void SetEditedOriginal(bool hasEditedOriginal)		{ m_editedOriginal = hasEditedOriginal; }

	// These are used for threading (for sending replies with the proper message reference info)
	void SetMessageId(const std::string& messageId)		{ m_messageId = messageId; }
	void SetInReplyTo(const std::string& inReplyTo)		{ m_inReplyTo = inReplyTo; }

	void SetServerUniqueId(const std::string& id)		{ m_serverUniqueId = id; }
	void SetServerConversationId(const std::string& id)	{ m_serverConversationId = id; }

	void SetPriority(Priority priority)					{ m_priority = priority; }

	// SendStatus setters
	void SetSent(bool sent)								{ m_sent = sent; }
	void SetHasFatalError(bool hasFatalError)			{ m_hasFatalError = hasFatalError; }
	void SetRetryCount(int retryCount)					{ m_retryCount = retryCount; }
	void SetSendError(MailError::ErrorCode sendError)	{ m_sendError = sendError; }

	// Getters
	MojObject					GetId()	const			{ return m_id; }
	MojObject					GetOriginalMsgId() const { return m_originalMsgId; }
	MojObject					GetFolderId() const		{ return m_folderId; }
	const std::string&			GetSubject() const		{ return m_subject; }
	const std::string&			GetPreviewText() const	{ return m_previewText; }
	MojInt64					GetDateReceived() const { return m_dateReceived; }
	DraftType					GetDraftType() const 	{ return m_draftType; }
	
	// Address getters
	const EmailAddressPtr&			GetFrom() const		{ return m_from; }
	const EmailAddressPtr&			GetReplyTo() const	{ return m_replyTo; }
	const EmailAddressListPtr&		GetTo() const		{ return m_to; }
	const EmailAddressListPtr&		GetCc() const		{ return m_cc; }
	const EmailAddressListPtr&		GetBcc() const		{ return m_bcc; }
	
	const EmailPartList&		GetPartList() const		{ return m_partList; }
	
	bool IsRead() const				{ return m_read; }
	bool IsReplied() const			{ return m_replied; }
	bool IsFlagged() const			{ return m_flagged; }
	bool HasAttachment() const		{ return m_hasAttachments; }
	bool HasEditedOriginal() const	{ return m_editedOriginal; }
	
	const std::string& GetMessageId() const				{ return m_messageId; }
	const std::string& GetInReplyTo() const				{ return m_inReplyTo; }

	const std::string& GetServerUniqueId() const		{ return m_serverUniqueId; }
	const std::string& GetServerConversationId() const	{ return m_serverConversationId; }

	Priority GetPriority() const						{ return m_priority; }

	// SendStatus getters
	bool 	IsSent() const								{ return m_sent; }
	bool 	HasFatalError() const						{ return m_hasFatalError; }
	int		GetRetryCount() const						{ return m_retryCount; }
	MailError::ErrorCode GetSendError() const			{ return m_sendError; }

protected:
	MojObject		m_id;
	MojObject		m_originalMsgId;
	MojObject		m_folderId;
	std::string		m_subject;
	std::string		m_previewText;
	DraftType		m_draftType;

	// Time email was received by server in UTC milliseconds since the epoch
	MojInt64		m_dateReceived;
	
	// Header addresses
	EmailAddressPtr		m_from;
	EmailAddressPtr		m_replyTo;
	EmailAddressListPtr	m_to;
	EmailAddressListPtr	m_cc;
	EmailAddressListPtr	m_bcc;
	
	EmailPartList		m_partList;
	
	// Flags -- make sure these are initialized in the constructor!
	bool			m_read;
	bool			m_replied;
	bool			m_flagged;
	bool			m_editedDraft;
	bool			m_hasAttachments;	// attachment icon
	bool			m_editedOriginal;

	// Used for threading emails
	std::string		m_messageId;
	std::string		m_inReplyTo;

	Priority		m_priority;

	// SendStatus
	bool			m_sent;
	bool			m_hasFatalError;
	int				m_retryCount;
	MailError::ErrorCode m_sendError;

	// Not currently used, may be useful later
	std::string		m_serverUniqueId;			// must be unique across all folders
	std::string		m_serverConversationId;
};

#endif /*EMAIL_H_*/
