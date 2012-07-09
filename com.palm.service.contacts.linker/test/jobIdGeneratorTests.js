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
/*global include, MojoTest, Assert, JobIdGenerator, console, IMPORTS, require:true */

if (typeof require === 'undefined') {
	require = IMPORTS.require;
}

var webos = require('webos');

webos.include("test/loadall.js");

function JobIdGeneratorTests() {
	
}

JobIdGeneratorTests.prototype.before = function (done) {
	done();
};

JobIdGeneratorTests.prototype.after = function (done) {
	done();
};

JobIdGeneratorTests.prototype.testGenerateIds = function (done) {
	var jobIdGenerator = new JobIdGenerator(),
		generatedJobIds = [],
		i,
		numIdsToGenerate = 17576,
		generatedJobId,
		actualJobId,
		stringToPrint = "";
		
	for (i = 0; i < numIdsToGenerate; i += 1) {
		actualJobId = jobIdGenerator.generateJobId();
		generatedJobId = actualJobId + "-dontclashwithotherproperties";
		Assert.requireFalse(generatedJobIds[generatedJobId], "JobIdGeneratorTests - there is an id that is not unique in the list of generated ids - value: " + generatedJobId + " at index: " + i);
		generatedJobIds[generatedJobId] = actualJobId;
	}
	
	Object.keys(generatedJobIds).forEach(function (key) {
		stringToPrint += generatedJobIds[key] + ",";
	});
	
	console.log(stringToPrint);
	
	done(MojoTest.passed);
};