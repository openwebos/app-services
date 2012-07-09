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
/*global MojoTest, include, IMPORTS, require:true, Future, Contact, DB, Autolinker, Person */

if (typeof require === 'undefined') {
	require = IMPORTS.require;
}

var webos = require('webos');

webos.include("test/loadall.js");

function SimilarIMTests() {
	
}

SimilarIMTests.prototype.after = function (done) {
	var future = new Future();
	
	future.now(this, function () {
		future.nest(DB.del({
			from: "com.palm.person:1"
		}));
	});
	
	future.then(this, function () {
		future.nest(DB.del({
			from: "com.palm.contact:1"
		}));
	});
	
	future.then(done);
};

SimilarIMTests.prototype.testSameIMSameType = function (reportResults) {
	var autoLinker = new Autolinker(),
		future = new Future(),
		contact1,
		contact2;
	
	future.now(this, function () {
		contact1 = new Contact({});
		contact2 = new Contact({});
			
		return autoLinker.performAutolink(undefined, undefined, undefined, [contact1, contact2]);
	});
	
	future.then(this, function () {
		return Person.findByContactIds([contact1.getId(), contact2.getId()]);
	});
	
	future.then(this, function () {
		var result = future.result,
			contactIds;
			
		if (result) {
			contactIds = result.getContactIds();
			
			contactIds = contactIds.map(function (contactId) {
				return contactId.getValue();
			});
			
			if (contactIds.indexOf(contact1.getId()) !== -1 && contactIds.indexOf(contact2.getId()) !== -1) {
				reportResults(MojoTest.passed);
			} else {
				reportResults(MojoTest.failed);
			}
		} else {
			reportResults(MojoTest.failed);
		}
	});
	
	return future;
};