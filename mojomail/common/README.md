Mojomail-Common
===============
Summary
--------   
Common Email Service Library used by other Email Services.

Description
-----------
This is the email common library which provides support code for the native
email transports for webOS.

This includes data models, a MIME email parser/writer, input/output stream
classes, HTTP/socket wrappers, and activity management classes.

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

To see all of the make targets that CMake has generated, issue:

    $ make help

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
