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
/*global include, MojoTest, Assert, YieldController, IMPORTS, require:true */

if (typeof require === 'undefined') {
	require = IMPORTS.require;
}

var webos = require('webos');

webos.include("test/loadall.js");

function YieldControllerTests() {
	
}

YieldControllerTests.prototype.before = function (done) {
	done();
};

YieldControllerTests.prototype.after = function (done) {
	done();
};

YieldControllerTests.prototype.testYieldController = function (done) {
	var yieldController = new YieldController();
	
	yieldController.addJobToYield("TestJob");
	Assert.require(yieldController.shouldJobYield("TestJob"), "YieldControllerTests - Job to yield was not marked to yield");
	
	yieldController.removeJob("TestJob");
	Assert.requireFalse(yieldController.shouldJobYield("TestJob"), "YieldControllerTests - Removed job was told to yield");
	
	yieldController.addJobToYield("TestJob2");
	yieldController.addJobToYield("TestJob3");
	Assert.require(yieldController.shouldJobYield("TestJob2"), "YieldControllerTests - Job to yield was not told to yield");
	Assert.require(yieldController.shouldJobYield("TestJob3"), "YieldControllerTests - Job to yield was not told to yield");
	
	yieldController.removeJob("TestJob2");
	Assert.requireFalse(yieldController.shouldJobYield("TestJob2"), "YieldControllerTests - Removed job was told to yield");
	Assert.require(yieldController.shouldJobYield("TestJob3"), "YieldControllerTests - Job to yield was not told to yield");
	
	
	yieldController.removeJob("TestJob3");
	Assert.requireFalse(yieldController.shouldJobYield("TestJob3"), "YieldControllerTests - Removed job was told to yield");
	
	done(MojoTest.passed);
};