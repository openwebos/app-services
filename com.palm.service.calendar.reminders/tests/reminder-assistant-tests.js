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
/*global reminderSetup, okStatusSetup, failStatusSetup, clearActivities, IMPORTS, require:true  */

if (typeof require === 'undefined') {
   require = IMPORTS.require;
}

var webos = require('webos');
webos.include("tests/loadall.js");
webos.include("tests/test-utils.js");


function ReminderAssistantTests() {
}

ReminderAssistantTests.timeoutInterval = 3000;

ReminderAssistantTests.prototype.before = function (done){
	rmdrLog(" ===================== CLEARING THE TABLE =====================");
	var future = clearDatabase();
	future.then(done);
};


//======================================================================
/*
 * Expected results of success:
 *  - A list of reminders with proper times
 *
 *  Prerequisites: Results of a SUCCESSFUL query to the event table (query error check is done in findReminderTimesSubroutine)
 *  Variations:
 *  - event query returns valid events with reminders to be scheduled
 *  - event query returns nothing
 *  - event query returns results but nothing to be scheduled (all in the past)
 *
 *  Failure cases: MojoDB failure. Can't control.
 */
//success variant
ReminderAssistantTests.prototype.testFindReminderTimes1 = function(done){
	var assistant = new ReminderAssistant();

	//Run this once, at the beginning of the first unit test to clear out any wake, autoclose, or dbchanged
	//activities that might have been set up during boot.
	clearActivities();

	//Make some events - all start at 8am
	var event1 = makeEvent(-1, true,  true,  5); //past repeat with alarm - in list
	var event2 = makeEvent(-1, true,  false, 0); //past repeat no alarm
	var event3 = makeEvent(-1, false, true,  5); //past one-off with alarm
	var event4 = makeEvent(-1, false, false, 0); //past one-off no alarm

	var event5 = makeEvent(1, true,  true,  10); //future repeat with alarm - in list
	var event6 = makeEvent(1, true,  false,  0); //future repeat no alarm
	var event7 = makeEvent(1, false, true,  60, 5); //future one-off with alarm 5 minutes long- in list
	var event8 = makeEvent(1, false, false,  0); //future one-off no alarm

	var pastStartTime = 0;
	var futureStartTime = 0;

	//Sigh. Since our events are created off the current time, if these are run before 8am, the date for the past events might be
	//different, so adjust accordingly.
	var date1 = new Date();
	var before8am = (date1.getHours() < 8);
	date1.set({
			millisecond: 0,
			second: 0,
			minute: 0,
			hour: 8
	});

	if (before8am) {
		pastStartTime = date1.getTime();
	}

	date1.addDays(1);
	futureStartTime = date1.getTime();
	if(!pastStartTime){
		pastStartTime = futureStartTime;
	}

	//Arrays of expected values. We're expecting events 1, 5 and 7 to be in the reminder list.
	//Event 1 starts in the past, so it might have a different start time.
	//Event 1 & 5 are an hour long, but event 7 is only 5 minutes, so it has a different end time.
	//Event 1's alarm is 5 minutes, event 5's is 10 minutes, and event 7's is 60 minutes before the start.
	//Event 1 & 5 both auto-close at one hour, but since event 7 is only 5 minutes long, it stays open for 15 minutes.
	//Event 1 & 5 are repeating, but event 7 is a one-off.
	var expectedEventIds = [];
	var expectedStartTimes = [pastStartTime, futureStartTime, futureStartTime];
	var expectedEndTimes = [(pastStartTime + 3600000), (futureStartTime + 3600000), (futureStartTime + 300000)];
	var expectedAlarmTimes = [(pastStartTime - (5 * 60000)), (futureStartTime - (10 * 60000)), (pastStartTime - (60 * 60000))];
	var expectedAutoCloseTimes = [(pastStartTime + 3600000), (futureStartTime + 3600000), (futureStartTime + 900000)];
	var expectedIsRepeating = [true, true, false];
	var expectedAttendee0 = {"commonName":"Moll Flanders","email":"molly@flanders.com","organizer":true};
	var expectedAttendee1 = {"calendarUserType":"INDIVIDUAL","commonName":"Dorian Gray","email":"dgray@picture.com","role":"REQ-PARTICIPANT"};

	//add events to the database
	var future = DB.put([event1, event2, event3, event4, event5, event6, event7, event8]);

	//Check the results of the put, and get the event ids for 1,5 & 7.  Then query for events.
	future.then(function(){
		var results = future.result.results;
		expectedEventIds = [results[0].id, results[4].id, results[6].id];

		//Get events.  We could query for just those with alarms, but let's put findReminderTimes to the test, yeah?
		future.nest(DB.find({from : "com.palm.calendarevent:1"}));
	});

	//TEST. pass results to findReminderTimes
	future.then(assistant, assistant.findReminderTimes);

	//verify content in reminderList
	future.then(function(){
		var reminderList = future.result.reminderList;
		UnitTest.require(reminderList.length === 3, "Find reminder times returned wrong number of results");
		var reminder;
		for(var i = 0; i < reminderList.length; i++){
			reminder = reminderList[i];
			UnitTest.require(reminder._kind == "com.palm.service.calendar.reminders:1", "Reminder["+i+"] has the wrong kind");
			UnitTest.require(reminder.eventId == expectedEventIds[i], "Reminder["+i+"] has the wrong event id. expected: "+expectedEventIds[i]+" got: "+reminder.eventId);
			UnitTest.require(reminder.subject == "My subject", "Reminder["+i+"] has the wrong subject");
			UnitTest.require(reminder.location == "My location", "Reminder["+i+"] has the wrong location");
			UnitTest.require(reminder.isAllDay === false, "Reminder["+i+"] has the wrong allDay value");
			UnitTest.require(reminder.startTime == expectedStartTimes[i], "Reminder["+i+"] has the wrong startTime. expected: "+expectedStartTimes[i]+" got: "+reminder.startTime);
			UnitTest.require(reminder.endTime == expectedEndTimes[i], "Reminder["+i+"] has the wrong endTime. expected: "+expectedEndTimes[i]+" got: "+reminder.endTime);
			UnitTest.require(reminder.showTime == expectedAlarmTimes[i], "Reminder["+i+"] has the wrong showTime. expected: "+expectedAlarmTimes[i]+" got: "+reminder.showTime);
			UnitTest.require(reminder.autoCloseTime == expectedAutoCloseTimes[i], "Reminder["+i+"] has the wrong autoCloseTime. expected: "+expectedAutoCloseTimes[i]+" got: "+reminder.autoCloseTime);
			UnitTest.require(reminder.isRepeating == expectedIsRepeating[i], "Reminder["+i+"] has the wrong isRepeating");
			UnitTest.require(reminder.calendarId == "F+Z8", "Reminder has the wrong calendar id");
			UnitTest.require(reminder.attendees.length === 2, "Reminder has the number of wrong attendees");
			UnitTest.require(reminder.attendees[0].email == expectedAttendee0.email, "Reminder has the wrong attendee info 1");
			UnitTest.require(reminder.attendees[1].email == expectedAttendee1.email, "Reminder has the wrong attendee info 2");
		}
		future.result = UnitTest.passed;
	});

	future.then(done);

};

//no events with alarms variant
ReminderAssistantTests.prototype.testFindReminderTimes2 = function(done){
	var assistant = new ReminderAssistant();

	//Make some events - none have alarms!
	var event1 = makeEvent(-1, true,  false,  5);
	var event2 = makeEvent(-1, true,  false, 0);
	var event3 = makeEvent(-1, false, false,  5);
	var event4 = makeEvent(-1, false, false, 0);

	var event5 = makeEvent(1, true,  false,  10);
	var event6 = makeEvent(1, true,  false,  0);
	var event7 = makeEvent(1, false, false,  60, 5);
	var event8 = makeEvent(1, false, false,  0);

	//add events to the database
	var future = DB.put([event1, event2, event3, event4, event5, event6, event7, event8]);

	//Query for events.
	future.then(function(){

		//Get events with alarms.
		future.nest(DB.find(
			{	from : "com.palm.calendarevent:1"
			,	where	:	[ {	prop: "alarm", op: "!=", val: null } ]
			}
		));
	});

	//TEST. pass (empty) results to findReminderTimes
	future.then(assistant, assistant.findReminderTimes);

	//verify content in reminderList
	future.then(function(){
		var reminderList = future.result.reminderList;
		UnitTest.require(reminderList.length === 0, "Find reminder times returned wrong number of results");
		future.result = UnitTest.passed;
	});

	future.then(done);

};

//No events with reminders to schedule variant
ReminderAssistantTests.prototype.testFindReminderTimes3 = function(done){
	var assistant = new ReminderAssistant();

	//Make some events - all alarms are in the past
	var event1 = makeEvent(-1, false, true,  5);
	var event2 = makeEvent(-1, false, true, 10);
	var event3 = makeEvent(-1, false, true, 15);
	var event4 = makeEvent(-1, false, true, 180);

	//add events to the database
	var future = DB.put([event1, event2, event3, event4]);

	//Query for events.
	future.then(function(){

		//Get events with alarms.
		future.nest(DB.find(
			{	from : "com.palm.calendarevent:1"
			,	where	:	[ {	prop: "alarm", op: "!=", val: null } ]
			}
		));
	});

	//TEST. pass results to findReminderTimes
	future.then(assistant, assistant.findReminderTimes);

	//verify content in reminderList
	future.then(function(){
		var reminderList = future.result.reminderList;
		UnitTest.require(reminderList.length === 0, "Find reminder times returned wrong number of results");
		future.result = UnitTest.passed;
	});

	future.then(done);

};

ReminderAssistantTests.prototype.testFindReminderTimes4 = function(done){
	var assistant = new ReminderAssistant();

	//Run this once, at the beginning of the first unit test to clear out any wake, autoclose, or dbchanged
	//activities that might have been set up during boot.
	clearActivities();


	//{"_kind": "com.palm.calendarevent:1","alarm": [],"allDay": false,"dtend":1278554400000,"dtstart":1278550800000,"location": "","note": "","subject": "NO alarm","tzId": "America\/Los_Angeles"},
	//{"_kind": "com.palm.calendarevent:1","alarm": [{"action": "email","alarmTrigger": {"value": "-PT15M","valueType": "DURATION"}},{"action": "sound","alarmTrigger": {"value": "-PT10M","valueType": "DURATION"}}],"allDay": false,"dtend":1278554400000,"dtstart":1278550800000,"location": "","note": "","subject": "Has email and sound, but NO display alarm","tzId": "America\/Los_Angeles"},
	//{"_kind": "com.palm.calendarevent:1","alarm": [{"action": "email","alarmTrigger": {"value": "-PT15M","valueType": "DURATION"}},{"action": "display","alarmTrigger": {"value": "-PT10M","valueType": "DURATION"}}],"allDay": false,"dtend":1278554400000,"dtstart":1278550800000,"location": "","note": "","subject": "Has display(10min) and email alarms","tzId": "America\/Los_Angeles"}

	//Make some events - all start at 8am
	var event1 = makeEvent(1, true, false, 0); //future repeat
	var event2 = makeEvent(1, true, false, 0); //future repeat
	var event3 = makeEvent(1, true, false, 0); //future repeat

	//no alarm
	event1.alarm = [];

	//has alarms but no display alarms
	event2.alarm = [{"action": "email","alarmTrigger": {"value": "-PT15M","valueType": "DURATION"}},{"action": "sound","alarmTrigger": {"value": "-PT10M","valueType": "DURATION"}}];

	//has alarms, has display alarms
	event3.alarm = [{"action": "email","alarmTrigger": {"value": "-PT15M","valueType": "DURATION"}},{"action": "display","alarmTrigger": {"value": "-PT10M","valueType": "DURATION"}}];

	var pastStartTime = 0;
	var futureStartTime = 0;

	//Sigh. Since our events are created off the current time, if these are run before 8am, the date for the past events might be
	//different, so adjust accordingly.
	var date1 = new Date();
	var before8am = (date1.getHours() < 8);
	date1.set({
			millisecond: 0,
			second: 0,
			minute: 0,
			hour: 8
	});

	if (before8am) {
		pastStartTime = date1.getTime();
	}

	date1.addDays(1);
	futureStartTime = date1.getTime();
	if(!pastStartTime){
		pastStartTime = futureStartTime;
	}

	//Arrays of expected values. We're expecting only event3 to be in the reminder list.
	//Event 3 is an hour long
	//Event 3's alarm is 10 minutes
	//Event 3 auto-closes at one hour
	//Event 3 is repeating
	var expectedEventIds = [];
	var expectedStartTimes = [futureStartTime];
	var expectedEndTimes = [(futureStartTime + 3600000)];
	var expectedAlarmTimes = [(futureStartTime - (10 * 60000))];
	var expectedAutoCloseTimes = [(futureStartTime + 3600000)];
	var expectedIsRepeating = [true];
	var expectedAttendee0 = {"commonName":"Moll Flanders","email":"molly@flanders.com","organizer":true};
	var expectedAttendee1 = {"calendarUserType":"INDIVIDUAL","commonName":"Dorian Gray","email":"dgray@picture.com","role":"REQ-PARTICIPANT"};

	//add events to the database
	var future = DB.put([event1, event2, event3]);

	//Check the results of the put, and get the event ids for event 3.  Then query for events.
	future.then(function(){
		var results = future.result.results;
		expectedEventIds = [results[2].id];

		//Get events.  We could query for just those with alarms, but let's put findReminderTimes to the test, yeah?
		future.nest(DB.find({from : "com.palm.calendarevent:1"}));
	});

	//TEST. pass results to findReminderTimes
	future.then(assistant, assistant.findReminderTimes);

	//verify content in reminderList
	future.then(function(){
		var reminderList = future.result.reminderList;
		UnitTest.require(reminderList.length === 1, "Find reminder times returned wrong number of results");
		var reminder;
		for(var i = 0; i < reminderList.length; i++){
			reminder = reminderList[i];
			UnitTest.require(reminder._kind == "com.palm.service.calendar.reminders:1", "Reminder["+i+"] has the wrong kind");
			UnitTest.require(reminder.eventId == expectedEventIds[i], "Reminder["+i+"] has the wrong event id. expected: "+expectedEventIds[i]+" got: "+reminder.eventId);
			UnitTest.require(reminder.subject == "My subject", "Reminder["+i+"] has the wrong subject");
			UnitTest.require(reminder.location == "My location", "Reminder["+i+"] has the wrong location");
			UnitTest.require(reminder.isAllDay === false, "Reminder["+i+"] has the wrong allDay value");
			UnitTest.require(reminder.startTime == expectedStartTimes[i], "Reminder["+i+"] has the wrong startTime. expected: "+expectedStartTimes[i]+" got: "+reminder.startTime);
			UnitTest.require(reminder.endTime == expectedEndTimes[i], "Reminder["+i+"] has the wrong endTime. expected: "+expectedEndTimes[i]+" got: "+reminder.endTime);
			UnitTest.require(reminder.showTime == expectedAlarmTimes[i], "Reminder["+i+"] has the wrong showTime. expected: "+expectedAlarmTimes[i]+" got: "+reminder.showTime);
			UnitTest.require(reminder.autoCloseTime == expectedAutoCloseTimes[i], "Reminder["+i+"] has the wrong autoCloseTime. expected: "+expectedAutoCloseTimes[i]+" got: "+reminder.autoCloseTime);
			UnitTest.require(reminder.isRepeating == expectedIsRepeating[i], "Reminder["+i+"] has the wrong isRepeating");
			UnitTest.require(reminder.calendarId == "F+Z8", "Reminder has the wrong calendar id");
			UnitTest.require(reminder.attendees.length === 2, "Reminder has the number of wrong attendees");
			UnitTest.require(reminder.attendees[0].email == expectedAttendee0.email, "Reminder has the wrong attendee info 1");
			UnitTest.require(reminder.attendees[1].email == expectedAttendee1.email, "Reminder has the wrong attendee info 2");
		}
		future.result = UnitTest.passed;
	});

	future.then(done);

};
//======================================================================
/*
 * Expected results of success:
 *  - Reminders added to the table with proper times
 *
 *  Prerequisites: Results of a query to the event table
 *  Variations:
 *  - event query returns valid events
 *  - event query returns nothing
 *  - event query failed
 *
 *  Failure cases: MojoDB failure. Can't control.
 */
//event query returns valid events  variant
ReminderAssistantTests.prototype.testFindReminderTimesSubroutine1 = function(done){
	var assistant = new ReminderAssistant();

	//Make some events - all start at 8am
	var event1 = makeEvent(-1, true,  true,  5); //past repeat with alarm - in list
	var event2 = makeEvent(-1, true,  false, 0); //past repeat no alarm
	var event3 = makeEvent(-1, false, true,  5); //past one-off with alarm
	var event4 = makeEvent(-1, false, false, 0); //past one-off no alarm

	var event5 = makeEvent(1, true,  true,  10); //future repeat with alarm - in list
	var event6 = makeEvent(1, true,  false,  0); //future repeat no alarm
	var event7 = makeEvent(1, false, true,  60, 5); //future one-off with alarm 5 minutes long- in list
	var event8 = makeEvent(1, false, false,  0); //future one-off no alarm

	var pastStartTime = 0;
	var futureStartTime = 0;

	//Sigh. Since our events are created off the current time, if these are run before 8am, the date for the past events might be
	//different, so adjust accordingly.
	var date1 = new Date();
	var before8am = (date1.getHours < 8);
	date1.set({
			millisecond: 0,
			second: 0,
			minute: 0,
			hour: 8
	});

	if (before8am) {
		pastStartTime = date1.getTime();
	}

	date1.addDays(1);
	futureStartTime = date1.getTime();
	if(!pastStartTime){
		pastStartTime = futureStartTime;
	}

	//Arrays of expected values. We're expecting events 1, 5 and 7 to be in the reminder list.
	//Event 1 starts in the past, so it might have a different start time.
	//Event 1 & 5 are an hour long, but event 7 is only 5 minutes, so it has a different end time.
	//Event 1's alarm is 5 minutes, event 5's is 10 minutes, and event 7's is 60 minutes before the start.
	//Event 1 & 5 both auto-close at one hour, but since event 7 is only 5 minutes long, it stays open for 15 minutes.
	//Event 1 & 5 are repeating, but event 7 is a one-off.
	var expectedEventIds = [];
	var expectedStartTimes = [pastStartTime, futureStartTime, futureStartTime];
	var expectedEndTimes = [(pastStartTime + 3600000), (futureStartTime + 3600000), (futureStartTime + 300000)];
	var expectedAlarmTimes = [(pastStartTime - (5 * 60000)), (futureStartTime - (10 * 60000)), (pastStartTime - (60 * 60000))];
	var expectedAutoCloseTimes = [(pastStartTime + 3600000), (futureStartTime + 3600000), (futureStartTime + 900000)];
	var expectedIsRepeating = [true, true, false];

	//add events to the database
	var future = DB.put([event1, event2, event3, event4, event5, event6, event7, event8]);

	//Check the results of the put, and get the event ids for 1,5 & 7.  Then query for events.
	future.then(function(){
		var results = future.result.results;
		expectedEventIds = [results[0].id, results[4].id, results[6].id];

		//Get events.  We could query for just those with alarms, but let's put findReminderTimes to the test, yeah?
		future.nest(DB.find(
			{	from : "com.palm.calendarevent:1"
			,	where	:	[ {	prop: "alarm", op: "!=", val: null } ]
			}
		));
	});

	//pass results to findReminderTimesSubroutine
	future.then(assistant, assistant.findReminderTimesSubroutine);

	//Verify the result from findReminderTimesSubroutine, then query the reminder table, looking for content
	future.then(function(){
		var retval = future.result.returnValue;
		var results = future.result.results;
		UnitTest.require(retval === true, "Find reminder times subroutine failed to put reminders in table");
		UnitTest.require(results.length === 3, "Find reminder times subroutine failed to put the right number of reminders in table");

		future.nest(DB.find({from: "com.palm.service.calendar.reminders:1"}));
	});

	future.then(function(){
		var retval = future.result.returnValue;
		var results = future.result.results;
		UnitTest.require(retval === true, "Find reminder times subroutine failed to put reminders in table");
		UnitTest.require(results.length === 3, "Find reminder times subroutine failed to put the right number of reminders in table");

		var reminder;
		for(var i = 0; i < results.length; i++){
			reminder = results[i];
			UnitTest.require(reminder._kind == "com.palm.service.calendar.reminders:1", "Reminder["+i+"] has the wrong kind");
			UnitTest.require(reminder.eventId == expectedEventIds[i], "Reminder["+i+"] has the wrong event id. expected: "+expectedEventIds[i]+" got: "+reminder.eventId);
			UnitTest.require(reminder.subject == "My subject", "Reminder["+i+"] has the wrong subject");
			UnitTest.require(reminder.location == "My location", "Reminder["+i+"] has the wrong location");
			UnitTest.require(reminder.isAllDay === false, "Reminder["+i+"] has the wrong allDay value");
			UnitTest.require(reminder.startTime == expectedStartTimes[i], "Reminder["+i+"] has the wrong startTime. expected: "+expectedStartTimes[i]+" got: "+reminder.startTime);
			UnitTest.require(reminder.endTime == expectedEndTimes[i], "Reminde["+i+"]r has the wrong endTime. expected: "+expectedEndTimes[i]+" got: "+reminder.endTime);
			UnitTest.require(reminder.showTime == expectedAlarmTimes[i], "Reminder["+i+"] has the wrong showTime. expected: "+expectedAlarmTimes[i]+" got: "+reminder.showTime);
			UnitTest.require(reminder.autoCloseTime == expectedAutoCloseTimes[i], "Reminder["+i+"] has the wrong autoCloseTime. expected: "+expectedAutoCloseTimes[i]+" got: "+reminder.autoCloseTime);
			UnitTest.require(reminder.isRepeating == expectedIsRepeating[i], "Reminder["+i+"] has the wrong isRepeating");

			//TODO: These fields are not hooked up yet
			//UnitTest.require(reminder.attendees == expectedEventAttendees[i], "Reminder has the wrong attendees");
			//UnitTest.require(reminder.emailAccountId == expectedEmailAccountIds[i], "Reminder has the wrong email account id");
		}
		future.result = UnitTest.passed;
	});

	future.then(done);
};

//event query returns nothing variant
ReminderAssistantTests.prototype.testFindReminderTimesSubroutine2 = function(done){
	var assistant = new ReminderAssistant();

	//Don't add events to the database, just query the empty table
	var future = DB.find(
			{	from : "com.palm.calendarevent:1"
			,	where	:	[ {	prop: "alarm", op: "!=", val: null } ]
			}
	);

	//pass results to findReminderTimesSubroutine
	future.then(assistant, assistant.findReminderTimesSubroutine);

	//Verify the result from findReminderTimesSubroutine, then query the reminder table, looking for content
	future.then(function(){
		var retval = future.result.returnValue;
		var results = future.result.results;
		UnitTest.require(retval === true, "Find reminder times subroutine failed to put reminders in table");
		UnitTest.require(results === undefined, "Find reminder times subroutine failed to put reminders in table");

		future.result = UnitTest.passed;
	});

	future.then(done);
};

//event query failed variant
ReminderAssistantTests.prototype.testFindReminderTimesSubroutine3 = function(done){
	var assistant = new ReminderAssistant();

	//Don't add events to the database, just query the empty table
	var future = new Foundations.Control.Future();
	future.result = {returnValue: false};

	//pass results to findReminderTimesSubroutine
	future.then(assistant, assistant.findReminderTimesSubroutine);

	//Verify the result from findReminderTimesSubroutine, then query the reminder table, looking for content
	future.then(function(){
		var retval = future.result.returnValue;
		var results = future.result.results;
		UnitTest.require(retval === false, "Find reminder times subroutine failed to put reminders in table");
		UnitTest.require(results === undefined, "Find reminder times subroutine failed to put reminders in table");

		future.result = UnitTest.passed;
	});

	future.then(done);
};
//======================================================================
/*
 * Expected results of success:
 *  - An activity is created for the next reminder time
 *
 *  Prerequisites: Result from a successful query to the reminder table
 *  Variations:
 *  - no previous activity exists, activity created - {returnValue: true, activityId: #}
 *  - previous activity exists and gets updated  - activity number does not change, activity time changes - {returnValue: true, rescheduled: true}
 *  - previous activity exists, reminder query returns no results - old activity completed, no new activity scheduled - {returnValue: true}
 *  - reminder query failed - no activity - {returnValue: false}
 *
 *  Failure cases: ActivityManager or MojoDB failure.
 *
 *  This test creates an activity that must be cancelled when the test is over.
 *
 */
/*
 * Possible responses from createOnWakeActivity:
 * Old activity completed and updated
 * future.result = {returnValue: true, noStatusChange: true};
 *
 * No old activity to complete, and no new activity to schedule
 * Old activity completed, but nothing new to schedule
 * future.result = {returnValue: true};
 *
 * No old activity to complete, and new activity scheduled
 * future.result = {returnValue: true, activityId: 9};
 *
 * Failure to create
 * future.result = {returnValue: false};
 */
//no previous activity exists, activity created variant
// This test creates an activity that must be cancelled when the test is over.
ReminderAssistantTests.prototype.testCreateOnWakeActivity1 = function(done){
	var assistant = new ReminderAssistant();

	var reminder1 = makeDummyReminder(1);
	var reminder2 = makeDummyReminder(1, 1);
	var reminder3 = makeDummyReminder(1, 2);
	var reminder4 = makeDummyReminder(1, 3);
	var now = new Date().getTime();

	//Put some reminders in the reminder table, but don't put anything in the status table
	var future = DB.put([reminder1, reminder2, reminder3, reminder4]);

	//Query the reminder table
	future.then(function(){
		future.nest(DB.find(
				{	from	: "com.palm.service.calendar.reminders:1"
				,	where	:
					[{	prop	: "showTime"
					,	op		: ">="
					,	val		: now
					}]
				,	orderBy	: "showTime"
				,	limit	: 1
				}
		));
	});

	//TEST. Pass results to createOnWakeActivity
	future.then(assistant, assistant.createOnWakeActivity);

	//Verify the result from createOnWakeActivity, then query the reminder table, looking for content
	future.then(function(){
		var retval = future.result.returnValue;
		var activityId = future.result.activityId;
		var noStatusChange = future.result.noStatusChange;
		//{returnValue: true, activityId: #}
		UnitTest.require(retval === true, "Failed to create wake activity 1");
		UnitTest.require(activityId > 0, "Failed to create wake activity 2");
		UnitTest.require(noStatusChange === undefined, "Failed to create wake activity 3");

		future.result = {activityId: activityId};
	});

	future.then(assistant, assistant.unitTestDone);

	future.then(function(){
		future.result = UnitTest.passed;
	});

	future.then(done);
};

//previous activity exists and needs to be completed/updated
// This test creates an activity that must be cancelled when the test is over.
ReminderAssistantTests.prototype.testCreateOnWakeActivity2 = function(done){
	var assistant = new ReminderAssistant();

	var reminder1 = makeDummyReminder(1);
	var reminder2 = makeDummyReminder(1, 1);
	var reminder3 = makeDummyReminder(1, 2);
	var reminder4 = makeDummyReminder(1, 3);
	var now = new Date().getTime();

	//Put some reminders in the reminder table
	var future = DB.put([reminder1, reminder2, reminder3, reminder4]);


	//Create an activity with the activity manager
	var activityArgs =	{	start	: true
						,	activity:
							{	"name"			: "calendar.reminders.wake"
							,	"description"	: "Time to show a calendar reminder"
							,	"type"			:
								{	"cancellable"	: true,
									"persist": true
								}
							,	"replace":true
							,	"schedule"		: {"start": "2038-01-01 12:00:00"}
							,	"callback"		:
									{	"method"	: "palm://com.palm.service.calendar.reminders/onWake"
									,	"params"	: {"showTime": now}
									}
							}
						};

	future.then(function(){
		future.nest(PalmCall.call("palm://com.palm.activitymanager/", "create", activityArgs));
	});


	//Put the activity number in the status table
	future.then(this, okStatusSetup);

	//Query the reminder table
	future.then(function(){
		future.nest(DB.find(
				{	from	: "com.palm.service.calendar.reminders:1"
				,	where	:
					[{	prop	: "showTime"
					,	op		: ">="
					,	val		: now
					}]
				,	orderBy	: "showTime"
				,	limit	: 1
				}
		));
	});


	//TEST. Pass results to createOnWakeActivity
	future.then(assistant, assistant.createOnWakeActivity);

	//Verify the result from createOnWakeActivity
	future.then(function(){
		var result = future.result;
		var retval = future.result.returnValue;
		var activityId = future.result.activityId;
		var noStatusChange = future.result.noStatusChange;
		//{returnValue: true, noStatusChange: true};
		UnitTest.require(retval === true, "Failed to create wake activity 1");
		UnitTest.require(activityId === undefined, "Failed to create wake activity 2");
		UnitTest.require(noStatusChange === true, "Failed to create wake activity 3");

		//TODO: CHECK THE ACTIVITY ID IN TABLE, SEE IT DIDNT' CHANGE

		future.result = result;
	});

	future.then(assistant, assistant.unitTestDone);

	future.then(function(){
		future.result = UnitTest.passed;
	});

	future.then(done);
};

//reminder query returns no results - no activity
ReminderAssistantTests.prototype.testCreateOnWakeActivity3 = function(done){
	var assistant = new ReminderAssistant();

	var now = new Date().getTime();

	//No status and no reminders in the table,  Query the reminder table
	var future = DB.find(
				{	from	: "com.palm.service.calendar.reminders:1"
				,	where	:
					[{	prop	: "showTime"
					,	op		: ">="
					,	val		: now
					}]
				,	orderBy	: "showTime"
				,	limit	: 1
				}
	);

	//TEST. Pass results to createOnWakeActivity
	future.then(assistant, assistant.createOnWakeActivity);

	//Verify the result from createOnWakeActivity
	future.then(function(){
		var retval = future.result.returnValue;
		var activityId = future.result.activityId;
		var noStatusChange = future.result.noStatusChange;
		//{returnValue: true};
		UnitTest.require(retval === true, "Failed to create wake activity 1");
		UnitTest.require(activityId === undefined, "Failed to create wake activity 2");
		UnitTest.require(noStatusChange === undefined, "Failed to create wake activity 3");

		future.result = UnitTest.passed;

	});

	//TODO: CHECK THE ACTIVITY ID IN TABLE, SEE THE ACTIVITY # IS ZERO

	future.then(done);
};

//reminder query fails - no activity
ReminderAssistantTests.prototype.testCreateOnWakeActivity4 = function(done){
	var assistant = new ReminderAssistant();

	//No status and no reminders in the table, set as though query result is false
	var future = new Foundations.Control.Future();
	future.result = {returnValue: false};

	//TEST. Pass results to createOnWakeActivity
	future.then(assistant, assistant.createOnWakeActivity);

	//Verify the result from createOnWakeActivity
	future.then(function(){
		var retval = future.result.returnValue;
		var activityId = future.result.activityId;
		var noStatusChange = future.result.noStatusChange;
		//{returnValue: false};
		UnitTest.require(retval === false, "Failed to create wake activity 1");
		UnitTest.require(activityId === undefined, "Failed to create wake activity 2");
		UnitTest.require(noStatusChange === undefined, "Failed to create wake activity 3");

		future.result = UnitTest.passed;

	});

	future.then(done);
};

// Old activity completed, but nothing new to schedule
// This test creates an activity, but the test should complete it
ReminderAssistantTests.prototype.testCreateOnWakeActivity5 = function(done){
	var assistant = new ReminderAssistant();

	var reminder1 = makeDummyReminder(-1);
	var reminder2 = makeDummyReminder(-1, 1);
	var reminder3 = makeDummyReminder(-1, 2);
	var reminder4 = makeDummyReminder(-1, 3);
	var now = new Date().getTime();

	//Put some reminders in the reminder table
	var future = DB.put([reminder1, reminder2, reminder3, reminder4]);


	//Create an activity with the activity manager
	var activityArgs =	{	start	: true
						,	activity:
							{	"name"			: "calendar.reminders.wake"
							,	"description"	: "Time to show a calendar reminder"
							,	"type"			:
								{	"cancellable"	: true,
									"persist": true
								}
							,	"replace":true
							,	"schedule"		: {"start": "2038-01-01 12:00:00"}
							,	"callback"		:
									{	"method"	: "palm://com.palm.service.calendar.reminders/onWake"
									,	"params"	: {"showTime": now}
									}
							}
						};

	future.then(function(){
		future.nest(PalmCall.call("palm://com.palm.activitymanager/", "create", activityArgs));
	});


	//Put the activity number in the status table
	future.then(this, okStatusSetup);

	//Query the reminder table
	future.then(function(){
		future.nest(DB.find(
				{	from	: "com.palm.service.calendar.reminders:1"
				,	where	:
					[{	prop	: "showTime"
					,	op		: ">="
					,	val		: now
					}]
				,	orderBy	: "showTime"
				,	limit	: 1
				}
		));
	});


	//TEST. Pass results to createOnWakeActivity
	future.then(assistant, assistant.createOnWakeActivity);

	//Verify the result from createOnWakeActivity
	future.then(function(){
		var result = future.result;
		var retval = future.result.returnValue;
		var activityId = future.result.activityId;
		var noStatusChange = future.result.noStatusChange;
		//{returnValue: true};
		UnitTest.require(retval === true, "Failed to create wake activity 1");
		UnitTest.require(activityId === undefined, "Failed to create wake activity 2");
		UnitTest.require(noStatusChange === undefined, "Failed to create wake activity 3");

		//TODO: CHECK THE ACTIVITY ID IN TABLE, SEE IT's ZERO

		future.result = UnitTest.passed;
	});

	future.then(done);
};

//======================================================================
/*
 * Expected results of success:
 *  - If an activity was created, then the status table is updated with the new wake activity id, and return status == new
 *  - If an activity was updated, then the status table remains the same and return status == updated
 *  - If no activity is scheduled, then the status table is updated with activity id == 0 and return status == none
 *  - If an error occurred in activity creation, then the status table is updated with activity id == 0 and return status == FAIL
 */
/*
 * Possible incoming responses from createOnWakeActivity:
 * Old activity completed and updated
 * future.result = {returnValue: true, noStatusChange: true};
 *
 * No old activity to complete, and no new activity to schedule
 * Old activity completed, but nothing new to schedule
 * future.result = {returnValue: true};
 *
 * No old activity to complete, and new activity scheduled
 * future.result = {returnValue: true, activityId: 9};
 *
 * Failure to create
 * future.result = {returnValue: false};
 */

//Activity creation failure case: incoming {returnValue: false}
ReminderAssistantTests.prototype.testSaveWakeActivityId1 = function(done){
	var assistant = new ReminderAssistant();

	var future = new Foundations.Control.Future();
	future.result = {returnValue: false };

	//TEST. Pass results to saveWakeActivityId
	future.then(assistant, assistant.saveWakeActivityId);

	//Verify the result from createOnWakeActivity
	future.then(function(){
		var retval = future.result.returnValue;
		var status = future.result.status;
		UnitTest.require(status == "FAIL", "Wrong status");
		future.result = UnitTest.passed;
	});

	future.then(done);
};

//No old activity to complete, and new activity scheduled
//incoming: {returnValue: true, activityId: 9};
ReminderAssistantTests.prototype.testSaveWakeActivityId2 = function(done){
	var assistant = new ReminderAssistant();

	var future = new Foundations.Control.Future();
	future.result = {returnValue: true, activityId: 9};

	//TEST. Pass results to saveWakeActivityId
	future.then(assistant, assistant.saveWakeActivityId);

	//Verify the result from createOnWakeActivity
	future.then(function(){
		var retval = future.result.returnValue;
		var status = future.result.status;
		UnitTest.require(retval === true, "Failed to pass on failure status");
		UnitTest.require(status == "new", "Wrong status");
		future.result = UnitTest.passed;
	});

	future.then(done);
};

//Old activity completed, but nothing new to schedule
//Incoming: {returnValue: true};
ReminderAssistantTests.prototype.testSaveWakeActivityId3 = function(done){
	var assistant = new ReminderAssistant();

	var future = new Foundations.Control.Future();
	future.result = {returnValue: true};

	//TEST. Pass results to saveWakeActivityId
	future.then(assistant, assistant.saveWakeActivityId);

	//Verify the result from createOnWakeActivity
	future.then(function(){
		var retval = future.result.returnValue;
		var status = future.result.status;
		UnitTest.require(retval === true, "Failed to pass on failure status");
		UnitTest.require(status == "none", "Wrong status");
		future.result = UnitTest.passed;
	});

	future.then(done);
};

//Old activity completed, but nothing new to schedule
//Incoming: {returnValue: true, noStatusChange: true};
ReminderAssistantTests.prototype.testSaveWakeActivityId4 = function(done){
	var assistant = new ReminderAssistant();

	var future = new Foundations.Control.Future();
	future.result = {returnValue: true, noStatusChange: true};

	//TEST. Pass results to saveWakeActivityId
	future.then(assistant, assistant.saveWakeActivityId);

	//Verify the result from createOnWakeActivity
	future.then(function(){
		var retval = future.result.returnValue;
		var status = future.result.status;
		UnitTest.require(retval === true, "Failed to pass on failure status");
		UnitTest.require(status == "updated", "Wrong status");
		future.result = UnitTest.passed;
	});

	future.then(done);
};

//======================================================================
/*
 * Expected results of success:
 *  - An activity is created for the next reminder time and the activityId is saved in the status table
 *
 *  Prerequisites: Reminders in the reminder table
 *  Variations:
 *  - activity is created and id is saved
 *  - no activity is created
 *
 *  Failure cases: ActivityManager or MojoDB failure.
 */
//activity is created variant
// This test creates an activity that must be cancelled when the test is over.
ReminderAssistantTests.prototype.testFindNextWakeTimeSubroutine1 = function(done){
	var assistant = new ReminderAssistant();
	var now = new Date().getTime();
	var originalActivityId;
	var retval;
	var count;
	var reminder1 = makeDummyReminder(1);
	var reminder2 = makeDummyReminder(1, 1);
	var reminder3 = makeDummyReminder(1, 2);
	var reminder4 = makeDummyReminder(1, 3);

	//put reminders in the table
	var future = DB.put([reminder1, reminder2, reminder3, reminder4]);

	//Create an activity with the activity manager
	var activityArgs =	{	start	: true
						,	activity:
							{	"name"			: "calendar.reminders.wake"
							,	"description"	: "Time to show a calendar reminder"
							,	"type"			:
								{	"cancellable"	: true
								,	"persist"	: true
								}
							,	"schedule"		: {"start": "2020-01-01 12:00:00"}
							,	"callback"		:
									{	"method"	: "palm://com.palm.service.calendar.reminders/onWake"
									,	"params"	: {"showTime": now}
									}
							}
						};

	future.then(function(){
		future.nest(PalmCall.call("palm://com.palm.activitymanager/", "create", activityArgs));
	});

	var activityId;
	future.then(function(){
		var result = future.result;
		activityId = future.result.activityId;
		future.result = result;
	});

	//Put the activity number in the status table
	future.then(this, okStatusSetup);

	//query the status table
	future.then(function(){
		future.nest(DB.find({"from": "com.palm.service.calendar.remindersstatus:1"}));
	});

	future.then(function(){
		originalActivityId = future.result.results[0].wakeActivityId;
		future.result = "go";
	});

	future.then(function(){
		future.nest(PalmCall.call("palm://com.palm.activitymanager/", "getDetails", {
			"activityId": activityId
		}));
	});

	future.then(function(){
		var result = future.result;
		future.result = result;
	});

	//TEST
	future.then(assistant, assistant.findNextWakeTimeSubroutine);

	future.then(function(){
		retval = future.result.returnValue;
		var status = future.result.status;
		UnitTest.require(retval === true, "Failed to create or save wake activity 1");
		UnitTest.require(status === "updated", "Created an activity when we shouldn't have");

		future.nest(DB.find({"from": "com.palm.service.calendar.remindersstatus:1"}));
	});

	future.then(function(){
		var result = future.result;
		retval = future.result.returnValue;
		count = future.result.results.length;
		var activityId = future.result.results[0].wakeActivityId;
		UnitTest.require(retval === true, "Failed to create wake activity 2");
		UnitTest.require(count === 1, "Failed to save state properly");
		UnitTest.require(activityId === originalActivityId, "Failed to save activityId");
		future.result = result;
	});

	future.then(function(){
		future.nest(PalmCall.call("palm://com.palm.activitymanager/", "getDetails", {
			"activityId": activityId
		}));
	});

	future.then(function(){
		var result = future.result;
		future.result = result;
	});

	future.then(assistant, assistant.unitTestDone);

	future.then(function(){
		future.result = UnitTest.passed;
	});

	future.then(done);
};

//no activity is created
ReminderAssistantTests.prototype.testFindNextWakeTimeSubroutine2 = function(done){
	var assistant = new ReminderAssistant();
	var retval;

	var future = new Foundations.Control.Future();

	//TEST nothing in the status table, and no reminders
	future.now(assistant, assistant.findNextWakeTimeSubroutine);

	//expect return value true, but no activityId, because nothing should have been saved
	future.then(function(){
		retval = future.result.returnValue;
		var status = future.result.status;
		UnitTest.require(retval === true, "Failed to create or save wake activity 1");
		UnitTest.require(status === "none", "Wrong status returned");

		future.nest(DB.find({"from": "com.palm.service.calendar.remindersstatus:1"}));
	});

	//expect return value true, but no results, because the status table should still be empty
	future.then(function(){
		retval = future.result.returnValue;
		var count = future.result.results.length;
		UnitTest.require(retval === true, "Failed to create wake activity 2");
		UnitTest.require(count === 0, "Failed to save state properly");

		future.result = UnitTest.passed;
	});

	future.then(done);
};

//======================================================================
/*
 * Expected results of success:
 *  - An activity is created for the next autoclose time
 *
 *  Prerequisites: Result from a successful query to the reminder table
 *  Variations:
 *  - no previous activity exists, activity created - {returnValue: true, activityId: #}
 *  - previous activity exists and gets updated  - activity number does not change, activity time changes - {returnValue: true, rescheduled: true}
 *  - previous activity exists, reminder query returns no results - old activity completed, no new activity scheduled - {returnValue: true}
 *  - reminder query failed - no activity - {returnValue: false}
 *
 *  Failure cases: ActivityManager or MojoDB failure.
 */
//no previous activity exists, activity created variant
ReminderAssistantTests.prototype.testCreateAutoCloseActivity1 = function(done){
	var assistant = new ReminderAssistant();

	var reminder1 = makeDummyReminder(1);
	var reminder2 = makeDummyReminder(1, 1);
	var reminder3 = makeDummyReminder(1, 2);
	var reminder4 = makeDummyReminder(1, 3);
	var now = new Date().getTime();

	//Put some reminders in the reminder table, but don't put anything in the status table
	var future = DB.put([reminder1, reminder2, reminder3, reminder4]);

	//Query the reminder table
	future.then(function(){
		future.nest(DB.find(
				{	from	: "com.palm.service.calendar.reminders:1"
				,	where	:
					[{	prop	: "autoCloseTime"
					,	op		: ">="
					,	val		: now
					}]
				,	orderBy	: "autoCloseTime"
				,	limit	: 1
				}
		));
	});

	//TEST. Pass results to createAutoCloseActivity
	future.then(assistant, assistant.createAutoCloseActivity);

	//Verify the result
	future.then(function(){
		var retval = future.result.returnValue;
		var activityId = future.result.activityId;

		UnitTest.require(retval === true, "Failed to create autoclose activity 1");
		UnitTest.require(activityId > 0, "Failed to create autoclose activity 2");

		future.result = {activityId: activityId};

	});

	future.then(assistant, assistant.unitTestDone);

	future.then(function(){
		future.result = UnitTest.passed;
	});

	future.then(done);
};

//previous activity exists and needs to be completed/updated
// This test creates an activity that must be cancelled when the test is over.
ReminderAssistantTests.prototype.testCreateAutoCloseActivity2 = function(done){
	var assistant = new ReminderAssistant();

	var reminder1 = makeDummyReminder(1);
	var reminder2 = makeDummyReminder(1, 1);
	var reminder3 = makeDummyReminder(1, 2);
	var reminder4 = makeDummyReminder(1, 3);
	var now = new Date().getTime();

	//Put some reminders in the reminder table
	var future = DB.put([reminder1, reminder2, reminder3, reminder4]);

	//Create an activity with the activity manager
	var activityArgs =	{	start	: true
						,	activity:
							{	"name"			: "calendar.reminders.autoclose"
							,	"description"	: "Time to close a calendar reminder"
							,	"type"			:
								{	"cancellable"	: true
								,	"persist"		: true
								}
							,	"schedule"		: {"start": "2020-01-01 12:00:00"}
							,	"callback"		:
									{	"method"	: "palm://com.palm.service.calendar.reminders/onAutoClose"
									,	"params"	: {"autoCloseTime": now}
									}
							}
						};

	future.then(function(){
		future.nest(PalmCall.call("palm://com.palm.activitymanager/", "create", activityArgs));
	});

	var activityId;
	future.then(function(){
		var result = future.result;
		activityId = future.result.activityId;
		future.result = result;
	});

	//Put the activity number in the status table
	future.then(this, okStatusSetup);

	future.then(function(){
		future.nest(PalmCall.call("palm://com.palm.activitymanager/", "getDetails", {activityId: activityId}));
	});
	future.then(function(){
		var result = future.result;
		future.result = result;
	});

	//Query the reminder table
	future.then(function(){
		future.nest(DB.find(
				{	from	: "com.palm.service.calendar.reminders:1"
				,	where	:
					[{	prop	: "autoCloseTime"
					,	op		: ">="
					,	val		: now
					}]
				,	orderBy	: "autoCloseTime"
				,	limit	: 1
				}
		));
	});

	//TEST. Pass results to createAutoCloseActivity
	future.then(assistant, assistant.createAutoCloseActivity);

	//Verify the result from createAutoCloseActivity
	future.then(function(){
		var result = future.result;
		var retval = future.result.returnValue;
		var noStatusChange = future.result.noStatusChange;

		//{returnValue: true, rescheduled: true}
		UnitTest.require(retval === true, "Failed to create autoclose activity 1");
		UnitTest.require(noStatusChange === true, "Created an activity when it should have been updated");

		//TODO: CHECK THE ACTIVITY ID IN TABLE, SEE IT DIDNT' CHANGE

		future.result = result;
	});

	future.then(function(){
		future.nest(PalmCall.call("palm://com.palm.activitymanager/", "getDetails", {activityId: activityId}));
	});

	future.then(function(){
		var result = future.result;
		future.result = result;
	});

	future.then(assistant, assistant.unitTestDone);

	future.then(function(){
		future.result = UnitTest.passed;
	});

	future.then(done);
};

//reminder query returns no results - no activity
ReminderAssistantTests.prototype.testCreateAutoCloseActivity3 = function(done){
	var assistant = new ReminderAssistant();

	var now = new Date().getTime();

	//No status and no reminders in the table,  Query the reminder table
	var future = DB.find(
				{	from	: "com.palm.service.calendar.reminders:1"
				,	where	:
					[{	prop	: "autoCloseTime"
					,	op		: ">="
					,	val		: now
					}]
				,	orderBy	: "autoCloseTime"
				,	limit	: 1
				}
	);

	//TEST. Pass results to createAutoCloseActivity
	future.then(assistant, assistant.createAutoCloseActivity);

	//Verify the result from createAutoCloseActivity
	future.then(function(){
		var retval = future.result.returnValue;
		var activityId = future.result.activityId;
		var rescheduled = future.result.rescheduled;
		//{returnValue: true}
		UnitTest.require(retval === true, "Failed to create autoclose activity 1");
		UnitTest.require(activityId === undefined, "Failed to create autoclose activity 2");
		UnitTest.require(rescheduled === undefined, "Failed to create autoclose activity 3");

		future.result = UnitTest.passed;

	});

	//TODO: CHECK THE ACTIVITY ID IN TABLE, SEE THE ACTIVITY # IS ZERO

	future.then(done);
};

//reminder query fails - no activity
ReminderAssistantTests.prototype.testCreateAutoCloseActivity4 = function(done){
	var assistant = new ReminderAssistant();

	var now = new Date().getTime();

	//No status and no reminders in the table, set as though query result is false
	var future = new Foundations.Control.Future();
	future.result = {returnValue: false};

	//TEST. Pass results to createOnWakeActivity
	future.then(assistant, assistant.createAutoCloseActivity);

	//Verify the result from createOnWakeActivity
	future.then(function(){
		var retval = future.result.returnValue;
		var activityId = future.result.activityId;
		var rescheduled = future.result.rescheduled;

		UnitTest.require(retval === false, "Failed to create autoclose activity 1");
		UnitTest.require(activityId === undefined, "Created an activity when we shouldn't have");
		UnitTest.require(rescheduled === undefined, "Failed to create autoclose activity 3");

		future.result = UnitTest.passed;

	});

	future.then(done);
};

//======================================================================
/*
 * Expected results of success:
 *  - If an activity was created, then the status table is updated with the new autoclose activity id, and return status == new
 *  - If an activity was updated, then the status table remains the same and return status == updated
 *  - If no activity is scheduled, then the status table is updated with activity id == 0 and return status == none
 *  - If an error occurred in activity creation, then the status table is updated with activity id == 0 and return status == FAIL
 */
/*
 * Possible incoming responses from createOnAutoCloseActivity:
 * Old activity completed and updated
 * future.result = {returnValue: true, noStatusChange: true};
 *
 * No old activity to complete, and no new activity to schedule
 * Old activity completed, but nothing new to schedule
 * future.result = {returnValue: true};
 *
 * No old activity to complete, and new activity scheduled
 * future.result = {returnValue: true, activityId: 9};
 *
 * Failure to create
 * future.result = {returnValue: false};
 */

//Activity creation failure case: incoming {returnValue: false}
ReminderAssistantTests.prototype.testSaveAutoCloseActivityId1 = function(done){
	var assistant = new ReminderAssistant();

	var future = new Foundations.Control.Future();
	future.result = {returnValue: false};

	//TEST. Pass results to saveAutoCloseActivityId
	future.then(assistant, assistant.saveAutoCloseActivityId);

	//Verify the result from saveAutoCloseActivityId
	future.then(function(){
		var retval = future.result.returnValue;
		var status = future.result.status;
		UnitTest.require(status == "FAIL", "Wrong status");
		future.result = UnitTest.passed;
	});

	future.then(done);
};

//No old activity to complete, and new activity scheduled
//incoming: {returnValue: true, activityId: 9};
ReminderAssistantTests.prototype.testSaveAutoCloseActivityId2 = function(done){
	var assistant = new ReminderAssistant();

	var future = new Foundations.Control.Future();
	future.result = {returnValue: true, activityId: 9};

	//TEST. Pass results to saveAutoCloseActivityId
	future.then(assistant, assistant.saveAutoCloseActivityId);

	//Verify the result from saveAutoCloseActivityId
	future.then(function(){
		var retval = future.result.returnValue;
		var status = future.result.status;
		UnitTest.require(retval === true, "Failed to pass on failure status");
		UnitTest.require(status == "new", "Wrong status");
		future.result = UnitTest.passed;
	});

	future.then(done);
};

//Old activity completed, but nothing new to schedule
//Incoming: {returnValue: true};
ReminderAssistantTests.prototype.testSaveAutoCloseActivityId3 = function(done){
	var assistant = new ReminderAssistant();

	var future = new Foundations.Control.Future();
	future.result = {returnValue: true};

	//TEST. Pass results to saveAutoCloseActivityId
	future.then(assistant, assistant.saveAutoCloseActivityId);

	//Verify the result from saveAutoCloseActivityId
	future.then(function(){
		var retval = future.result.returnValue;
		var status = future.result.status;
		UnitTest.require(retval === true, "Failed to pass on failure status");
		UnitTest.require(status == "none", "Wrong status");
		future.result = UnitTest.passed;
	});

	future.then(done);
};

//Old activity completed, but nothing new to schedule
//Incoming: {returnValue: true, noStatusChange: true};
ReminderAssistantTests.prototype.testSaveAutoCloseActivityId4 = function(done){
	var assistant = new ReminderAssistant();

	var future = new Foundations.Control.Future();
	future.result = {returnValue: true, noStatusChange: true};

	//TEST. Pass results to saveAutoCloseActivityId
	future.then(assistant, assistant.saveAutoCloseActivityId);

	//Verify the result from saveAutoCloseActivityId
	future.then(function(){
		var retval = future.result.returnValue;
		var status = future.result.status;
		UnitTest.require(retval === true, "Failed to pass on failure status");
		UnitTest.require(status == "updated", "Wrong status");
		future.result = UnitTest.passed;
	});

	future.then(done);
};

//======================================================================
/*
 * Expected results of success:
 *  - An activity is created for the next reminder time and the activityId is saved in the status table
 *
 *  Prerequisites: Reminders in the reminder table
 *  Variations:
 *  - activity is created and id is saved
 *  - no activity is created
 *
 *  Failure cases: ActivityManager or MojoDB failure.
 */
//activity is created variant
// This test creates an activity that must be cancelled when the test is over.
ReminderAssistantTests.prototype.testFindNextAutoCloseTimeSubroutine1 = function(done){
	var assistant = new ReminderAssistant();
	var now = new Date().getTime();
	var originalActivityId;
	var retval;
	var count;
	var reminder1 = makeDummyReminder(1);
	var reminder2 = makeDummyReminder(1, 1);
	var reminder3 = makeDummyReminder(1, 2);
	var reminder4 = makeDummyReminder(1, 3);

	//put reminders in the table
	var future = DB.put([reminder1, reminder2, reminder3, reminder4]);

	//Create an activity with the activity manager
	var activityArgs =	{	start	: true
						,	activity:
							{	"name"			: "calendar.reminders.autoclose"
							,	"description"	: "Time to autoclose a calendar reminder"
							,	"type"			:
								{	"cancellable"	: true,
								"persist": true
								}
							,	"schedule"		: {"start": "2038-01-01 12:00:00"}
							,	"callback"		:
									{	"method"	: "palm://com.palm.service.calendar.reminders/onAutoClose"
									,	"params"	: {"autoCloseTime": now}
									}
							}
						};

	future.then(function(){
		future.nest(PalmCall.call("palm://com.palm.activitymanager/", "create", activityArgs));
	});


	//Put the activity number in the status table
	future.then(this, okStatusSetup);

	//query the status table
	future.then(function(){
		future.nest(DB.find({"from": "com.palm.service.calendar.remindersstatus:1"}));
	});

	future.then(function(){
		originalActivityId = future.result.results[0].autoCloseActivityId;
		future.result = "go";
	});

	future.then(assistant, assistant.findNextAutoCloseTimeSubroutine);

	future.then(function(){
		retval = future.result.returnValue;
		count = future.result.count;
		UnitTest.require(retval === true, "Failed to create or save autoclose activity 1");
		UnitTest.require(count === undefined, "Created an activity when we shouldn't have");

		future.nest(DB.find({"from": "com.palm.service.calendar.remindersstatus:1"}));
	});

	future.then(function(){
		var result = future.result;
		retval = future.result.returnValue;
		count = future.result.results.length;
		var activityId = future.result.results[0].autoCloseActivityId;
		UnitTest.require(retval === true, "Failed to create autoclose activity 2");
		UnitTest.require(count === 1, "Failed to save state properly");
		UnitTest.require(activityId === originalActivityId, "Created activity when we should have updated");

		future.result = result;
	});

	future.then(assistant, assistant.unitTestDone);

	future.then(function(){
		future.result = UnitTest.passed;
	});

	future.then(done);
};

//no activity is created
ReminderAssistantTests.prototype.testFindNextAutoCloseTimeSubroutine2 = function(done){
	var assistant = new ReminderAssistant();
	var retval;

	var future = new Foundations.Control.Future();

	//nothing in the status table, and no reminders
	future.now(assistant, assistant.findNextAutoCloseTimeSubroutine);

	//expect return value true, but no activityId, because nothing should have been saved
	future.then(function(){
		retval = future.result.returnValue;
		var activityId = future.result.activityId;
		UnitTest.require(retval === true, "Failed to create or save autoclose activity 1");
		UnitTest.require(activityId === undefined, "Failed to save state properly");

		future.nest(DB.find({"from": "com.palm.service.calendar.remindersstatus:1"}));
	});

	//expect return value true, but no results, because the status table should still be empty
	future.then(function(){
		retval = future.result.returnValue;
		var count = future.result.results.length;
		UnitTest.require(retval === true, "Failed to create autoclose activity 2");
		UnitTest.require(count === 0, "Failed to save state properly");

		future.result = UnitTest.passed;
	});

	future.then(done);
};

//======================================================================

/*
 * Expected results of success:
 *  - If an activity was created, then the status table is updated with the new db-changed activity id, and return status == new
 *  - If an activity was updated, then the status table remains the same and return status == updated
 *  - If no activity is scheduled, then the status table is updated with activity id == 0 and return status == none
 *  - If an error occurred in activity creation, then the status table is updated with activity id == 0 and return status == FAIL
 *  - Note than in real life, an activity should ALWAYS exist, and case #3 should never happen.
 */
/*
 * Possible incoming responses from createOnAutoCloseActivity:
 * Old activity completed and updated
 * future.result = {returnValue: true, noStatusChange: true};
 *
 * No old activity to complete, and no new activity to schedule
 * Old activity completed, but nothing new to schedule
 * future.result = {returnValue: true};
 *
 * No old activity to complete, and new activity scheduled
 * future.result = {returnValue: true, activityId: 9};
 *
 * Failure to create
 * future.result = {returnValue: false};
 */

//Activity creation failure case: incoming {returnValue: false}
ReminderAssistantTests.prototype.testSaveDBChangedActivityId1 = function(done){
	var assistant = new ReminderAssistant();

	var future = new Foundations.Control.Future();
	future.result = {returnValue: false};

	//TEST. Pass results to saveDBChangedActivityId
	future.then(assistant, assistant.saveDBChangedActivityId);

	//Verify the result from saveDBChangedActivityId
	future.then(function(){
		var retval = future.result.returnValue;
		var status = future.result.status;
		UnitTest.require(status == "FAIL", "Wrong status");
		future.result = UnitTest.passed;
	});

	future.then(done);
};

//No old activity to complete, and new activity scheduled
//incoming: {returnValue: true, activityId: 9};
ReminderAssistantTests.prototype.testSaveDBChangedActivityId2 = function(done){
	var assistant = new ReminderAssistant();

	var future = new Foundations.Control.Future();
	future.result = {returnValue: true, activityId: 9};

	//TEST. Pass results to saveDBChangedActivityId
	future.then(assistant, assistant.saveDBChangedActivityId);

	//Verify the result from saveDBChangedActivityId
	future.then(function(){
		var retval = future.result.returnValue;
		var status = future.result.status;
		UnitTest.require(retval === true, "Failed to pass on failure status");
		UnitTest.require(status == "new", "Wrong status");
		future.result = UnitTest.passed;
	});

	future.then(done);
};

//Old activity completed, but nothing new to schedule
//Incoming: {returnValue: true};
ReminderAssistantTests.prototype.testSaveDBChangedActivityId3 = function(done){
	var assistant = new ReminderAssistant();

	var future = new Foundations.Control.Future();
	future.result = {returnValue: true};

	//TEST. Pass results to saveDBChangedActivityId
	future.then(assistant, assistant.saveDBChangedActivityId);

	//Verify the result from saveDBChangedActivityId
	future.then(function(){
		var retval = future.result.returnValue;
		var status = future.result.status;
		UnitTest.require(retval === true, "Failed to pass on failure status");
		UnitTest.require(status == "none", "Wrong status");
		future.result = UnitTest.passed;
	});

	future.then(done);
};

//Old activity completed, and restarted
//Incoming: {returnValue: true, noStatusChange: true};
ReminderAssistantTests.prototype.testSaveDBChangedActivityId4 = function(done){
	var assistant = new ReminderAssistant();

	var future = new Foundations.Control.Future();
	future.result = {returnValue: true, noStatusChange: true};

	//TEST. Pass results to saveDBChangedActivityId
	future.then(assistant, assistant.saveDBChangedActivityId);

	//Verify the result from saveDBChangedActivityId
	future.then(function(){
		var retval = future.result.returnValue;
		var status = future.result.status;
		UnitTest.require(retval === true, "Failed to pass on failure status");
		UnitTest.require(status == "updated", "Wrong status");
		future.result = UnitTest.passed;
	});

	future.then(done);
};

//======================================================================
/*
 * Expected results of success:
 *  - An activity is created for the query watch and the activityId is saved in the status table
 *
 *  Failure cases: ActivityManager or MojoDB failure.
 */
// This test creates an activity that must be cancelled when the test is over.
//First run setup - no previous existing activity
ReminderAssistantTests.prototype.testDBChangedActivitySubroutine1 = function(done){
	var assistant = new ReminderAssistant();
	var retval;

	//no reminders exist. Set up the watch.
	var future = new Foundations.Control.Future();
	future.now(this, okStatusSetup);

	future.then(function(){
		var result = future.result;
		future.result = {dbChangedActivityId: 0, isUnitTest: true};
	});

	future.then(assistant, assistant.dbChangedActivitySubroutine);

	future.then(function(){
		retval = future.result.returnValue;
		var status = future.result.status;
		UnitTest.require(retval === true, "Failed to pass on failure status");
		UnitTest.require(status == "new", "Wrong status");

		future.nest(DB.find({"from": "com.palm.service.calendar.remindersstatus:1"}));
	});

	future.then(function(){
		var result = future.result;
		retval = future.result.returnValue;
		var activityId = future.result.results[0].dbChangedActivityId;
		UnitTest.require(retval === true, "Failed to create autoclose activity 2");
		UnitTest.require(activityId !== 0, "Created activity when we should have updated");

		future.result = result;
	});

	future.then(assistant, assistant.unitTestDone);

	future.then(function(){
		future.result = UnitTest.passed;
	});

	future.then(done);
};


