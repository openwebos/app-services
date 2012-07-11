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
//          On Snooze
//----------------------------------------------------------------------------------------
/*
 * Expected results of success:
 *  - Reminder's updated showTime is set to post-snooze wake time
 *  - Activity is scheduled for wake
 *
 * Prerequisites:
 *  - reminderId matches a reminder in the reminder table
 *
 * Variations:
 *  - with a snooze duration passed in
 *  - without a snooze duration passed in
 *  - reminder ID doesn't match anything in the table
 *  - incorrect args - can't test because PalmCall catches the exception. Command line testing returns:
 *    {"exception":{"error":"Reminder does not exist: 2+60"},"returnValue":false}
 *
 */
if (typeof require === 'undefined') {
   require = IMPORTS.require;
}
var webos = require('webos');
webos.include("tests/loadall.js");
webos.include("tests/test-utils.js");

function SnoozeTests() {
}

SnoozeTests.timeoutInterval = 3000;

SnoozeTests.prototype.before = function (done){
	rmdrLog(" ===================== CLEARING THE TABLE =====================");
	var future = clearDatabase();
	future.then(done);
};


//With snooze duration variant
// This test creates an activity that must be cancelled when the test is over.
SnoozeTests.prototype.testOnSnooze1 = function(done){
	var assistant = new ReminderAssistant();
	var eventId;
	var reminderId;
	var snoozeDuration = 900000; //15 minutes
	var snoozeUntil;
	var alarmTime = 5;

	//Setup: We need an event in the database, and a corresponding reminder in the table
	var event = makeEvent(1, false, true, alarmTime);

	//what's our current occurrence?
	var eventStart = event.dtstart;
	var eventEnd = eventStart + 3600000;
	var eventAutoClose = eventEnd;
	var eventAlarm = eventStart - (alarmTime * 60000);

	//put the event in the database
	var future = DB.put([event]);

	//get the event Id, and call init.  This will create a reminder.
	//It will also schedule a wake activity, and an autoclose activity, and
	//put a status in the database so that the merge at the end of saveautoclose will work.
	future.then(function(){
		eventId = future.result.results[0].id;
		future.nest(PalmCall.call("palm://com.palm.service.calendar.reminders/", "onInit", {"unittest":true}));
	});

	future.then(function(){
		future.nest(DB.find({"from":"com.palm.service.calendar.remindersstatus:1"}));
	});

	//get the reminder Id of the reminder added by onInit
	var wakeActivityId;
	var autoCloseActivityId;
	future.then(function(){
		var result = future.result;
		wakeActivityId = result.results[0].wakeActivityId;
		autoCloseActivityId = result.results[0].autoCloseActivityId;

		future.nest(DB.find({"from":"com.palm.service.calendar.reminders:1"}));
	});

	//TEST. Save that reminder Id, and snooze the reminder
	future.then(function(){
		reminderId = future.result.results[0]._id;
		snoozeUntil = new Date().getTime() + snoozeDuration;
		future.nest(PalmCall.call("palm://com.palm.service.calendar.reminders/", "onSnooze", {"reminderId": reminderId, "snoozeDuration": snoozeDuration}));
	});

	//check the response from onSnooze, and get the updated reminder from the database
	future.then(function(){
		var retval = future.result.returnValue;
		var status = future.result.status;
		UnitTest.require(retval === true, "Failed to create or save autoclose activity 1");
		UnitTest.require(status == "updated", "Created an activity when we shouldn't have");

		future.nest(DB.find(
			{	from: "com.palm.service.calendar.reminders:1"
			,	where:
				[{	prop: "_id"
				,	op: "="
				,	val: reminderId
				}]
			}
		));
	});

	//Verify the new alarmTime scheduled for the reminder and the original autoCloseTime, and query for the status to check the activityId
	future.then(function(){
		//Trim off the milliseconds
		//Since snooze is based off of 'now + duration', and we don't know what that 'now' value is, we can't get an exact comparison here.
		//So we'll ballpark it within a second on either side.
		var snoozeRangeStart = snoozeUntil -1000;
		var snoozeRangeEnd = snoozeUntil + 1000;
		var snoozeTime = future.result.results[0].showTime;

		UnitTest.require((snoozeTime >= snoozeRangeStart && snoozeTime <= snoozeRangeEnd), "onSnooze - wrong showTime scheduled for reminder: "+ JSON.stringify(future.result)+" "+new Date(future.result.results[0].showTime));
		UnitTest.require(future.result.results[0].autoCloseTime === eventAutoClose, "onSnooze - wrong autoCloseTime scheduled for reminder: "+ JSON.stringify(future.result)+" "+new Date(future.result.results[0].autoCloseTime));
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


//With no snooze duration variant
// This test creates an activity that must be cancelled when the test is over.
SnoozeTests.prototype.testOnSnooze2 = function(done){
	var assistant = new ReminderAssistant();
	var eventId;
	var reminderId;
	var snoozeDuration = 300000; //5 minutes is the default
	var snoozeUntil;
	var alarmTime = 5;

	//Setup: We need an event in the database, and a corresponding reminder in the table
	//Get an event
	var event = makeEvent(1, false, true, alarmTime);

	//what's our current occurrence?
	var eventStart = event.dtstart;
	var eventEnd = eventStart + 3600000;
	var eventAutoClose = eventEnd;
	var eventAlarm = eventStart - (alarmTime * 60000);

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

	//TEST. Save that reminder Id, and snooze the reminder.  No snooze duration!
	future.then(function(){
		reminderId = future.result.results[0]._id;
		snoozeUntil = new Date().getTime() + snoozeDuration;

		future.nest(PalmCall.call("palm://com.palm.service.calendar.reminders/", "onSnooze", {"reminderId": reminderId}));
	});

	//check the response from onSnooze, and get the updated reminder from the database
	future.then(function(){
		var retval = future.result.returnValue;
		var status = future.result.status;
		UnitTest.require(retval === true, "Failed to create or save autoclose activity 1");
		UnitTest.require(status == "updated", "Created an activity when we shouldn't have");

		future.nest(DB.find(
			{	from: "com.palm.service.calendar.reminders:1"
			,	where:
				[{	prop: "_id"
				,	op: "="
				,	val: reminderId
				}]
			}
		));
	});

	//Verify the new showTime scheduled for the reminder and the original autoCloseTime, and query for the status to check the activityId
	future.then(function(){
		//Trim off the milliseconds
		//Since snooze is based off of 'now + duration', and we don't know what that 'now' value is, we can't get an exact comparison here.
		//So we'll ballpark it within a second on either side.
		var snoozeRangeStart = snoozeUntil -1000;
		var snoozeRangeEnd = snoozeUntil + 1000;
		var snoozeTime = future.result.results[0].showTime;

		UnitTest.require((snoozeTime >= snoozeRangeStart && snoozeTime <= snoozeRangeEnd), "onSnooze - wrong showTime scheduled for reminder: "+ JSON.stringify(future.result)+" "+new Date(future.result.results[0].showTime));
		UnitTest.require(future.result.results[0].autoCloseTime === eventAutoClose, "onSnooze - wrong autoCloseTime scheduled for reminder: "+ JSON.stringify(future.result)+" "+new Date(future.result.results[0].autoCloseTime));
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

//No matching reminder
//SnoozeTests.prototype.testOnSnooze3 = function(done){
//	var assistant = new ReminderAssistant();
//
//	//TEST. Call snooze on an empty database - nothing should match
//	var future = PalmCall.call("palm://com.palm.service.calendar.reminders/", "onSnooze", {"reminderId": "foo"});
//
//	//check the response from onSnooze, and get the updated reminder from the database
//	future.then(function(){
//		var exception = future.result.exception;
//		UnitTest.require(exception != undefined, "Should have gotten an exception and we didn't");
//
//		future.nest(DB.find({from: "com.palm.service.calendar.remindersstatus:1"}));
//	});
//
//	//Verify that a autoclose and wake activities were scheduled
//	future.then(function(){
//		var result = future.result;
//		UnitTest.require(future.result.returnValue === true, "Status query failed");
//		UnitTest.require(future.result.results.length === 1, "Status query returned wrong number of results");
//		UnitTest.require(future.result.results[0].wakeActivityId === 0, "didn't get an activity id");
//		UnitTest.require(future.result.results[0].autoCloseActivityId === 0, "didn't get an activity id");
//		future.result = UnitTest.passed;
//	});
//
//	future.then(done);
//};

