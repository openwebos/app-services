
Mojomail-Common
===============
This is the email common library which provides support code for the native
email transports for webOS.

This includes data models, a MIME email parser/writer, input/output stream
classes, HTTP/socket wrappers, and activity management classes.

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

## How to build

This release is provided for informational purpose only. No build support is provided at this time.

## Special notes

All packages that depend on email common (mojomail-*) must be rebuilt if any
header files in common are modified, otherwise otherwise they may not run
properly due to binary struct layout changes. The rebuilt packages should be
installed on the device together at the same time. This is especially important
if installing on a different version of webOS.

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
