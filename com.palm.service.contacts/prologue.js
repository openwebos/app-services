// @@@LICENSE
//
//      Copyright (c) 2010-2013 LG Electronics, Inc.
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
/*global IMPORTS, console */

var Foundations = IMPORTS.foundations,
	IO = IMPORTS["foundations.io"],
	_ = IMPORTS.underscore._,
	Assert = Foundations.Assert,
	Class = Foundations.Class,
	Contacts = IMPORTS.contacts,
	Person = Contacts.Person,
	Contact = Contacts.Contact,
	SortKey = Contacts.SortKey,
	BatchDBWriter = Contacts.Utils.BatchDBWriter,
	VCardExporter = Contacts.VCardExporter,
	VCardImporter = Contacts.VCardImporter,
	DB = Foundations.Data.DB,
	PalmCall = Foundations.Comms.PalmCall,
	Future = Foundations.Control.Future,
	PhoneNumber = Contacts.PhoneNumber;
