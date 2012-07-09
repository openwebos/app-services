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

/*jslint white: true, onevar: true, undef: true, eqeqeq: true, plusplus: true, bitwise: true, 
 regexp: true, newcap: true, immed: true, nomen: false, maxerr: 500 */
/*global include, MojoTest, JSON, console, palmGetResource, PersonFactory, Person, Future, Assert, Test, IMAddress, PersonType, Contact, PalmCall, PhoneNumber, ListWidget, DB, _, ContactsServiceAssistant */

include("test/loadall.js");

function PhotoBackupTests() {
	this.accountId = "";
}

PhotoBackupTests.prototype.before = function (done) {
	var future = new Future();
	
	future.now(this, function () {
		if (!this.accountId) {
			return PhotoBackupTests.createPalmProfileAccount();
		}
	});
	
	future.then(this, function () {
		var result = future.result;
		
		this.accountId = result.accountId;
		
		return PhotoBackupTests.createContacts(this.accountId);
	});
	
	future.then(this, function () {
		done();
	});
};

PhotoBackupTests.prototype.after = function (done) {
	var future = new Future();
	
	future.now(this, function () {
		if (this.accountId) {
			return PhotoBackupTests.deleteAccount(this.accountId);
		}
	});
	
	future.then(this, function () {
		this.accountId = "";
		
		return DB.del({
			from: "com.palm.contact:1"
		});
	});
	
	future.then(this, function () {
		done();
	});
};

PhotoBackupTests.expectedPhotoCount = 0;
PhotoBackupTests.testPhotoData = [{
		photos: [{ 
			"value": "/path/to/some/photovalueaustinsquare.png",
			"type": "type_square",
			"localPath": "/localpath/to/some/photoaustinsquare.png"
		}, { 
			"value": "/path/to/some/photovalueaustinbig.png",
			"type": "type_big",
			"localPath": "/localpath/to/some/photoaustinbig.png"
		}]
	}, {
		photos: [{ 
			"value": "/path/to/some/photovaluestevensquare.png",
			"type": "type_square",
			"localPath": "/localpath/to/some/photostevensquare.png"
		}]
	}, {
		photos: [{ 
			"value": "/path/to/some/photovaluebensquare.png",
			"type": "type_square",
			"localPath": "/localpath/to/some/photobensquare.png"
		}]
	}
];

PhotoBackupTests.isFilePathInExpectedTestPhotoData = function (path) {
	var length = PhotoBackupTests.getExpectedNumberOfPhotos(),
		i,
		tempPhotos,
		found = false,
		photoCompareFunc = function (photo) {
			return photo.value === path;
		};
	
	for (i = 0; i < length; i += 1) {
		tempPhotos = PhotoBackupTests.getPhotosForTestContactData(i);
		
		found = _.detect(tempPhotos, photoCompareFunc);
		
		if (found) {
			return true;
		}
	}
	
	return false;
};

PhotoBackupTests.getExpectedNumberOfPhotos = function () {
	return PhotoBackupTests.expectedPhotoCount;
};

PhotoBackupTests.getPhotosForTestContactData = function (index) {
	return PhotoBackupTests.testPhotoData[index].photos;
};

PhotoBackupTests.createContacts = function (accountId) {
	var decoratedContacts = [],
		contacts = [],
		future = new Future();
	
	future.now(function () {
		decoratedContacts.push(new Contact({
			"name": {
				"givenName": "Austin",
				"familyName": "Powers"
			},
			"accountId": accountId,
			"photos": PhotoBackupTests.getPhotosForTestContactData(0) // 2 photos
		}));
		
		decoratedContacts.push(new Contact({
			"name": {
				"givenName": "Steven",
				"familyName": "Powers"
			},
			"accountId": accountId,
			"photos": PhotoBackupTests.getPhotosForTestContactData(1) // 1 photo
		}));
		
		decoratedContacts.push(new Contact({
			"name": {
				"givenName": "Ben",
				"familyName": "Powers"
			},
			"accountId": "AwesomeNonPalmProfileAccount",
			"photos": PhotoBackupTests.getPhotosForTestContactData(2) // 1 photo
		}));
		
		decoratedContacts.push(new Contact({
			"name": {
				"givenName": "Luke",
				"familyName": "Skywalker"
			},
			"accountId": accountId
		}));
		
		PhotoBackupTests.expectedPhotoCount = 3;
		
		decoratedContacts.forEach(function (contact) {
			contacts.push(contact.getDBObject());
		});
		
		return DB.put(contacts);
	});
	
	return future;
};

PhotoBackupTests.createPalmProfileAccount = function () {
	var future = new Future();
	
	future.now(function () {
		var params = {};
		
		params = {
			templateId: "com.palm.palmprofile",
			capabilityProviders: [ { capability: "CONTACTS", id: "com.palm.palmprofile.contacts" } ],
			credentials: {},
			config: {},
			username: "testAccount"
		};
		
		return PalmCall.call("palm://com.palm.service.accounts/", "createAccount", params);
	});
	
	future.then(function (future) {
		var result = future.result;
		
		Assert.require(result.result, "AccountsFacade.createAccount's result does not have a result object");
		result = result.result;
		
		return {
			success: true,
			accountId: result._id,
			username: result.username
		};
	});
	
	return future;
};

PhotoBackupTests.deleteAccount = function (accountId) {
	return PalmCall.call("palm://com.palm.service.accounts", "deleteAccount", {
		accountId: accountId
	});
};

PhotoBackupTests.prototype.testPhotoPreBackup = function (done) {
	var contactsServiceAssistant = new ContactsServiceAssistant(),
		future = new Future();

	future.now(this, function () {
		return contactsServiceAssistant.photoPreBackup({
			maxTempBytes: 0,
			tempDir: "/you/dont/need/a/tempdir"
		});
	});
	
	future.then(this, function () {
		var result = future.result,
			files = result.files,
			version = result.version,
			hasMore = result.hasMore,
			incrementalKey = result.incrementalKey,
			description = result.description,
			failed = false;
			
		Assert.requireString(version, "PhotoBackupTests.testPhotoPreBackup - photoPreBackup did not return a version");
		Assert.requireFalse(hasMore, "PhotoBackupTests.testPhotoPreBackup - photoPreBackup set hasMore to true. This should not happen");
		Assert.requireFalse(!!incrementalKey, "PhotoBackupTests.testPhotoPreBackup - photoPreBackup returned a truthy incremental key");
		Assert.requireString(description, "PhotoBackupTests.testPhotoPreBackup - photoPreBackup did not return a description");
		Assert.requireDefined(files, "PhotoBackupTests.testPhotoPreBackup - photoPreBackup did not return a defined files list");
		
		Assert.require(files.length === PhotoBackupTests.getExpectedNumberOfPhotos(), "PhotoBackupTests.testPhotoPreBackup - photoPreBackup did not return the expected number of files for backup - expected: " + PhotoBackupTests.getExpectedNumberOfPhotos() + " actual: " + files.length);
		
		// Look for a file that is not in the list of expected test photo data
		failed = _.detect(files, function (file) {
			return !PhotoBackupTests.isFilePathInExpectedTestPhotoData(file);
		});
		
		Assert.require(!failed, "PhotoBackupTests.testPhotoPreBackup - One of the expected file paths were not found in the array of file paths to backup");
		
		done(MojoTest.passed);
	});
};