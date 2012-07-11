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
/*global DB, Foundations, PalmCall, rmdrLog */

/*
 * Callback for the activity manager when there are reminders to show.
 *
 * args expected:
 *		showTime: timestamp that the reminders are supposed to show
 *
 * Flow:
 * Query the reminder table for reminders that are supposed to be shown at the specified time
 * Launch the app to display the reminders
 * Schedule next wake time
 *
 * Call:
 * luna-send -n 1 palm://com.palm.service.calendar.reminders/onWake '{"showTime":1270543500000}'
 */

function WakeCommandAssistant(){
}

WakeCommandAssistant.prototype.run = function(outFuture){

	rmdrLog("********** IN ON WAKE!!!!");

	this.assistant = this.controller.service.assistant;
	var showTime = this.controller.args.showTime;

	//check our parameters
	if(showTime === undefined){
		var errorString = "Missing args! Need showTime.  Received: "+JSON.stringify(this.controller.args);
		throw errorString;
	}

	//Query for all alarms supposed to be going off now
	var reminders;
	var pastReminders;

	var future = new Foundations.Control.Future();

	future.now(this, this.findRemindersAtTime);

	future.then(this, this.checkQueryResult);

	future.then(function(){
		var result = future.result;
		reminders = result.reminders;
		var startSearchTime = showTime + 1000; //skip ahead one second, so we don't get what just woke up.
		future.result = {startSearchTime: startSearchTime};
	});

	future.then(this.assistant, this.assistant.findNextWakeTimeSubroutine);

	future.then(function(){
		var result = future.result;
		pastReminders = result.pastReminders;
		//if we found reminders in the past, we should show those too.
		if(pastReminders && pastReminders.length){
			Utils.debug("********** In onWake: appending past reminders");
			reminders = reminders.concat(pastReminders);
		}
		future.result = {reminders: reminders};
	});

	future.then(this, this.launchApp);

	future.then(function(future){
		var result = future.result;
		rmdrLog("********** In WAKE: finished");
		future.result = result;
	});

	return future;
};

WakeCommandAssistant.prototype.findRemindersAtTime = function(future){
	Utils.debug("********** In findRemindersAtTime");
	var result = future.result;
	future.nest(
		DB.find(
			{	from: "com.palm.service.calendar.reminders:1"
			,	where:
				[{	prop: "showTime"
				,	op: "="
				,	val: this.controller.args.showTime
				}]
			}
		)
	);
};

WakeCommandAssistant.prototype.checkQueryResult = function(future){
	Utils.debug("********** In checkQueryResult");
	var result = future.result;
	var returnValue = result.returnValue;

	//Reminder table query failed.
	if (returnValue === false) {
		rmdrLog("********** Reminder table query failed: "+JSON.stringify(future.result));
		future.result = {returnValue: true, reminders: []};
		return;
	}

	var reminders = future.result.results;
	var remindersLength = reminders && reminders.length;
	//Reminder query returned no results. Weird, but not a failure.
	if (remindersLength === 0) {
		future.result = {returnValue: true, reminders: []};
		return;
	}

	future.result = {returnValue: true, reminders: reminders};
};

WakeCommandAssistant.prototype.launchApp = function(future){
	var result = future.result;
	rmdrLog("********** In launchApp");
	var reminders = result.reminders;
	var remindersLength = reminders.length;

	var duplicateReminderCatcher = {};
	var remindersToShow = [];

	var reminder;
	var key;
	for(var i = 0; i < remindersLength; i++){
		reminder = reminders[i];

		//***NOV-102491 HACK***
		//We've got duplicate listings in the reminder table, indicating something bad happened.
		//Filter out duplicates until we can figure out what's causing it.
		key = "" + reminder.eventId + reminder.startTime;
		if(duplicateReminderCatcher[key] !== undefined){
			rmdrLog("********** Reminders service FOUND DUPLICATES. We are IN UTAH");
			continue;
		}
		else{
			duplicateReminderCatcher[key] = true;
		}

		//convert to strings so our times don't get truncated to 32-bit ints when going across the bus
		reminder.startTime =     reminder.startTime.toString();
		reminder.endTime =       reminder.endTime.toString();
		reminder.showTime =      reminder.showTime.toString();
		reminder.autoCloseTime = reminder.autoCloseTime.toString();
		remindersToShow.push(reminder);
		var logReminder = {
			id: reminder._id,
			eventId: reminder.eventId,
			subjectLength: reminder.subject && reminder.subject.length,
			showTime: reminder.showTime,
			startTime: reminder.startTime
		};
		rmdrLog("********** Sending to app: "+JSON.stringify(logReminder));
	}

	var remindersToShowLength = remindersToShow.length;
	rmdrLog("Sending to app: total "+remindersToShowLength+" reminders");
	if (remindersToShowLength > 0) {
		//launch the app and show reminders
		var appLaunchParams = {
			id: 'com.palm.app.calendar',
			params: {
				alarm: remindersToShow
			}
		};
		//Don't really care about the result of this future.
		var tempFuture = PalmCall.call("palm://com.palm.applicationManager", "open", appLaunchParams);
	}
	future.result = {returnValue: true};

};

WakeCommandAssistant.prototype.complete = function(activity){
	//do nothing - we're completing & restarting in the reminder assistant.
	rmdrLog("********** In WAKE: complete");
	return;
};
