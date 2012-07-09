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
/*global Foundations, Utils, DB, console, Console, Future, PalmCall, ContactsLib, Assert, Contact, NameExtended, Person, Globalization, yieldController, ContactLinkBackup, _, Contact, ManualLink, ContactsUtils, ManualUnlink, AutoLinkUnit, WatchRevisionNumber, ContactFactory, Person, PersonFactory, PhoneNumber, timingRecorder */

// Do linking rule for no names + company
///////////////////////////////////////////////////////////////

function Autolinker() {
	// var personID = "";
}

// This is called as the result of a luna-send to force a re-autolink of all the contacts
Autolinker.prototype.forceAutolink = function (controller, jobId) {
	var future = new Future();
	
	future.now(this, function () {
		return this.performAutolink(undefined, controller, jobId);
	});
	
	return future;
};

// This is called as the result of the activity manager telling us
// our watch changed
Autolinker.prototype.dbUpdatedRelinkChanges = function (controller, jobId) {
	var future = new Future();
	
	future.now(this, function () {
		var param = controller.args;
		return this.performAutolink(param.revChangedStart || -1, controller, jobId);
	});
	
	return future;
};

Autolinker.prototype.saveNewPersonAndContacts = function (controller, params) {
	var future = new Future(),
		saveResult,
		person,
		contacts;
	
	future.now(function () {
		Assert.requireObject(params.person, "Autolinker.saveNewPersonAndContacts: You must specify a person object to save.");
		Assert.requireArray(params.contacts, "Autolinker.saveNewPersonAndContacts: You must specify an array of contacts to save.");

		person = params.person;
		contacts = params.contacts;

		if (!(person instanceof Person)) {
			person = new Person(person);
		}

		contacts = contacts.map(function (contact) {
			if (!(contact instanceof Contact)) {
				return ContactFactory.createContactLinkable(contact);
			} else {
				return contact;
			}
		});

		return person._savePersonAttachingTheseContacts(contacts);
	});
	
	future.then(function () {
		saveResult = future.result;
		
		var newPerson = new Person(person.getDBObject());
		return controller.service.assistant.dispatchToPlugins("personAdded", newPerson);
	});
	
	future.then(function () {
		var result = future.result;
		
		contacts = contacts.map(function (contact) {
			return contact.getDBObject();
		});
		
		return { saveResult: saveResult, person: person.getDBObject(), contacts: contacts };
	});
	
	return future;
};

////////////////////// Ranking Functions /////////////////////////////

Autolinker.similarName = function (person, contactToAutoLink, currentWeights) {
	var future = new Future(),
		onlyCompanyMatch = false;
	
	future.now(function () {
		var queries = [], 
			index,
			len,
			organizationArr,
			organization,
			personArr,
			personIndex,
			personLen,
			name;
		
		//timingRecorder.startTimingForJob("Building_Similar_Name_Query");

		personArr = (person ? person.getNames().getArray().concat([contactToAutoLink.getName()]) : [contactToAutoLink.getName()]);
		personLen = personArr.length;
		for (personIndex = 0; personIndex < personLen; personIndex++) {
			name = personArr[personIndex]; 
			if (name.getFamilyName() && name.getGivenName()) {
				queries.push({
					method: "find",
					params: {
						query: {
							"from": Person.kind,
							"where": [{
								"prop": "names.familyName",
								"op": "=",
								"val": name.getFamilyName(),
								"collate": "primary"
							}, {
								"prop": "names.givenName",
								"op": "=",
								"val": name.getGivenName(),
								"collate": "primary"
							}]
						}
					}
				});
			}
		}
			
		// No names so use company name
		if (queries.length < 1) {
			organizationArr = contactToAutoLink.getOrganizations().getArray();
			len = organizationArr.length;
			for (index = 0; index < len; index++) {
				organization = organizationArr[index];
				// Only use company names where there is not a job title
				if (organization.getName() && !organization.getTitle()) {
					queries.push({
						method: "find",
						params: {
							query: {
								"from": Person.kind,
								"where": [{
									"prop": "organization.name",
									"op": "=",
									"val": organization.getName(),
									"collate": "primary"
								}, {
									"prop": "organization.title",
									"op": "=",
									"val": "",
									"collate": "primary"
								}]
							}
						}
					});
					
					onlyCompanyMatch = true;
				}
			}
		}

		//timingRecorder.stopTimingForJob("Building_Similar_Name_Query");
		
		if (queries.length > 0) {
			//timingRecorder.startTimingForJob("Executing_Similar_Name_Query");
			
			return DB.execute("batch", {
				operations: queries
			});
		} else {
			return null;
		}
	});
	
	future.then(function () {
		var result = future.result,
			index,
			len,
			peopleArr,
			person;
		
		if (result) {
			//timingRecorder.stopTimingForJob("Executing_Similar_Name_Query");

			//timingRecorder.startTimingForJob("Analyzing_Similar_Name_Query_Results");
			
			peopleArr = Autolinker.getPeopleFromBatchedResults(result);
			len = peopleArr.length;
			for (index = 0; index < len; index++) {
				person = PersonFactory.createPersonLinkable(peopleArr[index]);
				
				if (onlyCompanyMatch) {
					if (person.getName().getFullName() || person.getOrganization().getTitle()) {
						continue;
					}
				} else {
					if ((person.getName().getHonorificSuffix() !== contactToAutoLink.getName().getHonorificSuffix()) && (person.getName().getHonorificSuffix() || contactToAutoLink.getName().getHonorificSuffix())) {
						continue;
					}

					if ((person.getName().getMiddleName() !== contactToAutoLink.getName().getMiddleName()) && (person.getName().getMiddleName() || contactToAutoLink.getName().getMiddleName())) {
						continue;
					}
				}
				
				Autolinker.addSimilarityToCurrentWeightForPerson(AutoLinkUnit.SIMILAR_NAME, currentWeights, person, false);
			}
			
			//timingRecorder.stopTimingForJob("Analyzing_Similar_Name_Query_Results");
		}
		
		
		return currentWeights;
	});
	
	return future;
};

Autolinker.SIMILAR_PHONENUMBER_QUERY = {
	"from": Person.kind,
	"where": [{
		"prop": "phoneNumbers.normalizedValue",
		"op": "%"
	}]
};

Autolinker.addPhoneNumberToQuery = function (queries, phoneNumber) {
	var parsedPhoneNumber,
		normalizedPhoneNumber,
		query;
		
	parsedPhoneNumber = Globalization.Phone.parsePhoneNumber(phoneNumber.getValue());
	if (!parsedPhoneNumber) {
		//if parsing doesn't work for some reason, go to the next phoneNumber
		return;
	}
	normalizedPhoneNumber = PhoneNumber.normalizePhoneNumber(parsedPhoneNumber, true);
	if (!normalizedPhoneNumber) {
		//if normalizing doesn't work for some reason, go to the next phoneNumber
		return;
	}
	
	query = _.clone(Autolinker.SIMILAR_PHONENUMBER_QUERY);
	query.where[0].val = normalizedPhoneNumber;
	
	queries.push({
		method: "find",
		params: {
			query: query
		}
	});
	
	return parsedPhoneNumber;
};

Autolinker.similarPhoneNumber = function (person, contactToAutoLink, currentWeights) {
	var future = new Future(),
		parsedPhoneNumbers = {};
	
	future.now(function () {
		var queries = [],
			parsedPhoneNumber,
			numbersArr,
			index,
			len,
			phoneNumber;
		
		//timingRecorder.startTimingForJob("Building_Similar_PhoneNumber_Query");
		
		numbersArr = (person ? person.getPhoneNumbers().getArray().concat(contactToAutoLink.getPhoneNumbers().getArray()) : contactToAutoLink.getPhoneNumbers().getArray());
		len = numbersArr.length;
		for (index = 0; index < len; index++) {
			phoneNumber = numbersArr[index];
			if (phoneNumber.getType() === PhoneNumber.TYPE.MOBILE) {
				if (!parsedPhoneNumbers[phoneNumber.getValue()]) {
					parsedPhoneNumber = Autolinker.addPhoneNumberToQuery(queries, phoneNumber);
					parsedPhoneNumbers[phoneNumber.getValue()] = parsedPhoneNumber;
				}
			}
		}

		//timingRecorder.stopTimingForJob("Building_Similar_PhoneNumber_Query");

		if (queries.length > 0) {
			//timingRecorder.startTimingForJob("Executing_Similar_PhoneNumber_Query");
			
			return DB.execute("batch", {
				operations: queries
			});
		} else {
			return null;
		}
	});
	
	future.then(function () {
		var result = future.result,
			peopleFromDB,
			linkedPhoneNumbers,
			prefixNumberMapping = {},
			peopleArr,
			index,
			len,
			personIsSimilarOnNumber,
			pLPerson,
			newPersonPhoneNumbers,
			rawPerson;
		
		if (result) {
			//timingRecorder.stopTimingForJob("Executing_Similar_PhoneNumber_Query");
			
			//timingRecorder.startTimingForJob("Analyzing_Similar_PhoneNumber_Query_Results");
			
			linkedPhoneNumbers = person ? person.getPhoneNumbers().getArray().concat(contactToAutoLink.getPhoneNumbers().getArray()) : contactToAutoLink.getPhoneNumbers().getArray();
			
  // TODO: is this an incorrect use of map because it will not squash
  //       undefined values? (a more suspicious similar case is right below)
			linkedPhoneNumbers = linkedPhoneNumbers.map(function (phoneNumber) {
				if (phoneNumber.getType() === PhoneNumber.TYPE.MOBILE) {
					return phoneNumber;
				}
			});
			
			peopleArr = Autolinker.getPeopleFromBatchedResults(result);
			len = peopleArr.length;
			for (index = 0; index < len; index++) {
				rawPerson = peopleArr[index];
				pLPerson = PersonFactory.createPersonLinkable(rawPerson);
				newPersonPhoneNumbers = pLPerson.getPhoneNumbers().getArray();
				
  // TODO: isn't this an incorrect use of map because it will not squash
  //       undefined values but then we check the length afterwards.
				newPersonPhoneNumbers = newPersonPhoneNumbers.map(function (phoneNumber) {
					if (phoneNumber.getType() === PhoneNumber.TYPE.MOBILE) {
						return phoneNumber;
					}
				});
				
				if (newPersonPhoneNumbers.length < 1) {
					continue;
				}
				
				prefixNumberMapping = Autolinker.getPrefixNumberMapping(linkedPhoneNumbers, newPersonPhoneNumbers);
				
				personIsSimilarOnNumber = Object.keys(prefixNumberMapping).some(function (key) {
					var numbers = prefixNumberMapping[key],
						cLPNumbers = numbers.currentLinkedPerson,
						pLPNumbers = numbers.potentiallyLinkedPerson,
						numberDidntMatched;
					
					if (cLPNumbers.length > 0 && pLPNumbers.length > 0) {
						numberDidntMatched = cLPNumbers.some(function (clpNumber) {
							var parsedCLPNumber;
							
							if (!parsedPhoneNumbers[clpNumber.getValue()]) {
								parsedCLPNumber = Globalization.Phone.parsePhoneNumber(clpNumber.getValue());
								parsedPhoneNumbers[clpNumber.getValue()] = parsedCLPNumber;
							}
							
							parsedCLPNumber = parsedPhoneNumbers[clpNumber.getValue()];
							
							return pLPNumbers.some(function (plpNumber) {
								var parsedPLPNumber;
								
								if (!parsedPhoneNumbers[plpNumber.getValue()]) {
									parsedPLPNumber = Globalization.Phone.parsePhoneNumber(plpNumber.getValue());
									parsedPhoneNumbers[plpNumber.getValue()] = parsedPLPNumber;
								}
								
								parsedPLPNumber = parsedPhoneNumbers[plpNumber.getValue()];
								
								// If it matches return false to keep checking, otherwise return true and stop checking for matches for this number
								if (Globalization.Phone.comparePhoneNumbers(parsedCLPNumber, parsedPLPNumber) > 0) {
									return false;
								} else {
									return true;
								}
							});
						});
						
						if (numberDidntMatched) {
							return false;
						} else {
							Autolinker.addSimilarityToCurrentWeightForPerson(AutoLinkUnit.SIMILAR_MOBILE_PHONE_NUMBER, currentWeights, pLPerson, true);
							linkedPhoneNumbers = linkedPhoneNumbers.concat(newPersonPhoneNumbers);
							return true;
						}
					} else {
						return false;
					}
				});
			}
						
			//timingRecorder.stopTimingForJob("Analyzing_Similar_PhoneNumber_Query_Results");
		}
		
		return currentWeights;
	});
	
	return future;
};

Autolinker.getPrefixNumberMapping = function (linkedPhoneNumbers, newPersonNumbers) {
	var toReturn = {},
		lIndex,
		lLen,
		linkedPhoneNumber,
		nIndex,
		nLen,
		newPhoneNumber,
		normalizedPhoneNumber;
	
	lLen = linkedPhoneNumbers.length;
	for (lIndex = 0; lIndex < lLen; lIndex++) {
		linkedPhoneNumber = linkedPhoneNumbers[lIndex];
		if (!linkedPhoneNumber) {
			continue;
		}
		
		normalizedPhoneNumber = PhoneNumber.normalizePhoneNumber(linkedPhoneNumber.getValue(), true);
		if (!normalizedPhoneNumber) {
			//if normalizing doesn't work for some reason, go to the next phoneNumber
			continue;
		}
		
		if (!toReturn[normalizedPhoneNumber]) {
			toReturn[normalizedPhoneNumber] = { currentLinkedPerson: [], potentiallyLinkedPerson: [] };
		}
		
		toReturn[normalizedPhoneNumber].currentLinkedPerson.push(linkedPhoneNumber);
	}
	
	nLen = newPersonNumbers.length;
	for (nIndex = 0; nIndex < nLen; nIndex++) {
		newPhoneNumber = newPersonNumbers[nIndex];
		if (!newPhoneNumber) {
			continue;
		}
		
		normalizedPhoneNumber = PhoneNumber.normalizePhoneNumber(newPhoneNumber.getValue(), true);
		if (!normalizedPhoneNumber) {
			//if normalizing doesn't work for some reason, go to the next phoneNumber
			continue;
		}
		
		if (!toReturn[normalizedPhoneNumber]) {
			toReturn[normalizedPhoneNumber] = { currentLinkedPerson: [], potentiallyLinkedPerson: [] };
		}
		
		toReturn[normalizedPhoneNumber].potentiallyLinkedPerson.push(newPhoneNumber);
	}
	
	return toReturn;
};

Autolinker.SIMILAREMAILQUERY = {
	"from": Person.kind,
	"where": [{
		"prop": "emails.normalizedValue",
		"op": "="
	}]
};

Autolinker.similarEmail = function (person, contactToAutoLink, currentWeights) {
	var future = new Future();
	
	future.now(function () {
		var queries = [];
		
		//timingRecorder.startTimingForJob("Building_Similar_EmailAddress_Query");
		
		queries = Autolinker.buildSinglePropertyQueries(Autolinker.SIMILAREMAILQUERY, (person ? person.getEmails().getArray() : []).concat(contactToAutoLink.getEmails().getArray()), "getNormalizedValue");
		
		//timingRecorder.stopTimingForJob("Building_Similar_EmailAddress_Query");

		if (queries.length > 0) {
			//timingRecorder.startTimingForJob("Executing_Similar_EmailAddress_Query");

			return DB.execute("batch", {
				operations: queries
			});
		} else {
			return null;
		}
	});
	
	
	future.then(function () {
		var result = future.result,
		index,
		len,
		personArr,
		tempPerson;
		
		if (result) {
			//timingRecorder.stopTimingForJob("Executing_Similar_EmailAddress_Query");

			//timingRecorder.startTimingForJob("Analyzing_Similar_EmailAddress_Query_Results");
			
			personArr = Autolinker.getPeopleFromBatchedResults(result);
			len = personArr.length;
			for (index = 0; index < len; index++) {
				tempPerson = personArr[index];
				Autolinker.addSimilarityToCurrentWeightForPerson(AutoLinkUnit.SIMILAR_EMAIL, currentWeights, PersonFactory.createPersonLinkable(tempPerson), true);
			}

			//timingRecorder.stopTimingForJob("Analyzing_Similar_EmailAddress_Query_Results");
		}
		
		return currentWeights;
	});
	
	return future;
};

Autolinker.SIMILARIMQUERY = {
	"from": Person.kind,
	"where": [{
		"prop": "ims.normalizedValue",
		"op": "="
	}]
};

Autolinker.similarIM = function (person, contactToAutoLink, currentWeights) {
	var future = new Future();
	
	future.now(function () {
		var queries = [];
		
		//timingRecorder.startTimingForJob("Building_Similar_IMAddress_Query");
		
		queries = Autolinker.buildSinglePropertyQueries(Autolinker.SIMILARIMQUERY, (person ? person.getIms().getArray() : []).concat(contactToAutoLink.getIms().getArray()), "getNormalizedValue");
		
		//timingRecorder.stopTimingForJob("Building_Similar_IMAddress_Query");
		
		if (queries.length > 0) {
			//timingRecorder.startTimingForJob("Executing_Similar_IMAddress_Query");
			
			return DB.execute("batch", {
				operations: queries
			});
		} else {
			return null;
		}
	});
	
	future.then(function () {
		var result = future.result,
			index,
			len,
			peopleArr,
			tempPerson;
		
		if (result) {
			//timingRecorder.stopTimingForJob("Executing_Similar_IMAddress_Query");

			//timingRecorder.startTimingForJob("Analyzing_Similar_IMAddress_Query_Results");
			
			peopleArr = Autolinker.getPeopleFromBatchedResults(result);
			len = peopleArr.length;
			for (index = 0; index < len; index++) {
				tempPerson = peopleArr[index];
				Autolinker.addSimilarityToCurrentWeightForPerson(AutoLinkUnit.SIMILAR_IM, currentWeights, PersonFactory.createPersonLinkable(tempPerson), true);
			}

			//timingRecorder.stopTimingForJob("Analyzing_Similar_IMAddress_Query_Results");
		}
		
		return currentWeights;
	});
	
	return future;
};

Autolinker.manualLinks = function (person, contactToAutoLink, currentWeights) {
	var clb,
		theManualLinks = [],
		future = new Future();
	
	future.now(function () {
		clb = new ContactLinkBackup();
		//timingRecorder.startTimingForJob("Getting_ManualLinks_From_CLB");
		return clb.getManualLinks(contactToAutoLink);
	});
	
	future.then(function () {
		var result = future.result;
		//timingRecorder.stopTimingForJob("Getting_ManualLinks_From_CLB");
		//timingRecorder.startTimingForJob("Analyzing_ManualLinks_From_CLB_Results");
		if (result && result.results) {
			//Console.log(JSON.stringify(theFuture.result.results));
			theManualLinks = result.results;
			return Foundations.Control.mapReduce({
				map: function (manualLink) {
					var future = new Future();
					
					future.now(function () {
						return contactToAutoLink.getPersonFromCLBObject(_.extend(new ManualLink(), manualLink));
					});
					
					future.then(function () {
						var result = future.result,
							tempPeople = [],
							index,
							len,
							tempPerson;

						if (result.length > 1) {
							tempPeople[0] = result[0] || undefined;
							tempPeople[1] = result[1] || undefined;
						} else if (result.length > 0) {
							tempPeople[0] = result[0] || undefined;
						}

						len = tempPeople.length;
						for (index = 0; index < len; index++) {
							tempPerson = tempPeople[index];
							if (tempPerson) {
								Autolinker.addSimilarityToCurrentWeightForPerson(AutoLinkUnit.SIMILAR_CLB_MANUAL_LINK, currentWeights, tempPerson, true);
							}
						}
						
						return true;
					});
					
					return future;
				}
			}, theManualLinks);
		} else {
			Console.log("No manual links for contactToAutolink");
			return true;
		}
	});
	
	future.then(function () {
		var dummy = future.result;
		
		//timingRecorder.stopTimingForJob("Analyzing_ManualLinks_From_CLB_Results");
		
		return currentWeights;
	});
	
	return future;
};

Autolinker.manualUnlinks = function (person, contactToAutoLink, currentWeights) {
	var clb,
		contactsToGetCLB = [],
		contactsToGet = [],
		future = new Future();
	
	future.now(function () {
		clb = new ContactLinkBackup();
		// Assemble the contacts to get the clb links from
		contactsToGet = Autolinker.getContactIdsFromPeopleInWeights(currentWeights);

		//timingRecorder.startTimingForJob("Getting_ManualUnlinks_From_CLB");

		return DB.get(contactsToGet);
	});
	
	future.then(function () {
		// Store the Contacts and kick off the function to get the clbs for them
		var rawContacts = Utils.DBResultHelper(future.result),
			index,
			len,
			rawContact;
			
		if (rawContacts) {
			len = rawContacts.length;
			for (index = 0; index < len; index++) {
				rawContact = rawContacts[index];
				contactsToGetCLB.push(ContactFactory.createContactLinkable(rawContact));
			}
		}
		
		return Foundations.Control.mapReduce({
			map: function (contact) {
				var mapReduceFuture = new Future();
				
				mapReduceFuture.now(function () {
					return clb.getUnlinkLinks(contact);
				});
				
				mapReduceFuture.then(function () {
					var hasEntityA = false,
						hasEntityB = false,
						idOfHashA,
						idOfHashB,
						contactIds,
						addUnlinkSimilarityToOtherWeights = function (idToFind) {
							var index,
								len,
								weights,
								cIndex,
								cLen,
								contactId,
								tempWeight;
							
							weights = Object.keys(currentWeights);
							len = weights.length;
							for (index = 0; index < len; index++) {
								tempWeight = currentWeights[weights[index]];
								contactIds = tempWeight.person.getContactIds().getArray();

								cLen = contactIds.length;
								for (cIndex = 0; cIndex < cLen; cIndex++) {
									contactId = contactIds[cIndex];
									if (contactId.getValue() === idToFind) {
										Autolinker.addSimilarityToCurrentWeightForPerson(AutoLinkUnit.SIMILAR_CLB_MANUAL_UNLINK, currentWeights, tempWeight.person, true);
									}
								}
							}							
						},
						addUnlinkSimilarityToWeights = function (idToFind, weightToIgnore) {
							var weights,
								windex,
								wlen,
								tempWeight,
								cIndex,
								cLen,
								contactId;
							
							weights = Object.keys(currentWeights);
							wlen = weights.length;
							for (windex = 0; windex < wlen; windex++) {
								tempWeight = currentWeights[weights[windex]];
								if (weightToIgnore !== tempWeight) {
									contactIds = tempWeight.person.getContactIds().getArray();

									cLen = contactIds.length;
									for (cIndex = 0; cIndex < cLen; cIndex++) {
										contactId = contactIds[cIndex];
										if (contactId.getValue() === idToFind) {
											Autolinker.addSimilarityToCurrentWeightForPerson(AutoLinkUnit.SIMILAR_CLB_MANUAL_UNLINK, currentWeights, tempWeight.person, true);
										}
									}									
								}
							}
						},
						results = mapReduceFuture.result && mapReduceFuture.result.results;

					//timingRecorder.startTimingForJob("Getting_ManualUnlinks_From_CLB_For_A_Contact");

					Assert.require(results, "Autolinker.manualUnlinks: did not get a result when getting manualUnlinks");

					return Foundations.Control.mapReduce({
						map: function (manualUnlink) {
							var innerFuture = new Future(),
								idOfHashA,
								idOfHashB,
								contactToAutoLinkInHashA = false,
								contactToAutoLinkInHashB = false;

							innerFuture.now(function () {
								return Contact.getIdFromLinkHash(manualUnlink.contactEntityA);
							});

							innerFuture.then(function () {
								var result = innerFuture.result;

								idOfHashA = result;

								return Contact.getIdFromLinkHash(manualUnlink.contactEntityB);
							});

							innerFuture.then(function () {
								var result = innerFuture.result,
									index,
									len,
									weights,
									weight,
									cindex,
									clen,
									contactId;

								idOfHashB = result;

								if (idOfHashA === contactToAutoLink.getId()) {
									contactToAutoLinkInHashA = true;
								}

								if (idOfHashB === contactToAutoLink.getId()) {
									contactToAutoLinkInHashB = true;
								}

								if (contactToAutoLinkInHashA || contactToAutoLinkInHashB) {
									if (contactToAutoLinkInHashA) {
										addUnlinkSimilarityToOtherWeights(idOfHashB);
									} else {
										addUnlinkSimilarityToOtherWeights(idOfHashA);
									}
								} else {
									weights = Object.keys(currentWeights);
									len = weights.length;
									for (index = 0; index < len; index++) {
										weight = currentWeights[weights[index]];
										contactIds = weight.person.getContactIds().getArray();
										
										clen = contactIds.length;
										for (cindex = 0; cindex < clen; cindex++) {
											contactId = contactIds[cindex];
											if (contactId.getValue() === idOfHashA) {
												addUnlinkSimilarityToWeights(idOfHashB, weight);
											}

											if (contactId.getValue() === idOfHashB) {
												addUnlinkSimilarityToWeights(idOfHashA, weight);
											}
										}
									}
								}

								return true;
							});

							return innerFuture;
						}
					}, results).then(function (mapReduceFuture) {
						var result = mapReduceFuture.result;
						
						//timingRecorder.stopTimingForJob("Getting_ManualUnlinks_From_CLB_For_A_Contact");
						
						return true;
					});
				});
				
				return mapReduceFuture;
			}
		}, contactsToGetCLB);
	});
	
	future.then(function () {
		var dummy = future.result;
		
		//timingRecorder.stopTimingForJob("Getting_ManualUnlinks_From_CLB");
		
		return currentWeights;
	});
	
	return future;
	//////////////////////////////////////////////////////////////////
};

// Go through all of the weights we accumulated and get the contactIds
// for all of the people in the weight objects.
Autolinker.getContactIdsFromPeopleInWeights = function (weights) {
	var toReturn = [],
		contactIdObjects,
		cIds,
		index,
		len,
		tempWeight,
		contactId,
		indexContactId,
		lenContactId;
	
	cIds = Object.keys(weights);
	len = cIds.length;
	for (index = 0; index < len; index++) {
		tempWeight = cIds[index];
		tempWeight = weights[tempWeight];
		contactIdObjects = tempWeight.person.getContactIds().getArray();
		lenContactId = contactIdObjects.length;
		
		for (indexContactId = 0; indexContactId < lenContactId; indexContactId++) {
			contactId = contactIdObjects[indexContactId];
			toReturn.push(contactId.getValue());
		}
	}
	
	return toReturn;
};

Autolinker.getPeopleFromBatchedResults = function (batchResult) {
	var toReturn = [],
		responses,
		index,
		len;
	
	len = batchResult.responses.length;
	for (index = 0; index < len; index++) {
		responses = batchResult.responses[index];
		toReturn = toReturn.concat(responses.results);
	}

	return toReturn;
};

Autolinker.buildSinglePropertyQueries = function (query, objects, functionName) {
	return objects.map(function (object) {
		var queryCopy = _.clone(query);
		queryCopy.where = [];
		queryCopy.where[0] = _.clone(query.where[0]);
		queryCopy.where[0].val = object[functionName]();
		
		return {
			method: "find",
			params: {
				query: queryCopy
			}
		};
	});
};

Autolinker.addSimilarityToCurrentWeightForPerson = function (similarity, currentWeights, person, additive) {
	var autoLinkUnit;
	
	if (currentWeights[person.getId()]) {
		autoLinkUnit = currentWeights[person.getId()];
	} else {
		autoLinkUnit = new AutoLinkUnit();
		autoLinkUnit.person = person;
	}
	
	autoLinkUnit.addSimilarity(similarity, additive);
	currentWeights[person.getId()] = autoLinkUnit;
};

//////////////////////////////////////////////////////////////////////

Autolinker.completedOrYieldingAutolinker = function (controller, yielding, jobId, contactCurrentlyAutolinking, highestContactRev, contactProcessedCount, contactsToProcess, beforeMsec, accumulators) {
	var revIdToSave = -1,
		future = new Future();
	// We have processed all the changed contacts. Set up the watch to run. That is if we were called from
	// the watch
	
	future.now(function () {
		var afterDate,
			afterMsec;
		
		if (yielding) {
			console.log("Autolinker job - " + jobId + " got a request to yield!!!!!");
			console.log("Tearing down the autolink process as a response to the yield!!!!!");
			console.log("Setting up the next run of the autolinker as the result of a yield");
			afterDate = new Date();
			afterMsec = afterDate.getTime();
			afterMsec -= beforeMsec;

			console.log("Yield allowed us to only process " + contactProcessedCount + " out of " + contactsToProcess.length + " contacts in " + (afterMsec / 1000) + " seconds");
		}

		if (contactCurrentlyAutolinking) {
			if (highestContactRev < contactCurrentlyAutolinking.getRev()) {
				highestContactRev = contactCurrentlyAutolinking.getRev();
			} 

			revIdToSave = highestContactRev;
		} else {
			revIdToSave = highestContactRev;
		}

		Console.log("Saving last processed contact's revision number: " + revIdToSave);

		//timingRecorder.printTimings();
		
		if ((revIdToSave || revIdToSave === 0) && (revIdToSave > controller.lastProcessedContactRevId)) {
			controller.lastProcessedContactRevId = revIdToSave;
		}
		
		if (yielding) {
			accumulators.yielding = true;
		}
		
		return true;
	});
	
	return future;
};

Autolinker.prototype.performAutolink = function (startWatchRev, controller, jobId, processTheseContacts) {
	var beforeDate = new Date(),
		listSortOrder,
		future = new Future(),
		accumulators = {
			contactsToProcess: [],
			contactProcessedCount: 0,
			highestContactRev: startWatchRev,
			beforeMsec: beforeDate.getTime(),
			yielding: false
		};
	
	future.now(this, function () {
		var appPrefs = new ContactsLib.AppPrefs(future.callback(function () {
			var dummy = future.result;
			
			Console.log("Got list sortOrder");
			
			//store away the listSortOrder
			listSortOrder = appPrefs.get(ContactsLib.AppPrefs.Pref.listSortOrder);
			
			return true;
		}));
	});
	
	future.then(this, function () {
		var dummy = future.result;
		
		Console.log("Getting the contacts to autolink");
		
		//timingRecorder.startTimingForJob("Getting_All_Contacts_To_Process");
		return (processTheseContacts ? processTheseContacts : (startWatchRev ? this.getContactsAboveRevToAutolink(startWatchRev) : this.getAllContacts()));
	});
	
	future.then(this, function () {
		var currentContactProcessing = 0,
			processContactsFuture = new Future(true),
			that = this,
			contact,
			index,
			len,
			i;
		
		accumulators.contactsToProcess = future.result;
		
		Console.log("Starting contact processing");
		
		//timingRecorder.stopTimingForJob("Getting_All_Contacts_To_Process");
		
		// do not change this as we need a closure to call each of the contacts
		accumulators.contactsToProcess.forEach(function (contact, index) {
			processContactsFuture.then(function () {
				var shouldYield,
					dummy;
				
				try {
					dummy = processContactsFuture.result;
				} catch (e) {
					console.log("There was an exception processing a contact. Continuing processing of rest. Message - " + e.message ? e.message : "No message");
				}
				
				shouldYield = yieldController.shouldJobYield(jobId);
				
				if (!accumulators.yielding) {
					if (shouldYield) {
						if ((index - 1) >= 0) {
							return Autolinker.completedOrYieldingAutolinker(controller, true, jobId, ContactFactory.createContactLinkable(accumulators.contactsToProcess[index - 1]), accumulators.highestContactRev, accumulators.contactProcessedCount, accumulators.contactsToProcess, accumulators.beforeMsec, accumulators);
						} else {
							return Autolinker.completedOrYieldingAutolinker(controller, true, jobId, undefined, accumulators.highestContactRev, accumulators.contactProcessedCount, accumulators.contactsToProcess, accumulators.beforeMsec, accumulators);
						}
					} else {
						return Autolinker.getTheNextContactAndProcess(jobId, controller, listSortOrder, ContactFactory.createContactLinkable(contact), accumulators);
					}
				} else {
					// Let the chain unravel. 
					return true;
				}
			});
		});
		
		return processContactsFuture;
	});
	
	future.then(this, function () {
		var dummy = future.result;
		
		Console.log("Finished processing contacts");
		
		if (!accumulators.yielding) {
			return Autolinker.completedOrYieldingAutolinker(controller, false, jobId, undefined, accumulators.highestContactRev, accumulators.contactProcessedCount, accumulators.contactsToProcess, accumulators.beforeMsec, accumulators);
		} else {
			return true;
		}
	});
	

	return future;
};

Autolinker.getTheNextContactAndProcess = function (jobId, controller, listSortOrder, contactCurrentlyAutolinking, accumulators) {
	var afterDate,
		afterMsec,
		getCurrentWatchRevFuture,
		doneSaveNewWatchRevFuture,
		revIdToSave = -1,
		shouldYield,
		future = new Future();
		
	future.now(function () {
		Console.log("Processing A Contact");
		
		//timingRecorder.startTimingForJob("Processing_A_Contact");
		
		if (accumulators.highestContactRev < contactCurrentlyAutolinking.getRev()) {
			accumulators.highestContactRev = contactCurrentlyAutolinking.getRev();
		}
		//Console.log("Made a linkable contact");
		//processOneContactFuture.then(this, processTheNextContact);
		//this.getContactsPerson(contactCurrentlyAutolinking, processOneContactFuture);

		return Autolinker.getContactsPerson(contactCurrentlyAutolinking);
	});

	future.then(function () {
		var result = future.result,
			person = result.person,
			contacts = result.contacts;
		
		Console.log("Got Contacts Person");
			
		if (person || contacts) {
			accumulators.contactProcessedCount += 1;

			if (accumulators.contactProcessedCount % 10 === 0) {
				console.log("Processing Contact - " + accumulators.contactProcessedCount + " of " + accumulators.contactsToProcess.length);
			}

			if (contactCurrentlyAutolinking.markedForDelete()) {
				if (person) {
					if (person.getContactIds().getArray().length > 1) {
						Console.log("Removing contact from person");
						return Autolinker.removeAContactFromPerson(person, contactCurrentlyAutolinking, listSortOrder, controller);
					} else {
						Console.log("Removing a person");
						return Autolinker.removeAPerson(person, controller);
					}
				} else {
					Console.log("Contact marked for delete but it didn't have a person. Continuing.");
					return new Future(true);
				}
			}
		}
		
		Console.log("Starting autolink of contact");
		return Autolinker.doAutoLinkProcess(person, contacts, contactCurrentlyAutolinking, controller, listSortOrder);
	});
	
	future.then(function () {
		var dummy = future.result;
		
		//timingRecorder.stopTimingForJob("Processing_A_Contact");
		
		return true;
	});

	return future;
};

Autolinker.removeAPerson = function (person, controller) {
	var future = new Future();
	
	future.now(function () {
		return person.deletePerson();
	});
	
	future.then(function () {
		//timingRecorder.startTimingForJob("Dispatching_Linker_Plugins_Person_Removed");
		return controller.service.assistant.dispatchToPlugins("personRemoved", person); // isolated removal
	});
	
	future.then(function () {
		//timingRecorder.stopTimingForJob("Dispatching_Linker_Plugins_Person_Removed");
		return true;
	});
	
	return future;
};

Autolinker.removeAContactFromPerson = function (person, contactCurrentlyAutolinking, listSortOrder, controller) {
	var future = new Future(),
		personOriginal;
	
	future.now(function () {
		// Remove a contact from a person
		personOriginal = new Person(person.getDBObject());
		person.removeContactId(contactCurrentlyAutolinking.getId());
		// Just removing a contact from a person so we don't have to handling merging of person
		// data from an autolink
		//timingRecorder.startTimingForJob("Fixup_Remove_Contact_From_Person");
		return person.fixup(undefined, {
			listSortOrder: listSortOrder,
			timingRecorder: timingRecorder
		});
	});
	
	future.then(function () {
		//timingRecorder.stopTimingForJob("Fixup_Remove_Contact_From_Person");
		//timingRecorder.startTimingForJob("Remove_Contact_From_Person_Save");
		return person.save();
	});
	
	future.then(function () {
		//timingRecorder.stopTimingForJob("Remove_Contact_From_Person_Save");
		//timingRecorder.startTimingForJob("Dispatching_Linker_Plugins_Person_Changed");
		
		return controller.service.assistant.dispatchToPlugins("personChanged", personOriginal, person);
	});
	
	future.then(function () {
		//timingRecorder.stopTimingForJob("Dispatching_Linker_Plugins_Person_Changed");
		return true;
	});
	
	return future;
};

Autolinker.createNewActivity = function (revIdToSave) {
	var future = new Future();
	
	future.now(function () {
		return Autolinker.getCurrentWatchRev();
	});
	
	future.then(function () {
		var theRevision = future.result;
		if (theRevision) {
			theRevision.revisionNumber = revIdToSave;
			return theRevision.save();
		} else {
			// If this happens it will cause the first autolink to use the old rev so we will be re-autloinking
			// more contacts that we need to. But this way we don't lose any. A re-autolink should not cause problems
			Console.log("Saving the new rev failed. We will create the activity anyway with the contact's rev value.");
			return true;
		}
	});
	
	future.then(function () {
		console.log("Done with processing contacts and autolink - Now setting up new watch for changes! - With revision number: " + revIdToSave);
		return Autolinker.createActivityUsingRevisionNumber(revIdToSave);
	});
	
	return future;
};


// This is where all the autolinking for a single contact happens
Autolinker.doAutoLinkProcess = function (person, contacts, contactToAutoLink, controller, listSortOrder) {
	var future = new Future(),
		shouldAutolink = [];
	
	future.now(function () {
		Console.log("Doing Ranking Functions");
		return Autolinker.doRankingFunctions(person, contactToAutoLink);
	});
	
	future.then(function () {
		var weightedResults = future.result,
			wResultsArray,
			index,
			len,
			weightedResult,
			samePerson;
		
		//console.log("Done getting the rankings for contact to autolink -- " + JSON.stringify(weightedResults));
		wResultsArray = Object.keys(weightedResults);
		len = wResultsArray.length;
		for (index = 0; index < len; index++) {
			weightedResult = weightedResults[wResultsArray[index]];
		
			if (weightedResult.meetsWeightRequirement()) {
				samePerson = true;
				if (person) {
					if (weightedResult.person.getId() !== person.getId()) {
						samePerson = false;
						shouldAutolink.push(weightedResult.person);
					}
				} else {
					samePerson = false;
					shouldAutolink.push(weightedResult.person);
				}
				
				if (samePerson) {
					//Console.log("*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#\nThis person meets the weight requirement for auto-linking - But same person as currently linked. Won't be autolinked.\n" + JSON.stringify(weightedResult) + "\n--------------------------------------------------------------------");
				} else {
					//Console.log("****************************************\nThis person meets the weight requirement for auto-linking\n" + JSON.stringify(weightedResult) + "\n--------------------------------------------------------------------");
				}
			} else {
				//Console.log("########################################\nThis person does not meet the weight requirement for auto-linking\n" + JSON.stringify(weightedResult) + "\n--------------------------------------------------------------------");
			}
		}
		
		if (shouldAutolink.length > 0) {
			Console.log("Linking similar people together");
			return Autolinker.autolinkThesePeople(person, shouldAutolink, contactToAutoLink, controller);
		} else {
			Console.log("No similar people were found to autolink with the contact: " + contactToAutoLink);
			if (person) {
				return Autolinker.savePersonWithoutNewAutolinks(person, contacts, controller);
			} else {
				return Autolinker.saveNewPersonNoAutolinks(contactToAutoLink, controller, listSortOrder);
			}
		}
	});
	
	return future;
};

Autolinker.saveNewPersonNoAutolinks = function (contactToAutolink, controller, listSortOrder) {
	var future = new Future(),
		newPerson;
	
	future.now(function () {
		newPerson = PersonFactory.createPersonLinkable();
		newPerson.addContact(contactToAutolink);
		newPerson.setContacts([contactToAutolink]);
		
		Console.log("*$*$*$*$*$*$*$*$*$*$*$$*\n*$*$*$*$*$*$*$*\n" + newPerson + "\n*$*$*$*$*$**$$*\n" + contactToAutolink + "\n*$*$*$*$*$*$**$*$*$*$*$*$*$*$");
		//timingRecorder.startTimingForJob("Fixup_Added_Person_No_AutoLink");
		
		return newPerson.fixupFromObjects([contactToAutolink], ContactFactory.ContactType.LINKABLE, undefined, {
			listSortOrder: listSortOrder,
			timingRecorder: timingRecorder
		});
	});
	// This is the case where there was no person for this contact and he did not autolink to anyone.
	
	future.then(function () {
		var dummy = future.result;
		
		Console.log("Saving fixedup new person");
		//timingRecorder.stopTimingForJob("Fixup_Added_Person_No_AutoLink");
		//timingRecorder.startTimingForJob("Added_Person_No_AutoLink_Save");
		
		return newPerson.save();
	});

	future.then(function () {
		var dummy = future.result;
		
		//timingRecorder.stopTimingForJob("Added_Person_No_AutoLink_Save");
		//timingRecorder.startTimingForJob("Dispatching_Linker_Plugins_Person_Added");
		
		return controller.service.assistant.dispatchToPlugins("personAdded", newPerson); // isolated person add
	});
	
	future.then(function () {
		var dummy = future.result;
		
		//timingRecorder.stopTimingForJob("Dispatching_Linker_Plugins_Person_Added");
		
		return true;
	});
	
	return future;
};

Autolinker.savePersonWithoutNewAutolinks = function (person, contacts, controller, listSortOrder) {
	var future = new Future(),
		personOriginal;
	
	future.now(function () {
		// This is the case that a contact was updated and this change did not cause an autolink
		personOriginal = new Person(person.getDBObject());
		//timingRecorder.startTimingForJob("Fixup_No_AutoLink");
		
		return person.fixupFromObjects(contacts, ContactFactory.ContactType.LINKABLE, undefined, {
			listSortOrder: listSortOrder,
			timingRecorder: timingRecorder
		});
	});
	
	future.then(function () { 
		var dummy = future.result;
		
		//timingRecorder.stopTimingForJob("Fixup_No_AutoLink");
		//timingRecorder.startTimingForJob("No_AutoLinker_Person_Save");
		
		return person.save();
	});
	
	future.then(function () {
		var dummy = future.result;
		
		//timingRecorder.stopTimingForJob("No_AutoLinker_Person_Save");
		//timingRecorder.startTimingForJob("Dispatching_Linker_Plugins_Person_Changed");
		
		return controller.service.assistant.dispatchToPlugins("personChanged", personOriginal, person); // isolated change
	});
	
	future.then(function () {
		//timingRecorder.stopTimingForJob("Dispatching_Linker_Plugins_Person_Changed");
		
		return true;
	});
	
	return future;
};

Autolinker.autolinkThesePeople = function (person, shouldAutolink, contactToAutolink, controller, listSortOrder) {
	var future = new Future(),
		foundPersonInShouldAutoLink = false,
		personWithMostContacts,
		personWithMostContactsOriginal;
	
	future.now(function () {
		var mapReduceToGetContacts = [],
			indexOfMost = 0,
			mostContacts = 0,
			index,
			personToAutolink,
			len;
		
		len = shouldAutolink.length;
		for (index = 0; index < len; index++) {
			personToAutolink = shouldAutolink[index];
			if (mostContacts <= personToAutolink.getContactIds().getArray().length) {
				indexOfMost = index;
				mostContacts = personToAutolink.getContactIds().getArray().length;
			}

			if (person) {
				if (person.getId() === personToAutolink.getId()) {
					foundPersonInShouldAutoLink = true;
				}
			}

			mapReduceToGetContacts.push({ "function": personToAutolink.reloadContacts, object: personToAutolink });
		}
		
		personWithMostContacts = shouldAutolink[indexOfMost];
		
		// Make sure if the person is not already in the shouldAutolink array,
		// we should either add them or set them as the personWithMostContacts.
		if (person && !foundPersonInShouldAutoLink) {
			mapReduceToGetContacts.push({ "function": person.reloadContacts, object: person });
			
			if (person.getContactIds().getArray().length >= shouldAutolink[indexOfMost].getContactIds().getArray().length) {
				personWithMostContacts = person;
			} else {
				personWithMostContacts = shouldAutolink[indexOfMost];
				shouldAutolink.splice(indexOfMost, 1);
				shouldAutolink.push(person);
			}
		} else {
			shouldAutolink.splice(indexOfMost, 1);
		}

		return ContactsUtils.mapReduceAndVerifyResultsTrue(mapReduceToGetContacts);
	});
	
	future.then(function () {
		var result = future.result,
			contactIdSortedOrder,
			peopleIdsToDelete = [],
			index,
			len,
			tempPerson;
		
		// If the contactToAutoLink already was associated with a person don't call orderContactIds with contactToAutoLink.
		// Otherwise we need to include it in the call.
		if (person) {
			contactIdSortedOrder = Person.orderContactIds(personWithMostContacts, shouldAutolink);
		} else {
			contactIdSortedOrder = Person.orderContactIds(personWithMostContacts, shouldAutolink, contactToAutolink);
		}
		
		personWithMostContactsOriginal = new Person(personWithMostContacts.getDBObject());
		personWithMostContacts.getContactIds().clear();
		personWithMostContacts.getContactIds().add(contactIdSortedOrder);
		
		if (shouldAutolink.length) {
			len = shouldAutolink.length;
			for (index = 0; index < len; index++) {
				tempPerson = shouldAutolink[index]; 
				peopleIdsToDelete.push(tempPerson.getId());
			}
		
			return DB.del(peopleIdsToDelete);
		} else {
			return true;
		}
	});
	
	future.then(function () {
		var dummy = future.result;
		
		return Autolinker.fixupAndSaveMergedPeople(personWithMostContacts, personWithMostContactsOriginal, shouldAutolink, controller, listSortOrder);
	});
	
	return future;
};

Autolinker.fixupAndSaveMergedPeople = function (newPerson, originalPerson, shouldAutolink, controller, listSortOrder) {
	var future = new Future();
	
	future.now(function () {
		//timingRecorder.startTimingForJob("Merge_People_Fixup");
		return newPerson.fixup(shouldAutolink, {
			listSortOrder: listSortOrder,
			timingRecorder: timingRecorder
		});
	});
	
	future.then(function () {
		var dummy = future.result;
		
		//timingRecorder.stopTimingForJob("Merge_People_Fixup");
		//timingRecorder.startTimingForJob("Merge_People_Save");
		return newPerson.save();
	});
	
	future.then(function () {
		var dummy = future.result;
		
		//timingRecorder.stopTimingForJob("Merge_People_Save");
		//timingRecorder.startTimingForJob("Dispatching_Linker_Plugins_Person_Changed_And_Removed");
		
		// either new or changed contact information resulted in merged (but no new person created)
		return controller.service.assistant.dispatchToPlugins("personChanged", originalPerson, newPerson);
	});
	
	future.then(function () {
		var dummy = future.result;
		
		return Foundations.Control.mapReduce({
			map: function (person) {
				return controller.service.assistant.dispatchToPlugins("personRemoved", person);
			}
		}, shouldAutolink);
	});
	
	future.then(function () {
		var dummy = future.result;
		
		//timingRecorder.stopTimingForJob("Dispatching_Linker_Plugins_Person_Changed_And_Removed");
		return true;
	});
	
	return future;
};

Autolinker.doRankingFunctions = function (person, contactToAutoLink) {
	var future = new Future(),
		weightedResults = {};
	
	future.now(function () {
		Console.log("Getting similar names");
		return Autolinker.similarName(person, contactToAutoLink, weightedResults);
	});
	
	future.then(function () {
		var result = future.result;
		
		Console.log("Getting similar phone numbers");
		return Autolinker.similarPhoneNumber(person, contactToAutoLink, weightedResults);
	});
	
	future.then(function () {
		var result = future.result;
		
		Console.log("Getting similar emails");
		return Autolinker.similarEmail(person, contactToAutoLink, weightedResults);
	});
	
	future.then(function () {
		var result = future.result;
		
		Console.log("Getting similar ims");
		return Autolinker.similarIM(person, contactToAutoLink, weightedResults);
	});
	
	future.then(function () {
		var result = future.result;
		
		Console.log("Getting manual links");
		return Autolinker.manualLinks(person, contactToAutoLink, weightedResults);
	});
	
	future.then(function () {
		var result = future.result;
		
		Console.log("Getting manual unlinks");
		return Autolinker.manualUnlinks(person, contactToAutoLink, weightedResults);
	});
	
	future.then(function () {
		var result = future.result;
		
		Console.log("Returning weights");
		return weightedResults;
	});
	
	return future;
};

// if person is null then the contactToAutoLink does not have a person object yet
// TODO: make sure clb links are being restored
Autolinker.prototype.getLinksFromCLB = function (contact) {
	var clb = new ContactLinkBackup(),
		future = new Future(),
		toReturn = {};
	
	//Console.log(JSON.stringify(contact));
	
	future.now(this, function () {
		clb.getManualLinks(contact);
	});
	
	future.then(this, function () {
		//Console.log(JSON.stringify(theFuture.result));
		toReturn.manualLinks = future.result.results;
		return clb.getUnlinkLinks(contact);
	});
	
	future.then(this, function () {
		//Console.log(JSON.stringify(theFuture.result));
		toReturn.manualUnlinks = future.result.results;
		return toReturn;
	});
	
	return future;
};

Autolinker.getContactsPerson = function (contact) {
	var future = new Future(),
		person,
		contacts = [];
	
	future.now(function () {
		return DB.find({
			"from": Person.kind,
			"where": [{
				"prop": "contactIds",
				"op": "=",
				"val": contact.getId()
			}],
			"limit": 1
		});
	});
	
	// Get the person from the db query
	future.then(function () {
		var result = Utils.DBResultHelper(future.result);
		if (result && result.length > 0) {
			person = PersonFactory.createPersonLinkable(result[0]);
		}
		
		// If there was a person then go ahead and get the contacts associated with it.
		if (person) {
			return person.reloadContacts(ContactFactory.ContactType.LINKABLE);
		} else {
			return true;
		}
		
	});
	
	// Get the contacts from the db query
	future.then(function () {
		var result = future.result;
		
		if (result && result.length > 0) {
			return {
				"person": person,
				"contacts": future.result
			};
		} else {
			return {
				"person": person,
				"contacts": contacts
			};
		}
		
	});
	
	return future;
};

Autolinker.prototype.getAllContacts = function () {
	return this.getContactsAboveRevToAutolink();
};

Autolinker.prototype.getContactsAboveRevToAutolink = function (highestRev) {
	var future = new Future(),
		unDeletedContacts;
	
	future.now(this, function () {
		var query = {
			"from": Contact.kind,
			"incDel": true
		};
		
		query.where = highestRev ? [ { "prop": "_rev", "op": ">", "val": highestRev } ] : undefined;
		
		return DB.find(query);
	});
	
	future.then(this, function () {
		var result = future.result.results;
		
		return result;
	});
	
	return future;
};

Autolinker.prototype.getDeletedContactsAboveRevToRemove = function (highestRev) {
	var future = new Future();
	
	future.now(this, function () {
		var whereClause = [{
			"prop": "_del",
			"op": "=",
			"val": true
		}];
		
		if (highestRev) {
			whereClause.push({
				"prop": "_rev",
				"op": ">",
				"val": highestRev
			});
		}
		
		return DB.find({
			"from": Contact.kind,
			"where": whereClause
		});
	});
	
	return future;
};

Autolinker.restartActivityUsingRevisionNumber = function (activityId, revisionNumber) {
	var future = new Future();
	
	future.now(function () {
		var params = {
			activityId: activityId,
			restart: true,
			trigger: {
				"method": "palm://com.palm.db/watch",
				"params": {
					"query": {
						"from": Contact.kind,
						"where": [{
							"prop": "_rev",
							"op": ">",
							"val": revisionNumber
						}],
						"incDel": true,
						"limit": 1
					},
					"subscribe": true
				},
				"key": "fired"
			},
			callback: {
				"method": "palm://com.palm.service.contacts.linker/dbUpdatedRelinkChanges",
				"params": {
					"revChangedStart": revisionNumber
				}
			}
		};
		
		return PalmCall.call("palm://com.palm.activitymanager/", "complete", params);
	});
	
	return future;
};

Autolinker.createActivityUsingRevisionNumber = function (revisionNumber) {
	var future = new Future(),
		linkerWatch = {
			"activity": {
				"type": { 
					"persist": true,
					"userInitiated": true,
					"background": true
				},
				"name": "linkerWatch",
				"description": "This is the watch for kicking off the linker on changes",
				"trigger": {
					"method": "palm://com.palm.db/watch",
					"params": {
						"query": {
							"from": Contact.kind,
							"where": [{
								"prop": "_rev",
								"op": ">",
								"val": revisionNumber
							}],
							"incDel": true,
							"limit": 1
						},
						"subscribe": true
					},
					"key": "fired"
				},
				"callback": {
					"method": "palm://com.palm.service.contacts.linker/dbUpdatedRelinkChanges",
					"params": {
						"revChangedStart": revisionNumber
					}
				}
			},
			"start": true,
			"replace": true
		};
	
	future.now(function () {
		return PalmCall.call("palm://com.palm.activitymanager/", "create", linkerWatch);
	});
	
	future.then(function () {
		var result = future.result;
		
		Console.log("Activity Id - " + JSON.stringify(result.activityId));
		return true;
	});
	
	return future;
};

Autolinker.getCurrentWatchRev = function () {
	var future = new Future();
	
	future.now(function () {
		return DB.find({
			"from": WatchRevisionNumber.kind,
			"where": [{
				"prop": "_id",
				"op": "!=",
				"val": -1
			}],
			"limit": 1
		});
	});
	
	future.then(function () {
		var theRev,
			result = Utils.DBResultHelper(future.result);
			
		Console.log("The revision result from db - " + JSON.stringify(result));
		if (result && result.length > 0) {
			Console.log("Got a existing revision number from db");
			theRev = new WatchRevisionNumber(result[0]);
		} else {
			theRev = new WatchRevisionNumber();
		}
		
		return theRev;
	});
	
	return future;
};

/////////////////////////// Take this out before production //////////////////////////

Autolinker.prototype.setupWatch = function () {
	var future = new Future();
	
	future.now(this, function () {
		return Autolinker.getCurrentWatchRev();
	});
	
	future.then(this, function () {
		var result = future.result,
			revisionNumber = result.revisionNumber;
		
		return Autolinker.createActivityUsingRevisionNumber(revisionNumber);
	});
	
	return future;
};
