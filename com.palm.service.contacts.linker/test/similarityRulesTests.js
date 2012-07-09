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
/*global MojoTest, include, ContactGenerator, console */

include("test/loadall.js");

function SimilarityRulesTests() {
}

SimilarityRulesTests.prototype.testGetDisplayablePersonAndContactsById = function (reportResults) {
	var contactGenerator = new ContactGenerator();
		
	console.log("N - Random --> " + JSON.stringify(contactGenerator.getNRandomContacts(2)));
	console.log("N - SameVal SameType --> " + JSON.stringify(contactGenerator.getNSameValSameTypePhoneNumbers(5)));
	console.log("N - SameType --> " + JSON.stringify(contactGenerator.getNSameTypePhoneNumbers(2)));
	console.log("N - UniqueVal --> " + JSON.stringify(contactGenerator.getNUniqueValPhoneNumbers(3)));
	reportResults(MojoTest.passed);
	//reportResults(MojoTest.failed);
};