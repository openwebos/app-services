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
/*global DB, PalmCall, rmdrLog */

function InitCommandAssistant(){
}

/*
 * We want this to be called on boot, maybe from Calendar app on launch?
 *
 * args expected: none
 *
 * Flow:
 * Get events with alarms from the calendar events table
 * Build reminder service's table of next reminder times, next start times, next autoclose times
 * Query the reminder table for the next wake time
 * Schedule next wake time
 *
 * Call:
 * luna-send -n 1 palm://com.palm.service.calendar.reminders/onInit '{}'
 */
InitCommandAssistant.prototype.run = function(outFuture){
	rmdrLog("********** In INIT");

	var unittest = false;
	this.assistant = this.controller.service.assistant;
	var args = this.controller.args;
	if(args && args.unittest){
		unittest = true;
	}

	//Query for our last status
	var future = DB.find({from : "com.palm.service.calendar.remindersstatus:1"});

	//Check status - after this we should be good to go
	future.then(this.assistant, this.assistant.prepareForTakeoff);

	//if we already have a db changed activity in existence, it's better to handle this as a db-changed, so trigger one.
	//If we don't have a db-changed in existence, then this is the first time we're running or we were forced to start over, so
	//proceed as normal.
	future.then(this, function(){
		var result = future.result;
		var startedOver = false;

		if(result && result.dbChangedActivityId){
			rmdrLog("********** Previous status intact. Do db changed instead.");
			var tempFuture = PalmCall.call("palm://com.palm.service.calendar.reminders/", "onDBChanged", {"unittest": unittest});
			future.result = {returnValue: true, startedOver: false};
		}
		else{
			future.nest(this.doFullInit());
		}
	});

	return future;

};

InitCommandAssistant.prototype.doFullInit = function(outFuture){
	rmdrLog("********** In doFullInit");

	var result;
	var unittest = false;
	var args = this.controller.args;
	if(args && args.unittest){
		unittest = true;
	}

	//Get events from the events table
	var eventQuery =	{	from	:	"com.palm.calendarevent:1"
						,	orderBy	:	"_rev"
						};

	var lastRevNumber = 0;
	var future = DB.find(eventQuery);

	//Find the last rev number
	future.then(
		function(){
			result = future.result;
			var returnValue = result.returnValue;
			var events = result && result.results;
			var eventsLength = events && events.length;

			//get the last rev number from our query. the query is ordered by _rev.
			lastRevNumber = (eventsLength) ? events[eventsLength-1]._rev : 0;
			Utils.debug("********** Last rev number: "+lastRevNumber);

			future.result = {returnValue: returnValue, results: events, lastRevNumber: lastRevNumber};
		},
		function(){
			result = future.result;
			var returnValue = result.returnValue;
			future.result = {returnValue: returnValue, results: [], lastRevNumber: lastRevNumber};
		}
	);

	//Calculate alarm times and populate reminder table
	future.then(this.assistant, this.assistant.findReminderTimesSubroutine);

	//Save our lastRevNumber to the status table
	future.then(this.assistant, this.assistant.setStatusAndRev);

	future.then(
		function(){
			Utils.debug("********** Status and last rev number saved.");
			var result = future.result;
			future.result = {returnValue: true};

		},
		function(){
			//If the db.batch in the previous step failed, the next scan might cause duplicates.
			//Mark the status as failed and on the next init or DB-Changed, we'll start over
			var exception = future.exception;
			rmdrLog("********** ERROR: Couldn't save last rev number, setting fail status");
			future.nest(DB.merge({"from": "com.palm.service.calendar.remindersstatus:1"}, {"status": "FAIL"}));
		}
	);

	//Query reminder table for next wake time and schedule the activity
	future.then(this.assistant, this.assistant.findNextWakeTimeSubroutine);

	var wakeStatus;
	var autoCloseStatus;
	var dbWatchActivityId;
	var returnValue;
	//store the wake status
	future.then(function(){
		result = future.result;
		returnValue = result.returnValue;
		wakeStatus = result.status;
		future.result = result;
	});

	//Find the next autoclose time
	future.then(this.assistant, this.assistant.findNextAutoCloseTimeSubroutine);

	//store the autoclose status
	future.then(function(){
		result = future.result;
		returnValue = returnValue && result.returnValue;
		autoCloseStatus = result.status;
		future.result = {dbChangedActivityId: 0, lastRevNumber: lastRevNumber, isUnitTest: unittest};
	});

	//Set up the db changed activity
	future.then(this.assistant, this.assistant.dbChangedActivitySubroutine);

	//package up and send out our status
	future.then(function(){
		result = future.result;
		returnValue = returnValue && result.returnValue;
		var dbChangedStatus = result.status;
		var finalStatus = {returnValue: returnValue, wakeStatus: wakeStatus, autoCloseStatus: autoCloseStatus, dbChangedStatus: dbChangedStatus};
		rmdrLog("********** INIT COMPLETE: " + JSON.stringify(finalStatus));
		future.result = finalStatus;
	});
	return future;
};


