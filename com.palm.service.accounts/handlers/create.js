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

/*global _ Assert console DB Account Foundations PalmCall KeyStore Utils */

var CreateCommandAssistant = (function() {
	
	// Given an account, a password, some optional config, and a list of validators,
	// call all validators and aggregate the results.  If any fail, the whole thing fails.
	// returns a future for this subroutine
	function checkPassword(account, password, validators, config) {
		var future;

		function validate(v) {
			var addr, method;
			var pos = v.lastIndexOf('/');
			if (pos == -1) {
				throw new Error("invalid validator: " + v);
			}

			addr = v.substring(0, pos+1);
			method = v.substring(pos+1);

			return PalmCall.call(addr, method, {
				username: account.username,
				password: password,
				templateId: account.templateId,
				config: config
			});
		}

		function aggregate(results) {
			var future = new Foundations.Control.Future();
			var acc = {};
			var creds = {};

			results.forEach(function(r) {
				if (r.result) {
					_.extend(acc, r.result);
					_.extend(creds, r.result.credentials);
				} else {
					console.warn("validator returned error: " + r.exception);
				}
			});
			acc.credentials = creds;
			future.result = acc;

			return future;
		}

		future = Foundations.Control.mapReduce({
			map: validate,
			reduce: aggregate
		}, validators);

		return future;
	}
	
	// Given a template with a subset of capabilities selected,
	// create an account record referencing that template+capabilities.
	// Also optionally store a credentials object if given one.
	// Finally, kick off syncs for each selected capability, optionally
	// passing along the "config" object.
	//
	// The exact API to start syncing should be standardized for all
	// transports and template kinds.
	//
	// This method assumes that the credentials have already been
	// validated, so the account should unconditionally be created.
	function run(future) {
		var template;
		var selectedCapabilities = [];
		var i, j;
		var cap;
		var found = false;
		var account;
		var args = this.controller.args;
		var message = this.controller.message;
		var credentials = args.credentials;
		var config = args.config;

		// @@ temporary arg validator... should be done using json schema
		Assert.require(args.templateId, "missing templateId");
		Assert.require(args.capabilityProviders && _.isArray(args.capabilityProviders), "missing capabilityProviders");
		Assert.require(args.username, "missing username");
		Assert.require(!(args.password && args.credentials), "can't have both password and credentials");

		// match up a requested template ID to an existing one
		template = _.detect(this.controller.service.assistant.getTemplates(), function(t) {
			return (t.templateId === args.templateId);
		});
		if (!template) {
			throw new Error("no account template for id=" + args.templateId);
		}
		
		if (template.validator) {
			Assert.require(typeof args.password === "string" || typeof args.credentials === "object");
		}

		// Test for write permissions.  An exception will be thrown if it doesn't have permission
		Utils.hasPermission("writePermissions", template, message);

		// cross-reference capability IDs and select the intersection
		for (var i = 0; i < args.capabilityProviders.length; i++) {
			cap = args.capabilityProviders[i];
			
			// don't select any capability more than once
			if (_.pluck(selectedCapabilities, "id").indexOf(cap.id) !== -1) {
				continue;
			}

			for (var j = 0; j < template.capabilityProviders.length; j++) {
				if (cap.id === template.capabilityProviders[j].id) {
					Assert.require(template.capabilityProviders[j].capability, "capability missing!");
					selectedCapabilities.push(template.capabilityProviders[j]);
					found = true;
					break;
				}
			}
			if (!found) {
				console.log("Requested capability provider: " + cap.id + " not found, ignoring");
			} else {
				found = false;
			}
		}

		// if a record with this username and template id exists...
		future.nest(DB.find({
			from: Account.kind,
			where: [
				{
					prop: "beingDeleted",
					op: "=",
					val: false
				},
				{
					prop: "templateId",
					op: "=",
					val: template.templateId
				},
				{
					prop: "username",
					op: "=",
					val: args.username
				}
			]
		}));

		// ... error out.  Else, save.
		future.then(function() {
			var validators;
			var arr = future.result.results;
			if (arr && arr.length > 0) {
				future.setException({
					"message": "Unable to create a duplicate account",
					"errorCode": "DUPLICATE_ACCOUNT"
				});
				return;
			}

			account = new Account();
			account.templateId = template.templateId;
			account.username = args.username;
			account.alias = args.alias;
			account.beingDeleted = false;
			account.capabilityProviders = selectedCapabilities.map(function(c) {
				return { id: c.id, capability: c.capability };
			});
			if (typeof args._sync !== "undefined") {
				account._sync = args._sync;
			}

			// if a password is given, validate it against the default validator & any selected overriding validators
			if (args.password) {
				validators = [template.validator];
				selectedCapabilities.forEach(function(c) {
					if (c.validator) {
						validators.push(c.validator);
					}
				});

				future.nest(checkPassword(account, args.password, validators, config));
			} else {
				future.result = undefined;
			}
		});

		future.then(function() {
			var result = future.result;
			if (result) {

				credentials = result.credentials;
				if (result.config) {
					config = result.config;
				}
				if (result.templateId) {
					// XXX support changing templateId
				}
			}

			future.nest(DB.put([account]));
		});

		// save the initial credentials, if given
		future.then(function() {
			var putResponse = future.result.results;
			account._id = putResponse[0].id;

			future.nest(Utils.saveCredentials(account._id, credentials));
		});

		future.then(function() {
			var r = future.result;

			// Kickoff account creation in the background (don't wait for a response)
			PalmCall.call("palm://com.palm.service.accounts/notifyAccountCreated", "", {"accountId":account._id, "config": config});

			// Don't wait for responses.  Return the result now
			return { result: account };
		});
	}
	
	return function() {
		this.run = run;
	};
	
})();
