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
/*global DB, Foundations, PalmCall, UnitTest, rmdrLog */
/*global clearDatabase, makeRepeatingEvent, makeEvent, ReminderAssistant, IMPORTS, require:true   */

//----------------------------------------------------------------------------------------
//          On Wake
//----------------------------------------------------------------------------------------
/*
 * Expected results of success:
 *  - App is launched with list of reminders
 *  - Next wake time is scheduled
 *
 * Prerequisites:
 *  - alarmTime matches showTime of a reminder in the reminder table
 *
 * Variations:
 *  - showTime matches one or more reminders
 *  - showTime does not match anything in the table
 *  - incorrect args - can't test because PalmCall catches the exception. Command line testing returns:
 *
 */
if (typeof require === 'undefined') {
   require = IMPORTS.require;
}
var webos = require('webos');
webos.include("tests/loadall.js");
webos.include("tests/test-utils.js");

function WakeTests() {
}

WakeTests.timeoutInterval = 3000;

WakeTests.prototype.before = function (done){
	rmdrLog(" ===================== CLEARING THE TABLE =====================");
	var future = clearDatabase();
	future.then(done);
};


//showTime matches a reminder
// This test creates an activity that must be cancelled when the test is over.
WakeTests.prototype.testOnWake1 = function(done){
	var assistant = new ReminderAssistant();
	var wakeActivityId;
	var autoCloseActivityId;

	//Setup: We need events in the database, and corresponding reminders in the table
	var event1AlarmBefore = 15;
	var event2AlarmBefore = 5;
	var event1 = makeEvent(1, false, true, event1AlarmBefore);
	var event2 = makeEvent(1, false, true, event2AlarmBefore);

	//put the event in the database
	var future = DB.put([event1, event2]);

	//call init.  This will create a reminder.
	//It will also schedule a wake activity, and an autoclose activity, and
	//put a status in the database so that the merge at the end of saveautoclose will work.
	future.then(function(){
		future.nest(PalmCall.call("palm://com.palm.service.calendar.reminders/", "onInit", {"unittest":true}));
	});

	//query for the reminder status
	future.then(function(){
		future.nest(DB.find({from: "com.palm.service.calendar.remindersstatus:1"}));
	});

	//get the activity ids so we can compare later, then call wake
	future.then(function(){
		var result = future.result;
		wakeActivityId = result.results[0].wakeActivityId;
		autoCloseActivityId = result.results[0].autoCloseActivityId;

		//TEST. Wake up for the first event
		future.nest(PalmCall.call("palm://com.palm.service.calendar.reminders/", "onWake", {"showTime": event1.dtstart}));
	});

	//check the response from onWake, and get the updated wake activity from the database
	future.then(function(){
		var retval = future.result.returnValue;
		var status = future.result.status;
		UnitTest.require(retval === true, "Failed to create or save autoclose activity 1");
		UnitTest.require(status == "updated", "Created an activity when we shouldn't have");
		future.nest(DB.find({from: "com.palm.service.calendar.remindersstatus:1"}));
	});

	//TODO: Check the details of the updated activity

	//Get the updated reminder status from the database and check that the wake activity got scheduled
	future.then(function(){
		var result = future.result;
		UnitTest.require(future.result.returnValue === true, "Status query failed");
		UnitTest.require(future.result.results.length === 1, "Status query returned wrong number of results");
		future.result = result;
	});

	future.then(assistant, assistant.unitTestDone);

	future.then(function(){
		future.result = UnitTest.passed;
	});

	future.then(done);
};

//showTime does not match a reminder
// This test creates an activity that must be cancelled when the test is over.
WakeTests.prototype.testOnWake2 = function(done){
	var assistant = new ReminderAssistant();
	var wakeActivityId;
	var autoCloseActivityId;

	//Setup: We need events in the database, and corresponding reminders in the table
	var event1AlarmBefore = 15;
	var event2AlarmBefore = 5;
	var event1 = makeEvent(1, false, true, event1AlarmBefore);
	var event2 = makeEvent(1, false, true, event2AlarmBefore);

	//put the event in the database
	var future = DB.put([event1, event2]);

	//call init.  This will create a reminder.
	//It will also schedule a wake activity, and an autoclose activity, and
	//put a status in the database so that the merge at the end of saveautoclose will work.
	future.then(function(){
		future.nest(PalmCall.call("palm://com.palm.service.calendar.reminders/", "onInit", {"unittest":true}));
	});

	//query for the reminder status
	future.then(function(){
		future.nest(DB.find({from: "com.palm.service.calendar.remindersstatus:1"}));
	});

	//get the activity ids so we can compare later, then call wake
	future.then(function(){
		var result = future.result;
		wakeActivityId = result.results[0].wakeActivityId;
		autoCloseActivityId = result.results[0].autoCloseActivityId;

		//TEST. Wake up with a time that doesn't match any reminders
		future.nest(PalmCall.call("palm://com.palm.service.calendar.reminders/", "onWake", {"showTime": (event1.dtstart+3000)}));
	});

	//check the response from onWake, and get the updated wake activity from the database
	future.then(function(){
		var retval = future.result.returnValue;
		var status = future.result.status;
		UnitTest.require(retval === true, "Failed to create or save autoclose activity 1");
		UnitTest.require(status == "updated", "Created an activity when we shouldn't have");
		future.nest(DB.find({from: "com.palm.service.calendar.remindersstatus:1"}));
	});

	//Get the updated reminder status from the database and check that the wake activity got scheduled
	future.then(function(){
		var result = future.result;
		UnitTest.require(future.result.returnValue === true, "Status query failed");
		UnitTest.require(future.result.results.length === 1, "Status query returned wrong number of results");

		//TODO: Check the details of the updated activity

		future.result = result;
	});

	future.then(assistant, assistant.unitTestDone);

	future.then(function(){
		future.result = UnitTest.passed;
	});

	future.then(done);
};

