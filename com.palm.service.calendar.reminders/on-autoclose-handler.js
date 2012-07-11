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

function AutoCloseCommandAssistant(){
}


/* This is called by the activity manager to tell us that one or more reminders need to be auto-closed at a certain time.
 * args expected:
 *		autoCloseTime: timestamp indicating the time to autoclose a reminder
 *
 * Flow:
 * Query for all reminders with the specified autoclose time
 * Launch the app to close the reminders
 * Query for the events we just closed
 * Find reminder times & autoclose times for the events
 * Schedule next autoclose event
 *
 * Call:
 * luna-send -n 1 palm://com.palm.service.calendar.reminders/onAutoClose '{"autoCloseTime": 1270530000000}'
 */

AutoCloseCommandAssistant.prototype.run = function(outFuture){

	rmdrLog("********** In AUTOCLOSE ");

	this.assistant = this.controller.service.assistant;
	var args = this.controller.args;
	var autoCloseTime =  this.controller.args.autoCloseTime;
	var future;

	//check our parameters
	if(!autoCloseTime){
		rmdrLog("********** In AutoClose: no auto close time");
		var errorString = "Missing args! Need autoCloseTime.  Received: "+JSON.stringify(args);
		throw errorString;
	}

	//find the reminders we need to close
	future = DB.find({
		from: "com.palm.service.calendar.reminders:1",
		where: [{
			prop: "autoCloseTime",
			op: "=",
			val: autoCloseTime
		}]
	});

	//launch the app to close reminders, get list of eventIds and reminderIds back
	future.then(this, this.autoCloseReminders);

	//save eventIds and reminderIds, we'll need them in a bit
	var eventIds;
	var reminderIds;
	future.then(function(){
		var result = future.result;
		var returnValue = result.returnValue;
		eventIds = future.result.eventIds || [];
		reminderIds = future.result.reminderIds || [];
		future.result = {returnValue: returnValue, reminderIds: reminderIds};
	});

	//remove reminders using reminderIds
	future.then(this, this.removeReminders);

	//make sure the delete succeeded, and pass the eventIds on
	future.then(
		function(){
			var result = future.result;
			future.result = {returnValue: true, eventIds: eventIds};
		},
		function(){
			//If the db.del in the previous step failed, some reminders may be duplicated or will become zombie reminders in the table.
			//Mark the status as failed and on the next init or DB-Changed, we'll start over
			var exception = future.exception;
			rmdrLog("********** ERROR: The following reminders were not deleted: "+JSON.stringify(reminderIds));
			var tempFuture = DB.merge({"from": "com.palm.service.calendar.remindersstatus:1"}, {"status": "FAIL"});
			future.result = {returnValue: true, eventIds: eventIds};
		}
	);

	//Find the events we just autoclosed
	future.then(this, this.getEvents);

	//Feed the events back through to find their next alarm and autoclose time.
	future.then(this.assistant, this.assistant.findReminderTimesSubroutine);

	//Schedule the next wake.
	future.then(this.assistant, this.assistant.findNextWakeTimeSubroutine);

	//Schedule the next autoclose.
	future.then(this.assistant, this.assistant.findNextAutoCloseTimeSubroutine);

	future.then(function(future){
		var result = future.result;
		rmdrLog("********** In AUTOCLOSE: finished");
		future.result = result;
	});

	return future;
};

AutoCloseCommandAssistant.prototype.autoCloseReminders = function(future){
	rmdrLog("********** In autoCloseReminders");

	var result = future.result;
	var returnValue = result.returnValue;
	var reminderIds = [];
	var eventIds = [];

	if(returnValue === false || future.exception){
		rmdrLog("********** In autoCloseReminders: QUERY FAILED: "+JSON.stringify(result));
		//Query failed, but we don't care about keeping the status really. We can keep going, just with no results.
	}
	else{
		var reminders = result.results;
		var remindersLength = reminders && reminders.length;

		if (remindersLength) {
			rmdrLog("********** Reminder table query returned "+remindersLength+" results");

			var reminder;
			for(var i = 0; i < remindersLength; i++){
				reminder = reminders[i];
				eventIds.push(reminder.eventId);
				reminderIds.push(reminder._id);
			}

			//Launch the app, tell it which reminders to autoclose
			if (reminderIds.length > 0) {
				this.launchApp(reminderIds);
			}
		}
		else{
			//Reminder query returned nothing. Weird, but not a problem. We can keep going, just with no results.
			rmdrLog("********** Reminder table query returned zero results");
		}
	}

	return new Foundations.Control.Future({returnValue: true, reminderIds: reminderIds, eventIds: eventIds});
};

AutoCloseCommandAssistant.prototype.launchApp = function(reminderIds){
	rmdrLog("********** In launchApp: "+JSON.stringify(reminderIds));

	//launch the app and show reminders
	var appLaunchParams = {
		id: 'com.palm.app.calendar',
		params: {alarmClose: reminderIds}
	};

	//Don't really care about the result of this future.
	var tempFuture = PalmCall.call("palm://com.palm.applicationManager", "open", appLaunchParams);

};

AutoCloseCommandAssistant.prototype.removeReminders = function(future){
	var result = future.result;
	var returnValue = result.returnValue; //always true from previous step
	rmdrLog("********** AUTOCLOSE: In removeReminders: "+JSON.stringify(future.result));

	var reminderIds = result.reminderIds;
	if (reminderIds && reminderIds.length > 0) {
		return DB.del(reminderIds);
	}
	else{
		return new Foundations.Control.Future({returnValue: true});
	}
};

AutoCloseCommandAssistant.prototype.getEvents = function(future){
	var result = future.result;
	var returnValue = result.returnValue;
	rmdrLog("********** In getEvents: "+JSON.stringify(result));

	var eventIds = future.result.eventIds;
	if(eventIds && eventIds.length){
		return DB.get(eventIds);
	}
	else{
		return new Foundations.Control.Future({returnValue: true});
	}
};

AutoCloseCommandAssistant.prototype.complete = function(activity){
	//do nothing - we're completing & restarting in the reminder assistant.
	rmdrLog("********** In AUTOCLOSE: complete");
	return;
};
