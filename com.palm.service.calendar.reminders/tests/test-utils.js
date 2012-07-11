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
/*global DB, Foundations, PalmCall, UnitTest, rmdrLog, require */

function clearActivities(){
	var future = PalmCall.call("palm://com.palm.activitymanager/", "list", {});
	future.then(function(){
		var activities = future.result.activities;
		var len = activities.length;
		for(var i = 0; i < len; i++){
			var activity = activities[i];
			if(activity.state == "waiting" && activity.creator == "serviceId:com.palm.service.calendar.reminders"){
				rmdrLog("cancelling: "+JSON.stringify(activity));
				PalmCall.call("palm://com.palm.activitymanager/", "cancel", {"activityId": activity.activityId});
			}
		}
		future.result = {returnValue: true};
	});
}

function okStatusSetup (future){
	var activityId = "fake";
	if(future.result && future.result.activityId){
		activityId = future.result.activityId;
	}

	var statusObject = {
		_kind				: "com.palm.service.calendar.remindersstatus:1",
		lastRevNumber		: 200,
		status				: "OK",
		wakeActivityId		: activityId,
		autoCloseActivityId	: activityId
	};
	future.nest(DB.put([statusObject]));
}

function failStatusSetup (future){
	var statusObject = {
		_kind				: "com.palm.service.calendar.remindersstatus:1",
		lastRevNumber		: 200,
		status				: "FAIL",
		wakeActivityId		: 10,
		autoCloseActivityId	: 11
	};
	future.nest(DB.put([statusObject]));
}

function reminderSetup (future){
	var date1 = new Date();
	var date2 = new Date();
	var date3 = new Date();

	date1.addDays(1);
	date1.set({millisecond: 0, second: 0, minute: 0, hour: 8});
	var reminderObject1 = {
		"_kind"			: "com.palm.service.calendar.reminders:1",
		"eventId"		: "1n7",
		"subject"		: "Tomorrow at 8am, 1 hour long, alarm 1 hour before",
		"location"		: "",
		"isAllDay"		: false,
		"startTime"		: date1.getTime(),
		"endTime"		: date1.getTime()+3600000,  //duration: 1 hour
		"alarmTime"		: date1.getTime()-3600000,  //alarm: 1 hour before
		"showTime"		: date1.getTime()-3600000,  //alarm: 1 hour before
		"autoCloseTime"	: date1.getTime()+3600000,  //autoclose: at end time
		"isRepeating"	: true
	};

	date2.addDays(1);
	date2.set({millisecond: 0, second: 0, minute: 0, hour: 12});
	var reminderObject2 = {
		"_kind"			: "com.palm.service.calendar.reminders:1",
		"eventId"		: "1nJ",
		"subject"		: "Tomorrow at noon, 5 minutes long, alarm 5 minutes before",
		"location"		: "",
		"isAllDay"		: false,
		"startTime"		: date2.getTime(),
		"endTime"		: date2.getTime()+300000,  //duration: 5 minutes
		"alarmTime"		: date2.getTime()-300000,  //alarm: 5 minutes before
		"showTime"		: date2.getTime()-300000,  //alarm: 5 minutes before
		"autoCloseTime"	: date2.getTime()+900000,  //autoclose: 15 minutes after start
		"isRepeating"	: true
	};

	future.nest(DB.put([reminderObject1, reminderObject2]));
}

function makeEvent (numDaysFromNow, repeats, hasAlarm, alarmPeriod, duration){
	var date1 = new Date();
	date1.addDays(numDaysFromNow);
	date1.set({
		millisecond: 0,
		second: 0,
		minute: 0,
		hour: 8
	});
	var rrule = {"freq": "DAILY", "interval": 1};
	var alarm = [{	"alarmTrigger": {
						"valueType": "DURATION",
						"value": "-PT"+alarmPeriod+"M"
					}
				}];

	if (duration === undefined) {
		duration = 60; //1 hour
	}

	var eventObject = {
		"_kind": "com.palm.calendarevent:1",
		"subject": "My subject",
		"location": "My location",
		"rrule": (repeats ? rrule : null),
		"dtstart": date1.getTime(),
		"dtend": date1.getTime() + (duration*60000),
		"allDay": false,
		"tzId": "America/Los_Angeles",
		"note": "My note",
		"alarm": (hasAlarm ? alarm : null),
		"calendarId": "F+Z8",
		"attendees":
		[
			{"commonName":"Moll Flanders","email":"molly@flanders.com","organizer":true},
			{"calendarUserType":"INDIVIDUAL","commonName":"Dorian Gray","email":"dgray@picture.com","role":"REQ-PARTICIPANT"}
		]
	};

	return eventObject;
}

function makeRepeatingEvent (){
	var date2 = new Date();
	date2.addDays(-2);
	date2.set({
		millisecond: 0,
		second: 0,
		minute: 0,
		hour: 8
	});

	var repeatEventObject = {
		"_kind": "com.palm.calendarevent:1",
		"subject": "Two days ago, 8am, alarm 5 minutes before, repeats daily",
		"location": "My location",
		"rrule": {
			"freq": "DAILY",
			"interval": 1
		},
		"dtstart": date2.getTime(),
		"dtend": date2.getTime() + 3600000,
		"allDay": false,
		"tzId": "America/Los_Angeles",
		"note": "My note",
		"alarm": [{
			"alarmTrigger": {
				"valueType": "DURATION",
				"value": "-PT5M"
			}
		}]
	};
	return repeatEventObject;
}

function makeReminderForEvent (event, start, end, alarm, autoclose){

	var reminderObject1 = {
		"_kind"			: "com.palm.service.calendar.reminders:1",
		"eventId"		: event._id,
		"subject"		: event.subject,
		"location"		: event.location,
		"isAllDay"		: event.allDay,
		"startTime"		: start,
		"endTime"		: end,  //duration: 1 hour
		"alarmTime"		: alarm,  //alarm: 1 hour before
		"showTime"		: alarm,  //alarm: 1 hour before
		"autoCloseTime"	: autoclose,  //autoclose: at end time
		"isRepeating"	: true
	};
	return reminderObject1;

}

function makeDummyReminder (offsetFromToday, offsetFrom8am){

	if(!offsetFrom8am){
		offsetFromToday = 0;
	}

	if(!offsetFrom8am){
		offsetFrom8am = 0;
	}

	var date1 = new Date();
	date1.addDays(offsetFromToday);
	date1.set({hour: 8, minute: 0, second: 0, millisecond: 0});
	var startTime = date1.getTime() + (offsetFrom8am * 3600000);

	var reminderObject1 = {
		"_kind"			: "com.palm.service.calendar.reminders:1"
	,	"eventId"		: "3xt"
	,	"subject"		: "Foo"
	,	"location"		: "Blah"
	,	"isAllDay"		: false
	,	"startTime"		: startTime
	,	"endTime"		: startTime + 3600000  //duration: 1 hour
	,	"alarmTime"		: startTime - 3600000  //alarm: 1 hour before
	,	"showTime"		: startTime - 3600000  //alarm: 1 hour before
	,	"autoCloseTime"	: startTime + 3600000  //autoclose: at end time
	,	"isRepeating"	: true
	};
	return reminderObject1;

}

function clearDatabase (){
	var delStatusOp = {
		method: "del",
		params: {
			query: {
				from: "com.palm.service.calendar.remindersstatus:1"
			}
		}
	};

	var delRemindersOp = {
		method: "del",
		params: {
			query: {
				from: "com.palm.service.calendar.reminders:1"
			}
		}
	};

	var delEventsOp = {
		method: "del",
		params: {
			query: {
				from: "com.palm.calendarevent:1"
			}
		}
	};
	var operations = [delStatusOp, delRemindersOp, delEventsOp];

	return DB.execute("batch", {
		operations: operations
	});
}