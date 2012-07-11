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

function SnoozeCommandAssistant(){
}


/* This is called by the app to tell us that a reminder has been snoozed.
 * args expected:
 *		reminderId - id of the reminder to snooze
 *		snoozeDuration - (optional) length of time to snooze in milliseconds.  If not supplied, we'll use 5 minutes
 *
 * Flow:
 * Update the reminder with its new wake time
 * Find next wake time
 *
 * Call:
 * luna-send -n 1 palm://com.palm.service.calendar.reminders/onSnooze '{"reminderId":"2+60", "snoozeDuration": 500000}'
 * or
 * luna-send -n 1 palm://com.palm.service.calendar.reminders/onSnooze '{"reminderId":"2+60"}'
 */

SnoozeCommandAssistant.prototype.run = function(outFuture){
	rmdrLog("********** In ON SNOOZE");
	this.assistant = this.controller.service.assistant;
	var args = this.controller.args;

	var snoozeDuration = args.snoozeDuration;
	var reminderId = args.reminderId;

	if (!reminderId) {
		var errorString = "Missing args! Need reminderId.  Received: "+JSON.stringify(args);
		throw errorString;
	}

	if(!snoozeDuration){
		snoozeDuration = 300000; //5 minutes
	}

	var date = new Date();
	date.setMilliseconds(0);
	var snoozeUntil = date.getTime() + snoozeDuration;

	//Update the reminder with the new alarm time
	var future = DB.find(
		{	"from": "com.palm.service.calendar.reminders:1",
			"where":
			[
				{"prop":"_id","op":"=","val":reminderId}
			]
		}
	);

	future.then(function(){
		var result = future.result;
		var returnValue = result.returnValue;
		var results = result.results;
		if(returnValue && results && results.length){
			future.nest(DB.merge([{"_id": reminderId, "showTime": snoozeUntil}]));
		}
		else{
			rmdrLog("********** Tried to snooze a reminder that doesn't exist or query failed: "+reminderId);
			future.result = {returnValue: true};
		}
	});

	future.then(
		function(){
			var result = future.result;
			var returnValue = result.returnValue;
			future.result = {returnValue: true};
		},
		function(){
			//If the db.merge in the previous step failed, the reminder will become a zombie in the table.
			//Mark the status as failed and on the next init or DB-Changed, we'll start over
			var exception = future.exception;
			rmdrLog("********** ERROR: Snooze time merge failed for reminder: "+reminderId);
			var tempFuture = DB.merge({"from": "com.palm.service.calendar.remindersstatus:1"}, {"status": "FAIL"});
			future.result = {returnValue: true};
		});

	future.then(this.assistant, this.assistant.findNextWakeTimeSubroutine);

	future.then(this.assistant, this.assistant.findNextAutoCloseTimeSubroutine);

	future.then(function(){
		var result = future.result;
		rmdrLog("SNOOZE COMPLETE");
		future.result = result;
	});

	return future;

};

