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
/*global DB, Foundations, PalmCall, UnitTest, ReminderAssistant, rmdrLog */
/*global clearDatabase, makeRepeatingEvent, makeEvent, makeDummyReminder  */
/*global reminderSetup, okStatusSetup, failStatusSetup, IMPORTS, require:true  */
if (typeof require === 'undefined') {
   require = IMPORTS.require;
}
var webos = require('webos');
webos.include("tests/loadall.js");
webos.include("tests/test-utils.js");

function ReminderAssistantUtilTests() {
}


ReminderAssistantUtilTests.prototype.before = function (done){
	rmdrLog(" ===================== CLEARING THE TABLE =====================");
	var future = clearDatabase();
	future.then(done);
};


/*
 * Expected results of success:
 *  - Empty reminder status table
 *  - Empty reminder table
 *
 *  Prerequisites: none
 *  Variations: none
 *  Failure cases: MojoDB failure. Can't control.
 */
ReminderAssistantUtilTests.prototype.testStartOver = function(done){
	var assistant = new ReminderAssistant();
	var future = new Foundations.Control.Future();
	var retval;
	var results;

	//put some results in the reminder table and the status table
	future.now(this, reminderSetup);
	future.then(this, failStatusSetup);

	//query for the status we just put
	future.then(function(){
		future.nest(DB.find({from : "com.palm.service.calendar.remindersstatus:1"}));
	});

	//verify the status is there, and query for the reminders we just put
	future.then(function(){
		retval = future.result.returnValue;
		results = future.result.results;
		UnitTest.require(retval === true, "Failed to put a status, can't verify result");
		UnitTest.require(results.length > 0, "Failed to put a status, can't verify result");
		future.nest(DB.find({from : "com.palm.service.calendar.reminders:1"}));
	});

	//verify the reminders are there, and call startOver()
	future.then(function(){
		retval = future.result.returnValue;
		results = future.result.results;
		UnitTest.require(retval === true, "Failed to put reminders, can't verify result");
		UnitTest.require(results.length > 0, "Failed to put reminders, can't verify result");

		//TEST. This will do a nest, and set a result.
		assistant.startOver(future);
	});

	//verify that the call to startOver succeeded
	future.then(function(){
		//{returnValue: true, startedOver: true, allResults = [{...}]}
		retval = future.result.returnValue;
		var startedOver = future.result.startedOver;
		var allResults = future.result.allResults;
		UnitTest.require(retval === true, "Top level return value is false");
		UnitTest.require(startedOver === true, "Wrong value for startedOver");
		UnitTest.require(allResults !== undefined, "Wrong value for allResults");
		UnitTest.require(allResults.length === 1, "Wrong number of allResults"); //should only be batch delete

		var batchResult = allResults[0];
		var responses = allResults[0].responses;
		UnitTest.require(batchResult !== undefined, "Wrong value for batchResult");
		UnitTest.require(batchResult.returnValue === true, "Wrong value for batchResult.returnValue");
		UnitTest.require(responses !== undefined, "Wrong value for batchResult.responses");
		UnitTest.require(batchResult.responses.length === 2, "Wrong number of batchResult responses");

		//{"returnValue":true,"responses":[{"returnValue":true,"count":1},{"returnValue":true,"count":2}]}

		var response;
		var count;

		//del status response
		response = responses[0];
		retval = response.returnValue;
		count = response.count;
		UnitTest.require(retval === true, "Del operation 1 response failed");
		UnitTest.require(count === 1, "Del operation 1 deleted wrong number of items");

		//del reminders response
		response = responses[1];
		retval = response.returnValue;
		count = response.count;
		UnitTest.require(retval === true, "Del operation 2 response failed");
		UnitTest.require(count === 2, "Del operation 2 deleted wrong number of items");

		future.result = UnitTest.passed;
	});

	future.then(done);
};

/*
 * Expected results of success:
 *  - Status is ok
 *  - New rev number is recorded in status table
 *
 *  Prerequisites: assistant.lastRevNumber is set
 *  Variations: none
 *  Failure cases: MojoDB failure. Can't control.
 */
ReminderAssistantUtilTests.prototype.testPrepareForLanding = function(done){
	var assistant = new ReminderAssistant();
	var future = new Foundations.Control.Future();
	var retval;
	var results;
	var lastRevNumber = 1000;

	//put a status in the status table
	future.now(this, okStatusSetup);

	//query for the status we just put
	future.then(function(){
		future.nest(DB.find({from : "com.palm.service.calendar.remindersstatus:1"}));
	});

	//verify the status is there, and call setStatusAndRev
	future.then(function(){
		retval = future.result.returnValue;
		results = future.result.results;
		UnitTest.require(retval === true, "Failed to put a status, can't verify result");
		UnitTest.require(results.length > 0, "Failed to put a status, can't verify result");

		//TEST. This will do a nest, and set a result.
		future.lastRevNumber = lastRevNumber;
		assistant.setStatusAndRev(future);
	});

	//verify that the call to setStatusAndRev succeeded, and query for the status again
	future.then(function(){
		retval = future.result.returnValue;
		var responses = future.result.responses;
		UnitTest.require(retval === true, "Batch response top level return value is false");
		UnitTest.require(responses.length === 2, "Wrong number of responses from batch execute");

		var response;
		var count;
		//del response
		response = responses[0];
		retval = response.returnValue;
		count = response.count;
		UnitTest.require(retval === true, "Del operation response failed");
		UnitTest.require(count === 1, "Del operation deleted wrong number of items");

		//put response
		response = responses[1];
		retval = response.returnValue;
		results = response.results;
		UnitTest.require(retval === true, "Put operation response failed");
		UnitTest.require(results.length === 1, "Put operation put the wrong number of items");

		future.nest(DB.find({from : "com.palm.service.calendar.remindersstatus:1"}));
	});

	//verify that the query for status succeeded, and that it contains the right information
	future.then(function(){
		retval = future.result.returnValue;
		results = future.result.results;
		UnitTest.require(retval === true, "Query for status failed");
		UnitTest.require(results.length === 1, "Query returned wrong number of results");
		var status = results[0].status;
		var rev = results[0].lastRevNumber;
		UnitTest.require(status == "OK", "Got wrong status result: "+status);
		UnitTest.require(rev === lastRevNumber, "Got wrong rev number: "+rev);
		future.result = UnitTest.passed;
	});

	future.then(done);
};

/*
 * Expected results of success:
 *  - Status is fail
 *  - last rev number is zero
 *  - all reminders are deleted
 *
 *  Prerequisites: none
 *  Variations: none
 *  Failure cases: MojoDB failure. Can't control.
 */
ReminderAssistantUtilTests.prototype.testEmergencyLanding = function(done){
	var assistant = new ReminderAssistant();
	var future = new Foundations.Control.Future();
	var retval;
	var results;

	//put a status and some reminders in the tables
	future.now(this, okStatusSetup);
	future.then(this, reminderSetup);

	//query for the status we just put
	future.then(function(){
		future.nest(DB.find({from : "com.palm.service.calendar.remindersstatus:1"}));
	});

	//verify the status is there, and query for the reminders we just put
	future.then(function(){
		retval = future.result.returnValue;
		results = future.result.results;
		UnitTest.require(retval === true, "Failed to put a status, can't verify result");
		UnitTest.require(results.length > 0, "Failed to put a status, can't verify result");
		future.nest(DB.find({from : "com.palm.service.calendar.reminders:1"}));
	});

	//verify the reminders are there, and call emergencyLanding
	future.then(function(){
		retval = future.result.returnValue;
		results = future.result.results;
		UnitTest.require(retval === true, "Failed to put reminders, can't verify result");
		UnitTest.require(results.length > 0, "Failed to put reminders, can't verify result");

		//TEST. This will do a nest, and set a result.
		assistant.emergencyLanding(future);
	});

	//verify that the call to emergencyLanding succeeded, and query for the status again
	future.then(function(){
		retval = future.result.returnValue;
		var responses = future.result.responses;
		UnitTest.require(retval === true, "Batch response top level return value is false");
		UnitTest.require(responses.length === 2, "Wrong number of responses from batch execute");

		var response;
		var count;
		//del reminders response
		response = responses[0];
		retval = response.returnValue;
		count = response.count;
		UnitTest.require(retval === true, "Del operation 1 response failed");
		UnitTest.require(count === 2, "Del operation 1 deleted wrong number of items: "+count);

		//merge status response
		response = responses[1];
		retval = response.returnValue;
		count = response.count;
		UnitTest.require(retval === true, "Merge operation response failed");
		UnitTest.require(count === 1, "Merge operation merged wrong number of items: "+count);

		future.nest(DB.find({from : "com.palm.service.calendar.remindersstatus:1"}));
	});

	//verify that the query for status succeeded, and that it contains the right information
	future.then(function(){
		retval = future.result.returnValue;
		results = future.result.results;
		UnitTest.require(retval === true, "Query for status failed");
		UnitTest.require(results.length === 1, "Query returned wrong number of results");
		var status = results[0].status;
		var rev = results[0].lastRevNumber;
		UnitTest.require(status == "FAIL", "Got wrong status result: "+status);
		UnitTest.require(rev === 0, "Got wrong rev number: "+rev);
		future.result = UnitTest.passed;
	});

	future.then(done);
};

/*
 * Expected results of success:
 *  - assistant.lastRevNumber is set
 *
 *  Prerequisites: none
 *  Variations:
 *  - last status was failure: end result is empty status table and empty reminder table, lastRevNumber is zero
 *  - last status was Ok: lastRevNumber is a valid number
 *  - query failure, or empty table:
 *  Failure cases: MojoDB failure. Can't control.
 */
//Fail status case
ReminderAssistantUtilTests.prototype.testPrepareForTakeoff1 = function(done){
	var assistant = new ReminderAssistant();
	var future = new Foundations.Control.Future();
	var retval;
	var results;

	future.lastRevNumber = 1000;

	//put a status and some reminders in the tables
	future.now(this, failStatusSetup);
	future.then(this, reminderSetup);

	//query for the reminders we just put
	future.then(function(){
		future.nest(DB.find({from : "com.palm.service.calendar.reminders:1"}));
	});

	//verify the reminders are there, and query for the status we just put
	future.then(function(){
		retval = future.result.returnValue;
		results = future.result.results;
		UnitTest.require(retval === true, "Failed to put reminders, can't verify result");
		UnitTest.require(results.length > 0, "Failed to put reminders, can't verify result");
		future.nest(DB.find({from : "com.palm.service.calendar.remindersstatus:1"}));
	});

	//verify the status is there, and call prepare for takeoff with a fail status
	future.then(function(){
		retval = future.result.returnValue;
		results = future.result.results;
		UnitTest.require(retval === true, "Failed to put a status, can't verify result");
		UnitTest.require(results.length > 0, "Failed to put a status, can't verify result");

		//TEST. This will call into start over, obliterating the table, and set a result.
		assistant.prepareForTakeoff(future);
	});

	//verify that the call to startOver succeeded
	future.then(function(){
		UnitTest.require(future.lastRevNumber === 0, "LastRevNumber was not reset to 0");

		retval = future.result.returnValue;
		UnitTest.require(retval === true, "Top level return value is false");

		future.result = UnitTest.passed;
	});

	future.then(done);
};

//success variant
ReminderAssistantUtilTests.prototype.testPrepareForTakeoff2 = function(done){
	var assistant = new ReminderAssistant();
	var future = new Foundations.Control.Future();
	var retval;
	var results;

	//put a status and some reminders in the tables
	future.now(this, okStatusSetup);
	future.then(this, reminderSetup);

	//query for the reminders we just put
	future.then(function(){
		future.nest(DB.find({from : "com.palm.service.calendar.reminders:1"}));
	});

	//verify the reminders are there, and query for the status we just put
	future.then(function(){
		retval = future.result.returnValue;
		results = future.result.results;
		UnitTest.require(retval === true, "Failed to put reminders, can't verify result");
		UnitTest.require(results.length > 0, "Failed to put reminders, can't verify result");
		future.nest(DB.find({from : "com.palm.service.calendar.remindersstatus:1"}));
	});

	//verify the status is there, and call prepare for takeoff with a fail status
	future.then(function(){
		retval = future.result.returnValue;
		results = future.result.results;
		UnitTest.require(retval === true, "Failed to put a status, can't verify result");
		UnitTest.require(results.length > 0, "Failed to put a status, can't verify result");

		//TEST. This will set lastRevNumber, the tables should still be intact
		assistant.prepareForTakeoff(future);
	});

	//verify that lastRevNumber was set and the reminders and status are still in the table
	future.then(function(){
		retval = future.result.returnValue;
		UnitTest.require(retval === true, "return value is false");

		UnitTest.require(future.lastRevNumber === 200, "LastRevNumber was set to something else: "+JSON.stringify(future.lastRevNumber));

		future.result = UnitTest.passed;
	});

	future.then(done);
};

//empty table or misc. fail variant
ReminderAssistantUtilTests.prototype.testPrepareForTakeoff3 = function(done){
	var assistant = new ReminderAssistant();
	var future;
	var retval;
	var results;

	//before() method should have emptied the tables. Leave them empty, and query for the status
	future = DB.find({from : "com.palm.service.calendar.remindersstatus:1"});

	//verify the status is there, and call prepare for takeoff with a fail status
	future.then(function(){
		retval = future.result.returnValue;
		results = future.result.results;
		UnitTest.require(retval === true, "Failed to put a status, can't verify result");
		UnitTest.require(results.length === 0, "How did we get a status in the table?");

		//TEST. This will call into start over, obliterating the table, and set a result.
		assistant.prepareForTakeoff(future);
	});

	//verify that the call to startOver succeeded
	future.then(function(){
		UnitTest.require(future.lastRevNumber === 0, "LastRevNumber was not reset to 0");

		retval = future.result.returnValue;
		UnitTest.require(retval === true, "Top level return value is false");

		future.result = UnitTest.passed;
	});

	future.then(done);
};