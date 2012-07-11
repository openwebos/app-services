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

/*jslint bitwise: true, devel: true, eqeqeq: true, immed: true, maxerr: 500, newcap: true,
nomen: false, onevar: false, plusplus: false, regexp: true, undef: true, white: false */

/*global _, exports: true, ObjectUtils, stringify, Config */

/*
 * Convert a string into a timestamp
 * @param alarmString: a string that contains a date formatted either YYYYMMDDTHHMMSS or YYMMDDTHHMMSSZ
 * @returns: timestamp
 */
function utilParseDateTime(alarmString){
	var date;
	switch (alarmString.length) {
		//date time type
		case 15:
			date = Date.parseExact(alarmString, "yyyyMMddTHHmmss");
			break;

		//UTC date time type
		case 16:
			//TODO: adjust from UTC to the event's timezone!!!
			//We have to trim the Z, since parseExact doesn't seem able to parse it.
			var substring = alarmString.substr(0, 15);
			date = Date.parseExact(substring, "yyyyMMddTHHmmss");
			var utcOffset = date.getTimezoneOffset();
			date.addMinutes(-utcOffset);
			break;

		//malformed. can't evaluate
		default:
			return 0;
	}
	return date.getTime();
}

/*
 * Convert a duration into a timestamp
 * @param alarmString: a string that contains a duration as specified by RFC5545 (-PT15M)
 * @param startTime: the start time that is the origin for the duration
 * @returns: timestamp
 */
function utilParseDuration(alarmString, startTime){
	var weeks = 0;
	var days = 0;
	var hours = 0;
	var minutes = 0;
	var seconds = 0;

	var negative = 1;
	if(alarmString[0] === '-'){
		negative = -1;
	}
	//messy but effective and we only call exec once.
	//See calendar's formatting-utils getAlarmPartsFromString for an alternative
	var regexp = new RegExp(/P([0-9]{1,3}[WD])*T*([0-9]{1,3}H)*([0-9]{1,3}M)*([0-9]{1,3}S)*/);
	var tokens = regexp.exec(alarmString);
	if(!tokens){
		return 0;
	}

	var tokensLength = tokens.length;
	for(var i = 0; i < tokensLength; i++){
		var token = tokens[i];
		if (token === undefined) {
			continue;
		}

		var end = token.length -1;
		var unit = token[end];
		var numberStr = token.substr(0, end);
		var number = parseInt(numberStr, 10);
		switch(unit){
			case 'W':
				weeks = number * negative;
				break;
			case 'D':
				days = number * negative;
				break;
			case 'H':
				hours = number * negative;
				break;
			case 'M':
				minutes = number * negative;
				break;
			case 'S':
				seconds = number * negative;
				break;
		}
	}

	var date = new Date(startTime);
	date.add({seconds: seconds, minutes: minutes, hours: hours, days: days});
	date.addWeeks(weeks);

	return date.getTime();
}

/*
 * Given an event's start time and a calendar alarm object, calculate the time the alarm is supposed to go off.
 * @param startTime: timestamp of the start date and time of the event
 * @param alarm: an alarm object
 * @return timestamp representing when the alarm should go off.  If malformed alarm, will return null.
 */
function utilCalculateAlarmTime(startTime, alarm){
	if(!alarm){
		return 0;
	}

	if(!alarm.alarmTrigger){
		return 0;
	}

	var alarmType = alarm.alarmTrigger.valueType;
	var alarmString = alarm.alarmTrigger.value;
	var time;

	if(!alarmType || !alarmString){
		return 0;
	}

	switch(alarmType){
		case "DATE-TIME":
			time = utilParseDateTime(alarmString);
			break;
		default:
		case "DURATION":
			time = utilParseDuration(alarmString, startTime);
			break;
	}
	return time;
}

/*
 * @function utilGetUTCDateString
 * @param timestamp- timestamp which needs to be converted
 * @return a string representing the timestamp formated as YYYY-MM-DD HH:MM:SSZ in UTC
 */
function utilGetUTCDateString(timestamp){

	//UTC time format
	var date = new Date(timestamp);
	var year = date.getUTCFullYear();
	var month = date.getUTCMonth()+1;
	var day = date.getUTCDate();
	var hour= date.getUTCHours();
	var minute=date.getUTCMinutes();
	var second =date.getUTCSeconds();

	month	= (month > 9)	? month		: "0"+month;
	day		= (day > 9)		? day		: "0"+day;
	hour	= (hour > 9)	? hour		: "0"+hour;
	minute	= (minute > 9)	? minute	: "0"+minute;
	second	= (second > 9)	? second	: "0"+second;

	/*YYYY-MM-DD HH:MM:SS*/
	return (""+year+"-"+month+"-"+day+" "+hour+":"+minute+":"+second+"Z");
}

var Utils = {
	log: function () {
		var argsArr = Array.prototype.slice.call(arguments, 0);
		Utils._logBase("log", argsArr);
	},

	warn: function () {
		var argsArr = Array.prototype.slice.call(arguments, 0);
		Utils._logBase("warn", argsArr);
	},

	error: function () {
		var argsArr = Array.prototype.slice.call(arguments, 0);
		Utils._logBase("error", argsArr);
	},

	debug: function() {
		if (Config && Config.logs === "debug") {
			var argsArr = Array.prototype.slice.call(arguments, 0);
			Utils._logBase("log", argsArr);
		}
	},

	_logBase: function (method, argsArr) {
		var data = argsArr.reduce(function (accumulatedMessage, curArg) {
			if (typeof curArg === "string") {
				return accumulatedMessage + curArg;
			} else {
				return accumulatedMessage + JSON.stringify(curArg);
			}
		}, "");

		if (Config && Config.logs === "verbose") {
			// I want ALL my logs!
			data = data.split("\n");
			var i, pos, datum;
			for (i = 0; i < data.length; ++i) {
				datum = data[i];
				if (datum.length < 500) {
					console[method](datum);
				} else {
					// Do our own wrapping
					for (pos = 0; pos < datum.length; pos += 500) {
						console[method](datum.slice(pos, pos + 500));
					}
				}
			}
		} else {
			console[method](data);
		}
	},

	logAndThrow: function (errMsg) {
		Utils.error(errMsg);
		throw new Error(errMsg);
	}
};
