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
/*global Utils, DB, console, Console, Future, _ */

function JobIdGenerator() {
	this.charsToUse = "abcdefghijklmnopqrstuvwxyz";
	this.firstIndex = 0;
	this.secondIndex = 0;
	this.thirdIndex = 0;
}

// This method with generate up to 17576 unique job ids.
JobIdGenerator.prototype.generateJobId = function () {
	var toReturn;
	
	if (this.firstIndex >= this.charsToUse.length) {
		this.firstIndex = 0;
		this.secondIndex += 1;
	}
	
	if (this.secondIndex >= this.charsToUse.length) {
		this.secondIndex = 0;
		this.thirdIndex += 1;
	}
	
	// If we have reached this point then our job ids will start having collisions.
	// The chances of having thirdIndex above 26 within a single run of the linker
	// is highly unlikely.
	if (this.thirdIndex >= this.charsToUse.length) {
		this.firstIndex = 0;
		this.secondIndex = 0;
		this.thirdIndex = 0;
		console.error("JobIdGenerator just rolled over the limit of the number of job ids it can generate!!");
	}
	
	toReturn = this.charsToUse[this.thirdIndex] + this.charsToUse[this.secondIndex] + this.charsToUse[this.firstIndex];
	
	this.firstIndex += 1;
	
	return toReturn;
};