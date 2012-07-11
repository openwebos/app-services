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
/*global clearDatabase, makeRepeatingEvent, makeEvent, ReminderAssistant, utilGetUTCDateString, IMPORTS, require:true  */

//----------------------------------------------------------------------------------------
//          On DB Changed
//----------------------------------------------------------------------------------------
/*
 * Expected results of success:
 *  - For new events or updated events, the reminder table has new/updated entries
 *  - For events that were deleted, or whose alarms were removed, the corresponding reminders are removed from the reminder table
 *  - Activity is scheduled for wake
 *  - Activity is scheduled for autoclose
 *  - lastRevNumber is stored in the status table
 *
 * Prerequisites: none
 *
 * Variations:
 *  - new event with alarm added - add entry to reminder table
 *  - new event without alarm added - no new entry to reminder table
 *  - updated event with alarm added - add entry to reminder table
 *  - updated event - had alarm, but alarm removed - entry removed from reminder table
 *  - event with alarm deleted - entry removed from reminder table
 */
if (typeof require === 'undefined') {
   require = IMPORTS.require;
}
var webos = require('webos');
webos.include("tests/loadall.js");
webos.include("tests/test-utils.js");

function DBChangedTests() {
}

DBChangedTests.timeoutInterval = 3000;

DBChangedTests.prototype.before = function (done){
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

DBChangedTests.prototype.after = function (done){
	var future = clearDatabase();
	future.then(done);
};

//New events added to the table
// This test creates an activity that must be cancelled when the test is over.
DBChangedTests.prototype.testOnDBChanged1 = function(done){
	var assistant = new ReminderAssistant();
	var eventId0;
	var eventId1;
	var eventId2;
	var baseRevNumber;

	//Setup: Make one event with an alarm, just so we can get a reasonable last rev number for the event table,
	//otherwise our test takes too long.
	var baseEvent = makeEvent(1, false, true, 5);

	//Make some events  - one with alarm, one without
	var eventWithReminder = makeEvent(1, false, true, 5);
	var eventWithoutReminder = makeEvent(1, false, false, 0);
	var expectedReminderTime = eventWithReminder.dtstart - (5 * 60000);

	//put the base event in the database
	var future = DB.put([baseEvent]);

	//get the event Id, and call init.  This will create a reminder.
	//It will also schedule a wake activity, and an autoclose activity, and
	//put a status in the database so that the merge at the end of saveautoclose will work.
	future.then(function(){
		eventId0 = future.result.results[0].id;
		baseRevNumber = future.result.results[0].rev;
		future.nest(PalmCall.call("palm://com.palm.service.calendar.reminders/", "onInit", {"unittest":true}));
	});

	//query for the status
	future.then(function(){
		future.nest(DB.find({"from":"com.palm.service.calendar.remindersstatus:1"}));
	});

	//get the activityIds, put the other events in the database
	var wakeActivityId;
	var autoCloseActivityId;
	var dbChangedActivityId;
	future.then(function(){
		var result = future.result;
		wakeActivityId = result.results[0].wakeActivityId;
		autoCloseActivityId = result.results[0].autoCloseActivityId;
		dbChangedActivityId = result.results[0].dbChangedActivityId;
		future.nest(DB.put([eventWithReminder, eventWithoutReminder]));
	});


	//get the event Ids and call onDBChanged
	future.then(function(){
		eventId1 = future.result.results[0].id;
		eventId2 = future.result.results[1].id;

		//TEST. Tell the service the calendar table changed
		future.nest(PalmCall.call("palm://com.palm.service.calendar.reminders/", "onDBChanged", {}));
	});

	//check the response from onDBChanged, and get the new reminder(s) from the database
	future.then(function(){
		var retval = future.result.returnValue;
		var wakeStatus = future.result.wakeStatus;
		var autoCloseStatus = future.result.autoCloseStatus;
		var dbChangedStatus = future.result.dbChangedStatus;
		UnitTest.require(retval === true, "Failed to create or save autoclose activity 1");
		UnitTest.require(wakeStatus == "updated", "Created an activity when we shouldn't have");
		UnitTest.require(autoCloseStatus == "updated", "Created an activity when we shouldn't have");

		//Strangely, even when there is an existing activity for DB Changed, we get a new number - maybe because of trigger?
		UnitTest.require(dbChangedStatus == "new", "Created an activity when we shouldn't have");

		future.nest(DB.find({
			from: "com.palm.service.calendar.reminders:1"
		}));
	});

	//Verify the new reminder - should be two
	future.then(function(){
		var result = future.result;
		UnitTest.require(result.results.length === 2, "OnDBChanged - created wrong number of reminders: wanted 2");

		//we should have a reminder for the base event
		var reminder = result.results[0];
		UnitTest.require(reminder.eventId === eventId0, "OnDBChanged - reminder created for wrong event");

		//and a reminder for the event we added
		reminder = result.results[1];
		UnitTest.require(reminder.eventId === eventId1, "OnDBChanged - reminder created for wrong event");
		UnitTest.require(reminder.showTime === expectedReminderTime, "OnDBChanged - created reminder for wrong time");
		UnitTest.require(reminder.autoCloseTime === eventWithReminder.dtend, "OnDBChanged - reminder set to close at the wrong time");

		future.nest(DB.find({
			from: "com.palm.service.calendar.remindersstatus:1"
		}));
	});

	//Verify that lastRevNumber was updated, autoclose and wake activities were scheduled
	future.then(function(){
		var result = future.result;
		UnitTest.require(future.result.returnValue === true, "Status query failed");
		UnitTest.require(future.result.results.length === 1, "Status query returned wrong number of results");
		UnitTest.require(future.result.results[0].lastRevNumber != baseRevNumber, "didn't update the last rev number");
		UnitTest.require(future.result.results[0].wakeActivityId === wakeActivityId, "didn't get an activity id");
		UnitTest.require(future.result.results[0].autoCloseActivityId === autoCloseActivityId, "didn't get an activity id");
		UnitTest.require(future.result.results[0].dbChangedActivityId !== 0, "didn't get an db-changed activity id");
		future.result = result;
	});

	future.then(assistant, assistant.unitTestDone);

	future.then(function(){
		future.result = UnitTest.passed;
	});

	future.then(done);
};


//Update events in the table
// This test creates an activity that must be cancelled when the test is over.
DBChangedTests.prototype.testOnDBChanged2 = function(done){
	var assistant = new ReminderAssistant();
	var eventIds = [];
	var baseRevNumber;
	var retval;
	var count;
	var expectedWakeTime;
	var expectedAutoCloseTime;

	//Setup: Make some events, so we can get a reasonable last rev number for the event table,
	//and so we have something to update. Three with reminders, three without.
	var event1 = makeEvent(1, false, true, 5);
	var event2 = makeEvent(1, false, true, 10);
	var event3 = makeEvent(1, false, true, 15);

	var event4 = makeEvent(1, false, false, 0);
	var event5 = makeEvent(1, false, false, 0);
	var event6 = makeEvent(1, false, false, 0);

	var events = [event1, event2, event3, event4, event5, event6];
	var expectedAlarmTimes = [event1.dtstart - (5 * 60000), event2.dtstart - (10 * 60000), event3.dtstart - (15 * 60000)];
	var updatedAlarmTimes = [event4.dtstart - (5 * 60000), event5.dtstart - (5 * 60000), event6.dtstart - (5 * 60000)];

	expectedWakeTime = updatedAlarmTimes[0];
	expectedAutoCloseTime = event4.dtend;

	//put the events in the database
	var future = DB.put(events);

	//get the event Ids, and call init.  This will create reminders.
	//It will also schedule a wake activity, and an autoclose activity, and
	//put a status in the database so that the merge at the end of saveautoclose will work.
	future.then(function(){
		for(var i = 0; i < 6; i++){
			eventIds.push(future.result.results[i].id);
		}
		future.nest(PalmCall.call("palm://com.palm.service.calendar.reminders/", "onInit", {"unittest":false}));
	});

	//Check the result and query for the status
	future.then(function(){
		retval = future.result.returnValue;
		var wakeStatus = future.result.wakeStatus;
		var autoCloseStatus = future.result.autoCloseStatus;
		var dbChangedStatus = future.result.dbChangedStatus;
		UnitTest.require(retval === true, "Failed to create or save autoclose activity 1");
		UnitTest.require(wakeStatus == "new", "Created an activity when we shouldn't have");
		UnitTest.require(autoCloseStatus == "new", "Created an activity when we shouldn't have");
		//Strangely, even when there is an existing activity for DB Changed, we get a new number - maybe because of trigger?
		UnitTest.require(dbChangedStatus == "new", "Created an activity when we shouldn't have");
		future.nest(DB.find({"from":"com.palm.service.calendar.remindersstatus:1"}));
	});

	//get the activityIds, and query for the reminders
	var wakeActivityId;
	var autoCloseActivityId;
	var dbChangedActivityId;
	future.then(function(){
		var result = future.result;
		wakeActivityId = result.results[0].wakeActivityId;
		autoCloseActivityId = result.results[0].autoCloseActivityId;
		dbChangedActivityId = result.results[0].dbChangedActivityId;

		future.nest(DB.find({from: "com.palm.service.calendar.reminders:1"}));
	});


	//Verify the new reminders - should have three, for the first three events
	future.then(function(){
		var result = future.result;
		UnitTest.require(result.results.length === 3, "OnDBChanged - created wrong number of reminders: wanted 3");
		for (var i = 0; i < 3; i++) {
			var reminder = result.results[i];
			UnitTest.require(reminder.eventId === eventIds[i], "OnDBChanged - created reminder for wrong event");
			UnitTest.require(reminder.showTime === expectedAlarmTimes[i], "OnDBChanged - created reminder for wrong time: " + reminder.showTime + " / " + expectedAlarmTimes[i]);
		}

		future.result = UnitTest.passed;
	});

	//Update the events we just made - set their event ids, remove reminders for the the three that had reminders, add reminders for the three that didn't
	future.then(function(){
		for(var i = 0; i < 6; i++){
			var event = events[i];
			event._id = eventIds[i];
			if(i < 3){
				event.alarm = null;
			}
			else{
				event.alarm = [{
					"alarmTrigger": {
						"valueType": "DURATION",
						"value": "-PT5M"
					}
				}];
			}
		}

		future.nest(DB.merge(events));
	});

	future.then(function(){
		var result = future.result;
		UnitTest.require(result.returnValue === true, "OnDBChanged - failed to merge events");
		UnitTest.require(result.results.length === 6, "OnDBChanged - merged wrong number of events");

		//TEST. Tell the service the calendar table changed
		future.nest(PalmCall.call("palm://com.palm.service.calendar.reminders/", "onDBChanged", {}));
	});

	//check the response from onDBChanged, and get the new reminder(s) from the database
	future.then(function(){
		var retval = future.result.returnValue;
		var wakeStatus = future.result.wakeStatus;
		var autoCloseStatus = future.result.autoCloseStatus;
		var dbChangedStatus = future.result.dbChangedStatus;
		UnitTest.require(retval === true, "Failed to create or save autoclose activity 1");
		UnitTest.require(wakeStatus == "updated", "Created an activity when we shouldn't have");
		UnitTest.require(autoCloseStatus == "updated", "Created an activity when we shouldn't have");
		//Strangely, even when there is an existing activity for DB Changed, we get a new number - maybe because of trigger?
		UnitTest.require(dbChangedStatus == "new", "Failed to create an activity");

		future.nest(DB.find({
			from: "com.palm.service.calendar.reminders:1"
		}));
	});

	//Verify the new reminders - should have three - one for the last three events
	future.then(function(){
		var result = future.result;
		UnitTest.require(result.results.length === 3, "OnDBChanged - created wrong number of reminders: wanted 3");

		for (var i = 0; i < 3; i++) {
			var reminder = result.results[i];
			UnitTest.require(reminder.eventId === eventIds[i + 3], "OnDBChanged - created reminder for wrong event: " + reminder.eventId);
			UnitTest.require(reminder.showTime === updatedAlarmTimes[i], "OnDBChanged - created reminder for wrong time");
		}

		future.nest(DB.find({
			from: "com.palm.service.calendar.remindersstatus:1"
		}));
	});

	//Verify that lastRevNumber was updated, autoclose and wake activities were scheduled. Get wake activity details
	future.then(function(){
		UnitTest.require(future.result.returnValue === true, "Status query failed");
		UnitTest.require(future.result.results.length === 1, "Status query returned wrong number of results");
		UnitTest.require(future.result.results[0].lastRevNumber != baseRevNumber, "didn't update the last rev number");
		UnitTest.require(future.result.results[0].wakeActivityId === wakeActivityId, "didn't get an activity id");
		UnitTest.require(future.result.results[0].autoCloseActivityId === autoCloseActivityId, "didn't get an activity id");
		UnitTest.require(future.result.results[0].dbChangedActivityId !== 0, "didn't get an activity id");

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
		UnitTest.require(activity.callback.params.showTime === expectedWakeTime, "Activity has the wrong alarm time parameter: "+activity.callback.params.showTime+" / " +expectedWakeTime);

		future.nest(PalmCall.call("palm://com.palm.activitymanager/", "getDetails", {"activityId": autoCloseActivityId}));
	});

	//verify details fo the autoclose activity
	future.then(function(){
		var result = future.result;
		var activity = future.result.activity;
		UnitTest.require(activity !== undefined, "Activity doesn't exist!");

		var start = activity.schedule.start;
		var expStart = utilGetUTCDateString(expectedAutoCloseTime);
		UnitTest.require(start == expStart, "Activity has the wrong schedule string");

		UnitTest.require(activity.callback.method == "palm://com.palm.service.calendar.reminders/onAutoClose", "Activity has the wrong callback");
		UnitTest.require(activity.callback.params.autoCloseTime === expectedAutoCloseTime, "Activity has the wrong alarm time parameter");
		future.result = result;
	});

	future.then(assistant, assistant.unitTestDone);

	future.then(function(){
		future.result = UnitTest.passed;
	});

	future.then(done);
};


//Event with alarm deleted
DBChangedTests.prototype.testOnDBChanged3 = function(done){
	var eventId;
	var baseRevNumber;
	var retval;
	var status;

	//Setup: Make one event with an alarm, just so we can get a reasonable last rev number for the event table,
	//otherwise our test takes too long.
	var baseEvent = makeEvent(1, false, true, 5);

	//put the event in the database
	var future = DB.put([baseEvent]);

	//get the event Ids and baseRevNumber, and call init.  This will create reminders.
	//It will also schedule a wake activity, and an autoclose activity, and
	//put a status in the database so that the merge at the end of saveautoclose will work.
	future.then(function(){
		eventId = future.result.results[0].id;
		baseRevNumber = future.result.results[0].rev;
		future.nest(PalmCall.call("palm://com.palm.service.calendar.reminders/", "onInit", {"unittest":false}));
	});

	//query for the status
	future.then(function(){
		future.nest(DB.find({"from":"com.palm.service.calendar.remindersstatus:1"}));
	});

	//get the activityIds, and Delete the event from the database
	var wakeActivityId;
	var autoCloseActivityId;
	var dbChangedActivityId;
	future.then(function(){
		var result = future.result;
		wakeActivityId = result.results[0].wakeActivityId;
		autoCloseActivityId = result.results[0].autoCloseActivityId;
		dbChangedActivityId = result.results[0].dbChangedActivityId;
		future.nest(DB.del([eventId]));
	});

	//TEST. Tell the service the calendar table changed
	future.then(function(){
		future.nest(PalmCall.call("palm://com.palm.service.calendar.reminders/", "onDBChanged", {}));
	});

	var dbWatchActivityId;
	//check the response from onDBChanged, and get the new reminder(s) from the database
	future.then(function(){
		var retval = future.result.returnValue;
		var wakeStatus = future.result.wakeStatus;
		var autoCloseStatus = future.result.autoCloseStatus;
		var dbChangedStatus = future.result.dbChangedStatus;
		UnitTest.require(retval === true, "Failed to create or save autoclose activity 1");
		UnitTest.require(wakeStatus == "none", "Created an activity when we shouldn't have");
		UnitTest.require(autoCloseStatus == "none", "Created an activity when we shouldn't have");
		//Strangely, even when there is an existing activity for DB Changed, we get a new number - maybe because of trigger?
		UnitTest.require(dbChangedStatus  == "new", "Failed to create an activity");

		future.nest(DB.find({
			from: "com.palm.service.calendar.reminders:1"
		}));
	});

	//Verify the reminder was deleted
	future.then(function(){
		var result = future.result;
		UnitTest.require(result.results.length === 0, "OnDBChanged - created wrong number of reminders: wanted 0");
		future.nest(DB.find({
			from: "com.palm.service.calendar.remindersstatus:1"
		}));
	});

	//Verify that lastRevNumber was updated, autoclose and wake activities were canceled
	future.then(function(){
		UnitTest.require(future.result.returnValue === true, "Status query failed");
		UnitTest.require(future.result.results.length === 1, "Status query returned wrong number of results");
		UnitTest.require(future.result.results[0].lastRevNumber != baseRevNumber, "didn't update the last rev number");
		UnitTest.require(future.result.results[0].wakeActivityId === 0, "didn't get an activity id");
		UnitTest.require(future.result.results[0].autoCloseActivityId === 0, "didn't get an activity id");
		future.result = UnitTest.passed;
	});

	future.then(function(){
		future.result = UnitTest.passed;
	});

	future.then(done);
};

