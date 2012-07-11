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

function DBChangedCommandAssistant(){
}

/*
 * Callback for the activity manager when the event table changes.
 *
 * args expected: none
 *
 * Flow:
 * Get the last revision number we processed from the database
 * Get all event table changes since that revision number
 * Remove reminders associated with the events that were changed/added/deleted
 * Get events that were changed/added
 * Find reminder times for the events that were changed/added
 * Record the latest revision number in our status table
 * Schedule the next wake time
 *
 * Call:
 * luna-send -n 1 palm://com.palm.service.calendar.reminders/onDBChanged '{}'
 */
DBChangedCommandAssistant.prototype.run = function(outFuture){
	rmdrLog("********** In onDBChanged");
	this.assistant = this.controller.service.assistant;
	var args = this.controller.args;
	var eventIds;
	var newOrChangedEventIds;
	var deletedEventIds;
	var status = "OK";

	var dbChangedActivityId = 0;
	var lastRevNumber = 0;
	var unittest = false;

	if(args.unittest){
		unittest = true;
		rmdrLog("********** In onDBChanged: UNITTEST");
		return new Foundations.Control.Future({returnValue: true});
	}

	//query for the last revision number
	var future = DB.find({from : "com.palm.service.calendar.remindersstatus:1"});

	//Check the results, get the last revision number and db changed activity id
	future.then(this.assistant, this.assistant.prepareForTakeoff);

	//store the db changed activity id and lastRevNumber
	future.then(function(){
		var result = future.result;
		lastRevNumber = result.lastRevNumber;
		dbChangedActivityId = result.dbChangedActivityId;
		future.result = {lastRevNumber: lastRevNumber};
	});

	//query event table for what's changed & deleted since the last rev number
	future.then(this, this.getEvents);

	future.then(function(){
		var result = future.result;
		var returnValue = result.returnValue;
		var responses = result.responses;
		future.result = {returnValue: returnValue, responses: responses, lastRevNumber: lastRevNumber};
	});

	//filter into eventId lists
	future.then(this, this.processDBChanges);

	//store our various id arrays, pass on the deletedEventIds
	future.then(function(){
		rmdrLog("********** DB changes processed");
		var result = future.result;
		eventIds = result.eventIds;
		newOrChangedEventIds = result.newOrChangedEventIds;
		deletedEventIds = result.deletedEventIds;
		lastRevNumber = result.lastRevNumber;
		rmdrLog("********** Deleted events: "+JSON.stringify(deletedEventIds));
		rmdrLog("********** New or Changed events: "+JSON.stringify(newOrChangedEventIds));

		future.result = {deletedEventIds: deletedEventIds};
	});

	future.then(this, this.notifyAppOfDeletedEvents);

	//pass on the eventIds
	future.then(function(){
		var result = future.result;
		future.result = {eventIds: eventIds};
	});

	//Remove everything that changed from the reminder table.
	//This will clear out any that got deleted, and make it so that
	//both new and updated events can just be added to the table
	future.then(this, this.removeRemindersByEventId);

	//If the delete succeeded, the newOrChangedEventIds for events to reschedule are passed onward.
	//If the delete failed, some reminders may be duplicated or will become zombie reminders in the table.
	future.then(
		function(){
			rmdrLog("********** After remove reminders by event id");
			var result = future.result;
			future.result = {eventIds: newOrChangedEventIds};
		},
		function(){
			//If the db.del in the previous step failed, some reminders may be duplicated or will become zombie reminders in the table.
			//Mark the status as failed and on the next init or DB-Changed, we'll start over
			var exception = future.exception;
			status = "FAIL";
			rmdrLog("********** ERROR: Reminders associated with the following eventIds were not deleted: "+JSON.stringify(eventIds));
			var tempFuture = DB.merge({"from": "com.palm.service.calendar.remindersstatus:1"}, {"status": "FAIL"});
			future.result = {eventIds: newOrChangedEventIds};
		}
	);

	//query for the events to reschedule
	future.then(this, this.getEventsById);

	future.then(function(){
		var result = future.result;
		future.result =
		{	returnValue: result.returnValue
		,	results: result.results
		,	lastRevNumber: lastRevNumber
		};
	});

	future.then(this.assistant, this.assistant.findReminderTimesSubroutine);

	future.then(function(){
		var result = future.result;
		var tempLastRevNumber = result.lastRevNumber || lastRevNumber;
		future.result = {lastRevNumber: tempLastRevNumber, status: status};
	});

	//Set the status and lastRevNumber in our status table
	future.then(this.assistant, this.assistant.saveLastRevNumber);

	future.then(
		function(){
			var result = future.result;
			future.result = {returnValue: true, newOrChangedEventIds: newOrChangedEventIds};

		},
		function(){
			//If the db.merge in the previous step failed, the next scan might cause duplicates.
			//Mark the status as failed and on the next init or DB-Changed, we'll start over
			var exception = future.exception;
			rmdrLog("********** ERROR: Couldn't save last rev number");
			var tempFuture = DB.merge({"from": "com.palm.service.calendar.remindersstatus:1"}, {"status": "FAIL"});
			future.result = {returnValue: true, newOrChangedEventIds: newOrChangedEventIds};
		}
	);

	//For all events that are changed, we need to send their reminders to the app
	//In the app - if they're live, reminder manager will replace them with the new versions.
	//If there's no new reminder for the event, we need to close any live reminders.
	//If there is a new reminder for the event, but it shouldn't be showing now, we need to close it.
	future.then(this, this.getRemindersByEventId);

	future.then(function(){
		var result = future.result;
		future.result = {returnValue: result.returnValue, responses: result.responses, newOrChangedEventIds: newOrChangedEventIds};
	});

	future.then(this, this.updateLiveReminders);

	//Find next wake time
	future.then(this.assistant, this.assistant.findNextWakeTimeSubroutine);

	var wakeStatus;
	var autoCloseStatus;
	var returnValue;
	var result;
	//store the wake status
	future.then(function(){
		rmdrLog("********** After find next wake time subroutine");
		result = future.result;
		returnValue = result.returnValue;
		wakeStatus = result.status;
		future.result = result;
	});

	//Find the next autoclose time
	future.then(this.assistant, this.assistant.findNextAutoCloseTimeSubroutine);

	//store the autoclose status
	future.then(function(){
		rmdrLog("********** After find next auto close subroutine");
		result = future.result;
		returnValue = returnValue && result.returnValue;
		autoCloseStatus = result.status;
		future.result = {dbChangedActivityId: dbChangedActivityId, lastRevNumber: lastRevNumber, isUnitTest: unittest};
	});

	//Set up the db changed activity
	future.then(this.assistant, this.assistant.dbChangedActivitySubroutine);

	//package up and send out our status
	future.then(function(){
		rmdrLog("********** After db changed activity subroutine");
		result = future.result;
		var dbChangedStatus = result.status;
		future.result = {returnValue: true, wakeStatus: wakeStatus, autoCloseStatus: autoCloseStatus, dbChangedStatus: dbChangedStatus};
	});

	return future;
};

DBChangedCommandAssistant.prototype.getEvents = function(future){
	rmdrLog("********** In getEvents");
	var result = future.result;
	var lastRevNumber = result.lastRevNumber;

	var delEventsClause =
		{	method		: "find"
		,	params	:
			{	query:
				{	from	: "com.palm.calendarevent:1"
				,	where	:
					[
						{	"prop"	:	"_del"
						,	"op"	:	"="
						,	"val"	:	true
						},
						{	"prop"	:	"_rev"
						,	"op"	:	">"
						,	"val"	:	lastRevNumber
						}
					]
				,	orderBy	: "_rev"
				}
			}
		};

	var eventsClause =
		{	method		: "find"
		,	params	:
			{	query:
				{	from	: "com.palm.calendarevent:1"
				,	where	:
					[
						{	"prop"	:	"_rev"
						,	"op"	:	">"
						,	"val"	:	lastRevNumber
						}
					]
				,	orderBy	: "_rev"
				}
			}
		};

	var operations = [delEventsClause, eventsClause];


	//batch request our queries
	return DB.execute("batch", {operations: operations});


};

/*
 * Assumes you're coming from a batch query on the event table.  The batch contained two queries, one for
 * the first for deleted events, and the second for new/changed events.
 * Deletes all instances in the reminder table, (whether the event itself was deleted, added, or updated),
 * then packages up the ones that still exist and have alarms as a response and calls findReminderTimes.
 */
DBChangedCommandAssistant.prototype.processDBChanges = function(outFuture){
	rmdrLog("********** In processDBChanges");
	var result = outFuture.result;
	var lastRevNumber = result.lastRevNumber;
	var future;
	//Event table query failed.  If we keep our old rev number, we'll recover on the next dbchanged.
	if (result.returnValue === false || outFuture.exception) {
		rmdrLog("********** Event table query failed");
		//we failed. If we use the same revNumber, we could get into an infinite loop. Skip ahead one, hopefully we'll work out of it
		return new Foundations.Control.Future({returnValue: true, eventIds: [], newOrChangedEventIds: [], deletedEventIds: [], lastRevNumber: lastRevNumber+1});
	}

	var delEventsResponse = result.responses && result.responses[0];
	var delEvents = delEventsResponse && delEventsResponse.results;
	var delEventsLength = delEvents && delEvents.length;

	var eventsResponse = result.responses && result.responses[1];
	var events = eventsResponse && eventsResponse.results;
	var eventsLength = events && events.length;

	//Event query returned nothing or part of the response was undefined.
	//Weird, but if we keep our old rev number, we'll recover on the next dbchanged.
	if(!delEventsLength && !eventsLength){
		rmdrLog("********** Event table query returned zero results");
		//we failed. If we use the same revNumber, we could get into an infinite loop. Skip ahead one, hopefully we'll work out of it
		return new Foundations.Control.Future({returnValue: true, eventIds: [], newOrChangedEventIds: [], deletedEventIds: [], lastRevNumber: lastRevNumber+1});
	}

	//Get the last rev number
	var delEventRev = "";
	var eventRev = "";
	if(delEventsLength > 0){
		delEventRev = delEvents[delEventsLength-1]._rev;
	}
	if(eventsLength){
		eventRev = events[eventsLength-1]._rev;
	}
	lastRevNumber = (delEventRev > eventRev ? delEventRev : eventRev);

	//Cycle through both sets, and get all affected event ids
	//For the items that weren't deleted, save their ids for possible rescheduling
	var eventIds = [];
	var newOrChangedEventIds = [];
	var deletedEventIds = [];
	var event;
	var i;

	//Get the event ids of all the deleted events
	for(i = 0; i < delEventsLength; i++){
		event = delEvents[i];

		eventIds.push(event._id);
		deletedEventIds.push(event._id);
	}

	//Get the event ids of all the changed/new events
	//Find the eventId of the things that need to be rescheduled
	for(i = 0; i < eventsLength; i++){
		event = events[i];
		eventIds.push(event._id);
		if(event._del !== true && event.alarm && event.alarm.length > 0 && event.alarm[0].alarmTrigger){
			newOrChangedEventIds.push(event._id);
		}
	}

	rmdrLog("********** Setting lastRevNumber: "+lastRevNumber);
	return new Foundations.Control.Future({returnValue: true, eventIds: eventIds, newOrChangedEventIds: newOrChangedEventIds, deletedEventIds: deletedEventIds, lastRevNumber: lastRevNumber});
};

DBChangedCommandAssistant.prototype.notifyAppOfDeletedEvents = function(future){
	var result = future.result;
	var deletedEventIds = result.deletedEventIds;
	if (deletedEventIds && deletedEventIds.length > 0) {
		rmdrLog("********** In notifyAppOfDeletedEvents: removing reminders from app");

		//launch the app and show reminders
		var appLaunchParams = {
			id: 'com.palm.app.calendar',
			params: {
				alarmDeleted: deletedEventIds
			}
		};

		//Don't really care about the result of this future.
		var tempFuture = PalmCall.call("palm://com.palm.applicationManager", "open", appLaunchParams);
	}
	future.result = {returnValue: true};
};

DBChangedCommandAssistant.prototype.removeRemindersByEventId = function(future){
	rmdrLog("********** In removeRemindersByEventId");
	var result = future.result;

	var eventIds = future.result.eventIds;
	var eventIdsLength = eventIds.length;

	//build our list of del operations
	var operations = [];
	var clause;
	for(var i = 0; i < eventIdsLength; i++){
		clause =
			{	method		: "del"
			,	params	:
				{	query:
					{	from: "com.palm.service.calendar.reminders:1"
					,	where:
						[{	prop: "eventId"
						,	op: "="
						,	val: eventIds[i]
						}]
					}

				}
			};
		operations.push(clause);
	}
	if(operations.length > 0){
		//batch request our del operations
		return DB.execute("batch", {operations: operations});
	}
	else{
		return new Foundations.Control.Future({returnValue: true});
	}

};

DBChangedCommandAssistant.prototype.getEventsById = function(future){
	rmdrLog("********** In getEventsById");
	var result = future.result;
	var eventIds = future.result.eventIds;

	if(eventIds && eventIds.length){
		//query event table
		return DB.get(eventIds);
	}
	else{
		return new Foundations.Control.Future({returnValue: true});
	}
};

DBChangedCommandAssistant.prototype.getRemindersByEventId = function(future){
	rmdrLog("********** In getRemindersByEventId");
	var result = future.result;

	var eventIds = result.newOrChangedEventIds;
	var eventIdsLength = eventIds && eventIds.length;

	//build our list of del operations
	var operations = [];
	var clause;
	for (var i = 0; i < eventIdsLength; i++) {
		clause = {
			method: "find",
			params: {
				query: {
					from: "com.palm.service.calendar.reminders:1",
					where: [{
						prop: "eventId",
						op: "=",
						val: eventIds[i]
					}]
				}

			}
		};
		operations.push(clause);
	}

	if (operations.length) {
		//batch request our del operations
		return DB.execute("batch", {operations: operations});
	}
	else {
		return new Foundations.Control.Future({	returnValue: true});
	}
};

DBChangedCommandAssistant.prototype.updateLiveReminders = function(future){
	rmdrLog("********** In updateLiveReminders");
	var result = future.result;
	var returnValue = result.returnValue;
	var eventIds = result.newOrChangedEventIds;

	//Oh well, the app doesn't get updated.
	if(returnValue === false || future.exception){
		return new Foundations.Control.Future({returnValue: true});
	}

	//Nothing to update
	var responses = result.responses;
	if(!responses || responses.length === 0){
		rmdrLog("********** No reminders to update");
		return new Foundations.Control.Future({returnValue: true});
	}

	//possibilities:
	//- event text changed.  The reminder should still show and just be replaced in the app.
	//- event time changed, moved earlier: reminder should maybe be closed if event is over.
	//- event time changed, moved later: reminder should maybe be closed if event hasn't started yet.
	var responsesLength = responses.length;
	var remindersToUpdate = [];
	var eventIdsToClose = [];

	var now = new Date();
	now.setMilliseconds(0);
	now = now.getTime();

	for(var i = 0; i < responsesLength; i++){
		var response = responses[i];
		if(response && response.returnValue === true && response.results){
			var responseResults = response.results;
			var responseResultsLength = response.results.length;
			//if no reminder was made for this event, then the reminder is probably over, and we should close it.
			if(responseResultsLength === 0){
				rmdrLog("********** No reminder, sending eventId to be closed: "+eventIds[i]);
				eventIdsToClose.push(eventIds[i]);
				continue;
			}

			//we should only have one reminder in the response, but just in case...
			for(var j = 0; j < responseResultsLength; j++){
				var reminder = responseResults[j];
				if(now >= reminder.alarmTime && now <= reminder.autoCloseTime){
					rmdrLog("********** Sending to the app to be updated: " + reminder._id + " / " + reminder.eventId);
					reminder.startTime =     reminder.startTime.toString();
					reminder.endTime =       reminder.endTime.toString();
					reminder.showTime =      reminder.showTime.toString();
					reminder.autoCloseTime = reminder.autoCloseTime.toString();
					remindersToUpdate.push(responseResults[j]);
				}
				else{
					rmdrLog("********** Sending to the app to be closed: "+eventIds[i]);
					eventIdsToClose.push(eventIds[i]);
				}
			}
		}
	}

	if (remindersToUpdate.length > 0 || eventIdsToClose.length > 0) {
		//launch the app and show reminders
		var appLaunchParams = {
			id: 'com.palm.app.calendar',
			params: {
				alarmUpdated: {update: remindersToUpdate, close: eventIdsToClose}
			}
		};

		//Don't really care about the result of this future.
		var tempFuture = PalmCall.call("palm://com.palm.applicationManager", "open", appLaunchParams);
	}

	return new Foundations.Control.Future({returnValue: true});
};

DBChangedCommandAssistant.prototype.complete = function(activity){
	//do nothing - we're completing & restarting in the reminder assistant.
	rmdrLog("********** In DBCHANGED: complete");
	return;
};
