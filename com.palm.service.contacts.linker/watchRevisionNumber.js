// @@@LICENSE
//
//      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

/*jslint white: true, onevar: true, undef: true, eqeqeq: true, plusplus: true, bitwise: true, 
 regexp: true, newcap: true, immed: true, nomen: false, maxerr: 500 */
/*global DB, console, Console, Future, PalmCall, Contact, Person, ContactLinkBackup, _, ManualLink, ManualUnlink */

// Object for wrapping the information about our last autolink contact revision that was processed

function WatchRevisionNumber(objectToExtend) {
	this._kind = WatchRevisionNumber.kind;
	if (objectToExtend) {
		this.initWithObject(objectToExtend);
	} else {
		this.init();
	}
}

WatchRevisionNumber.kind = "com.palm.linker.rev:1";

WatchRevisionNumber.prototype.init = function () {
	this.revisionNumber = 0;
	this.fromDB = false;
};

WatchRevisionNumber.prototype.initWithObject = function (theObject) {
	this.init();
	this.revisionNumber = theObject.revisionNumber;
	this._id = theObject._id;
	this._rev = theObject._rev;
	this._kind = theObject._kind;
	this.fromDB = true;
};

WatchRevisionNumber.prototype.save = function () {
	if (this.fromDB) {
		return DB.merge([this]);
	} else {
		return DB.put([this]);
	}
};