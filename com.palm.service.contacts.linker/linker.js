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
/*global require:true Utils, DB, console, Console, Future, Foundations, Assert, MojoLoader, Globalization: true,
JobIdGenerator, YieldController, TimingRecorder, Contact, Person, ContactLinkBackup, Autolinker, 
_, ManualUnlink, ManualLink, ContactPluralField, WatchRevisionNumber, ErrorCodes, PersonFactory, 
RECORD_TIMINGS_FOR_SPEED, process */

/* Notes about the plugin architecture:
 * Applications or JS-based services can provide a library that will be notified when a person is added, removed, or changed.
 * Loadable libraries which to plugin to the linker must provide a json file:
 * /etc/palm/contact_linker_plugins/<name_of_app_or_service>.json
 * The library can optionally export the following functions. Functions must return a future or an undefined value.
 * exports.personAdded = function(person) // passed a person object whenever a person is added
 * exports.personRemoved = function(person) // passed a person object whenever a person is removed
 * exports.personChanged = function(personOld, personNew) // passed two person objects: the old version and the new version
 * exports.syncDone = function() // called when the sync is completed. Service must then query com.palm.person:1
 */

function LinkerAssistant() {
	// path to look for contact linker plugin definitions
	var PLUGIN_PATH = "/etc/palm/contact_linker_plugins/",
		plugins = []; // array of plugins to notify for changes
	
	// private helper loads the plugins from PLUGIN_PATH
	function loadPlugins() {
		var future = new Future(),
			fs = require('fs');
			
		// helper reads the plugin file into this.plugins
		function handlePluginFile(file) {
			var pluginLibrary;

			try {
				if (file.match(/\.json$/)) {
					pluginLibrary = JSON.parse(Foundations.Comms.loadFile(PLUGIN_PATH + file));
					Assert.require(pluginLibrary && pluginLibrary.name, "Plugin library at path " + file + " doesn't contain attribute 'name' " + JSON.stringify(pluginLibrary));
					plugins.push({
						name: pluginLibrary.name,
						object: MojoLoader.require(pluginLibrary)[pluginLibrary.name]
					});
				} else {
					console.log("Linker plugin skipping non-json file: " + file);
				}

			} catch (e) {
				console.error("FAILED LOADING CONTACT LINKER PLUGIN at path " + file + ": " + JSON.stringify(e));
			}
		}
		
		future.now(function () {
			return fs.readdirSync(PLUGIN_PATH);
				//IO.Directory.getFiles(PLUGIN_PATH, /\.json$/));
		});

		future.then(this, function () {
			try {
				future.result.forEach(handlePluginFile);
			} catch (e) {
				console.error("Failed retrieving files from " + PLUGIN_PATH);
			}
			future.result = undefined;
		});

		return future;
	}
	
	this.setup = function () {
		try {
			Globalization = (MojoLoader.require({
				"name": "globalization",
				"version": "1.0"
			})).globalization.Globalization;
		} catch (e) {
			console.log("Error: failed to load Globalization: " + e.message);
		}

		var future = loadPlugins();
		return future;
	};
	
	// call methodName with args on registered plugins
	this.dispatchToPlugins = function (methodName /*, variable args*/ ) {
		var restOfArgs = _.toArray(arguments).slice(1);
		
		function dispatchPlugin(plugin) {
			var future = new Future().immediate();
			if (_.isFunction(plugin.object[methodName])) {
				future.then(function () {
					var result = plugin.object[methodName].apply(undefined, restOfArgs);
					Assert.require(result !== undefined, "Plugin " + plugin.name + "'s " + methodName + " return undefined value");
					return result;
				});
				future.then(function () {
					var e = future.getException();
					if (e) {
						console.error("Exception in plugin " + plugin.name + "'s " + methodName + ": " + JSON.stringify(e));
					}
					return true; // continue
				});
			}
			return future;
		}
		
		return Foundations.Control.mapReduce({map: dispatchPlugin}, plugins);
	};
	

}

LinkerAssistant.prototype.deletePerson = function (params) {
	var future = new Future(),
		person,
		personFuture,
		linkedContacts;

	future.now(this, function () {
		future.result = false;
		if (params && params.personId) {
			personFuture = Person.getDisplayablePersonAndContactsById(params.personId);
			personFuture.then(this, function () {
				try {
					person = personFuture.result;
					if (person) {
						linkedContacts = person.getContacts();
						if (linkedContacts && linkedContacts.length === 0) { // This person doesn't have any contacts associated with it, allow the user to delete it from the database.
							person.deletePerson();
							future.result = true;
						} else {
							console.error("Uncaught: This person is associated with contacts therefore it cannot be deleted");
						}
					}
				} catch (e) {
					console.error("LinkerAssistant.deletePerson encounterend an error: " + e.toString());
				}
			});
		}
	});
	
	return future;
};

/////////////// Manual Linking /////////////////

LinkerAssistant.verifyManualLinkParams = function (params) {
	var resultMessage;
	
	resultMessage = "";
	if (!params.personToLinkTo) {
		resultMessage = "The 'personToLinkTo' parameter was not specified. ";
	}
	
	if (!params.personToLink) {
		resultMessage += "The 'personToLink' parameter was not specified.";
	}
	
	return { 
		valid: (resultMessage.length === 0),
		resultMessage: resultMessage
	};
};

LinkerAssistant.prototype.manuallyLink = function (controller) {
	var future = new Future(),
		personToLink,
		personToLinkId,
		personToLinkTo,
		personToLinkToId,
		personToLinkToOriginal,
		person1OriginalContacts,
		person2OriginalContacts;
	
	future.now(this, function () {
		var params = controller.args,
			validParamsResult = LinkerAssistant.verifyManualLinkParams(params);
		
		if (validParamsResult.valid) {
			//Console.log("Fetching the people to manually link");
			personToLinkToId = params.personToLinkTo;
			personToLinkId = params.personToLink;
			// Get the two person records we are trying to link from the database
			return DB.get([personToLinkToId, personToLinkId]);
		} else {
			throw new Error(validParamsResult.resultMessage);
		}
	});
	
	future.then(this, function () {
		var results = Utils.DBResultHelper(future.result);
		
		if (results && results.length > 0) {
			results.forEach(function (result) {
				var person = PersonFactory.createPersonLinkable(result);
				
				if (person.markedForDelete()) {
					Console.log("One of the people you are trying to link with is marked for delete. Can not link with a person that is marked for deletion");
					throw new Error("Can not link with a person that is marked for deletion");
				}
				
				// Set the person objects to make the link
				if (person.getId() === personToLinkToId) {
					personToLinkTo = person;
					personToLinkToOriginal = new Person(personToLinkTo.getDBObject());
				} else if (person.getId() === personToLinkId) {
					personToLink = person;
				}
			});
			
			if (personToLinkTo && personToLink) {
				person1OriginalContacts = personToLinkTo.copyOfContactIds();
				person2OriginalContacts = personToLink.copyOfContactIds();

				personToLinkTo.mergeContactIds(personToLink.getContactIds().getArray());
				
				return personToLinkTo.fixup([personToLink]);
			} else {
				throw new Error("Trying to fetch one of the people objects failed. One of them probably does not exist in the db.");
			}
			
		} else {
			throw new Error("Neither of the people you want to link exist in the db");
		}
	});
	
	future.then(this, function () {
		var result = future.result;
		
		return personToLinkTo.save();
	});
	
	future.then(this, function () {
		var result = future.result;
		
		if (!result) {
			throw new Error("Updating of personToLinkTo failed");
		} else {
			return true;
		}
	});
	
	future.then(this, function () {
		var result = future.result;
		
		return controller.service.assistant.dispatchToPlugins("personChanged", personToLinkToOriginal, personToLinkTo); // removed contact info (but not person)
	});
	
	future.then(this, function () {
		var result = future.result;
		// Delete the orphaned person record
		return DB.del([personToLink.getId()]);
	});
	
	future.then(this, function () {
		var result = future.result;

		if (!result.results.length) {
			throw new Error("Deleting of personToLink failed");
		} else {
			return true; // continue
		}
	});
	
	future.then(this, function () {
		var dummy = future.result;
		
		return controller.service.assistant.dispatchToPlugins("personRemoved", personToLink); // isolated removal
	});
	
	future.then(this, function () {
		var dummy = future.result,
			clb = new ContactLinkBackup();
		
		return clb.performedManualLink(person1OriginalContacts, person2OriginalContacts);
	});
	
	future.then(this, function () {
		var result = future.result;
		
		Console.log("Done with clb");
		return true;
	});
	
	return future;
};

LinkerAssistant.prototype.manuallyUnlink = function (controller) {
	var params, personToRemoveLinkFromId, contactIdToRemoveFromPerson,
		personToRemoveLinkFrom, personToRemoveLinkFromOriginal, person,
		future = new Future();
		
	future.now(this, function () {
		var resultMessage = "";
		
		params = controller.args;
		
		if (!params.personToRemoveLinkFrom) {
			resultMessage = "The 'personToRemoveLinkFrom' parameter was not specified. ";
		}
		
		if (!params.contactToRemoveFromPerson) {
			resultMessage += "The 'contactToRemoveFromPerson' parameter was not specified.";
		}
		
		if (resultMessage.length > 0) {
			throw new Error(resultMessage);
		}
		
		// Fetch the person record to remove the contact from
		personToRemoveLinkFromId = params.personToRemoveLinkFrom;
		contactIdToRemoveFromPerson = params.contactToRemoveFromPerson;
		// Get the person record we are trying to remove the link from the database
		return DB.get([personToRemoveLinkFromId]);
	});
	
	future.then(this, function () {
		var person,
			contactRemoved,
			result = Utils.DBResultHelper(future.result);
		
		if (result && result.length > 0) {
			Console.log(JSON.stringify(result[0]));
			
			person = PersonFactory.createPersonLinkable(result[0]);
			
			Console.log(JSON.stringify(person.getContactIds().getArray()));
			
			if (person.markedForDelete()) {
				Console.log("The person you are trying to remove the link from is marked for delete. Can not remove a link from a person that is marked for deletion");
				throw new Error("Can not remove the link from a person that is marked for deletion");
			}
			
			// Set the person object to remove the link from
			personToRemoveLinkFrom = person;
			personToRemoveLinkFromOriginal = new Person(personToRemoveLinkFrom.getDBObject());
			
			if (personToRemoveLinkFrom) {
				// Remove the contactId of the contact you are trying to unlink 
				Console.log("The old personToLinkTo contactsIds: " + personToRemoveLinkFrom.getContactIds());
				contactRemoved = personToRemoveLinkFrom.getContactIds().remove(contactIdToRemoveFromPerson);
				Console.log("Removed contactId - " + personToRemoveLinkFrom);
				// Throw exeception that the contact did not exist in this persons
				// contactId
				if (!contactRemoved) {
					Console.log("The contact you are trying to unlink does not currently belong to this person");
					throw new Error("The contact you are trying to unlink does not belong to the person you specified");
				}
				
				Console.log("The new personToLinkTo contactsIds: " + personToRemoveLinkFrom.getContactIds());
				
				// Save the updated personToLinkTo record
				return personToRemoveLinkFrom.fixup();
			} else {
				throw new Error("Trying to fetch the person object failed. It probably does not exist in the db.");
			}
		} else {
			throw new Error("The person to remove the link from does not exist in the db.");
		}
	});
	
	future.then(this, function () {
		var result = future.result;
		
		return personToRemoveLinkFrom.save();
	});
	
	future.then(this, function () {
		var result = future.result;
		
		return controller.service.assistant.dispatchToPlugins("personChanged", personToRemoveLinkFromOriginal, personToRemoveLinkFrom);
	});
	
	future.then(this, function () {
		Console.log("In finish update of personToLinkTo");
		var result = future.result;
		
		person = PersonFactory.createPersonLinkable();
		if (result) {
			Console.log("Updating of person was successful");
			// Create a person for the contact that was just unlinked
			person.getContactIds().add(contactIdToRemoveFromPerson);
			
			// Save the updated personToLinkTo record
			return person.fixup();
		} else {
			Console.log("Updating person record failed");
			throw new Error("Updating of personToRemoveLinkFrom failed");
		}
	});
	
	future.then(this, function () {
		var result = future.result;
		
		return person.save();
	});
	
	future.then(this, function () {
		var result = future.result;
		return controller.service.assistant.dispatchToPlugins("personAdded", person);
	});
	
	future.then(this, function (future) {
		var result = future.result,
			clb = new ContactLinkBackup();
		Console.log("In finish add of new person");
		//Console.log(JSON.stringify(future.result));
		if (result) {
			Console.log("Adding new person record was successful");
			Console.log("Kicking off performed manual unlink in CLB");
			
			return clb.performedUnlink(personToRemoveLinkFrom, contactIdToRemoveFromPerson);
		} else {
			console.log("Adding new person record failed");
			throw new Error("Adding person record for unlink contact failed");
		}
	});
	
	future.then(this, function () {
		var result = future.result;
		Console.log("Done with clb");
		return result;  
	});
	
	return future;
};

////////////////////////////////////////////////


var linkerService = linkerService || new LinkerAssistant();

//
// luna-send -n 1 palm://com.palm.service.contacts.linker/manualLink '{ "personToLinkTo":id, "personToLink":id }'
//
function ManualLinkerAssistant() {
	return {
		run: function (future) {
			future.nest(linkerService.manuallyLink(this.controller));
		}
	};
}

//
// luna-send -n 1 palm://com.palm.service.contacts.linker/manualUnlink '{ "personToRemoveLinkFrom":id, "contactToRemoveFromPerson":id }'
//
function ManualUnlinkerAssistant() {
	return {
		run: function (future) {
			future.nest(linkerService.manuallyUnlink(this.controller));
		}
	};
}


/***************** Autolinker methods *********************/
var autolinker = autolinker || new Autolinker();
var jobIdGenerator = jobIdGenerator || new JobIdGenerator();
var yieldController = yieldController || new YieldController();
var autoLinkerRunning = autoLinkerRunning || false;
var timingRecorder = new TimingRecorder(RECORD_TIMINGS_FOR_SPEED, process);

//
// Autolinker hack on boot method. Please take me out when done!!!!!!!!
//
// luna-send -n 1 palm://com.palm.service.contacts.linker/setupWatch '{}'
//
function SetupWatchAssistant() {
	return {
		run: function (future) {
			future.nest(autolinker.setupWatch());
		},
		complete: function (activity) {
			// normally, would modify activity and complete with restart
			return;
		}
	};
}


// 
// luna-send -n 1 palm://com.palm.service.contacts.linker/forceAutolink '{}'
//
function ForceAutolinkerAssistant() {
	var jobId = jobIdGenerator.generateJobId();
	
	return {
		run: function (future) {
			Assert.requireFalse(autoLinkerRunning, "The autolinker was called to run autolink when it is already performing an autolink!!!!!");
			autoLinkerRunning = true;
			future.nest(autolinker.forceAutolink(this.controller, jobId));
		},
		"yield": function () {
			yieldController.addJobToYield(jobId);
		},
		complete: function (activity) {
			// normally, would modify activity and complete with restart
			autoLinkerRunning = false;
			return;
		}
	};
}

//
// luna-send -n 1 palm://com.palm.service.contacts.linker/dbUpdatedRelinkChanges '{ "revChangedStart":revValue }'
//

function UpdatedDBLinkAssistant() {
	var jobId = jobIdGenerator.generateJobId();
	
	return {
		run: function (future) {
			Assert.requireFalse(autoLinkerRunning, "The autolinker was called to run autolink when it is already performing an autolink!!!!!");
			autoLinkerRunning = true;
			console.log("From autolink watch callback - Rev: " + this.controller.args.revChangedStart);
			this.controller.lastProcessedContactRevId = this.controller.args.revChangedStart ? this.controller.args.revChangedStart : -1;
			future.nest(autolinker.dbUpdatedRelinkChanges(this.controller, jobId));
		},
		"yield": function () {
			console.log("Autolinker received a request to yield!!!");
			yieldController.addJobToYield(jobId);
		},
		complete: function (activity) {
			var future = new Future();
			
			future.now(this, function () {
				return this.controller.service.assistant.dispatchToPlugins("syncDone");
			});
			
			future.then(this, function () {
				var dummy = future.result;
				
				autoLinkerRunning = false;
				return Autolinker.restartActivityUsingRevisionNumber(activity._activityId, this.controller.lastProcessedContactRevId);
			});
			
			future.then(this, function () {
				var dummy = future.result;
				
				console.log("Activity Updated");
				return true;
			});
			
			return future;
		}
	};
}

//
// luna-send -n 1 palm://com.palm.service.contacts.linker/yieldAutolink '{"jobId": "aaa"}'
//

function AutolinkerYieldAssistant() {
	return {
		run: function (future) {
			Assert.requireString(this.controller.args.jobId, "You must specify a job id to yield!");
			yieldController.addJobToYield(this.controller.args.jobId);
			future.result = "Job with id " + this.controller.args.jobId + " is requested to yield";
		}
	};
}

/////////////// Only allow a user to manually delete a person without any associated contacts from the database ////////
// This should only be used in special cases as outlined in [DFISH-25142]
// luna-send -n 1 palm://com.palm.service.contacts.linker/deleteOrphanedPerson '{"personId": personId}'
function DeleteOrphanedPersonAssistant() {
	return {
		run: function (future) {
			future.nest(linkerService.deletePerson(this.controller.args));
		}
	};
}

//
// luna-send -n 1 palm://com.palm.service.contacts.linker/saveNewPersonAndContacts '{"person": { personObject }, "contacts": [{ contactObjects }] }'
//
function SaveNewPersonAndContactsAssistant() {
	return {
		run: function (future) {
			future.nest(autolinker.saveNewPersonAndContacts(this.controller, this.controller.args));
		}
	};
}

/*****************************************/

/************* Contact Link Backup methods ****************/
var contactLinkBackupService = contactLinkBackupService || new ContactLinkBackup();

//
// luna-send -n 1 palm://com.palm.service.contacts.linker/resetCLBDB '{}'
//
function ContactLinkBackupResetDBAssistant() {
	return {
		run: function (future) {
			contactLinkBackupService.resetDB(future);
		}
	};
}

//
// luna-send -n 1 palm://com.palm.service.contacts.linker/performedManualLink '{ "person1Contacts" : [array of contactIds], "person2Contacts" : [array of contactIds] }'
//
function ContactLinkBackupPerformedManualLink() {
	return {
		run: function (future) {
			future.nest(contactLinkBackupService.performedManualLinkDBus(this.controller.args));
		}
	};
}

//
// luna-send -n 1 palm://com.palm.service.contacts.linker/performedUnlink '{ "personOrPersonId": personObject|personId, "contactOrContactId": contactObject|contactId }'
//
function ContactLinkBackupPerformedUnlink() {
	return {
		run: function (future) {
			future.nest(contactLinkBackupService.performedUnlinkDBus(this.controller.args));
		}
	};
}

//
// luna-send -n 1 palm://com.palm.service.contacts.linker/getManualLinks '{ "contactOrContactId": contactObject|contactId }'
//
function ContactLinkBackupGetManualLinks() {
	return {
		run: function (future) {
			future.nest(contactLinkBackupService.getManualLinksDBus(this.controller.args));
		}
	};
}

//
// luna-send -n 1 palm://com.palm.service.contacts.linker/getUnlinkLinks '{ "contactOrContactId": contactObject|contactId }'
//
function ContactLinkBackupGetUnlinkLinks() {
	return {
		run: function (future) {
			future.nest(contactLinkBackupService.getUnlinkLinksDBus(this.controller.args));
		}
	};
}
