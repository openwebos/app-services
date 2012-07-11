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
/*global DB, PalmCall, Foundations, utilGetUTCDateString, utilCalculateAlarmTime, Calendar, rmdrLog */

function ReminderAssistant() {
}


ReminderAssistant.prototype = {

	/*
	 * Assumes you're coming from a query on the event table, and that future.result contains events.
	 * Will calculate alarm times for each event, and add these to the reminder table.
	 */
	findReminderTimesSubroutine: function(outFuture){
		Utils.debug("********** In findReminderTimesSubroutine");
		var result = outFuture.result;
		var returnValue = result.returnValue;
		var lastRevNumber;

		//Event table query failed.  Pass on the result.
		if (returnValue === false || outFuture.exception) {
			rmdrLog("********** In findReminderTimesSubroutine: Event table query failed ");
			return new Foundations.Control.Future(result);
		}

		//No events? Weird, but not catastrophic... exit quietly
		var events = result.results;
		var eventsLength = events && events.length;
		if(!eventsLength){
			rmdrLog("********** In findReminderTimesSubroutine: NO results");
			//return new Foundations.Control.Future({returnValue: true, status: "NO_RESULTS"});
			return new Foundations.Control.Future({returnValue: true});
		}

		//recycle the result so we can pass it into findReminderTimes.
		var future = new Foundations.Control.Future(
			{	returnValue: result.returnValue
			,	results: result.results
			,	skipPastStartTime: result.skipPastStartTime
			,	eventDismissed:	result.eventDismissed
			,	lastRevNumber: result.lastRevNumber
			}
		);

		future.then(this, this.findReminderTimes);

		future.then(function(){
			var result = future.result;
			lastRevNumber = result.lastRevNumber;
			future.result = result;
		});

		//Add reminders to the reminder table
		future.then(this, this.addRemindersToTable);

		future.then(
			function(){
				var result = future.result;
				Utils.debug("********** Completed findReminderTimesSubroutine");
				future.result = {returnValue: true, lastRevNumber: lastRevNumber, status: "OK"};
			},
			function(){
				//If the db.put in the previous step failed, some reminders were lost.
				//Mark the status as failed and on the next init or DB-Changed, we'll start over
				var exception = future.exception;
				rmdrLog("********** ERROR: New reminders were lost!");
				var tempFuture = DB.merge({"from": "com.palm.service.calendar.remindersstatus:1"}, {"status": "FAIL"});
				future.result = {returnValue: true, lastRevNumber: lastRevNumber, status: "FAIL"};
			}
		);
		return future;
	},

	addRemindersToTable: function(outFuture){
		rmdrLog("********** In addRemindersToTable");
		var result = outFuture.result;
		var reminderList = result.reminderList;
		if (reminderList && reminderList.length) {
			return DB.put(reminderList);
		}
		else {
			return new Foundations.Control.Future({returnValue: true});
		}
	},

	findDisplayAlarm: function(eventAlarm){
		var displayAlarmIndex = null;
		var numAlarms = eventAlarm && eventAlarm.length;
		for(var i = 0; i < numAlarms; i++){
			//if it's not a 'display' alarm, skip it
			if(eventAlarm[i].action && eventAlarm[i].action.toLowerCase() != "display"){
				continue;
			}
			if(eventAlarm[i].alarmTrigger &&
				eventAlarm[i].alarmTrigger.valueType == "DURATION" &&
				eventAlarm[i].alarmTrigger.value &&
				eventAlarm[i].alarmTrigger.value.toLowerCase() != "none") {
				return i;
			}
		}
		return displayAlarmIndex;
	},

	findReminderTimes: function(outFuture){
		rmdrLog("********** In findReminderTimes: lastRevNumber: "+outFuture.result.lastRevNumber);
		var result = outFuture.result;
		var events = result && result.results;
		var revNumber = result.lastRevNumber || 0;
		var skipPastStartTime = result.skipPastStartTime;
		var eventDismissed = result.eventDismissed;

		var reminderList = [];
		var now = new Date().getTime();
		var startTime;
		var isRepeatEvent;
		var alarmTime;
		var showTime;
		var autoCloseTime;
		var duration;
		var event;
		var displayAlarmIndex;

		var eventsLength = events && events.length;
		var eventManager = new Calendar.EventManager();

		if(eventDismissed && skipPastStartTime){
			now = skipPastStartTime+1000;
		}

		var future = eventManager.findNextOccurrenceTimezonePrep(events);

		future.then(function(){
			var result = future.result;
			Utils.debug("*************** findReminderTimes: timezone manager prepped");
			future.result = result;
		});

		future.then(this, function(){
			//for each event with an alarm, find the next alarm time
			rmdrLog("********** In findReminderTimes: events to process: "+eventsLength);
			for (var i = 0; i < eventsLength; i++) {
				event = events[i];
				if (event._rev > revNumber) {
					revNumber = event._rev;
				}

				if(!event.alarm){
					continue;
				}
				if(event.alarm.length === 0){
					continue;
				}

				displayAlarmIndex = this.findDisplayAlarm(event.alarm);
				if(displayAlarmIndex === null){
					continue;
				}

				//If the event is already over, and it doesn't have an rrule, skip it.
				if (event.dtend < now && !event.rrule) {
					continue;
				}

				if(eventDismissed && !event.rrule){
					continue;
				}

				//If the event does have an rrule, but the rrule is ended, skip it.
				if (event.rrule && event.rrule.freq && event.rrule.until && event.rrule.until < now) {
					continue;
				}

				//If you have an rrule, and it repeats forever, or the repeat-until hasn't expired yet, find start time
				if ((event.rrule && event.rrule.freq) && (!event.rrule.until || (event.rrule.until && event.rrule.until > now))) {
					//this will return the next occurrence that spans into now.  But we need the next occurrence that starts after now.
					Utils.debug("********** In findReminderTimes: repeating event: (start, end): ("+event.dtstart + ", "+event.dtend+")");
					Utils.debug("********** In findReminderTimes: rrule: "+JSON.stringify(event.rrule));
					Utils.debug("********** In findReminderTimes: alarm: "+JSON.stringify(event.alarm[displayAlarmIndex].alarmTrigger));
					startTime = eventManager.findNextOccurrence(event, now);
					rmdrLog("********** In findReminderTimes: next occurrence: "+startTime);
					isRepeatEvent = true;
					if (startTime === false) {
						continue;
					}
				}
				//otherwise, it's a single event.
				else {
					startTime = event.dtstart;
					isRepeatEvent = false;
				}

				//find the alarm time.
				alarmTime = utilCalculateAlarmTime(startTime, event.alarm[displayAlarmIndex]);
				showTime = (alarmTime < now && startTime >= now) ? startTime : alarmTime;

				rmdrLog("********** In findReminderTimes: next alarm time: "+showTime + " / " + new Date(showTime));

				//Keep it open at least 15 minutes ~ 900000ms
				duration = event.dtend - event.dtstart;
				autoCloseTime = (duration < 900000) ? (startTime+900000) : (startTime+duration);

				var shouldBeShowingNow = (!eventDismissed && now >= showTime && now <= autoCloseTime);

				//If the alarm time is in the future, add it to the list
				if (showTime >= now || shouldBeShowingNow) {
					Utils.debug("********** In findReminderTimes: pushing event "+i);
					reminderList.push({
						_kind			: "com.palm.service.calendar.reminders:1",
						eventId			: event._id,
						subject			: event.subject,
						location		: event.location,
						isAllDay		: event.allDay,
						attendees		: event.attendees,
						calendarId		: event.calendarId,
						startTime		: startTime,
						endTime			: startTime + duration,
						alarmTime		: alarmTime,
						showTime		: showTime,
						autoCloseTime	: autoCloseTime,
						isRepeating		: isRepeatEvent
					});
				}
			}

			future.result = {reminderList: reminderList, lastRevNumber: revNumber};
		});

		return future;
	},

	//================================================================================
	/*
	 * Requires activityId and isUnitTest set on the future.
	 * Will set up a query watch activity on the reminder table
	 */
	dbChangedActivitySubroutine: function(outFuture){
		var result = outFuture.result;
		rmdrLog("********** In dbChangedActivitySubroutine, lastRevNumber: "+result.lastRevNumber);
		var activityId = result.dbChangedActivityId;
		var isUnitTest = result.isUnitTest;
		var lastRevNumber = result.lastRevNumber;
		var activityProps = this.getDBWatchActivityProps(activityId, isUnitTest, lastRevNumber);
		var future = PalmCall.call("palm://com.palm.activitymanager", activityProps.method, activityProps.activityArgs);

		future.then(this, this.saveDBChangedActivityId);

		return future;
	},

	getDBWatchActivityProps: function(activityId, isUnitTest, lastRevNumber){
		rmdrLog("********** In getDBWatchActivityProps: activityId: "+activityId+ ", revNumber: "+lastRevNumber);
		var method;
		var activityArgs;

		var eventQuery =	{	from	:	"com.palm.calendarevent:1"
							,	where	:	[ {	prop: "_rev", op: ">", val: lastRevNumber } ]
							,   incDel	:	true
							};

		if(activityId && !isUnitTest){
			method = "complete";
			activityArgs =	{	"activityId": activityId
							,	"restart":true
							,	"trigger"		:
								{	"key"	: "fired"
								,	"method": "palm://com.palm.db/watch"
								,	"params": {"query" : eventQuery}
								}
							};
		}
		else{
			method = "create";
			activityArgs = {	start	: true
							,	replace : true
							,	activity:
								{	"name"			: "calendar.reminders.dbchanged"
								,	"description"	: "Calendar database changed, check for new reminders"
								,	"callback"		:
									{	"method" : "palm://com.palm.service.calendar.reminders/onDBChanged"
									,	"params" : {"unittest": isUnitTest}
									}
								,	"trigger"		:
									{	"key"	: "fired"
									,	"method": "palm://com.palm.db/watch"
									,	"params": {"query" : eventQuery}
									}
								,	"type"			:
									{	"immediate"	: true
									,	"priority"	: "low"
									,	"persist"	: true
									}
								}
							};
		}

		return {method: method, activityArgs: activityArgs};
	},

	saveDBChangedActivityId: function(outFuture){
		Utils.debug("********** In saveDBChangedActivityId");
		var result = outFuture.result;
		var status;
		if(result.activityId){
			Utils.debug("********** In saveDBChangedActivityId: db-changed activity id: "+result.activityId);
		}

		var future;
		if(result.returnValue === true && result.activityId){
			status = "created";
			future = DB.merge({"from": "com.palm.service.calendar.remindersstatus:1"}, {"dbChangedActivityId": result.activityId});
		}
		//If activitymanager/complete was used, we will not have an activity id, and our activity # doesn't need to change.
		else if(result.returnValue === true && !result.activityId){
			status = "updated";
			future = new Foundations.Control.Future(result);
		}
		else {
			status = "failed";
			future = DB.merge({"from": "com.palm.service.calendar.remindersstatus:1"},{"dbChangedActivityId": 0});
		}

		future.then(function(){
			result = future.result;//result of merge or same as it was before
			future.result = {returnValue: true, status: status, result: result};
		});

		return future;
	},
	//================================================================================

	/*
	 * Doesn't care where you came from.
	 * Will query reminder table for next earliest alarm time, and schedule a wake activity with activity manager.
	 */
	findNextWakeTimeSubroutine: function(outFuture){

		rmdrLog("********** In findNextWakeTimeSubroutine");
		var result = outFuture.result;

		var date = new Date();
		date.setMilliseconds(0);
		var now = date.getTime();

		var fromOnWake = false;
		var hasPastReminders = false;
		var pastReminders;

		var startSearchTime;
		var query;

		//If we have a startSearchTime, we are coming from onWake.  It's possible the startSearchTime is significantly in the past,
		//and our next wake time is also in the past, causing a rapid refire of the onWake activity.  To avoid that, when using
		//startSearchTime, try to find if other reminders should also have been shown by now, and only schedule wakeActivities that
		//are in the future.
		if(result.startSearchTime){
			Utils.debug("********** In findNextWakeTimeSubroutine: using search time: "+result.startSearchTime);
			startSearchTime = result.startSearchTime;
			fromOnWake = true;
			query = {	from	: "com.palm.service.calendar.reminders:1"
					,	where	:
						[{	prop	: "showTime"
						,	op		: ">="
						,	val		: startSearchTime
						}]
					,	orderBy	: "showTime"
					};
		}
		else{
			startSearchTime = now;
			query = {	from	: "com.palm.service.calendar.reminders:1"
					,	where	:
						[{	prop	: "showTime"
						,	op		: ">="
						,	val		: startSearchTime
						}]
					,	orderBy	: "showTime"
					,	limit	: 1
					};

		}

		var future = DB.find(query);

		future.then(
			function(){
				Utils.debug("********** In findNextWakeTimeSubroutine: Reminder table query succeeded");

				var result = future.result;
				var results = result.results;
				var newResults = [];
				var resultsLength = results && results.length;
				//look for reminders in the past, and set them aside for now. We will send them to the app to be shown.
				//It's possible that there will be no corresponding autoclose activity for these reminders,
				//but since finding past reminders indicates we were already in some anomolous state, that's probably okay.
				if(fromOnWake && resultsLength){
					var i = 0;
					pastReminders = [];
					while(i < resultsLength && results[i].showTime <= now){
						pastReminders.push(results[i]);
						Utils.debug("********** In findNextWakeTimeSubroutine: found past reminders");
						hasPastReminders = true;
						i++;
					}

					//Set up the result so it's the next alarm from now
					if(i < resultsLength){
						Utils.debug("********** In findNextWakeTimeSubroutine: skipping ahead to "+results[i].showTime);
						newResults.push(results[i]);
					}
					result.results = newResults;
				}

				future.result = result;
			},
			function(){
				var result = future.result;
				var returnValue = result.returnValue;
				if (returnValue === false || future.exception) {
					rmdrLog("********** In findNextWakeTimeSubroutine: Reminder table query failed");
					//Although this is an error, we don't want to obliterate the table or update our existing wake activity.
					future.result = {returnValue: false, dbError: true};
				}
			}
		);

		future.then(this, this.createOnWakeActivity);

		future.then(this, this.saveWakeActivityId);

		future.then(function(){
			var result = future.result;
			Utils.debug("********** In findNextWakeTimeSubroutine: after saveWakeActivityId");
			if (hasPastReminders) {
				Utils.debug("********** In findNextWakeTimeSubroutine: passing on past reminders");
				result = future.result;
				result.pastReminders = pastReminders;
				future.result = result;
			}
			else{
				future.result = result;
			}
		});

		return future;
	},

	/*
	 * @function createOnWakeActivity
	 * Assumes you're coming from a query to the reminder table, and that future.result contains reminders
	 *
	 * Sample response JSON for sucessful activity creation:  {"activityId": 1, "returnValue": true}
	 * Sample response JSON when activity fires : {"activityId": 1, "event": "start", "returnValue": true}
	 */
	createOnWakeActivity: function(outFuture){
		Utils.debug("********** In createOnWakeActivity");
		var result = outFuture.result;
		var returnValue = result.returnValue;
		var dbError = result.dbError;
		var activityId;
		var reminders;
		var remindersLength;
		var statusAction = "none";

		if (returnValue === false || dbError) {
			rmdrLog("********** In createOnWakeActivity: Reminder table query failed");
			//Although this is an error, we don't want to obliterate the table.
			return new Foundations.Control.Future(result);
		}

		reminders = result.results;
		remindersLength = reminders && reminders.length;

		//Nate says we should "complete and update" the activity.
		var future = DB.find({"from": "com.palm.service.calendar.remindersstatus:1"});

		future.then(function(){
			result = future.result;
			if (result.returnValue === true && result.results.length > 0) {
				activityId = result.results[0].wakeActivityId;
			}
			//If status query failed, we won't get an activityId, and we'll create an activity.
			//If the activity already existed, we've specified create with replace, so it should just overwrite it.

			var activityArgs;
			var activityManagerMethod;

			if (!remindersLength){
				if(activityId){
					rmdrLog("********** In createOnWakeActivity: no reminders, preexisting activity");
					//Have an old activity to complete, and no new activity to schedule
					statusAction = "completed";
					activityManagerMethod = "complete";
					activityArgs = {
						"activityId": activityId
					};
				}
				else{
					//do nothing
					statusAction = "none";
					rmdrLog("********** In createOnWakeActivity: no reminders, no preexisting activity");
				}
			}
			else {

				var date = reminders[0].showTime;
				var dateString = utilGetUTCDateString(date);
				rmdrLog("********** In createOnWakeActivity: next wake: "+ dateString + " / "+date + " / "+reminders[0]._id);

				if (activityId) {
					//Have an old activity to reschedule
					statusAction = "updated";
					activityManagerMethod = "complete";
					activityArgs = {	"activityId": activityId
									,	"restart":true
									,	"schedule":	{ "start": dateString }
									,	"callback":
										{	"method": "palm://com.palm.service.calendar.reminders/onWake"
										,	"params": {	"showTime": date}
										}
									};
				}
				else {
					//No old activity to reschedule, so make a new one
					statusAction = "created";
					activityManagerMethod = "create";
					activityArgs = {	"start"		: true
									,	"replace"	: true
									,	"activity"	:
										{	"name"			: "calendar.reminders.wake"
										,	"description"	: "Time to show a calendar reminder"
										,	"type"			:
											{	"persist"		: true
											,	"foreground"	: true
											}
										,	"schedule"	: {"start": dateString}
										,	"callback"	:
											{	"method"	: "palm://com.palm.service.calendar.reminders/onWake"
											,	"params"	: {"showTime": date}
											}
										}
									};
				}
			}

			if(activityManagerMethod && activityArgs){
				Utils.debug("********** In createOnWakeActivity: "+activityManagerMethod);
				future.nest(PalmCall.call("palm://com.palm.activitymanager/", activityManagerMethod, activityArgs));
			}
			else{
				future.result = {returnValue: true};
			}

		});

		future.then(function(){
			var result = future.result;
			Utils.debug("********** In createOnWakeActivity done");
			future.result = {returnValue: result.returnValue, statusAction: statusAction, result: result};
		});

		return future;
	},

	saveWakeActivityId: function(outFuture){
		Utils.debug("********** In saveWakeActivityId");
		var result = outFuture.result;
		var returnValue = result.returnValue;
		var dbError = result.dbError;

		//db error earlier
		if (dbError) {
			rmdrLog("********** In saveWakeActivityId: Reminder table query failed");
			//Although this is an error, we don't want to obliterate the status.
			return new Foundations.Control.Future(result);
		}

		var future;
		var statusAction = result.statusAction;
		//ReturnValue is false but no dbError means the call to activity manager failed.
		//Either we didn't complete or we didn't create.
		if(returnValue === false){
			//If we failed to create, we should mark that we have no activity.
			//If we failed to complete, future attempts to complete will probably also fail. We
			//should probably try to overwrite the activity next time, so make it look like we have no activity.
			statusAction = "failed";
			rmdrLog("********** In saveWakeActivityId: Activity manager call failed to "+statusAction);
			future = DB.merge({"from": "com.palm.service.calendar.remindersstatus:1"}, {"wakeActivityId": 0});
		}

		else {
			if(statusAction === "none"){
				rmdrLog("********** In saveWakeActivityId: no action");
				future = DB.merge({"from": "com.palm.service.calendar.remindersstatus:1"}, {"wakeActivityId": 0});
			}
			else if(statusAction === "completed"){
				rmdrLog("********** In saveWakeActivityId: completed");
				future = DB.merge({"from": "com.palm.service.calendar.remindersstatus:1"}, {"wakeActivityId": 0});
			}
			else if(statusAction === "created"){
				rmdrLog("********** In saveWakeActivityId: new");
				future = DB.merge({"from": "com.palm.service.calendar.remindersstatus:1"}, {"wakeActivityId": result.result.activityId});
			}
			else if(statusAction === "updated"){
				rmdrLog("********** In saveWakeActivityId: updated");
				//no change required for the status - activityId is the same
				future = new Foundations.Control.Future({returnValue: true});
			}
		}

		future.then(function(){
			Utils.debug("********** In saveWakeActivityId done");
			result = future.result;//result of merge or same as it was before
			future.result = {returnValue: result.returnValue, status: statusAction};
		});

		return future;
	},

	//================================================================================

	findNextAutoCloseTimeSubroutine: function(outFuture){
		Utils.debug("********** In findNextAutoCloseTimeSubroutine");
		var result = outFuture.result;
		var date = new Date();
		date.setMilliseconds(0);
		var now = date.getTime();
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

		future.then(
			function(){
				var result = future.result;
				Utils.debug("********** In findNextAutoCloseTimeSubroutine: Reminder table query succeeded");
				future.result = result;
			},
			function(){
				var result = future.result;
				var returnValue = result.returnValue;
				if (returnValue === false || future.exception) {
					rmdrLog("********** In findNextAutoCloseTimeSubroutine: Reminder table query failed");
					//Although this is an error, we don't want to obliterate the table or update our existing wake activity.
					future.result = {returnValue: false, dbError: true};
				}
			}
		);
		future.then(this, this.createAutoCloseActivity);

		future.then(this, this.saveAutoCloseActivityId);

		return future;
	},

	createAutoCloseActivity: function(outFuture){

		rmdrLog("********** In createAutoCloseActivity");
		var result = outFuture.result;
		var returnValue = result.returnValue;
		var dbError = result.dbError;
		var activityId;
		var reminders;
		var remindersLength;
		var statusAction = "none";

		if (returnValue === false || dbError) {
			rmdrLog("********** In createAutoCloseActivity: Reminder table query failed");
			//Although this is an error, we don't want to obliterate the table.
			return new Foundations.Control.Future(result);
		}

		reminders = result.results;
		remindersLength = reminders && reminders.length;

		//Nate says we should "complete and update" the activity.
		var future = DB.find({"from": "com.palm.service.calendar.remindersstatus:1"});

		future.then(function(){
			result = future.result;
			//If status query failed, we won't get an activityId, and we'll create an activity.
			//If the activity already existed, we've specified create with replace, so it should just overwrite it.
			if (future.result.returnValue === true && future.result.results.length > 0) {
				activityId = future.result.results[0].autoCloseActivityId;
			}

			var activityArgs;
			var activityManagerMethod;

			if (!remindersLength){
				if(activityId){
					Utils.debug("********** In createOnWakeActivity: no reminders, preexisting activity");
					//Have an old activity to complete, and no new activity to schedule
					statusAction = "completed";
					activityManagerMethod = "complete";
					activityArgs = {
						"activityId": activityId
					};
				}
				else{
					//do nothing
					statusAction = "none";
					rmdrLog("********** In createOnWakeActivity: no reminders, no preexisting activity");
				}
			}
			else{
				var date = reminders[0].autoCloseTime;
				var dateString = utilGetUTCDateString(date);
				rmdrLog("********** In createAutoCloseActivity. Next close: "+ dateString + " / "+date + " / "+reminders[0]._id);

				if(activityId){
					statusAction = "updated";
					activityManagerMethod = "complete";
					activityArgs = {	"activityId": activityId
									,	"restart":true
									,	"schedule":	{ "start": dateString }
									,	"callback":
										{	"method": "palm://com.palm.service.calendar.reminders/onAutoClose"
										,	"params": {	"autoCloseTime": date}
										}
									};
				}
				else{
					statusAction = "created";
					activityManagerMethod = "create";
					activityArgs =	{	"start"		: true
									,	"replace"	: true
									,	"activity"	:
										{	"name"			: "calendar.reminders.autoclose"
										,	"description"	: "Time to autoclose a calendar reminder"
										,	"type"			:
											{	"persist"		: true
											,	"foreground"	: true
											}
										,	"schedule"		: {"start": dateString}
										,	"callback"		:
											{	"method"	: "palm://com.palm.service.calendar.reminders/onAutoClose"
											,	"params"	: {"autoCloseTime": date}
											}
										}
									};
				}
			}

			if(activityManagerMethod && activityArgs){
				future.nest(PalmCall.call("palm://com.palm.activitymanager/", activityManagerMethod, activityArgs));
			}
			else{
				future.result = {returnValue: true};
			}
		});

		future.then(function(){
			Utils.debug("********** In createAutoCloseActivity: "+statusAction);
			var result = future.result;
			future.result = {returnValue: result.returnValue, statusAction: statusAction, result: result};
		});

		return future;
	},

	saveAutoCloseActivityId: function(outFuture){
		Utils.debug("********** In saveAutoCloseActivityId");
		var result = outFuture.result;
		var returnValue = result.returnValue;
		var dbError = result.dbError;

		//db error earlier
		if (dbError) {
			rmdrLog("********** In saveAutoCloseActivityId: Reminder table query failed");
			//Although this is an error, we don't want to obliterate the status.
			return new Foundations.Control.Future(result);
		}

		var future;
		var statusAction = result.statusAction;
		//ReturnValue is false but no dbError means the call to activity manager failed.
		//Either we didn't complete or we didn't create.
		if(returnValue === false){
			//If we failed to create, we should mark that we have no activity.
			//If we failed to complete, future attempts to complete will probably also fail. We
			//should probably try to overwrite the activity next time, so make it look like we have no activity.
			statusAction = "failed";
			rmdrLog("********** In saveAutoCloseActivityId: Activity manager call failed to "+statusAction);
			future = DB.merge({"from": "com.palm.service.calendar.remindersstatus:1"}, {"autoCloseActivityId": 0});
		}

		else {
			if(statusAction === "none"){
				rmdrLog("********** In saveAutoCloseActivityId: no action");
				future = DB.merge({"from": "com.palm.service.calendar.remindersstatus:1"}, {"autoCloseActivityId": 0});
			}
			else if(statusAction === "completed"){
				rmdrLog("********** In saveAutoCloseActivityId: completed");
				future = DB.merge({"from": "com.palm.service.calendar.remindersstatus:1"}, {"autoCloseActivityId": 0});
			}
			else if(statusAction === "created"){
				rmdrLog("********** In saveAutoCloseActivityId: new");
				future = DB.merge({"from": "com.palm.service.calendar.remindersstatus:1"}, {"autoCloseActivityId": result.result.activityId});
			}
			else if(statusAction === "updated"){
				rmdrLog("********** In saveAutoCloseActivityId: updated");
				//no change required for the status - activityId is the same
				future = new Foundations.Control.Future({returnValue: true});
			}
		}

		future.then(function(){
			Utils.debug("********** In saveAutoCloseActivityId done");
			result = future.result;//result of merge or same as it was before
			future.result = {returnValue: result.returnValue, status: statusAction};
		});

		return future;
	},

	//================================================================================

	//Checks the reminderStatus table for the last exit code and revision number.
	//If bad things happened last time, it will blow the table away, and we will start over.
	//Otherwise, set the lastRevNumber
	prepareForTakeoff: function (outFuture){

		var result = outFuture.result;
		var returnValue = result.returnValue;
		var queryResult = result.results;
		var queryResultLength = queryResult && queryResult.length;

		if (returnValue && queryResultLength > 0 && queryResult[0].status == "OK") {
			Utils.debug("********** In prepare for takeoff: getting last rev #: "+JSON.stringify(queryResult));
			return new Foundations.Control.Future(
				{	returnValue: true
				,	lastRevNumber: queryResult[0].lastRevNumber
				,	dbChangedActivityId: queryResult[0].dbChangedActivityId
				});

		}
		else if(returnValue && queryResultLength === 0){
			rmdrLog("********** In prepare for takeoff: first run");
			return new Foundations.Control.Future({returnValue: true, lastRevNumber: 0, dbChangedActivityId: 0});
		}
		else{
			rmdrLog("********** In prepare for takeoff: starting over!");
			var future = new Foundations.Control.Future(result);
			//If we're here, we've already failed once.  If this fails, then we're doomed.
			future.then(this, this.startOver);

			future.then(function(){
				var result = future.result;
				rmdrLog("********** In prepare for takeoff: starting over complete");
				future.result = {returnValue: true, lastRevNumber: 0, dbChangedActivityId: 0};
			});

			return future;
		}
	},

	//NOBODY IS CALLING THIS RIGHT NOW EXCEPT UNIT TESTS
	//Deletes the old status from the reminderStatus table, and replaces it with a failure status
	emergencyLanding: function (outFuture){
		rmdrLog("********** In emergency landing");
		var result = outFuture.result;
		var delRemindersOp = {
			method: "del",
			params: {
				query: { from: "com.palm.service.calendar.reminders:1" }
			}
		};

		var mergeReminderStatusOp = {
			method: "merge",
			params: {
				query: { from: "com.palm.service.calendar.remindersstatus:1" }
			,	props: {lastRevNumber: 0, status: "FAIL"}
			}
		};

		var operations = [delRemindersOp, mergeReminderStatusOp];

		//batch request our operations
		return DB.execute("batch", {operations: operations});
	},

	//Deletes the old status from the reminderStatus table, and replaces it with a success status
	//SHOULD ONLY BE CALLED FROM INIT
	setStatusAndRev: function(outFuture){
		var result = outFuture.result;
		var lastRevNumber = result.lastRevNumber || 0;
		var status = result.status || "OK";
		Utils.debug("********** In setStatusAndRev: lastRev: "+lastRevNumber);
		var delStatusOp = {
			method: "del",
			params: {
				query: {
					from: "com.palm.service.calendar.remindersstatus:1"
				}
			}
		};

		var setStatusOp = {
			method: "put",
			params: {
				objects: [{
					_kind: "com.palm.service.calendar.remindersstatus:1",
					lastRevNumber: lastRevNumber,
					status: status
				}]
			}
		};
		var operations = [delStatusOp, setStatusOp];

		//batch request our operations
		return DB.execute("batch", {operations: operations});
	},

	//Merges the lastRevNumber into the status
	//SHOULD ONLY BE CALLED BY ON-DB-CHANGED
	saveLastRevNumber: function(outFuture){
		Utils.debug("********** In saveLastRevNumber"+outFuture.result.lastRevNumber);
		var result = outFuture.result;
		var lastRevNumber = result.lastRevNumber;
		var status = result.status;
		//0 is an acceptable lastRevNumber
		if(lastRevNumber !== undefined){
			return DB.merge({"from": "com.palm.service.calendar.remindersstatus:1"}, {"lastRevNumber": lastRevNumber, "status": status});
		}
		else{
			return new Foundations.Control.Future({returnValue: true});
		}

	},

	//Deletes the contents of the reminderStatus table, and the contents of the reminder table.
	startOver: function(outFuture){
		rmdrLog("********** In start over");
		var result = outFuture.result;
		var results = result.results;

		//save the activity ids
		var wakeActivityId = results && results.wakeActivityId;
		var autoCloseActivityId = results && results.autoCloseActivityId;
		var dbChangedActivityId = results && results.dbChangedActivityId;

		var delStatusOp = {
			method: "del",
			params: {
				query: { from: "com.palm.service.calendar.remindersstatus:1" }
			}
		};

		var delRemindersOp = {
			method: "del",
			params: {
				query: { from: "com.palm.service.calendar.reminders:1" }
			}
		};

		var operations = [delStatusOp, delRemindersOp];
		var future = DB.execute("batch", {operations: operations});
		var allResults = [];
		if(wakeActivityId){
			Utils.debug("********** cancelling wake activity. previous result: "+JSON.stringify(future.result));
			future.then(function(){
				//TODO: if the batch delete failed, we're doomed.
				var result = future.result;
				allResults.push(result);
				future.nest(PalmCall.call("palm://com.palm.activitymanager", "cancel", {"activityId": wakeActivityId}));
			});
		}

		if(autoCloseActivityId){
			Utils.debug("********** cancelling autoclose activity. previous result: "+JSON.stringify(future.result));
			future.then(function(){
				var result = future.result;
				allResults.push(result);
				future.nest(PalmCall.call("palm://com.palm.activitymanager", "cancel", {"activityId": autoCloseActivityId}));
			});
		}

		if(dbChangedActivityId){
			Utils.debug("********** cancelling db changed activity. previous result: "+JSON.stringify(future.result));
			future.then(function(){
				var result = future.result;
				allResults.push(result);
				future.nest(PalmCall.call("palm://com.palm.activitymanager", "cancel", {"activityId": dbChangedActivityId}));
			});
		}

		future.then(function(){
			Utils.debug("********** finishing starting over. previous result: "+JSON.stringify(future.result));
			var result = future.result;
			allResults.push(result);
			future.result = {returnValue: true, startedOver: true, allResults: allResults};
		});
		return future;
	},

	unitTestDone: function(outFuture){
		rmdrLog("=========== IN UNIT TEST DONE");
		var result = outFuture.result;
		var activityId = result.activityId;
		if(activityId){
			return PalmCall.call("palm://com.palm.activitymanager/", "cancel", {"activityId": activityId});
		}

		var wakeActivityId;
		var autoCloseActivityId;
		var dbChangedActivityId;
		var future = DB.find({"from": "com.palm.service.calendar.remindersstatus:1"});

		future.then(function(){
			wakeActivityId = future.result.results[0].wakeActivityId;
			autoCloseActivityId = future.result.results[0].autoCloseActivityId;
			dbChangedActivityId = future.result.results[0].dbChangedActivityId;
			if (wakeActivityId && wakeActivityId != "fake") {
				future.nest(PalmCall.call("palm://com.palm.activitymanager/", "cancel", {"activityId": wakeActivityId}));
			}
			else{
				future.result = {returnValue: true};
			}
		});

		future.then(function(){
			if(autoCloseActivityId && autoCloseActivityId != wakeActivityId && autoCloseActivityId != "fake"){
				future.nest(PalmCall.call("palm://com.palm.activitymanager/", "cancel", {"activityId": autoCloseActivityId}));
			}
			else{
				future.result = {returnValue: true};
			}
		});

		future.then(function(){
			if(dbChangedActivityId){
				future.nest(PalmCall.call("palm://com.palm.activitymanager/", "cancel", {"activityId": dbChangedActivityId}));
			}
			else{
				future.result = {returnValue: true};
			}
		});

		return future;
	}

};
