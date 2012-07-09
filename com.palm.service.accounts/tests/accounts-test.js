// @@@LICENSE
//
//      Copyright (c) 2009-2011 Hewlett-Packard Development Company, L.P.
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

/*jslint devel: true, onevar: false, undef: true, eqeqeq: true, bitwise: true, 
regexp: true, newcap: true, immed: true, nomen: false, maxerr: 500 */

/*global IMPORTS, MojoLoader, Test */

var webos = IMPORTS.require('webos'),
	palmbus = IMPORTS.require('palmbus'),
	sys = IMPORTS.require('sys');
webos.include("test/loadall.js");


try {
	var libraries = MojoLoader.require(
		{ name: "foundations", version: "1.0" },
		{ name: "underscore", version: "1.0" }
	);
	var Future = libraries.foundations.Control.Future;
	var PalmCall = libraries.foundations.Comms.PalmCall;
	var DB = libraries.foundations.Data.DB;
	var _ = libraries.underscore._;
	var exec = IMPORTS.require("child_process").exec;
} catch (Error) {
	console.error(Error);
}

function AccountsTests() {
	if (!AccountsTests._handle) {
		AccountsTests._handle = new palmbus.Handle("", false);
		PalmCall.register(AccountsTests._handle);
	}
}

AccountsTests.timeoutInterval = 10000;

AccountsTests.prototype.after = function(done) {
	if (this.accountId) {
		this.lunaSend("palm://com.palm.db", "del", {ids: [this.accountId]}, "com.palm.configurator").then(done);
	} else {
		done();
	}
};

AccountsTests.prototype.lunaSend = function (service, method, params, caller) {
	var json = JSON.stringify(params);

	var future = new Future();
	future.now(function () {
		var cmd = "luna-send -n 1 " + (caller ? "-a " + caller : "") + " " + service + "/" + method + " '" + json + "'";
		// sys.debug("SENT CMD: " + cmd);
		exec(cmd, function (error, stdout, stderr) {
			if (error) {
				future.exception = error;
			} else {
				// sys.debug("STDOUT for " + method + ": " + stdout);
				// sys.debug("STDERR for " + method + ": " + stderr);
				var output = JSON.parse(stdout);
				if (output.exception) {
					future.exception = output;
				} else {
					future.result = output;
				}
			}
		});
	});
	return future;
};

AccountsTests.prototype.testTestFramework = function() {
	return Test.passed;
};

AccountsTests.prototype.testListTemplates = function(done) {
	var future = PalmCall.call("palm://com.palm.service.accounts/", "listAccountTemplates", {});
	future.then(function() {
		var result = future.result;
		// console.log("result of service call=" + JSON.stringify(result));
		
		this.allTemplatesCount = result.results.length;
		
		Test.require(result.returnValue);
		Test.require(result.results && result.results.length > 0);
		future.result = Test.passed;
	});
	future.then(done);
};

AccountsTests.prototype.testListTemplatesFiltered = function(done) {
	var future = PalmCall.call("palm://com.palm.service.accounts/", "listAccountTemplates", {
		capability: "CONTACTS"
	});
	future.then(function() {
		var result = future.result;
		// console.log("result of service call=" + JSON.stringify(result));
		Test.require(result.returnValue);
		Test.require(result.results && result.results.length > 0 && result.results.length < this.allTemplatesCount);
		future.result = Test.passed;
	});
	future.then(done);
};

AccountsTests.prototype.createAccountBase = function(creator, extraProps) {
	
	// create an account using the first template found
	var template;
	
	var future = PalmCall.call("palm://com.palm.service.accounts/", "listAccountTemplates", {});
	future.then(this, function () {
		var result = future.result;
		var account;
		
		template = result.results[0];
		future.template = template;
		
		
		account = _.extend({
			templateId: template.templateId,
			capabilityProviders: [ { id: template.capabilityProviders[0].id } ],  // create an account with just the first capability enabled
			username: "testuser@domain.com"
		}, extraProps);

		future.nest(this.lunaSend("palm://com.palm.service.accounts/", "createAccount", account, creator));
	});
	future.then(this, function (join) {
		this.accountId = join.result.result && join.result.result._id;
		if (!this.accountId) {
			throw join.result;
		}
		return join.result;
	});

	return future;
};

AccountsTests.prototype.createAccountWithCredentials = function(creator) {
	return this.createAccountBase(creator, {
		credentials: {
			common: {
				password: "secret",
				authToken: "321891387984670935"
			},
			imap: {
				authToken: "FBAEDBECDA9A98D675DC7E90936F310"
			}
		}
	});
};

AccountsTests.prototype.createAccountWithPassword = function(creator) {
	return this.createAccountBase(creator, {
		password: "secret"
	});
};

AccountsTests.prototype.testCreateAccount = function(done) {
	var future = this.createAccountWithCredentials("com.palm.foo");
	
	future.then(function() {
		var response, template, account;
		
		response = future.result;
		account = response.result;
		template = future.template;
		// console.log("createAccount returned: " + JSON.stringify(result));
		Test.require(response.returnValue);
		Test.requireEqual(template.templateId, account.templateId);
		Test.requireEqual(1, account.capabilityProviders.length);
		Test.requireEqual(template.capabilityProviders[0].id, account.capabilityProviders[0].id);
		Test.require(typeof account._id !== "undefined");
		
		future.result = Test.passed;
	});
	future.then(done);
};

AccountsTests.prototype.testModifyAccountAlias = function(done) {
	var newAlias = "new alias";
	
	var future = this.createAccountWithCredentials("com.palm.foo");
	future.then(this, function () {
		var response = future.result;
		future.nest(this.lunaSend("palm://com.palm.service.accounts/", "modifyAccount", {
			accountId: response.result._id,
			object: {
				alias: newAlias
			}
		}, "com.palm.foo"));
	});
	
	future.then(function() {
		var response = future.result;
		Test.require(response.returnValue);
		future.result = Test.passed;
	});
	
	future.then(done);
};

AccountsTests.prototype.testGetAccountInfo = function(done) {
	var future = this.createAccountWithCredentials("com.palm.foo");
	future.then(this, function () {
		var response = future.result;
		
		future.nest(this.lunaSend("palm://com.palm.service.accounts/", "getAccountInfo", {
			accountId: response.result._id
		}, "com.palm.foo"));
	});
	
	future.then(function() {
		var result = future.result;
		// console.log("******* got account info: " + JSON.stringify(result));
	
		Test.require(result.returnValue, 1);
		Test.require(result.result, 2);
		Test.require(result.result.username, 3);
				
		Test.require(result.result.loc_name, 4);
		Test.require(result.result.icon, 5);
		Test.require(result.result.capabilityProviders && result.result.capabilityProviders.length > 0, 6);
	
		future.result = Test.passed;
	});
	future.then(done);
};

AccountsTests.prototype.testListAccounts = function(done) {
	var future = this.createAccountWithCredentials("com.palm.foo"),
		template;
	
	function list(params) {
		return PalmCall.call("palm://com.palm.service.accounts/", "listAccounts", params);
	}
	
	future.then(function() {
		var r;
		r = future.result;
		template = future.template;
		
		// test basic query
		future.nest(list({}));
	});
	
	future.then(function() {
		var result = future.result;

		Test.require(result.returnValue);
		Test.require(result.results && result.results.length);
		
		// test that template metadata came over as well
		Test.require(result.results[0].loc_name);
		Test.require(result.results[0].icon);
		Test.require(result.results[0].capabilityProviders && result.results[0].capabilityProviders.length > 0);
		
		// test querying by templateId, positive case
		future.nest(list({
			templateId: template.templateId
		}));
		
	});
	
	future.then(function() {
		var result = future.result;
		
		Test.require(result.returnValue);
		Test.require(result.results && result.results.length === 1);
		
		// test querying by templateId, negative case
		future.nest(list({
			templateId: "fail"
		}));
	});
	
	future.then(function() {
		var result = future.result;
		
		Test.require(result.returnValue);
		Test.require(result.results && result.results.length === 0);
		
		// test querying by capability, positive case
		future.nest(list({
			capability: template.capabilityProviders[0].capability
		}));
	});
	
	future.then(this, function() {
		var result = future.result;
		
		Test.require(result.returnValue);
		Test.require(result.results && result.results.some(function (item) {
				return item._id === this.accountId;
			}, this));
		
		// test querying by capability, negative case
		future.nest(list({
			capability: "FAIL"
		}));
	});
	
	future.then(function() {
		var result = future.result;
		
		Test.require(result.returnValue);
		Test.require(result.results && result.results.length === 0);
		
		future.result = Test.passed;
	});
	
	future.then(done);
};

AccountsTests.prototype.testHasCredentials = function(done) {
	var future = this.createAccountWithCredentials("com.palm.foo");
	
	future.then(function() {
		var response = future.result;
		
		future.nest(PalmCall.call("palm://com.palm.service.accounts/", "hasCredentials", {
			accountId: response.result._id
		}));
	});
	
	future.then(function() {
		var response;
		response = future.result;
		
		Test.require(response.returnValue);
		Test.require(response.value);
		future.result = Test.passed;
	});
	
	future.then(done);
};

AccountsTests.prototype.testDeleteAccount = function(done) {
	var future = this.createAccountWithCredentials("com.palm.foo");
	
	future.then(this, function() {
		var response = future.result;
		
		future.nest(this.lunaSend("palm://com.palm.service.accounts/", "deleteAccount", {
			accountId: response.result._id
		}, "com.palm.foo"));
	});
	
	future.then(function() {
		var r;
		r = future.result;
		future.result = Test.passed;
	});
	future.then(done);
};

AccountsTests.prototype.testReadCredentials = function (done) {
	var future = this.createAccountWithCredentials("com.palm.foo");

	future.then(this, function () {
		var response = future.result;
		future.nest(this.lunaSend("palm://com.palm.service.accounts", "readCredentials", {
			accountId: response.result._id,
			name: "common"
		}, "com.palm.foo"));
	});

	future.then(this, function () {
		try {
			Test.requireEqual(future.result.credentials.password, "secret");
			Test.requireEqual(future.result.credentials.authToken, "321891387984670935");
			return Test.passed;
		} catch (e) {
			return Test.failed;
		}
	});

	future.then(done);
};

AccountsTests.prototype.testWriteCredentials = function (done) {
	var future = this.createAccountWithCredentials("com.palm.foo");

	future.then(this, function () {
		var response = future.result;
		future.nest(this.lunaSend("palm://com.palm.service.accounts", "writeCredentials", {
			accountId: response.result._id,
			name: "common",
			credentials: {
				password: "sauce",
				authToken: "0987654321234567890"
			}
		}, "com.palm.foo"));
	});

	future.then(this, function () {
		Test.require(future.result.returnValue);
		return true;
	});

	future.then(this, function () {
		future.getResult();
		future.nest(this.lunaSend("palm://com.palm.service.accounts", "readCredentials", {
			accountId: this.accountId,
			name: "common"
		}, "com.palm.foo"));
	});

	future.then(this, function () {
		try {
			Test.requireEqual(future.result.credentials.password, "sauce");
			Test.requireEqual(future.result.credentials.authToken, "0987654321234567890");
			return Test.passed;
		} catch (e) {
			return Test.failed;
		}
	});

	future.then(done);
};

// should fail if neither password nor credentials are passed in
AccountsTests.prototype.testNegativeCreateAccountWithNothing = function(done) {
	
	var future = this.createAccountBase("com.palm.foo");
	
	future.then(function() {
		var result;
		
		try {
			result = future.result;
			future.result = Test.failed;
		} catch (e) {
			future.result = Test.passed;
		}
	});
	future.then(done);
};

AccountsTests.prototype.testNegativeCreateAccountWithBadCaller = function (done) {
	var future = this.createAccountWithCredentials("com.foo.bar");

	future.then(function () {
		var result;
		
		try {
			result = future.result;
			future.result = Test.failed;
		} catch (e) {
			Test.requireEqual(future.exception.errorText, "Permission denied.");
			future.result = Test.passed;
		}
	});
	future.then(done);
};

AccountsTests.prototype.testNegativeModifyAccountAliasWithBadCaller = function (done) {
	var newAlias = "new alias";
	
	var future = this.createAccountWithCredentials("com.palm.foo");
	future.then(this, function () {
		var response = future.result;
		future.nest(this.lunaSend("palm://com.palm.service.accounts/", "modifyAccount", {
			accountId: response.result._id,
			object: {
				alias: newAlias
			}
		}));
	});
	
	future.then(function() {
		try {
			future.getResult();
			future.result = Test.failed;
		} catch (e) {
			Test.requireEqual(future.exception.errorText, "Permission denied.");
			future.result = Test.passed;
		}
	});
	
	future.then(done);
};

AccountsTests.prototype.testNegativeGetAccountInfoWithBadCaller = function (done) {
	var future = this.createAccountWithCredentials("com.palm.foo");
	future.then(this, function () {
		var response = future.result;
		
		future.nest(this.lunaSend("palm://com.palm.service.accounts/", "getAccountInfo", {
			accountId: response.result._id
		}));
	});
	
	future.then(function() {
		try {
			future.getResult();
			future.result = Test.failed;
		} catch (e) {
			Test.requireEqual(future.exception.errorText, "Permission denied.");
			future.result = Test.passed;
		}
	});
	future.then(done);
};

AccountsTests.prototype.testNegativeDeleteAccountWithBadCaller = function (done) {
	var future = this.createAccountWithCredentials("com.palm.foo");
	
	future.then(this, function() {
		var response = future.result;
		
		future.nest(this.lunaSend("palm://com.palm.service.accounts/", "deleteAccount", {
			accountId: response.result._id
		}));
	});
	
	future.then(function() {
		try {
			future.getResult();
			future.result = Test.failed;
		} catch (e) {
			Test.requireEqual(future.exception.errorText, "Permission denied.");
			future.result = Test.passed;
		}
	});
	
	future.then(done);
};

AccountsTests.prototype.testNegativeReadCredentialsWithBadCaller = function (done) {
	var future = this.createAccountWithCredentials("com.palm.foo");

	future.then(this, function () {
		var response = future.result;
		future.nest(this.lunaSend("palm://com.palm.service.accounts", "readCredentials", {
			accountId: response.result._id,
			name: "common"
		}));
	});

	future.then(this, function () {
		try {
			future.getResult();
			future.result = Test.failed;
		} catch (e) {
			Test.requireEqual(future.exception.errorText, "Permission denied.");
			future.result = Test.passed;
		}
	});

	future.then(done);
};

AccountsTests.prototype.testNegativeWriteCredentialsWithBadCaller = function (done) {
	var future = this.createAccountWithCredentials("com.palm.foo");

	future.then(this, function () {
		var response = future.result;
		future.nest(this.lunaSend("palm://com.palm.service.accounts", "writeCredentials", {
			accountId: response.result._id,
			name: "common",
			credentials: {
				password: "sauce",
				authToken: "0987654321234567890"
			}
		}));
	});

	future.then(this, function () {
		try {
			future.getResult();
			future.result = Test.failed;
		} catch (e) {
			Test.requireEqual(future.exception.errorText, "Permission denied.");
			future.result = Test.passed;
		}
	});

	future.then(done);
};
