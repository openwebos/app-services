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
/*global Utils, console, Console, DB, _, Future, Contact, Foundations, Person, ContactFactory, PersonFactory, Assert, ContactLinkable */

// Contact Link Backup

function ManualLink() {
	this._kind = ManualLink.kind;
	this.contactEntityA = "";
	this.contactEntityB = "";
}

ManualLink.kind = "com.palm.manualLink:1";
ManualLink.register = {
	owner: "com.palm.contacts",
	id: ManualLink.kind,
	indexes: [{
		props: [{
			"name": "contactEntityA"
		}]
	}, {
		props: [{
			"name": "contactEntityB"
		}]
	}, {
		props: [{
			"name": "contactEntityA"
		}, {
			"name": "contactEntityB"
		}]
	}]
};

function ManualUnlink() {
	this._kind = ManualUnlink.kind;
	this.contactEntityA = "";
	this.contactEntityB = "";
}
ManualUnlink.kind = "com.palm.manualUnlink:1";
ManualUnlink.register = {
	owner: "com.palm.contacts",
	id: ManualUnlink.kind,
	indexes: [{
		props: [{
			"name": "contactEntityA"
		}]
	}, {
		props: [{
			"name": "contactEntityB"
		}]
	}, {
		props: [{
			"name": "contactEntityA"
		}, {
			"name": "contactEntityB"
		}]
	}]
};

function ContactLinkBackup() {

}

ContactLinkBackup.prototype.displayDBs = function (future) {

	var dbFuture = new Future();
	
	if (!Console.willDisplay()) {	
		future.result = true;
	} else {
		dbFuture.nest(DB.find({ "from": ManualLink.kind }));
	}
	
	dbFuture.then(this, function (dbDoneFuture) {
		var i,
			result = Utils.DBResultHelper(dbDoneFuture.result);
			
		console.log("\n\n---------------------------------------------------------------------------------------------------------\nManual Link Table\n---------------------------------------------------------------------------------------------------------");
		for (i = 0; i < result.length; i += 1) {
			console.log(JSON.stringify(result[i]) + "");
			
		}
		console.log("---------------------------------------------------------------------------------------------------------\n");
		dbDoneFuture.nest(DB.find({
			"from": ManualUnlink.kind,
			"where": [{
				"prop": "_id",
				"op": "!=",
				"val": -20
			}]
		}));
	}).then(this, function (dbDoneFuture) {
		console.log("\n\n---------------------------------------------------------------------------------------------------------\nManual Unlink Table\n---------------------------------------------------------------------------------------------------------");
		var i,
			result = Utils.DBResultHelper(dbDoneFuture.result);
			
		for (i = 0; i < result.length; i += 1) {
			console.log(JSON.stringify(result[i]) + "");
			
		}
		console.log("---------------------------------------------------------------------------------------------------------\n\n");
		future.result = true;
	});
};

ContactLinkBackup.prototype.performedManualLinkDBus = function (params) {
	var future = new Future();
	
	future.now(this, function () {
		var errorMessage = "";

		if (!params.person1Contacts) {
			errorMessage = "The 'person1Contacts' parameter was not specified. ";
		}

		if (!params.person2Contacts) {
			errorMessage = "The 'person2Contacts' parameter was not specified.";
		}

		if (errorMessage.length > 0) {
			throw new Error(errorMessage);
		}
		
		return this.performedManualLink(params.person1Contacts, params.person2Contacts);
	});
	
	return future;
};

ContactLinkBackup.prototype.performedManualLink = function (person1ContactIds, person2ContactIds) {
	var future = new Future(),
		allLinks = [],
		allContactHashes = [],
		additionalUnlinksToAdd = [],
		unlinksFromDBMayAddA = [],
		unlinksFromDBMayAddB = [];
	
	// Get all the contactIds from the people being merged and get the
	// contact records from the db.
	future.now(this, function () {
		var arrayToCopyTo,
			arrayToCopyOther;
		
		if (person1ContactIds && person2ContactIds) {
			arrayToCopyTo = person1ContactIds.length > person2ContactIds.length ? person1ContactIds.slice(0, person1ContactIds.length) : person2ContactIds.slice(0, person2ContactIds.length);
			arrayToCopyOther = person1ContactIds.length > person2ContactIds.length ? person2ContactIds.slice(0, person2ContactIds.length) : person1ContactIds.slice(0, person1ContactIds.length);
			
			arrayToCopyOther.forEach(function (contact) {
				arrayToCopyTo.push(contact);
			});
			
			return DB.get(arrayToCopyTo);
		} else {
			throw new Error("The person1ContactIds or person2ContactIds is undefined.");
		}
	});
	
	future.then(this, function () {
		var results = Utils.DBResultHelper(future.result),
			filledPerson2ContactsInArray = false,
			person1ContactObjects = [],
			person2ContactObjects = [],
			personContactObjectLinkHashes = {},
			manualLinkObjects = [];
		
		if (results && results.length > 0) {
			results.forEach(function (result) {
				var tempContact,
					foundIn1 = false;
				
				tempContact = ContactFactory.createContactLinkable(result);
				
				foundIn1 = person1ContactIds.some(function (contactId) {
					return tempContact.getId() === contactId;
				});
				
				if (foundIn1) {
					person1ContactObjects.push(tempContact);
				} else {
					person2ContactObjects.push(tempContact);
				}
			});
			
			return Foundations.Control.mapReduce({
				map: function (person1Contact) {
					var mapFuture = new Future(),
						person1LinkHash;
					
					mapFuture.now(function () {
						if (!personContactObjectLinkHashes[person1Contact.getId()]) {
							return ContactLinkable.getLinkHash(person1Contact);
						} else {
							return personContactObjectLinkHashes[person1Contact.getId()];
						}
					});
					
					mapFuture.then(function () {
						person1LinkHash = mapFuture.result;
						
						personContactObjectLinkHashes[person1Contact.getId()] = person1LinkHash;
						
						allContactHashes.push(person1LinkHash.linkHash);
						
						return Foundations.Control.mapReduce({
							map: function (person2Contact) {
								var innerMapFuture = new Future(),
									person2LinkHash;

								innerMapFuture.now(function () {
									if (!personContactObjectLinkHashes[person2Contact.getId()]) {
										return ContactLinkable.getLinkHash(person2Contact);
									} else {
										return personContactObjectLinkHashes[person2Contact.getId()];
									}
								});

								innerMapFuture.then(function () {
									var tempManualLink = new ManualLink(),
										tempManualUnlink = new ManualUnlink(),
										tempManualUnlinkInvert = new ManualUnlink();
									
									person2LinkHash = innerMapFuture.result;
									
									personContactObjectLinkHashes[person2Contact.getId()] = person2LinkHash;
									
									tempManualLink.contactEntityA = person1LinkHash.linkHash;
									tempManualLink.contactEntityB = person2LinkHash.linkHash;
									manualLinkObjects.push(tempManualLink);

									tempManualUnlink.contactEntityA = person1LinkHash.linkHash;
									tempManualUnlink.contactEntityB = person2LinkHash.linkHash;

									tempManualUnlinkInvert.contactEntityA = person2LinkHash.linkHash;
									tempManualUnlinkInvert.contactEntityB = person1LinkHash.linkHash;

									allLinks.push(tempManualUnlink);
									allLinks.push(tempManualUnlinkInvert);

									if (!filledPerson2ContactsInArray) {
										allContactHashes.push(person2LinkHash.linkHash);
									}
									
									return true;
								});

								return innerMapFuture;
							}
						}, person2ContactObjects).then(function (innerMapReduceFuture) {
							var result = innerMapReduceFuture.result;
							filledPerson2ContactsInArray = true;

							return true;
						});
					});
					
					return mapFuture;
				}
			}, person1ContactObjects).then(function (mapReduceFuture) {
				var result = mapReduceFuture.result;
				
				// Insert the manual links into the manual link table
				return DB.put(manualLinkObjects);
			});
		} else {
			throw new Error("Did not get the contacts to create the backups"); 
		}
	});
	
	future.then(this, function () {
		var result = Utils.DBResultHelper(future.result);
		
		if (result) {
			// Remove the manual unlinks for the contacts being merged together
			return Foundations.Control.mapReduce({
				map: function (data) {
					return DB.find({
						"from": data._kind,
						"where": [{
							"prop": "contactEntityA",
							"op": "=",
							"val": data.contactEntityA
						}, {
							"prop": "contactEntityB",
							"op": "=",
							"val": data.contactEntityB
						}]
					});
				}
			}, allLinks);
		} else {
			throw new Error("Adding manual links to clb failed");
		}
	});
	
	future.then(this, function () {
		var results = future.result,
			manualUnlinks = [];
			
		results.forEach(function (result) {
			result = Utils.DBResultHelper(result.result);
			manualUnlinks = manualUnlinks.concat(result);
		});
		
		if (manualUnlinks.length > 0) {
			manualUnlinks = manualUnlinks.map(function (manualUnlink) {
				return manualUnlink._id;
			});
			
			return DB.del(manualUnlinks);
		} else {
			return "No ManualUnlinks to remove";
		}
	});
	
	future.then(this, function () {
		var dummy = future.result;
		
		return Foundations.Control.mapReduce({
			map: function (data) {
				return DB.find({
					"from": ManualUnlink.kind,
					"where": [{
						"prop": "contactEntityA",
						"op": "=",
						"val": data
					}]
				});
			}
		}, allContactHashes);
	});
	
	future.then(this, function () {
		var results = future.result;
			
		results.forEach(function (result) {
			result = Utils.DBResultHelper(result.result);
			unlinksFromDBMayAddA = unlinksFromDBMayAddA.concat(result);
		});
			
		return Foundations.Control.mapReduce({
			map: function (data) {
				return DB.find({
					"from": ManualUnlink.kind,
					"where": [{
						"prop": "contactEntityB",
						"op": "=",
						"val": data
					}]
				});
			}
		}, allContactHashes);
	});
	
	future.then(this, function () {
		var results = future.result,
			tempManualUnlink,
			that = this;
		
		results.forEach(function (result) {
			result = Utils.DBResultHelper(result.result);
			unlinksFromDBMayAddB = unlinksFromDBMayAddB.concat(result);
		});
		
		// Do the check for A
		unlinksFromDBMayAddA.forEach(function (unlink) {
			var linkHashInDB = _.extend(new ManualUnlink(), unlink);
			
			allContactHashes.forEach(function (contactHash) {
				if (!that.isContactHashInArray(contactHash, linkHashInDB.contactEntityB, unlinksFromDBMayAddA)) {
					if (!that.isContactHashInArray(contactHash, linkHashInDB.contactEntityB, additionalUnlinksToAdd)) {
						if (contactHash && linkHashInDB.contactEntityB) {
							tempManualUnlink = new ManualUnlink();
							tempManualUnlink.contactEntityA = contactHash;
							tempManualUnlink.contactEntityB = linkHashInDB.contactEntityB;
							additionalUnlinksToAdd.push(tempManualUnlink);
						}
					}
				}
			});
		});
		
		// Do the check for B
		unlinksFromDBMayAddB.forEach(function (unlink) {
			var linkHashInDB = _.extend(new ManualUnlink(), unlink);
			
			allContactHashes.forEach(function (contactHash) {
				if (!that.isContactHashInArray(linkHashInDB.contactEntityA, contactHash, unlinksFromDBMayAddB)) {
					if (!that.isContactHashInArray(linkHashInDB.contactEntityA, contactHash, additionalUnlinksToAdd)) {
						if (linkHashInDB.contactEntityA && contactHash) {
							tempManualUnlink = new ManualUnlink();
							tempManualUnlink.contactEntityA = linkHashInDB.contactEntityA;
							tempManualUnlink.contactEntityB = contactHash;
							additionalUnlinksToAdd.push(tempManualUnlink);
						}
					}
				}
			});
		});
		
		return DB.put(additionalUnlinksToAdd);
	});
	
	future.then(this, function () {
		return true;
	});
	
	return future;
};

ContactLinkBackup.CHECK_IN_ENTITY_A = 1;
ContactLinkBackup.CHECK_IN_ENTITY_B = -1;
ContactLinkBackup.CHECK_IN_BOTH_ENTITIES = 0;

ContactLinkBackup.prototype.isContactHashInArray = function (contactHashAToCheck, contactHashBToCheck, arrayToCheckIn) {
	var i,
		tempObjectHash;
	
	for (i = 0; i < arrayToCheckIn.length; i += 1) {
		tempObjectHash = _.extend(new ManualUnlink(), arrayToCheckIn[i]);
		//console.log(tempObjectHash.contactEntityA + " === " + contactHashAToCheck + "  " + tempObjectHash.contactEntityB + " === " + contactHashBToCheck);
		if (tempObjectHash.contactEntityA === contactHashAToCheck && tempObjectHash.contactEntityB === contactHashBToCheck) {
			return true;
		}
	}
	
	return false;
};

ContactLinkBackup.prototype.performedUnlinkDBus = function (params) {
	var future = new Future();
	
	future.now(this, function () {
		var errorMessage = "";

		if (!params.person) {
			errorMessage = "The 'person' parameter was not specified. ";
		}

		if (!params.contactId) {
			errorMessage = "The 'contactId' parameter was not specified.";
		}

		if (errorMessage.length > 0) {
			throw new Error(errorMessage);
		}
		
		return this.performedUnlink(params.person, params.contactId);
	});
	
	return future;
};

ContactLinkBackup.prototype.performedUnlink = function (person, contactId) {
	var future = new Future(),
		contactUnlinked, personUnlinkedFrom,
		hadToFetchContactId = false,
		manualUnlinksToAdd = [],
		allManualLinksToRemove = [];
	
	future.now(this, function () {
		if (person && contactId) {
			console.log("Performing manual unlink from clb");
			
			personUnlinkedFrom = person;
				
			var contactsIdsCopy = personUnlinkedFrom.copyOfContactIds();
			hadToFetchContactId = true;
			contactsIdsCopy.push(contactId);
			return DB.get(contactsIdsCopy);
		} else {
			throw new Error("The personOrPersonId or contactOrContactId is undefined.");
		}
	});
	
	future.then(this, function () {
		var tempManualUnlink,
			tempContact,
			tempManualLink,
			notFound = true,
			i,
			result = Utils.DBResultHelper(future.result);
		
		if (result) {
			//console.log(JSON.stringify(future.result) + " Getting persons contacts result");
			if (hadToFetchContactId) {
				for (i = result.length - 1; i >= 0; i -= 1) {
					tempContact = ContactFactory.createContactLinkable(result[i]);
					//console.log("Temp contact " + tempContact);
					if (tempContact.getId() === contactId) {
						notFound = false;
						//console.log("Found contact unlinked in results: " + tempContact);
						result.splice(i, 1);
						//console.log("Removed contact unlinked from result array: " + JSON.stringify(result));
						contactUnlinked = tempContact;
						break;
					}
				}
				
				if (notFound) {
					throw new Error("The contact that was unlinked could not be found for clb");
				}
			}
			
			return ContactLinkable.getLinkHash(contactUnlinked).then(function (getLinkHashFuture) {
				var contactUnlinkedLinkhash = getLinkHashFuture.result;
				
				return Foundations.Control.mapReduce({
					map: function (contact) {
						var mapFuture = new Future(),
							tempContact = ContactFactory.createContactLinkable(contact);

						mapFuture.now(function () {
							return ContactLinkable.getLinkHash(tempContact);
						});

						mapFuture.then(function () {
							var tempContactLinkHash = mapFuture.result;
							
							tempManualUnlink = new ManualUnlink();
							tempManualUnlink.contactEntityA = tempContactLinkHash.linkHash;
							tempManualUnlink.contactEntityB = contactUnlinkedLinkhash.linkHash;
							manualUnlinksToAdd[manualUnlinksToAdd.length] = tempManualUnlink;

							tempManualLink = new ManualLink();
							tempManualLink.contactEntityA = tempContactLinkHash.linkHash;
							tempManualLink.contactEntityB = contactUnlinkedLinkhash.linkHash;
							allManualLinksToRemove[allManualLinksToRemove.length] = tempManualLink;

							tempManualLink = new ManualLink();
							tempManualLink.contactEntityA = contactUnlinkedLinkhash.linkHash;
							tempManualLink.contactEntityB = tempContactLinkHash.linkHash;
							allManualLinksToRemove[allManualLinksToRemove.length] = tempManualLink;
							
							return true;
						});

						return mapFuture;
					}
				}, result);
			});
		} else {
			console.log("Getting person's contacts failed in clb");
			throw new Error("Getting person's contacts failed in clb");
		}
	});
	
	future.then(this, function () {
		var result = future.result;
		//console.log("Manual Unlinks to add: " + JSON.stringify(manualUnlinksToAdd));
		//console.log("All Links to remove: " + JSON.stringify(allManualLinksToRemove));

		return DB.put(manualUnlinksToAdd);
	});
	
	future.then(this, function () {
		var result = Utils.DBResultHelper(future.result);

		if (result) {
			//console.log("Added " + result.length + " links to the unlink table");
			return Foundations.Control.mapReduce({
				map: function (data) {
					return DB.find({
						"from": data._kind,
						"where": [{
							"prop": "contactEntityA",
							"op": "=",
							"val": data.contactEntityA
						}, {
							"prop": "contactEntityB",
							"op": "=",
							"val": data.contactEntityB
						}]
					});
				}
			}, allManualLinksToRemove);
		} else {
			throw new Error("Adding links to the unlink table failed");
		}
	});
	
	future.then(this, function () {
		var results = future.result,
			toEraseIds = [],
			i,
			manualLinks = [];
			
		results.forEach(function (result) {
			result = Utils.DBResultHelper(result.result);
			manualLinks = manualLinks.concat(result);
		});
		  
		if (manualLinks.length > 0) {
			//console.log(JSON.stringify(removeManualLinksFutureDone.result));

			for (i = 0; i < manualLinks.length; i += 1) {
				toEraseIds[i] = manualLinks[i]._id;
			}
			
			return DB.del(toEraseIds);
		} else {
			return true;
		}
	});
	
	future.then(this, function () {
		return {
			msg: "Done in clb"
		};
	});

	return future;
};

ContactLinkBackup.prototype.getManualLinksDBus = function (params) {
	var future = new Future();
	
	future.now(this, function () {
		Assert.require(params.contactOrContactId, "The 'contactOrContactId' parameter was not specified");
		
		return this.getManualLinks(params.contactOrContactId);
	});
	
	return future;
};

ContactLinkBackup.prototype.getManualLinks = function (contactOrContactId) {
	var toReturn = [],
		contactHash,
		future = new Future();
		
	future.now(this, function () {
		
		Assert.require(contactOrContactId, "ContactLinkBackup.getUnlinkLinks: you must pass in a defined contactOrContactId");
		
		if (typeof contactOrContactId !== "object") {
			return this.getContact(contactOrContactId);
		} else {
			return contactOrContactId;
		}
	});
		//this.displayDBs(aFuture);
	future.then(this, function () {
		var contact = future.result;
		
		Assert.require(contact, "ContactLinkBackup.getManualLinks: failed to get the contact to get links from");
		
		return contact.getLinkHash();
	});
	
	future.then(this, function () {
		var result = future.result;
		
		contactHash = result.linkHash;
		//console.log("Getting entries from manual link for contactEntityA = " + contactHash);
			
		return DB.find({
			"from": ManualLink.kind,
			"where": [{
				"prop": "contactEntityA",
				"op": "=",
				"val": contactHash
			}]
		});
	});
	
	future.then(this, function () {
		var results = Utils.DBResultHelper(future.result);
		
		Assert.require(results, "ContactLinkBackup.getManualLinks: Trying to fetch the contactEntityA manualLink failed");
		
		results.forEach(function (manualLinkRecord) {
			toReturn.push(_.extend(new ManualLink(), manualLinkRecord));
		});
		
		return DB.find({
			"from": ManualLink.kind,
			"where": [{
				"prop": "contactEntityB",
				"op": "=",
				"val": contactHash
			}]
		});
	});
	
	future.then(this, function () {
		var results = Utils.DBResultHelper(future.result);
		
		Assert.require(results, "ContactLinkBackup.getManualLinks: Trying to fetch the contactEntityB manualLink failed");
		
		results.forEach(function (manualLinkRecord) {
			toReturn.push(_.extend(new ManualLink(), manualLinkRecord));
		});
		
		return {
			"results": toReturn
		};
	});
	
	return future;
};

ContactLinkBackup.prototype.getContact = function (contactId) {
	var future = new Future();
	
	future.now(this, function () {
		return DB.get([contactId]);
	});
	
	future.then(this, function () {
		var result = Utils.DBResultHelper(future.result);
	
		Assert.require(result, "ContactLinkBackup.getUnlinkLinks: Trying to fetch the contact for clb failed");
		
		return (result[0] ? ContactFactory.createContactLinkable(result[0]) : null);
	});
};

ContactLinkBackup.prototype.getUnlinkLinksDBus = function (params) {
	var future = new Future();
	
	future.now(this, function () {
		Assert.require(params.contactOrContactId, "The 'contactOrContactId' parameter was not specified");
		
		return this.getUnlinkLinks(params.contactOrContactId);
	});
	
	return future;
};

ContactLinkBackup.prototype.getUnlinkLinks = function (contactOrContactId) {
	var toReturn = [],
		future = new Future(),
		contactHash;
		
	future.now(this, function () {
		Assert.require(contactOrContactId, "ContactLinkBackup.getUnlinkLinks: you must pass in a defined contactOrContactId");
		
		if (typeof contactOrContactId !== "object") {
			return this.getContact(contactOrContactId);
		} else {
			return contactOrContactId;
		}
	});
	
	future.then(this, function () {
		var contact = future.result;
		
		Assert.require(contact, "ContactLinkBackup.getUnlinkLinks: failed to get the contact to get unlinks from");
		
		return contact.getLinkHash();
	});

	future.then(this, function () {
		var result = future.result;

		contactHash = result.linkHash;
		
		return DB.find({
			"from": ManualUnlink.kind,
			"where": [{
				"prop": "contactEntityA",
				"op": "=",
				"val": contactHash
			}]
		});
	});
	
	future.then(this, function () {
		var results = Utils.DBResultHelper(future.result);
		
		Assert.require(results, "ContactLinkBackup.getUnlinkLinks: Trying to fetch the contactEntityA manualUnlink failed");
		
		results.forEach(function (manualUnlinkRecord) {
			toReturn.push(_.extend(new ManualUnlink(), manualUnlinkRecord));
		});
		
		return DB.find({
			"from": ManualUnlink.kind,
			"where": [{
				"prop": "contactEntityB",
				"op": "=",
				"val": contactHash
			}]
		});
	});
	
	future.then(this, function () {
		var results = Utils.DBResultHelper(future.result);
			
		Assert.require(results, "ContactLinkBackup.getUnlinkLinks: Trying to fetch the contactEntityB manualUnlink failed");
		
		results.forEach(function (manualUnlinkRecord) {
			toReturn.push(_.extend(new ManualUnlink(), manualUnlinkRecord));
		});
		
		return {
			"results": toReturn
		};
	});
	
	return future;
};

ContactLinkBackup.prototype.resetDB = function (future) {
	var manualLinksCleared, manualUnlinksCleared = 0;
	/*var manualLink1 = new ManualLink();
	 manualLink1.contactEntityA = "1111";
	 manualLink1.contactEntityB = "2222";
	 var manualLink2 = new ManualLink();
	 manualLink2.contactEntityA = "3333";
	 manualLink2.contactEntityB = "1111";
	 var manualUnlink1 = new ManualUnlink();
	 manualUnlink1.contactEntityA = "2222";
	 manualUnlink1.contactEntityB = "3333";
	 var manualUnlink2 = new ManualUnlink();
	 manualUnlink2.contactEntityA = "3333";
	 manualUnlink2.contactEntityB = "4444";
	 future.nest(DB.put([manualLink1, manualLink2, manualUnlink1, manualUnlink2]));
	 */
	future.now(this, function (future) {
		future.nest(DB.del({
			"from": ManualLink.kind
		}));
	});
	
	future.then(this, function (future) {
		var result = future.result;
		
		if (result && typeof result.count === "number") {
			manualLinksCleared = result.count;
			//console.log("DB successfully cleared of " + result.count + " manualLinks");
			future.nest(DB.del({
				"from": ManualUnlink.kind
			}));
		} else {
			//console.log("DB not cleared of manualLinks");
			future.exception = {
				error: "DB not cleared of manualLinks"
			};
		}
	});
	
	future.then(this, function (future) {
		var result = future.result;
		
		if (result && typeof result.count === "number") {
			manualUnlinksCleared = result.count;
			//console.log("DB successfully cleared of " + result.count + " manualUnlinks");
			future.result = {
				msg: "DB cleared of " + manualLinksCleared + " manualLinks and " + manualUnlinksCleared + " manualUnlinks"
			};
		} else {
			//console.log("DB not cleared of manualUnlinks");
			future.exception = {
				error: "DB not cleared of manualUnlinks"
			};
		}
	});
};
