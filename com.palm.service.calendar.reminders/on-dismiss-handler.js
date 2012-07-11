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
/*global DB, Foundations, rmdrLog */

function DismissCommandAssistant(){
}


/* This is called by the app to tell us that a reminder has been dismissed.
 * args expected:
 *		eventId - eventId of the reminder that was dismissed
 *		startTime = start timestamp of this occurrence if the event
 *
 * Flow:
 * Get the dismissed event from the database
 * Find the next reminder time of the event
 * Schedule the next wake time
 *
 * Call:
 * luna-send -n 1 palm://com.palm.service.calendar.reminders/onDismiss '{"reminderId": "2+6z", "eventId": "2+5x", "startTime": 1270530000000}'
 */

DismissCommandAssistant.prototype.run = function(outFuture){
	rmdrLog("********** In ON DISMISS");
	this.assistant = this.controller.service.assistant;

	var args = this.controller.args;
	var reminderId = args.reminderId;
	var eventId = args.eventId;
	var startTime = args.startTime;
	var future;

	if(reminderId === undefined || eventId === undefined || startTime === undefined){
		var errorString = "Missing args! Need reminderId, eventId, and startTime.  Received: "+JSON.stringify(args);
		throw errorString;
	}

	//When a reminder is dismissed, the startTime field in the database for this reminder could be set to the event's start time,
	//or the next alarm time, and we don't know which.  If the reminder was dismissed before its start time, it should NOT refire
	//at the start time.  So read the event from the event table in the database, and feed it back through to be rescheduled, but
	//set the flags to tell the assistant to start looking after the start time of this occurrence.
	future = DB.del([reminderId]);

	//make sure the delete succeeded
	future.then(
		function(){
			var result = future.result;
			future.result = {returnValue: true};
		},
		function(){
			//If the db.del in the previous step failed, some reminders may be duplicated or will become zombie reminders in the table.
			//Mark the status as failed and on the next init or DB-Changed, we'll start over
			var exception = future.exception;
			rmdrLog("********** ERROR: The following reminder was not deleted: "+JSON.stringify(reminderId));
			var tempFuture = DB.merge({"from": "com.palm.service.calendar.remindersstatus:1"}, {"status": "FAIL"});
			future.result = {returnValue: true};
		}
	);

	//Get the event from the database
	future.then(function(){
		rmdrLog("********** Deleted reminder: "+reminderId + ", getting event: "+eventId);
		var result = future.result;
		future.nest(DB.get([eventId]));
	});

	//Feed the events back through to find their next alarm and autoclose time.
	//Set flags, find next reminder time
	future.then(function(){
		var result = future.result;
		future.result = {returnValue: result.returnValue, results: result.results, eventDismissed: true, skipPastStartTime: startTime};
	});

	future.then(this.assistant, this.assistant.findReminderTimesSubroutine);

	future.then(this.assistant, this.assistant.findNextWakeTimeSubroutine);

	future.then(this.assistant, this.assistant.findNextAutoCloseTimeSubroutine);

	future.then(function(){
		var result = future.result;
		rmdrLog("********** In DISMISS: finished");
		future.result = result;
	});

	return future;

};
