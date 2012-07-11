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

/*global IMPORTS, require */

var Calendar = IMPORTS.calendar;
var Foundations = IMPORTS.foundations;
var DB = Foundations.Data.DB;
var PalmCall = Foundations.Comms.PalmCall;
//NOV-108635
if (typeof require === 'undefined') {
   require = IMPORTS.require;
}
var logger = require('pmloglib');

var Config = {
	//logs: "debug" 	// used by utils.js to control logging
};

function rmdrLog(string){
	//logger.log("rmdrsvc: "+string);
	console.error("rmdrsvc: "+string);
}

// Import DateJS's Date class modifications into the service's Date object:
(new Calendar.Utils()).importDateJS (Date);
