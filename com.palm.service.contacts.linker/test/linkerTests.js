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
/*global console, include, MojoLoader, DB, Person, Future, ManualLink, ManualUnlink, Contact, ContactPluralField, LinkerAssistant, _, MojoTest */
//TODO: remove console from the global list when we switch to something else

// Tests for the manual linker and unlinker component
//
// triton -I /usr/palm/frameworks -I /usr/palm/frameworks/private /usr/palm/frameworks/private/mojotest/version/1.0/tools/test_runner.js -- testSpec.json
//
include("test/loadall.js");

//var libraries = MojoLoader.require({
//		name: "mojotest",
//		version: "1.0"
//	}),
//	MojoTest = libraries.mojotest.MojoTest;
/*
function resetPeople(future) {
	var resetPeopleFuture = new Future(),
		resetContactsFuture = new Future();
	
	resetPeopleFuture.nest(DB.del({
		"from": Person.kind
	}));
	DB.del({
		"from": ManualLink.kind
	});
	DB.del({
		"from": ManualUnlink.kind
	});
	
	resetPeopleFuture.then(this, function (resetFuture) {
		if (resetFuture && typeof resetFuture.result === "number") {
			console.log("DB successfully cleared of " + resetFuture.result + " people");
			resetContactsFuture.nest(DB.del({
				"from": Contact.kind
			}));
		} else {
			console.log("DB not cleared of people");
			future.exception = {
				error: "DB not cleared of people"
			};
		}
	});
	
	resetContactsFuture.then(this, function (contactsFuture) {
		if (contactsFuture && typeof contactsFuture.result === "number") {
			console.log("DB successfully cleared of " + contactsFuture.result + " contacts");
			//		this.makeTestPeople(future);
			future.result = true;
		} else {
			console.log("DB not cleared of contacts");
			future.exception = {
				error: "DB not cleared of contacts"
			};
		}
	});
}

function createPeople(peopleToMake, future) {
	var personsFuture = new Future();
	
	personsFuture.nest(DB.put(peopleToMake));
	
	personsFuture.then(this, function (personSaveFuture) {
		if (personSaveFuture.result && personSaveFuture.result.length > 0) {
			console.log("Successsfully wrote " + personSaveFuture.result.length + " people");
			
			var ids = [],
				i;
			
			for (i = 0; i < personSaveFuture.result.length; i += 1) {
				ids[i] = personSaveFuture.result[i].id;
			}
			
			future.result = ids;
		}
	});
}

function createContacts(contactsToMake, future) {
	var contactsFuture = new Future();
	
	contactsFuture.nest(DB.put(contactsToMake));
	
	contactsFuture.then(this, function (contactSaveFuture) {
		if (contactSaveFuture.result && contactSaveFuture.result.length > 0) {
			console.log("Successsfully wrote " + contactSaveFuture.result.length + " contacts");
			
			var ids = [],
				i;
			
			for (i = 0; i < contactSaveFuture.result.length; i += 1) {
				ids[i] = contactSaveFuture.result[i].id;
			}
			
			future.result = ids;
		}
		
	});
}*/

function AutoLinkerTests() {
}
/*
function getNoLinkContacts(future) {
	var contact1 = new Contact(),
		contact2 = new Contact(),
		contact3 = new Contact(),
		contact4 = new Contact(),
		contact5 = new Contact(),
		contact6 = new Contact(),
		contact7 = new Contact(),
		contact8 = new Contact(),
		contact9 = new Contact(),
		contact10 = new Contact(),
		contact6Email,
		contact8Email,
		contact9PhoneNumber1,
		contact9PhoneNumber2,
		contact10IM1,
		contactsFuture = new Future(),
		personsFuture = new Future(),
		theContacts;
	
	contact1.name = {};
	contact1.name.familyName = "last1";
	contact1.name.givenName = "first1";
	contact1.displayName = "lksfjlsafdjk";
	contact1.photos = [{
		value: "alsdjfafs;lk",
		type: {
			"label": "work"
		},
		primary: true
	}, {
		value: "alsdjfafs;lk-small",
		type: {
			"label": "work"
		},
		primary: true
	}];
	
	contact2.phoneNumbers = [];
	contact2.displayName = "displayName2";
	contact2.phoneNumbers[0] = {
		value: "1111111111",
		type: "work"
	};
	
	contact3.name = {};
	contact3.name.familyName = "last3";
	contact3.name.givenName = "first3";
	contact3.photos = [{
		value: "alsdjfafs;lk",
		type: {
			"label": "work"
		},
		primary: true
	}, {
		value: "alsdjfafs;lk-small",
		type: {
			"label": "work"
		},
		primary: true
	}];
	
	contact4.name = {};
	contact4.name.familyName = "last4";
	contact4.name.givenName = "first4";
	
	contact5.name = {};
	contact5.name.familyName = "last5";
	contact5.name.givenName = "first5";
	
	contact6.name = {};
	contact6.name.familyName = "last4";
	contact6.name.givenName = "first4";
	contact6.phoneNumbers = [];
	contact6.phoneNumbers[0] = {
		value: "1111111111",
		type: "work"
	};
	contact6Email = new ContactPluralField();
	contact6Email.value = "last8@gmail.com";
	contact6Email.type = "work";
	contact6Email.primary = false;
	contact6.emails = [contact6Email];
	contact6.ims = [{
		value: "last4",
		type: "gtalk"
	}];
	
	contact7.name = {};
	contact7.name.familyName = "last7";
	
	contact8.name = {};
	contact8.name.familyName = "last8";
	contact8.name.givenName = "first8";
	contact8Email = new ContactPluralField();
	contact8Email.value = "last8@gmail.com";
	contact8Email.type = "work";
	contact8Email.primary = false;
	contact8.emails = [contact8Email];
	
	contact9.name = {};
	contact9.name.familyName = "last9";
	contact9.name.givenName = "first9";
	contact9PhoneNumber1 = new ContactPluralField();
	contact9PhoneNumber1.value = "1111111111";
	contact9PhoneNumber1.type = "home";
	contact9PhoneNumber1.primary = true;
	contact9PhoneNumber2 = new ContactPluralField();
	contact9PhoneNumber2.value = "2222222222";
	contact9PhoneNumber2.type = "work";
	contact9PhoneNumber2.primary = false;
	contact9.phoneNumbers = [contact9PhoneNumber1, contact9PhoneNumber2];
	
	contact10.name = {};
	contact10.name.familyName = "last10";
	contact10.name.givenName = "first10";
	contact10IM1 = new ContactPluralField();
	contact10IM1.value = "testIm";
	contact10IM1.type = "home";
	contact10IM1.primary = true;
	contact10.ims = [contact10IM1];
	
	
	theContacts = [contact1, contact2, contact3, contact4, contact5, contact6, contact7, contact8, contact9, contact10];
	
	createContacts(theContacts, contactsFuture);
	
	
	contactsFuture.then(this, function (theFuture) {
		if (theFuture.result && theFuture.result.length) {
			for (var i = 0; i < theFuture.result.length; i += 1) {
				theContacts[i]._id = theFuture.result[i];
			}
			
			future.result = theContacts;
		}
	});
}

AutoLinkerTests.prototype.testNoLinks = function (reportResults) {
	var getNoLinkContactsFuture = new Future();
	
	getNoLinkContacts(getNoLinkContactsFuture);
	
	getNoLinkContactsFuture.then(this, function (theFuture) {
		//console.log(JSON.stringify(theFuture.result));
		
		reportResults(MojoTest.passed);
	});
	
};
*/
function ManualLinkerTests() {
	this.manualLinker = new LinkerAssistant();
}
/*
// This test verifies that calling manuallyLink with no parameters
// throws an exception
ManualLinkerTests.prototype.testNoParameters = function (reportResults) {
	var resetPeopleFuture = new Future(),
		doneManualFuture = new Future(),
		params = {};
	
	resetPeople(resetPeopleFuture);
	
	resetPeopleFuture.then(this, function (theFuture) {
		params = {};
		this.manualLinker.manuallyLink(doneManualFuture, params);
	});
	
	doneManualFuture.then(this, function (theFuture) {
		if (theFuture.exception) {
			console.log(theFuture.exception.error);
			MojoTest.requireEqual(theFuture.exception.error, "The 'personToLinkTo' parameter was not specified. The 'personToLink' parameter was not specified.", "The error does not indicate both parameters are missing");
			
			reportResults(MojoTest.passed);
		} else {
			reportResults(MojoTest.failed);
		}
	});
};

// This test verifies that calling manuallyLink with the personToLinkTo
// parameter missing throws an exception
ManualLinkerTests.prototype.testNoPersonToLinkToParameter = function (reportResults) {
	var future = new Future(),
		params = {
			personToLink: 12
		};//this.idsOfTestPeople[1] };
	this.manualLinker.manuallyLink(future, params);
	
	future.then(this, function (theFuture) {
		if (theFuture.exception) {
			console.log(JSON.stringify(theFuture.exception.error));
			MojoTest.requireEqual(theFuture.exception.error, "The 'personToLinkTo' parameter was not specified. ", "The error returned is not indicating the personToLinkToParameters is missing");
			reportResults(MojoTest.passed);
		} else {
			reportResults(MojoTest.failed);
		}
	});
};

// This test verifies that calling manuallyLink with the personToLink
// parameter missing throws an exception
ManualLinkerTests.prototype.testNoPersonToLinkParameter = function (reportResults) {
	var future = new Future(),
		params = {
			personToLinkTo: 1
		};
	
	future.now(this, function (future) {
		this.manualLinker.manuallyLink(future, params);
	});
	
	future.then(this, function (theFuture) {
		if (theFuture.exception) {
			console.log(JSON.stringify(theFuture.exception.error));
			MojoTest.requireEqual(theFuture.exception.error, "The 'personToLink' parameter was not specified.", "The error returned is not indicating the personToLinkParameter is missing");
			reportResults(MojoTest.passed);
		} else {
			reportResults(MojoTest.failed);
		}
	});
};


// This test verifies that calling manuallyLink with valid parameters 
// does not throw an execption and it properly updates the person record 
// 
ManualLinkerTests.prototype.testManualLinkWorked = function (reportResults) {
	var future = new Future(),
		myFuture,
		resetPeopleFuture = new Future(),
		params = {},
		idsOfPeople = [],
		peopleAfterManualLink,
		peopleBeforeManualLink;
	
	resetPeople(resetPeopleFuture);
	
	resetPeopleFuture.then(this, function (theFuture) {
		var person1 = new Person(),
			person2 = new Person(),
			contact1 = new Contact(),
			contact2 = new Contact(),
			contactsFuture = new Future(),
			personsFuture = new Future();
		
		person1.name = {
			givenName: "first",
			familyName: "lastName"
		};
		person1.names = [];
		person1.names[0] = {
			givenName: "first1",
			familyName: "last1"
		};
		person1.names[1] = {
			givenName: "first2",
			familyName: "last2"
		};
		person1.emails = [];
		person1.emails[0] = {
			value: "last8@gmail.com",
			type: "work"
		};
		person1.phoneNumbers = [];
		person1.phoneNumbers[0] = {
			value: "1111111111",
			type: "work"
		};
		person1.ims = [];
		person1.ims[0] = {
			value: "last4",
			type: "gtalk"
		};
		
		person2.name = {
			givenName: "first2",
			familyName: "lastName2"
		};
		person2.names = [];
		person2.names[0] = {
			givenName: "first3",
			familyName: "last3"
		};
		person2.names[1] = {
			givenName: "first4",
			familyName: "last4",
			middleName: "middle4"
		};
		person2.emails = [];
		person2.emails[0] = {
			value: "last8@gmail.com",
			type: "work"
		};
		person2.phoneNumbers = [];
		person2.phoneNumbers[0] = {
			value: "1111111111",
			type: "work"
		};
		person2.ims = [];
		person2.ims[0] = {
			value: "last12",
			type: "yahoo"
		};
		
		contact1.name = {};
		contact1.name.familyName = "last1";
		contact1.name.givenName = "first1";
		contact1.displayName = "lksfjlsafdjk";
		contact1.photos = [{
			value: "alsdjfafs;lk",
			type: {
				"label": "work"
			},
			primary: true
		}, {
			value: "alsdjfafs;lk-small",
			type: {
				"label": "work"
			},
			primary: true
		}];
		
		contact2.phoneNumbers = [];
		contact2.displayName = "displayName2";
		contact2.phoneNumbers[0] = {
			value: "1111111111",
			type: "work"
		};
		
		createContacts([contact1, contact2], contactsFuture);
		
		contactsFuture.then(this, function (contactSaveFuture) {
			if (contactSaveFuture.result && contactSaveFuture.result.length > 0) {
				person1.contactsIds[0] = contactSaveFuture.result[0];
				person2.contactsIds[0] = contactSaveFuture.result[1];
				
				createPeople([person1, person2], personsFuture);
			}
			
		});
		
		personsFuture.then(this, function (personSaveFuture) {
			if (personSaveFuture.result && personSaveFuture.result.length > 0) {
				idsOfPeople[0] = personSaveFuture.result[0];
				idsOfPeople[1] = personSaveFuture.result[1];
				
				future.result = true;
			}
		});
		
	});
	
	params = {
		personToLinkTo: idsOfPeople[0],
		personToLink: idsOfPeople[1]
	};
	
	
	peopleAfterManualLink = [];
	peopleBeforeManualLink = [];
	
	future.then(this, function (theFuture) {
		future.nest(DB.get([idsOfPeople[0], idsOfPeople[1]]));
	}).then(this, function (theFuture) {
		if (theFuture.result && theFuture.result.length > 0) {
			for (var i = 0; i < theFuture.result.length; i += 1) {
				peopleBeforeManualLink[i] = _.extend(new Person(), theFuture.result[i]);
			}
			future.result = true;
		} else {
			future.exception = {
				error: "Trying to get the before image of the people failed"
			};
			reportResults(future.exception.error);
		}
	}).then(this, function (theFuture) {
		params.personToLinkTo = idsOfPeople[0];
		params.personToLink = idsOfPeople[1];
		console.log("Params1 - " + JSON.stringify(params));
		this.manualLinker.manuallyLink(future, params);
	}).then(this, function (theFuture) {
		if (theFuture.exception) {
			console.log(theFuture.exception.error);
			console.log("heya");
			reportResults(theFuture.exception.error);
		} else {
			future.nest(DB.get([idsOfPeople[0], idsOfPeople[1]]));
		}
	}).then(this, function (theFuture) {
		var i,
			j,
			temp,
			idsToFind = [],
			found;
		
		if (theFuture.result && theFuture.result.length > 0) {
			for (i = 0; i < theFuture.result.length; i += 1) {
				peopleAfterManualLink[i] = _.extend(new Person(), theFuture.result[i]);
			}
			
			if (peopleAfterManualLink[0]._id !== idsOfPeople[0]._id) {
				temp = peopleAfterManualLink[0];
				peopleAfterManualLink[0] = peopleAfterManualLink[1];
				peopleAfterManualLink[1] = temp;
			}
			
			if (!peopleAfterManualLink[1]._del) {
				future.exception = {
					error: "The abandoned person record is not marked for delete"
				};
				reportResults(future.exception.error);
			}
			
			for (i = 0; i < peopleBeforeManualLink[0].contactsIds.length; i += 1) {
				idsToFind[i] = peopleBeforeManualLink[0].contactsIds[i];
			}
			
			for (i = 0; i < peopleBeforeManualLink[1].contactsIds.length; i += 1) {
				idsToFind[peopleBeforeManualLink[0].contactsIds.length + i] = peopleBeforeManualLink[0].contactsIds[i];
			}
			
			console.log(JSON.stringify(idsToFind) + "\n" + JSON.stringify(peopleAfterManualLink[0].contactsIds));
			
			for (i = 0; i < idsToFind.length; i += 1) {
				found = false;
				for (j = 0; j < peopleAfterManualLink[0].contactsIds.length - 1; j += 1) {
					if (peopleAfterManualLink[0].contactsIds[j] === idsToFind[i]) {
						found = true;
						break;
					}
				}
				
				if (!found) {
					future.exception = {
						error: "Not all contacts were copied into the personToLinkTo person"
					};
					reportResults(future.exception.error);
				}
				
			}
			
		} else {
			future.exception = {
				error: "Trying to get the after image of the person failed"
			};
			reportResults(future.exception.error);
		}
		
	}).then(this, function (theFuture) {
		if (theFuture.exception) {
			console.log("exception");
			console.log(JSON.stringify(theFuture.exception.error));
			reportResults(theFuture.exception.error);
		} else {
			console.log("no exception");
			reportResults(MojoTest.passed);
		}
	});
	
};*/


function ManualUnlinkerTests() {
	/*
	 var params = { "personToLinkTo": this.idsOfTestPeople[0],
	 "personToLink": this.idsOfTestPeople[1] };
	 */
}

/*
 ManualUnlinkerTests.prototype.testParameters = function () {
 MojoTest.requireEqual(2, 1+1, "one plus one must equal two.");
 return MojoTest.passed;
 };
 */

