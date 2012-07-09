// @@@LICENSE
//
//      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

/*jslint white: true, onevar: true, undef: true, eqeqeq: true, plusplus: true, bitwise: true, 
regexp: true, newcap: true, immed: true, nomen: false, maxerr: 500 */
/*global IMPORTS, console, require:true, process */

var Foundations = IMPORTS.foundations,
	_ = IMPORTS.underscore._,
	ContactsLib = IMPORTS.contacts,
	Person = ContactsLib.Person,
	Contact = ContactsLib.Contact,
	ContactLinkable = ContactsLib.ContactLinkable,
	PersonFactory = ContactsLib.PersonFactory,
	ContactFactory = ContactsLib.ContactFactory,
	Name = ContactsLib.Name,
	EmailAddress = ContactsLib.EmailAddress,
	PhoneNumber = ContactsLib.PhoneNumber,
	IMAddress = ContactsLib.IMAddress,
	ContactPluralField = ContactsLib.ContactPluralField,
	DB = Foundations.Data.DB,
	PalmCall = Foundations.Comms.PalmCall,
	Future = Foundations.Control.Future,
	ContactsUtils = ContactsLib.Utils,
	Assert = Foundations.Assert,
	IO = IMPORTS["foundations.io"],
	TimingRecorder = ContactsLib.Utils.TimingRecorder;

if (typeof require === "undefined") {
	require = IMPORTS.require;
}

//var console.log = require('pmloglib');
	
var SUPER_TOTALLY_AWESOME_DEBUG_MESSAGES = false;
var RECORD_TIMINGS_FOR_SPEED = false;

var Console = {};

Console.log = function (message) {
	if (SUPER_TOTALLY_AWESOME_DEBUG_MESSAGES) {
		console.log(message);
	}
};

Console.willDisplay = function () {
	return SUPER_TOTALLY_AWESOME_DEBUG_MESSAGES;
};

