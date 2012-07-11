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
/*global DB, PalmCall, UnitTest, rmdrLog */
/*global clearDatabase, makeRepeatingEvent, makeEvent, ReminderAssistant, IMPORTS, require:true  */

//----------------------------------------------------------------------------------------
//          On AutoClose
//----------------------------------------------------------------------------------------
/*
 * Expected results of success:
 *  - Reminder's showTime is set to next alarm
 *  - Reminder's updated autoCloseTime is set to next autoCloseTime
 *  - Activity is scheduled for wake
 *  - Activity is scheduled for autoClose
 *
 * Prerequisites:
 *  - autoCloseTime matches autoCloseTime of a reminder in the reminder table
 *
 * Variations:
 *  - if the event is a repeating event, its reminder should be rescheduled
 *  - if the event is not a repeating event, its reminder should not be rescheduled
 *  - incorrect args - can't test because PalmCall catches the exception. Command line testing returns:
 *
 */
if (typeof require === 'undefined') {
   require = IMPORTS.require;
}
var webos = require('webos');
webos.include("tests/loadall.js");
webos.include("tests/test-utils.js");

function AutoCloseTests() {
}

AutoCloseTests.timeoutInterval = 3000;

AutoCloseTests.prototype.before = function (done){
	rmdrLog(" ===================== CLEARING THE TABLE =====================");
	var future = clearDatabase();

//	future.then(function(){
//		future.nest(PalmCall.call("luna://com.palm.activitymanager", "list", {}));
//	});
//
//	future.then(function(){
//		var result = future.result;
//		var activities = result.activities;
//		for(var i = 0; i < activities.length; i++){
//			var activity = activities[i];
//			if(activity.creator.serviceId == "com.palm.service.calendar.reminders"){
//				PalmCall.call("luna://com.palm.activitymanager", "cancel", {"activityId": activity.activityId});
//			}
//		}
//		future.result = result;
//	});

	future.then(done);
};


//Repeating event - should get rescheduled
// This test creates an activity that must be cancelled when the test is over.
AutoCloseTests.prototype.testOnAutoClose1 = function(done){
	var assistant = new ReminderAssistant();
	var eventId;
	var reminderId;

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

	//TEST. Save that reminder Id, and autoClose the reminder
	future.then(function(){
		reminderId = future.result.results[0]._id;
		future.nest(PalmCall.call("palm://com.palm.service.calendar.reminders/", "onAutoClose", {"autoCloseTime": currentInstanceAutoClose}));
	});

	//check the response from onAutoClose, and get the updated reminder from the database
	future.then(function(){
		var retval = future.result.returnValue;
		var status = future.result.status;
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

	//Verify the times scheduled for the reminder, and query for the status to check the activityIds
	//NOTE: By definition an autoclosed reminder's start time should have been in the past. But
	//we used onInit to schedule it, so it was already the next occurrence. Therefore the times should
	//not have changed.  The reminderId should be different though.
	future.then(function(){
		UnitTest.require(future.result.results.length === 1, "onAutoClose - wrong number of reminders scheduled: wanted 1");
		UnitTest.require(future.result.results[0]._id !== reminderId, "onAutoClose - didn't reschedule the reminder");
		UnitTest.require(future.result.results[0].showTime === currentInstanceAlarm, "onAutoClose - wrong showTime scheduled for reminder");
		UnitTest.require(future.result.results[0].autoCloseTime === currentInstanceAutoClose, "onAutoClose - wrong autoCloseTime scheduled for reminder");
		future.nest(DB.find({from: "com.palm.service.calendar.remindersstatus:1"}));
	});

	//Verify that wake and autoclose activities got scheduled
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


//Single-instance event - should not get rescheduled
AutoCloseTests.prototype.testOnAutoClose2 = function(done){
	var eventId;
	var reminderId;

	//Setup: We need a non-repeating event with an alarm in the database, and a corresponding reminder in the table
	//By definition, this event should be over with, so schedule it for yesterday.
	//Get an event
	var event = makeEvent(-1, false, true, 5);

	//put the event in the database
	var future = DB.put([event]);

	//get the event Id, and call init.  This will create a reminder.
	//It will also schedule a wake activity, and an autoclose activity, and
	//put a status in the database so that the merge at the end of saveautoclose will work.
	future.then(function(){
		eventId = future.result.results[0].id;
		future.nest(PalmCall.call("palm://com.palm.service.calendar.reminders/", "onInit", {"unittest":true}));
	});

	//add a reminder for our event - onInit will not have scheduled it, because it's in the past
	//But if we make it in the future, onAutoClose would reschedule it, so we'll have to fudge a little here.
	future.then(function() {
		future.nest(DB.put(
			[
				{
					"_kind"			: "com.palm.service.calendar.reminders:1",
					"eventId"		: eventId,
					"subject"		: event.subject,
					"location"		: event.location,
					"isAllDay"		: event.allDay,
					"startTime"		: event.dtstart,
					"endTime"		: event.dtend,
					"alarmTime"		: event.dtstart-300000,
					"showTime"		: event.dtstart-300000,
					"autoCloseTime"	: event.dtend,
					"isRepeating"	: false
				}
			]
		));
	});

	//Save that reminder Id, and autoClose the reminder
	future.then(function(){
		reminderId = future.result.results[0].id;
		future.nest(PalmCall.call("palm://com.palm.service.calendar.reminders/", "onAutoClose", {"autoCloseTime": event.dtend}));
	});

	//check the response from onAutoClose
	future.then(function(){
		var retval = future.result.returnValue;
		var status = future.result.status;
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
		UnitTest.require(future.result.returnValue === true, "onAutoClose - query failed");
		UnitTest.require(future.result.results.length === 0, "Failed to delete past reminder");
		future.nest(DB.find({from: "com.palm.service.calendar.remindersstatus:1"}));
	});

	//Verify that a autoclose and wake activities were not scheduled
	future.then(function(){
		UnitTest.require(future.result.returnValue === true, "Status query failed");
		UnitTest.require(future.result.results.length === 1, "Status query failed");
		UnitTest.require(future.result.results[0].wakeActivityId === 0, "Created an activity when we shouldn't have");
		UnitTest.require(future.result.results[0].autoCloseActivityId === 0, "Created an activity when we shouldn't have");
		future.result = UnitTest.passed;
	});

	future.then(done);
};

