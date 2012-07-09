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

/*jslint white: true, onevar: true, undef: true, eqeqeq: true, plusplus: true, bitwise: true 
 regexp: true, newcap: true, immed: true, nomen: false, maxerr: 500 */
/*global Utils, DB, console, Class, VCardExporter, Future, Contacts, PalmCall, _, Foundations, Contact, Person, SortKey, Assert, BatchDBWriter, IO, Globalization, PhoneNumber, MojoLoader, Globalization: true*/

var ContactsServiceAssistant = Class.create({
	initialize: function () {
	},
	
	setup: function () {
		try {
			Globalization = MojoLoader.require({
				"name": "globalization",
				"version": "1.0"
			}).globalization.Globalization;
		} catch (e) {
			console.log("Error: failed to load Globalization: " + e.message);
		}
	},
	
	photoPreBackup: function (params) {
		var future = new Future();
		
		// Get the current accounts
		future.now(this, function () {
			return PalmCall.call("palm://com.palm.service.accounts/", "listAccounts", {"templateId": "com.palm.palmprofile"});
		});
		
		// Loop through all the accounts trying to find one with the templateId for palmprofile and
		// return that accountId
		future.then(this, function () {
			var result = future.result,
				accountId,
				account;
			
			account = result.results[0];
			
			Assert.requireDefined(account, "ContactsService.photoPreBackup - There were no palm profile accounts in the account list");
			
			// Now that we have the accountId of the palm profile account, get the contacts
			// that have an accountId that matches the id of the palm profile account
			return Contact.getContactsByAccountId(account._id);
		});
		
		// Pull the photos out of the contacts that match the account id
		// of the palm profile account
		future.then(this, function () {
			var contacts = future.result,
				filePaths = [];
			
			contacts.forEach(function (contact) {
				var photos = contact.getPhotos().getArray();
				
				photos.forEach(function (photo) {
					//we deliberately back up the original photo, not the filecache version (i.e. value, not localPath) 
					//because the photo can be recreated from the original when needed after the restore
					filePaths.push(photo.getValue());
				});
			});
			
			return {
				version: ContactsServiceAssistant.VERSION,
				description: "Backup of " + filePaths.length + " palm profile contact photos.",
				files: filePaths,
				ignoreMissingFiles: true
			};
		});
		
		return future;
	},
	
	vCardCountContacts: function (params) {
		var future = new Future();
		
		future.now(this, function () {
			var filePath = params.filePath,
				contactCount;
			
			Assert.requireString(filePath, "vCardCountContacts did not specify a filePath for the vcard to count");
			
			contactCount = Contacts.vCardImporter.countContacts(ContactsServiceAssistant.getPathFromFileTypeSchemaPath(filePath));
			
			return {
				count: contactCount
			};
		});
		
		return future;
	},
	
	readVCard: function (params) {
		var future = new Future();
		
		future.now(this, function () {
			var filePath = params.filePath,
				vCardImporter;
			
			Assert.requireString(filePath, "readVCard did not specify a filePath for the vcard to read");
			
			filePath = ContactsServiceAssistant.getPathFromFileTypeSchemaPath(filePath);
			
			vCardImporter = new Contacts.vCardImporter({
				filePath: filePath
			});
			
			return vCardImporter.readVCard();
		});
		
		future.then(this, function () {
			var result = future.result;
			
			return {
				contacts: ContactsServiceAssistant.convertArrayOfContactsForBus(result)
			};
		});
		
		return future;
	},
	
	importVCard: function (params) {
		var future = new Future();
		
		future.now(this, function () {
			var filePath = params.filePath,
				vCardImporter;
			
			Assert.requireString(filePath, "importVCard did not specify a filePath for the vcard to import");
			
			
			filePath = ContactsServiceAssistant.getPathFromFileTypeSchemaPath(filePath);
			
			vCardImporter = new Contacts.vCardImporter({
				filePath: filePath
			});
			
			return vCardImporter.importVCard();
		});
		
		future.then(this, function () {
			var result = future.result;
			
			return {
				contacts: ContactsServiceAssistant.convertArrayOfContactsForBus(result)
			};
		});
		
		return future;
	},
	
	vCardExportOne: function (future, params) {
		var innerFuture = new Future();
		
		innerFuture.now(this, function () {
			var onlyPhoneNumbers = params.onlyPhoneNumbers || false,
				filePath = params.filePath || "",
				charset = params.charset || undefined,
				vCardVersion = params.vCardVersion || undefined,
				personId = params.personId || undefined,
				useFileCache = params.useFileCache || false,
				vCardExporter = new VCardExporter({ filePath: filePath, vCardVersion: vCardVersion, charset: charset, useFileCache: useFileCache });
			innerFuture.nest(vCardExporter.exportOne(personId, onlyPhoneNumbers));
		});
		
		future.nest(innerFuture);
	},
	
	vCardExportAll: function (future, params) {
		var innerFuture = new Future();
			
		innerFuture.now(this, function () {
			var onlyPhoneNumbers = params.onlyPhoneNumbers || false,
				filePath = params.filePath || "",
				charset = params.charset || undefined,
				vCardVersion = params.vCardVersion || undefined,
				vCardExporter = new VCardExporter({ filePath: filePath, vCardVersion: vCardVersion, charset: charset });
			innerFuture.nest(vCardExporter.exportAll(onlyPhoneNumbers));
		});
		
		future.nest(innerFuture);
	},
	
	favoritePerson: function (params, applicationID) {
		var future = new Future();
		
		future.now(this, function () {
			return Person._favoritePerson(params, applicationID);
		});
		
		return future;
	},
	
	setFavoriteDefault: function (params, applicationID) {
		var future = new Future();
		
		future.now(this, function () {
			return Person._setFavoriteDefault(params, applicationID);
		});
		
		return future;
	},
	
	unfavoritePerson: function (params) {
		var future = new Future();
		
		future.now(this, function () {
			return Person._unfavoritePerson(params);
		});
		
		return future;
	},

	setCroppedContactPhoto: function (params) {
		var future = new Future();
		//console.log("setCroppedContactPhoto called with params: " + JSON.stringify(params));
		
		future.now(this, function () {
			return Person._setCroppedContactPhotoPerson(params);
		});
		
		return future;
	},

	/*
	 * Refetch a photo for a Palm Profile contact. This is called when the cropped version of the
	 * contact's photo has expired out of the file cache and is needed again. This recrops and
	 * rescales the image, and saves the path to the newly cropped/scaled image back into the DB
	 * again. 
	 * 
	 * If the cropInfo is not available in the contact, then we use a really bad default
	 * that puts the focus on the center of the image and does not do any scaling or cropping.
	 * Essentially, it just copies the image to the destination. This is useful for contact photos
	 * from older versions of the OS that are already cropped and scaled properly, but need to be
	 * copied into the file cache now.
	 * 
	 * @param {Object} params - should contain the contactId of the contact to update, and the
	 * photoId of the photo that seems to be missing from the cache 
	 */
	refetchPhoto: function (params) {
		var future = new Future(), contact;
		
		future.now(this, function () {
			// console.log("refetchPhoto: getting contact with id " + params.contactId);
			return DB.get([params.contactId]);
		});
		
		future.then(this, function () {
			var photoId = params.photoId,
				photos,
				thePhoto,
				rawContact = future.result,
				cropInfo;
			
			if (rawContact) {
				contact = Contacts.ContactFactory.create(Contacts.ContactType.EDITABLE, rawContact.results[0]);
				photos = contact.getPhotos().getArray();
				thePhoto = _.detect(photos, function (photo) {
					return (photo.getId() === photoId);
				});
				if (thePhoto) {
					if (!thePhoto.getDBObject().cropInfo) {
						// need to implement "scale to fit" in the com.palm.image service so that we can use it here
						// as the default action if the previous cropInfo is not available
						// This should never happen now that palm profile big/square photos are saved to /media/internal/
						// If for some reason it does, scale=1.0 will just copy the original photo
						cropInfo = {
							window: {
								focusX: 0.5,
								focusY: 0.5
							},
							scale: 1.0 
						};
					} else {
						cropInfo = thePhoto.getDBObject().cropInfo;
					}
					
					future.nest(contact.setCroppedContactPhoto(thePhoto.getValue(), cropInfo, thePhoto.getType()));
				} else {
					console.log("refetchPhoto: could not refetch thumbnail photo because there is no photo with photoId " + photoId + " on the contact with id " + contact.getId());
					return "";
				}
			} else {
				return "";
			}
		});
		
		future.then(this, function () {
			var result = future.result;
			// only save the contact if the previous then() returned an actual cropped photo path
			if (typeof(result) === 'string' && result.length > 0 && contact) {
				return contact.save();
			}
			
			return result;
		});
		
		return future;
	},

	deleteContactImage: function (future, params) {
		var innerFuture = new Future();
		
		innerFuture.now(this, function () {
			var path = params.path;
			if (path) {
				IO.Posix.unlink(path);
			}
		});
		
		future.nest(innerFuture);
	},

	sortOrderChanged: function (outerFuture, params) {
		var future = new Future(),
			batchDBWriter,
			activity,
			newSortOrder,
			prefsObjId;
		
		//TODO: should we be doing this in smaller pieces and rescheduling a background activity?
		//		maybe, since it could take a while (how long for 10k contacts?)
		//		maybe not, since it's guaranteed to be user-initiated
		
		future.now(this, function () {
			var appPrefs;
			
			activity = params.$activity;
			batchDBWriter = new BatchDBWriter();
			batchDBWriter.setBufferCommitSize(100); // The default is 500, but when the user is using the contacts app it slows down the user's ability to scroll through the list
				
			//first check to see if this call is really an error response
			if (activity && activity.trigger && !activity.trigger.returnValue) {
				//setting up the watch failed
				//TODO: should I cancel the activity here somehow?
				throw Foundations.Err.create(activity.trigger.errorCode, activity.trigger.errorText);
			}
			
			//console.log("\n\n\n in contacts service.sortOrderChanged.  params: " + Contacts.Utils.stringify(params) + "\n\n\n");
			
			
			//now, fetch the current sort order using the appprefs object
			appPrefs = new Contacts.AppPrefs(future.callback(this, function () {
				var dummy = future.result,
					findQuery,
					mapFunction;
				
				newSortOrder = appPrefs.get(Contacts.AppPrefs.Pref.listSortOrder);
				prefsObjId = appPrefs.get("_id");
				
				//console.log("\n\n\n in AppPrefs callback. listSortOrder: " + Contacts.Utils.stringify(newSortOrder));
				//console.log("in AppPrefs callback. prefsObjId: " + Contacts.Utils.stringify(prefsObjId) + "\n\n\n");
				
				//if we got the first run property, we don't want to do anything.  This prevents us from running when the watch is set up.
				//TODO: is this true, or should we just not have replace:true in the activity file?
				if (params.isFirstRun) {
					future.result = true;
					return;
				}
				
				//TODO: fetch the shouldConvertToPinyin preference
				
				//now that we got the prefs, we do the sortKey rewriting
				findQuery = {
					select: ["_id", "name", "organization"],
					from: Contacts.Person.kind,
					limit: 500
				};
				mapFunction = function (person) {
					//console.log("\n person: " + Contacts.Utils.stringify(person) + "\n");
					
					var future = SortKey.generateSortKey(person, {
						listSortOrder: newSortOrder
					});
					
					future.then(this, function () {
						var newSortKey = future.result;
						
						batchDBWriter.batchWrite(person, 'sortKey', newSortKey);
						
						future.result = true;
					});
					
					return future;
				};
				future.nest(Contacts.Utils.dbMap(findQuery, mapFunction));
			}));
		});
		
		future.then(this, function () {
			var dummy = future.result;
			
			//now commit all those sortKey writes we queued up
			batchDBWriter.commitBatchedWrites();
			
			//now that we're done, let's set up the watch again
			future.nest(this._restartSortOrderDBWatch(activity, prefsObjId, newSortOrder));
		});
		
		future.then(this, function () {
			var result = future.result;
			
			//console.log("\n\n\n restart watch result: " + Contacts.Utils.stringify(result) + "\n\n\n");
			
			//TODO: do something like this somehow?
			//		probably not, since our mojodb query is not changing, but maybe
			//cardStage = Mojo.Controller.appController.getStageController("email");
			//if (cardStage) {
			//	cardStage.delegateToSceneAssistant('invalidateMessageList');
			//}
			
			future.result = true;
		});
		
		outerFuture.nest(future);
	},
	
	_restartSortOrderDBWatch: function (activity, prefsObjId, oldSortOrder) {
		var params = {
			activityId: activity.activityId,
			restart: true,
			callback: {
				"method": "palm://com.palm.service.contacts/sortOrderChanged",
				"params": {}
			},
			trigger: {
				method: "palm://com.palm.db/watch",
				params: {
					query: {
						from: Contacts.AppPrefs.dbKind,
						where: [{
							prop: "_id",
							op: "=",
							val: prefsObjId
						}, {
							prop: Contacts.AppPrefs.Pref.listSortOrder,
							op: "!=",
							val: oldSortOrder
						}]
					},
					subscribe: true
				},
				key: "fired"
			}
		};
		
		//console.log("\n params to activity manager complete: " + Contacts.Utils.stringify(params) + "\n");
		
		return PalmCall.call("palm://com.palm.activitymanager/", "complete", params);
	},
	
	
	_setUpSortOrderDBWatch: function (prefsObjId, oldSortOrder) {
		console.log("\n\n setting up watch \n\n");
		var activityParams = {
			"activity": {
				"name": "sortOrderWatch",
				"description": "This is the watch for updating the sortKey on every person when the sort order is changed in the contacts app",
				"type": {
					"persist": true,
					"foreground": true
				},
				"trigger": {
					"method": "palm://com.palm.db/watch",
					"params": {
						"query": {
							"from": Contacts.AppPrefs.dbKind,
							"where": [{
								"prop": Contacts.AppPrefs.Pref.listSortOrder,
								"op": "!=",
								"val": oldSortOrder
							}]
						},
						"subscribe": true
					},
					"key": "fired"
				},
				"callback": {
					"method": "palm://com.palm.service.contacts/sortOrderChanged",
					"params": {}
				}
			},
			"start": true,
			"replace": true
		};
		
		if (prefsObjId) {
			activityParams.activity.trigger.params.query.where.push({
				"prop": "_id",
				"op": "=",
				"val": prefsObjId
			});
		}
		
		//console.log("\n activityParams: " + Contacts.Utils.stringify(activityParams) + "\n");
		
		return PalmCall.call("palm://com.palm.activitymanager/", "create", activityParams);
	},
	
	updatePersonLocaleAllAssistant: function (params) {
		
		var future = new Future(),
			dbFuture, 
			query,
			startTime = new Date(),
			endTime,
			batchDBWriter = new BatchDBWriter(true),
			newContactsRegion = params.locale || Globalization.Locale.getCurrentPhoneRegion(),
			futurePrefs,
			appPrefs,
			bufferCommitSize = 500, //write back to db after accumulated this many persons
			querySize = 500; //query in batches of this many persons from db for re-normalization
	
		console.info("Contacts phone number globalization : buffer commit size of " + bufferCommitSize + " set");
	
		futurePrefs = new Future();
		appPrefs = new Contacts.AppPrefs(futurePrefs.callback(this, function () {
				var dummy = futurePrefs.result;
				appPrefs.set(Contacts.AppPrefs.Pref.contactsPhoneRegion, newContactsRegion);
			}));
		
		//limit the amount of data taken from the database at once to 'querySize' records
		future.now(this, function (myFuture) {
			query = {
					select: ["_id", "phoneNumbers"],
					from: Contacts.Person.kind,
					limit: querySize 
				};
				
			var retrieveAllPersons = function (innerFuture) {
				var result = innerFuture.result,
					innerNestedFuture,
					persons = result.results,
					nextPageKey = result.next,
					personIndex, 
					phoneNumberIndex,
					phoneNumbers,
					value,
					parsedPhoneNumber,
					normalizedValue,
					phoneNumberValue;
					
				for (personIndex = 0; personIndex < persons.length; personIndex += 1) {
					phoneNumbers = Foundations.ObjectUtils.clone(persons[personIndex].phoneNumbers) || [];
					for (phoneNumberIndex = 0; phoneNumberIndex < phoneNumbers.length; phoneNumberIndex += 1) {
						value = parsedPhoneNumber = normalizedValue = phoneNumberValue = null;
						phoneNumberValue = phoneNumbers[phoneNumberIndex].value;
						//set dummy xx_ for the phoneNumberParser to take correctly the region from this string
						parsedPhoneNumber = Globalization.Phone.parsePhoneNumber(phoneNumberValue, "xx_" + newContactsRegion); 
						normalizedValue = PhoneNumber.normalizePhoneNumber(parsedPhoneNumber);
						phoneNumbers[phoneNumberIndex].normalizedValue = normalizedValue;
					}
					batchDBWriter.batchWrite(persons[personIndex], 'phoneNumbers', phoneNumbers);
				}

				if (!nextPageKey) {
					//if we didn't get a nextPageKey, we're done, so set innerFuture.result to unwind the chain
					innerFuture.result = true;
				} else {
					//else we got a page key, so we must do another query to get the next page
					query.page = nextPageKey;
					innerNestedFuture = DB.find(query);
					innerNestedFuture.then(this, retrieveAllPersons);
					innerFuture.nest(innerNestedFuture);
				}
			}; 
			//start the database query chain
			dbFuture = DB.find(query);
			dbFuture.then(this, retrieveAllPersons);
			myFuture.nest(dbFuture);
			
			return myFuture;
		});	
		
		future.then(this, function () {
			var dummy = future.result;
			//now commit all those re-normalized writes we queued up
			batchDBWriter.commitBatchedWrites();
			
			endTime = new Date();
			console.info("The re-parsing of the contacts to phone region '" + newContactsRegion + "' took " + ((endTime.valueOf() - startTime.valueOf()) / 1000).toString() + " seconds");
			future.result = {returnValue: true};
		});
		
		return future;
	}
});

ContactsServiceAssistant.getPathFromFileTypeSchemaPath = function (fileTypeSchemaPath) {
	var fileSchemaType = "file://",
		fileSchemaLength = fileSchemaType.length;
	
	Assert.requireString(fileTypeSchemaPath, "getPathFromFileTypeSchemaPath did not specify a filePath for the vcard to count");
	
	Assert.require(fileTypeSchemaPath.indexOf(fileSchemaType) === 0, "getPathFromFileTypeSchemaPath did not specify a target to a local file");
	
	return fileTypeSchemaPath.substr(fileSchemaLength);
};

ContactsServiceAssistant.convertArrayOfContactsForBus = function (contacts) {
	return contacts.map(function (contact) {
		return contact.getDBObject();
	});
};

Contacts.Utils.defineConstant("VERSION", "1.0", ContactsServiceAssistant);

// TODO: this should be fixed to actually get the application id from the bus.
//       for now hardcoding phone app since they are the only ones that can set favorites
//       besides the contacts app.
function getApplicationId(args) {
	if (args.defaultData && args.defaultData.appId) {
		return args.defaultData.appId;
	} else { 
		return "com.palm.app.phone";
	}
}

var contactsServiceAssistant = contactsServiceAssistant || new ContactsServiceAssistant();

function PhotoPreBackupAssistant() {
	return {
		run: function (future) {
			future.nest(contactsServiceAssistant.photoPreBackup(this.controller.args));
		}
	};
}

function VCardCountContactsAssistant() {
	return {
		run: function (future) {
			future.nest(contactsServiceAssistant.vCardCountContacts(this.controller.args));
		}
	};
}

function VCardReadAssistant() {
	return {
		run: function (future) {
			future.nest(contactsServiceAssistant.readVCard(this.controller.args));
		}
	};
}

function VCardImportAssistant() {
	return {
		run: function (future) {
			future.nest(contactsServiceAssistant.importVCard(this.controller.args));
		}
	};
}

//
// luna-send -n 1 palm://com.palm.service.contacts/vCardExportOne '{ filePath: "filePath", personId: "personId", [vCardVersion: "3.0"|"2.1"], [onlyPhoneNumbers: true|false], [charset: "UTF-8"|"US-ASCII"]}'
//
function VCardExportOneAssistant() {
	return {
		run: function (future) {
			contactsServiceAssistant.vCardExportOne(future, this.controller.args);
		}
	};
}

//
// luna-send -n 1 palm://com.palm.service.contacts/vCardExportAll '{ filePath: "filePath", [vCardVersion: "3.0"|"2.1"], [onlyPhoneNumbers: true|false], [charset: "UTF-8"|"US-ASCII"]}'
//
function VCardExportAllAssistant() {
	return {
		run: function (future) {
			contactsServiceAssistant.vCardExportAll(future, this.controller.args);
		}
	};
}

//
// luna-send -n 1 palm://com.palm.service.contacts/favoritePerson '{ personId: "personId", [defaultData: { value: "1234", contactPointType: "ContactsLib.ContactPointTypes.PhoneNumber", listIndex: 3, [auxData: { }] }]}'
//
function FavoritePersonAssistant() {
	return {
		run: function (future) {
			future.nest(contactsServiceAssistant.favoritePerson(this.controller.args, getApplicationId(this.controller.args)));
		}
	};
}

//
// luna-send -n 1 palm://com.palm.service.contacts/setFavoriteDefault '{ personId: "1", defaultData: { value: "1234", contactPointType: "ContactsLib.ContactPointTypes.PhoneNumber", listIndex: 3, [auxData: { }] }}'
//
function SetFavoriteDefaultAssistant() {
	return {
		run: function (future) {
			future.nest(contactsServiceAssistant.setFavoriteDefault(this.controller.args, getApplicationId(this.controller.args)));
		}
	};
}

//
// luna-send -n 1 palm://com.palm.service.contacts/unfavoritePerson '{ personId: "1" }'
//
function UnfavoritePersonAssistant() {
	return {
		run: function (future) {
			future.nest(contactsServiceAssistant.unfavoritePerson(this.controller.args));
		}
	};
}

//
// luna-send -n 1 palm://com.palm.service.contacts/setCroppedContactPhoto '{ personId: "1" }'
//
function SetCroppedContactPhotoAssistant() {
	return {
		run: function (future) {
			future.nest(contactsServiceAssistant.setCroppedContactPhoto(this.controller.args));
		}
	};
}

//
// luna-send -n 1 palm://com.palm.service.contacts/findPersonByPhone '{ 
//						phoneNumber: "phone number as a string",
//						params: (optional) {
//							returnAllMatches: (optional) true to return an array of matches rather than just the best match,
//							includeMatchingItem: (optional) true to return the exact item that matched along with the person in an object:
//								{ item: matching item, person: matching person }
//						}
//					}'
//
function FindPersonByPhoneAssistant() {
	return {
		run: function (future) {
			var args = this.controller.args,
				params = args.params || {};
			
			//override the personType to make sure it's a raw object, since we have to be able to send it across the bus
			params.personType = Contacts.PersonType.RAWOBJECT;
			future.nest(Contacts.Person.findByPhone(args.phoneNumber, params));
		}
	};
}

function SortOrderChangedAssistant() {
	return {
		run: function (future) {
			contactsServiceAssistant.sortOrderChanged(future, this.controller.args);
		},
		complete: function (activity) {
			// TODO: this could possibly be reworked to use the restartSortOrderWatch function but I couldn't think of a good way
			// to call it given that it takes a bunch of extra params not available here.
			return;
		}
	};
}

function SetUpWatchAssistant() {
	return {
		run: function (future) {
			future.nest(contactsServiceAssistant._setUpSortOrderDBWatch(undefined, ""));
		},
		complete: function (activity) {
			return;
		}
	};
}

function DeleteContactImageAssistant() {
	return {
		run: function (future) {
			contactsServiceAssistant.deleteContactImage(future, this.controller.args);
		}
	};
}

function RefetchPhotoAssistant() {
	return {
		run: function (future) {
			future.nest(contactsServiceAssistant.refetchPhoto(this.controller.args));
		}
	};
}

function updatePersonLocaleAllAssistant() {
	return {
		run: function (future) {
			future.nest(contactsServiceAssistant.updatePersonLocaleAllAssistant(this.controller.args));
		}
	};
}
