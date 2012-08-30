Quick Info
-----------
Service for various contacts-related methods. Service consists of a set of JavaScript files, as well as JSON configuration files

Set of js files / -  Source Code 
 
JSON files / -
-------------
sources.json -  This file contains the list of all Javascript files and librabries that are loaded into the current service. 

services.json - This file defines what services are provided on the Palm bus by the current Mojo Service. Typically, Mojo Service provides a single service with multiple method calls.


activities/ - 
------------
com.palm.service.contacts.sortorder.json/- Configuration file used to register a watch. This is the watch for updating the sortKey on every person when the sort order is changed in the contacts app.


backup/ - 
-------
com.palm.service.contacts / - This file is used to register your service for backup.Once a service is registered, it will get callbacks from the Backup Service to perform the backup operation.

db/ - 
-----
Every object in the database has a Kind. DB folder has kinds and permission files to define database kinds and the permissions that determine what a caller can do with objects of a Kind.

# Copyright and License Information

All content, including all source code files and documentation files in this repository except otherwise noted are: 

 Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.

All content, including all source code files and documentation files in this repository except otherwise noted are:
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this content except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
