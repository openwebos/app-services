Mojomail-SMTP
==============

SMTP transport service for sending emails.

Summary
----------
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

Dependencies
============
Below are the tools and libraries required to build.
	- gcc 4.3
	- cmake 2.8.7 
	- pkg-config
	- make (any version)

	- boost 1.39 or later
	- icu4c 3.8 or later
	- libcurl 7.21 or later
	- glib 2.0 or later
	- zlib
	- openwebos/app-services/mojomail/common libraries.
	- openwebos/Db8 libraries (libmojocore, libmojoluna)
	- openwebos/libpalmsocket 2.0.0
	- openwebos/libsandbox 2.0.0
	- openwebos/cmake-modules-webos

How to Build on Linux
=====================

## Building

Once you have downloaded the source, execute the following to build it (after
changing into the directory under which it was downloaded):

    $ mkdir BUILD
    $ cd BUILD
    $ cmake ..
    $ make
    $ sudo make install

The directory under which the files are installed defaults to
<tt>/usr/local/webos</tt>.
You can install them elsewhere by supplying a value for
<tt>WEBOS_INSTALL_ROOT</tt>
when invoking <tt>cmake</tt>. For example:

    $ cmake -D WEBOS_INSTALL_ROOT:PATH=$HOME/projects/openwebos ..
    $ make
    $ make install

will install the files in subdirectories of <tt>$HOME/projects/openwebos</tt>.

Specifying <tt>WEBOS_INSTALL_ROOT</tt> also causes <tt>pkg-config</tt> to look
in that tree first before searching the standard locations. You can specify
additional directories to be searched prior to this one by setting the
the <tt>PKG_CONFIG_PATH</tt> environment variable.

# Copyright and License Information

All content, including all source code files and documentation files in this repository are: 

 Copyright (c) 2010-2013 LG Electronics, Inc.

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
