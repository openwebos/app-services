Mojomail-SMTP
==============

SMTP transport service for sending emails.

# Basic operation

When an IMAP/POP account is created, mojomail-smtp creates a database watch
on email records belonging to the outbox folder. New email records created
in the outbox will be picked up by the SMTP transport and sent to the SMTP
server specified in the account.

The bus method palm://com.palm.smtp/sendMail is used to queue emails in
the outbox from the email app. A related bus method
palm://com.palm.smtp/saveDraft is used to create or update draft emails
in the drafts folder. Uploading drafts is handled by incoming mail
transport, which should have a watch on the drafts folder for each account.

After an email is successfully sent, the email record is updated to set
sendStatus.sent: true and flags.visible: false. The incoming mail transport
service is then responsible for copying the email to the sent folder
and deleting the sent email from the outbox.

NOTE: Unlike most transports, SMTP does not have its own account capability
but rather is part of the mail capability. The incoming transport service
(IMAP/POP) is responsible for managing the lifecycle of the account by
calling SMTP's accountCreated/accountEnabled/etc bus methods.

# How to Build on Linux

## Dependencies

### Tools
* gcc 4.3
* make (any version)
* pkg-config

### Libraries
* boost 1.39 or later
* icu4c 3.8 or later
* libcurl 7.21 or later
* glib 2.0 or later
* zlib
* mojodb libraries (libmojocore, libmojoluna)
* PmLogLib
* libpalmsocket
* libsandbox
* openwebos/app-services/mojomail/common libraries.

## How to build

This release is provided for informational purpose only. No build support is provided at this time.


# Copyright and License Information

All content, including all source code files and documentation files in this repository are: 

 Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.

All content, including all source code files and documentation files in this repository are:
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this content except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.