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

/*jslint laxbreak: true, white: false */
/*global MojoLoader, IMPORTS, require:true  */
var exports = {},
	loadall;

if (typeof require === 'undefined') {
   require = IMPORTS.require;
}

if (!loadall) {
	loadall = true;
	var fs = require('fs');
	var manifest = JSON.parse(fs.readFileSync("/usr/palm/services/com.palm.service.calendar.reminders/sources.json")),
		i,
		entry,
		lib;

	// import libraries first
	for (i = 0; i < manifest.length; i += 1) {
		entry = manifest[i];
		if (entry.library) {
			lib = MojoLoader.require(entry.library);
			IMPORTS[entry.library.name] = lib[entry.library.name];
		}
	}

	// import source files
	var webos = require('webos');
	for (i = 0; i < manifest.length; i += 1) {
		entry = manifest[i];
		if (entry.source) {
			webos.include(entry.source);
		}
	}
}
