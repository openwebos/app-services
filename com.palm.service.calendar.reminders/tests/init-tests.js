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
/*global clearDatabase, makeRepeatingEvent, makeEvent, ReminderAssistant, utilGetUTCDateString, IMPORTS, require:true   */

//----------------------------------------------------------------------------------------
//          On Init
//----------------------------------------------------------------------------------------
/*
 * Expected results of success:
 *  - Required: Reminder status table has a status containing lastRevNumber, wakeActivity, and autoCloseActivity
 *  - Maybe: Wake activity is scheduled
 *  - Maybe: AutoClose activity is scheduled
 *
 * Prerequisites: none
 *
 * Variations:
 *  - if there are events with alarms in the future, activities should be scheduled
 *  - if there are no events with alarms in the future, no activities should be scheduled
 *  - if there are no events in the table at all
 */
if (typeof require === 'undefined') {
   require = IMPORTS.require;
}
var webos = require('webos');
webos.include("tests/loadall.js");
webos.include("tests/test-utils.js");

function InitTests() {
}

InitTests.timeoutInterval = 3000;

InitTests.prototype.before = function (done){
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

//Reminders to be scheduled variant
// This test creates an activity that must be cancelled when the test is over.
InitTests.prototype.testOnInit1 = function(done){
	var assistant = new ReminderAssistant();
	var event1Alarm = 5;
	var event2Alarm = 5;
	var event3Alarm = 30;
	var event4Alarm = 0;


	var event1Duration = 60;
	var event2Duration = 60;
	var event3Duration = 5;
	var event4Duration = 60;

	//Setup: We need some events in the database
	//All created events start at 8am and last 1 hour unless specified otherwise
	var event1 = makeRepeatingEvent(); //alarm 5 minutes before, repeats daily, started 2 days ago
	var event2 = makeEvent(1, false, true, event2Alarm, event2Duration);  //alarm 5 minutes before
	var event3 = makeEvent(1, false, true, event3Alarm, event3Duration);  //alarm 30 minutes before, 5 minutes long (autoclose minimum is 15 minutes)
	var event4 = makeEvent(1, false, false, event4Alarm, event4Duration);  //no alarm


	//put the events in the database
	var future = DB.put([event1, event2, event3, event4]);

	var eventIds = [];
	var expectedValues;
	var expectedLastRevNumber;
	var expectedWakeTime;
	var expectedAutoCloseTime;

	//get the event Ids, figure out our expected values
	future.then(function(){
		expectedLastRevNumber = future.result.results[3].rev; //Not the last rev number, but second to last, because the
		//last event doesn't have an alarm, and therefore won't be present in our query

		for(var i = 0; i < 4; i++){
			eventIds.push(future.result.results[i].id);
		}

		//what are the expected reminder values?
		var now = new Date();
		if (now.getHours() > 8) {
			now.addDays(1);
		}
		now.set({millisecond: 0, second: 0, minute: 0, hour: 8});
		var event1Start = now.getTime();
		var event1ExpectedValues = {
			id:			eventIds[0],
			start:		event1Start,
			end:		event1Start + (event1Duration * 60000),
			alarm:		event1Start - (event1Alarm * 60000),
			autoClose:	event1Start + (event1Duration * 60000)
		};

		var event2Start = event2.dtstart;
		var event2ExpectedValues = {
			id:			eventIds[1],
			start:		event2Start,
			end:		event2Start + (event2Duration * 60000),
			alarm:		event2Start - (event2Alarm * 60000),
			autoClose:	event2Start + (event2Duration * 60000)
		};

		var event3Start = event3.dtstart;
		var event3ExpectedValues = {
			id:			eventIds[2],
			start:		event3Start,
			end:		event3Start + (event3Duration * 60000),
			alarm:		event3Start - (event3Alarm * 60000),
			autoClose:	event3Start + (15 * 60000) //since the event is only 5 minutes, we're expecting autoclose minimum of 15 mins
		};
		expectedWakeTime = event3Start - (event3Alarm * 60000);
		expectedAutoCloseTime = event3Start + (15 * 60000);
		expectedValues = [event1ExpectedValues, event2ExpectedValues, event3ExpectedValues];

		//TEST. Start the service for the first time....
		future.nest(PalmCall.call("palm://com.palm.service.calendar.reminders/", "onInit", {"unittest":true}));
	});

	//no previous activity exists, activity created - {returnValue: true, status: new}
	//previous activity exists and gets updated  - {returnValue: true, status: updated}
	//previous activity exists, reminder query returns no results - {returnValue: true, status: none}
	//no previous activity exists, reminder query returns no results - {returnValue: true, status: none}
	//reminder query failed - no activity - {returnValue: false}

	var dbWatchActivityId;
	//check the response from onInit, and get the reminders from the database
	future.then(function(){
		var retval = future.result.returnValue;
		var wakeStatus = future.result.wakeStatus;
		var autoCloseStatus = future.result.autoCloseStatus;
		var dbChangedStatus = future.result.dbChangedStatus;
		UnitTest.require(retval === true, "Failed to create or save the db changed activity 1");
		UnitTest.require(wakeStatus == "new", "Failed to create a wake activity");
		UnitTest.require(autoCloseStatus == "new", "Failed to create an autoclose activity");
		UnitTest.require(dbChangedStatus == "new", "Failed to create a db-changed activity");

		future.nest(DB.find({from: "com.palm.service.calendar.reminders:1"}));
	});

	//Verify the times scheduled for the reminder, and query for the status to check the activityIds
	future.then(function(){
		var results = future.result.results;
		UnitTest.require(results.length === 3, "Created the wrong number of reminders: wanted 3");
		for(var i = 0; i < 3; i++){
			var reminder = results[i];
			UnitTest.require(reminder.eventId === expectedValues[i].id, "Created reminder["+i+"] with wrong event id");
			UnitTest.require(reminder.startTime === expectedValues[i].start, "Created reminder["+i+"] with wrong start time");
			UnitTest.require(reminder.endTime === expectedValues[i].end, "Created reminder["+i+"] with wrong end time");
			UnitTest.require(reminder.showTime === expectedValues[i].alarm, "Created reminder["+i+"] with wrong alarm time");
			UnitTest.require(reminder.autoCloseTime === expectedValues[i].autoClose, "Created reminder["+i+"] with wrong autoclose time");
		}
		future.nest(DB.find({from: "com.palm.service.calendar.remindersstatus:1"}));
	});

	var wakeActivityId;
	var autoCloseActivityId;
	var dbChangedActivityId;
	//Verify the status, and query activity manager for details of the wake activity
	future.then(function(){
		UnitTest.require(future.result.returnValue === true, "Status query failed");
		UnitTest.require(future.result.results.length === 1, "Status query returned wrong number of results");
		var result = future.result.results[0];
		UnitTest.require(result.status == "OK", "didn't get a status");
		UnitTest.require(result.lastRevNumber === expectedLastRevNumber, "didn't get a last rev number: "+result.lastRevNumber + " / "+ expectedLastRevNumber);
		UnitTest.require(result.wakeActivityId !== 0, "didn't get a wake activity id");
		UnitTest.require(result.autoCloseActivityId !== 0, "didn't get an autoclose activity id");
		UnitTest.require(result.dbChangedActivityId !== 0, "didn't get an db-changed activity id");
		wakeActivityId = result.wakeActivityId;
		autoCloseActivityId = result.autoCloseActivityId;
		dbChangedActivityId = result.dbChangedActivityId;

		future.nest(PalmCall.call("palm://com.palm.activitymanager/", "getDetails", {"activityId": wakeActivityId}));
	});

	//Verify details of the wake activity, and query activity manager for details of the autoclose activity
	future.then(function(){
		var activity = future.result.activity;
		UnitTest.require(activity !== undefined, "Activity doesn't exist!");

		var start = activity.schedule.start;
		var expStart = utilGetUTCDateString(expectedWakeTime);
		UnitTest.require(start == expStart, "Activity has the wrong schedule string: "+expStart +" / "+start);

		UnitTest.require(activity.callback.method == "palm://com.palm.service.calendar.reminders/onWake", "Activity has the wrong callback");
		UnitTest.require(activity.callback.params.showTime === expectedWakeTime, "Activity has the wrong alarm time parameter");

		future.nest(PalmCall.call("palm://com.palm.activitymanager/", "getDetails", {"activityId": autoCloseActivityId}));
	});

	//verify details of the autoclose activity, and query activity manager for details of the db-changed activity
	future.then(function(){
		var result = future.result;
		var activity = future.result.activity;
		UnitTest.require(activity !== undefined, "Activity doesn't exist!");

		var start = activity.schedule.start;
		var expStart = utilGetUTCDateString(expectedAutoCloseTime);
		UnitTest.require(start == expStart, "Activity has the wrong schedule string");

		UnitTest.require(activity.callback.method == "palm://com.palm.service.calendar.reminders/onAutoClose", "Activity has the wrong callback");
		UnitTest.require(activity.callback.params.autoCloseTime === expectedAutoCloseTime, "Activity has the wrong alarm time parameter");

		future.nest(PalmCall.call("palm://com.palm.activitymanager/", "getDetails", {"activityId": dbChangedActivityId}));
	});

	//verify details of the db-changed activity
	future.then(function(){
		var result = future.result;
		var activity = future.result.activity;
		UnitTest.require(activity !== undefined, "Activity doesn't exist!");
		var revNumber = activity.trigger.params.query.where[0].val;
		UnitTest.require(revNumber == expectedLastRevNumber, "Activity has the wrong last rev number");

		UnitTest.require(activity.callback.method == "palm://com.palm.service.calendar.reminders/onDBChanged", "Activity has the wrong callback");
		future.result = result;
	});

	future.then(assistant, assistant.unitTestDone);

	future.then(function(){
		future.result = UnitTest.passed;
	});

	future.then(done);
};


//No reminders to be scheduled variant
InitTests.prototype.testOnInit2 = function(done){

	var event1Alarm = 5;
	var event2Alarm = 5;
	var event3Alarm = 30;
	var event4Alarm = 0;


	var event1Duration = 60;
	var event2Duration = 60;
	var event3Duration = 5;
	var event4Duration = 60;

	//Setup: We need some events in the database
	//All created events start at 8am and last 1 hour unless specified otherwise.  All are in the past and should be finished
	var event1 = makeEvent(-1, false, true, event1Alarm, event1Duration);  //alarm 5 minutes before
	var event2 = makeEvent(-1, false, true, event2Alarm, event2Duration);  //alarm 5 minutes before
	var event3 = makeEvent(-2, false, true, event3Alarm, event3Duration);  //alarm 30 minutes before, 5 minutes long (autoclose minimum is 15 minutes)
	var event4 = makeEvent(-3, false, false, event4Alarm, event4Duration);  //no alarm


	//put the events in the database
	var future = DB.put([event1, event2, event3, event4]);

	var expectedLastRevNumber;

	//get the last rev number
	future.then(function(){
		expectedLastRevNumber = future.result.results[3].rev; //Not the last rev number, but second to last

		//TEST. Start the service for the first time....
		future.nest(PalmCall.call("palm://com.palm.service.calendar.reminders/", "onInit", {"unittest":true}));
	});

	//no previous activity exists, activity created - {returnValue: true, status: new}
	//previous activity exists and gets updated  - {returnValue: true, status: updated}
	//previous activity exists, reminder query returns no results - {returnValue: true, status: none}
	//no previous activity exists, reminder query returns no results - {returnValue: true, status: none}
	//reminder query failed - no activity - {returnValue: false}

	var dbWatchActivityId;
	//check the response from onInit, and get the reminders from the database
	future.then(function(){
		var retval = future.result.returnValue;
		var wakeStatus = future.result.wakeStatus;
		var autoCloseStatus = future.result.autoCloseStatus;
		dbWatchActivityId = future.result.dbWatchActivityId;
		UnitTest.require(retval === true, "Failed to create or save autoclose activity 1");
		UnitTest.require(wakeStatus == "none", "Created an activity when we shouldn't have");
		UnitTest.require(autoCloseStatus == "none", "Created an activity when we shouldn't have");
		UnitTest.require(dbWatchActivityId !== 0, "Failed to create an activity");

		future.nest(DB.find({from: "com.palm.service.calendar.reminders:1"}));
	});

	//Verify that no reminders were scheduled, and query for the status to check the activityIds
	future.then(function(){
		var results = future.result.results;
		UnitTest.require(results.length === 0, "Created the wrong number of reminders: wanted 0");
		future.nest(DB.find({from: "com.palm.service.calendar.remindersstatus:1"}));
	});

	//Verify the status
	future.then(function(){
		UnitTest.require(future.result.returnValue === true, "Status query failed");
		UnitTest.require(future.result.results.length === 1, "Status query returned wrong number of results");
		var result = future.result.results[0];
		UnitTest.require(result.status == "OK", "didn't get a status");
		UnitTest.require(result.lastRevNumber === expectedLastRevNumber, "didn't get a last rev number: "+result.lastRevNumber + " / "+ expectedLastRevNumber);
		UnitTest.require(result.wakeActivityId === 0, "got a wake activity when we shouldn't have");
		UnitTest.require(result.autoCloseActivityId === 0, "got an autoclose activity when we shouldn't have");
		future.result = UnitTest.passed;
	});

	//The DB Changed activity will fire, and should complete itself, so we don't have to clean it up.

	future.then(done);
};

////No reminders to be scheduled variant - completely empty table
InitTests.prototype.testOnInit3 = function(done){

	//TEST. Start the service for the first time....
	var future = PalmCall.call("palm://com.palm.service.calendar.reminders/", "onInit", {"unittest":true});

	var dbWatchActivityId;
	//check the response from onInit, and get the reminders from the database
	future.then(function(){
		var retval = future.result.returnValue;
		var wakeStatus = future.result.wakeStatus;
		var autoCloseStatus = future.result.autoCloseStatus;
		dbWatchActivityId = future.result.dbWatchActivityId;
		UnitTest.require(retval === true, "Failed to create or save autoclose activity 1");
		UnitTest.require(wakeStatus == "none", "Created an activity when we shouldn't have");
		UnitTest.require(autoCloseStatus == "none", "Created an activity when we shouldn't have");
		UnitTest.require(dbWatchActivityId !== 0, "Failed to create an activity");

		future.nest(DB.find({from: "com.palm.service.calendar.reminders:1"}));
	});

	//Verify that no reminders were scheduled, and query for the status to check the activityIds
	future.then(function(){
		var results = future.result.results;
		UnitTest.require(results.length === 0, "Created the wrong number of reminders: wanted 0");
		future.nest(DB.find({from: "com.palm.service.calendar.remindersstatus:1"}));
	});


	//Verify the status
	future.then(function(){
		UnitTest.require(future.result.returnValue === true, "Status query failed");
		UnitTest.require(future.result.results.length === 1, "Status query returned wrong number of results");
		var result = future.result.results[0];
		UnitTest.require(result.status == "OK", "didn't get a status");
		UnitTest.require(result.wakeActivityId === 0, "got a wake activity when we shouldn't have");
		UnitTest.require(result.autoCloseActivityId === 0, "got an autoclose activity when we shouldn't have");
		future.result = UnitTest.passed;
	});

	future.then(done);
};
