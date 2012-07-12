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

#include "data/Email.h"

Email::Email()
: m_draftType(Email::DraftTypeNew),
  m_dateReceived(0),
  m_to(new EmailAddressList),
  m_cc(new EmailAddressList),
  m_bcc(new EmailAddressList),
  m_read(false),
  m_replied(false),
  m_flagged(false),
  m_editedDraft(false),
  m_hasAttachments(false),
  m_editedOriginal(false),
  m_priority(Priority_Normal),
  m_sent(false),
  m_hasFatalError(false),
  m_retryCount(0),
  m_sendError(MailError::NONE)
{
}

Email::~Email()
{
}
