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
/*global palmGetResource */

function ContactGenerator() {
	this.data_contacts = JSON.parse(palmGetResource("test/contacts.json"));
	this.data_names = JSON.parse(palmGetResource("test/names.json"));
	this.data_phoneNumbers = JSON.parse(palmGetResource("test/phoneNumbers.json"));
	this.data_emails = JSON.parse(palmGetResource("test/emails.json"));
	this.data_ims = JSON.parse(palmGetResource("test/ims.json"));
}
	
ContactGenerator.prototype.getNRandomContacts = function (n) {
	var toReturn = [],
		tempData,
		i;
	
	tempData = this.data_contacts.data;
	
	for (i = 0; i < n && i < tempData.length; i += 1) {
		toReturn.push(tempData[i]);
	}
	
	return toReturn;
};

ContactGenerator.prototype.getNUniqueValPhoneNumbers = function (n) {
	var toReturn = [],
		tempData,
		tempObject,
		randNumber = 0,//lowest, highest);
		i,
		factor = 4;
	
	tempData = this.data_phoneNumbers.data;
	
	for (i = 0; i < n; i += 1) {
		randNumber = factor * i;
		randNumber = this.getRandNumber(randNumber, randNumber + 4);
		tempObject = tempData[randNumber];
		toReturn.push(tempObject);
	}
	
	return toReturn;

	
};

ContactGenerator.prototype.getNSameValSameTypePhoneNumbers = function (n) {
	var toReturn = [],
		tempData,
		randNumber = this.getRandNumber(0, 4),//lowest, highest);
		i;
	
	tempData = this.data_phoneNumbers.data;
	
	tempData = tempData[randNumber];
	
	for (i = 0; i < n; i += 1) {
		toReturn.push(tempData);
	}
	
	return toReturn;
};

ContactGenerator.prototype.getNSameTypePhoneNumbers = function (n) {
	var toReturn = [],
		tempData,
		i,
		j;
	
	tempData = this.data_phoneNumbers.data;
	
	for (i = 0, j = 0; j < n  && i < tempData.length; i += 4, j += 1) {
		toReturn.push(tempData[i]);
	}
	
	return toReturn;
};

ContactGenerator.prototype.getRandNumber = function (lowest, highest) {
	return lowest + 2;
};