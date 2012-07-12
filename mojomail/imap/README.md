Mojomail-IMAP
===============

IMAP sync transport for webOS.

# Directory layout

    src/                    - main classes
        activity/           - code for creating commonly used activities
        client/             - misc logic used by ImapClient and ImapSession
        commands/           - commands run by ImapClient and ImapSession
        data/               - database adapter classes
        exceptions/         - exception classes
        parser/             - IMAP tokenizer and fetch response parser code
        protocol/           - response parsers for IMAP server responses
        sync/               - code for diff'ing local and remote email and folder state
	
    test/                   - unit test code

    GrammarAnaylzer/        - used to generate Parser.{cpp,h} from src/parser/Grammar.txt

    files/
        etc/palm/db_kinds   - database kinds
        usr/palm/public     - account templates

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