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
//          On Dismiss
//----------------------------------------------------------------------------------------
/*
 * Expected results of success:
 *  - Reminder's updated showTime is set to next alarm
 *  - Activity is scheduled for wake
 *  - Activity is scheduled for autoClose
 *
 * Prerequisites:
 *  - reminderId matches a reminder in the reminder table
 *  - eventId matches an event in the event table
 *  - startTime matches start time of an event in the event table
 *
 * Variations:
 *  - if the event is a repeating event, its reminder should be rescheduled for the next alarm time
 *  - if the event is not a repeating event, its reminder should not be rescheduled
 *  - incorrect args - can't test because PalmCall catches the exception. Command line testing returns:
 *    {"exception":{"error":"Missing args! Need reminderId, eventId, and startTime.  Received: {}"},"returnValue":false}
 *
 */
if (typeof require === 'undefined') {
   require = IMPORTS.require;
}
var webos = require('webos');
webos.include("tests/loadall.js");
webos.include("tests/test-utils.js");

function DismissTests() {
}

DismissTests.timeoutInterval = 3000;

DismissTests.prototype.before = function (done){
	rmdrLog(" ===================== CLEARING THE TABLE =====================");
	var future = clearDatabase();
	future.then(done);
};


//Repeating event success variant
// This test creates an activity that must be cancelled when the test is over.
DismissTests.prototype.testOnDismiss1 = function(done){
	var assistant = new ReminderAssistant();
	var eventId;
	var reminderId;
	var retval;
	var status;

	//Setup: We need a repeating event in the database, and a corresponding reminder in the table
	var event = makeRepeatingEvent();

	//what's our current occurrence and current autoclose?
	var eventStart = new Date(event.dtstart);
	var currentInstanceStart = new Date();
	var now = new Date();
	if(now.getHours() > 8){
		currentInstanceStart.addDays(1);
	}
	currentInstanceStart.set({millisecond: 0, second: 0, minute: 0, hour: eventStart.getHours()});
	var currentInstanceAutoClose = currentInstanceStart.getTime() + 3600000;
	var currentInstanceAlarm = currentInstanceStart.getTime()-300000;

	//what's our next occurrence and next autoClose?
	var nextInstanceStart = new Date();
	nextInstanceStart.addDays(1);
	if(now.getHours() > 8){
		nextInstanceStart.addDays(1);
	}
	nextInstanceStart.set({millisecond: 0, second: 0, minute: 0, hour: eventStart.getHours()});
	var nextInstanceEnd = nextInstanceStart.getTime()+3600000;
	var nextInstanceAutoClose = nextInstanceEnd;
	var nextInstanceAlarm = nextInstanceStart.getTime()-300000;

	//put the event in the database
	var future = DB.put([event]);

	//get the event Id, and call init.  This will create a reminder.
	//It will also schedule a wake activity, and an autoclose activity, and
	//put a status in the database so that the merge at the end of saveautoclose will work.
	future.then(function(){
		eventId = future.result.results[0].id;
		future.nest(PalmCall.call("palm://com.palm.service.calendar.reminders/", "onInit", {"unittest":true}));
	});

	//query for the status
	future.then(function(){
		future.nest(DB.find({"from":"com.palm.service.calendar.remindersstatus:1"}));
	});

	//get the activityIds, the query for the reminder Id of the reminder added by onInit
	var wakeActivityId;
	var autoCloseActivityId;
	future.then(function(){
		var result = future.result;
		wakeActivityId = result.results[0].wakeActivityId;
		autoCloseActivityId = result.results[0].autoCloseActivityId;

		future.nest(DB.find({"from":"com.palm.service.calendar.reminders:1"}));
	});

	//TEST. Save that reminder Id, and dismiss the reminder
	future.then(function(){
		reminderId = future.result.results[0]._id;
		future.nest(PalmCall.call("palm://com.palm.service.calendar.reminders/", "onDismiss", {"reminderId": reminderId, "eventId": eventId, "startTime": currentInstanceStart.getTime()}));
	});

	//check the response from onDismiss, and get the updated reminder from the database
	future.then(function(){
		retval = future.result.returnValue;
		status = future.result.status;
		UnitTest.require(retval === true, "Failed to create or save autoclose activity 1");
		UnitTest.require(status == "updated", "Created an activity when we shouldn't have");

		future.nest(DB.find(
			{	from: "com.palm.service.calendar.reminders:1"
			,	where:
				[{	prop: "eventId"
				,	op: "="
				,	val: eventId
				}]
			}
		));
	});

	//Verify the times scheduled for the reminder, and query for the status to check the activityId
	future.then(function(){
		UnitTest.require(future.result.results.length === 1, "onDismiss - wrong number of reminders scheduled: wanted 1");
		UnitTest.require(future.result.results[0].showTime === nextInstanceAlarm, "onDismiss - wrong showTime scheduled for reminder");
		UnitTest.require(future.result.results[0].autoCloseTime === nextInstanceAutoClose, "onDismiss - wrong autoCloseTime scheduled for reminder");
		future.nest(DB.find({from: "com.palm.service.calendar.remindersstatus:1"}));
	});

	//Verify that a autoclose and wake activities were scheduled
	future.then(function(){
		var result = future.result;
		UnitTest.require(future.result.returnValue === true, "Status query failed");
		UnitTest.require(future.result.results.length === 1, "Status query returned wrong number of results");
		UnitTest.require(future.result.results[0].wakeActivityId === wakeActivityId, "didn't get an activity id");
		UnitTest.require(future.result.results[0].autoCloseActivityId === autoCloseActivityId, "didn't get an activity id");
		future.result = result;
	});

	future.then(assistant, assistant.unitTestDone);

	future.then(function(){
		future.result = UnitTest.passed;
	});

	future.then(done);
};


//Non-repeating event success variant
DismissTests.prototype.testOnDismiss2 = function(done){
	var eventId;
	var reminderId;
	var retval;
	var status;

	//Setup: We need a non-repeating event in the database, and a corresponding reminder in the table
	var event = makeEvent(1, false, true, 5);

	//put the event in the database
	var future = DB.put([event]);

	//get the event Id, and call init.  This will create a reminder.
	//It will also schedule a wake activity, and an autoclose activity, and
	//put a status in the database so that the merge at the end of saveautoclose will work.
	future.then(function(){
		eventId = future.result.results[0].id;
		future.nest(PalmCall.call("palm://com.palm.service.calendar.reminders/", "onInit", {"unittest":true}));
	});

	//query for the status
	future.then(function(){
		future.nest(DB.find({"from":"com.palm.service.calendar.remindersstatus:1"}));
	});

	//get the activityIds, the query for the reminder Id of the reminder added by onInit
	var wakeActivityId;
	var autoCloseActivityId;
	future.then(function(){
		var result = future.result;
		wakeActivityId = result.results[0].wakeActivityId;
		autoCloseActivityId = result.results[0].autoCloseActivityId;

		future.nest(DB.find(
			{	from: "com.palm.service.calendar.reminders:1"
			,	where:
				[{	prop: "eventId"
				,	op: "="
				,	val: eventId
				}]
			}));
	});

	//Save that reminder Id, and dismiss the reminder
	future.then(function(){
		reminderId = future.result.results[0]._id;
		future.nest(PalmCall.call("palm://com.palm.service.calendar.reminders/", "onDismiss", {"reminderId": reminderId, "eventId": eventId, "startTime": event.dtstart}));
	});

	//check the response from onDismiss
	future.then(function(){
		retval = future.result.returnValue;
		status = future.result.status;
		UnitTest.require(retval === true, "Failed to create or save autoclose activity 1");
		UnitTest.require(status == "none", "Created an activity when we shouldn't have");

		future.nest(DB.find(
			{	from: "com.palm.service.calendar.reminders:1"
			,	where:
				[{	prop: "eventId"
				,	op: "="
				,	val: eventId
				}]
			}
		));
	});

	//Verify that the reminder was deleted, and nothing else is scheduled for this event, then query for the status
	future.then(function(){
		UnitTest.require(future.result.returnValue === true, "onDismiss - query failed");
		UnitTest.require(future.result.results.length === 0, "Failed to delete past reminder");
		future.nest(DB.find({from: "com.palm.service.calendar.remindersstatus:1"}));
	});

	//Verify that a autoclose and wake activities were not scheduled
	future.then(function(){
		UnitTest.require(future.result.returnValue === true, "Status query failed");
		UnitTest.require(future.result.results.length === 1, "Status query returned wrong number of results");
		UnitTest.require(future.result.results[0].wakeActivityId === 0, "Created an activity when we shouldn't have");
		UnitTest.require(future.result.results[0].autoCloseActivityId === 0, "Created an activity when we shouldn't have");
		future.result = UnitTest.passed;
	});

	future.then(done);
};
