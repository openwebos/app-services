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

/*global DB Assert _ Account Foundations KeyStore PalmCall console Utils */
// Notify capability providers that an account is being deleted
// 1. The account has its "beingDeleted" flag set already 
// 2. Notify each enabled capability provider using 'onEnabled' callback
// 3. Notify each capability provider using 'onDelete' callback
// 4. After all providers respond positively, delete all credentials
// 5. Delete the account itself

// Deleting an account is NOT the same as modifying it to remove
// all capability providers. Modifying it leaves the account around
// so that credentials can still be used.

function notifyAccountDeletedCommandAssistant() {
	
	this.run = function(future) {
		var args = this.controller.args;
		var serviceAssistant = this.controller.service.assistant;
		var message = this.controller.message;
		var account;
		var annotated;

		console.log("notifyAccountDeletedCommandAssistant: deleting account " + args.accountId);
		
		// It sounds tempting to set the sync state for the account to "DELETE" so that a dashboard appears
		// but that would not be good if the first attempt at deleting the account failed and subsequent
		// attempt to delete the account occurred hours later. 

		// Read existing account record
		future.nest(DB.get([args.accountId]));

		// Wait a little bit to allow the UI to respond to the account being deleted
		future.then(function() {
			var r = future.result;
			setTimeout(future.callback(this, function() {
			  future.result = r;
			}), 2000);
		});

		future.then(function() {
			var result = future.result.results;
						
			Assert.require(result && result.length);
			account = result[0];
			try {
				// Combine the account and the template
				annotated = Account.annotate.call(account, serviceAssistant.getTemplates());
			} catch (e) {
				console.log("ServiceAccounts::DELETE - Annotate failed = " + JSON.stringify(e));
			}

			// Get the array of currently enabled capabilities
			var onEnabledCps = annotated.capabilityProviders.filter(function(cp) {
				return (!!cp.onEnabled && cp._id);
			});
			
			// Call onEnabled=false for each enabled capability
			if (onEnabledCps.length) {
				for (var i=0, c=0, len=onEnabledCps.length; i < len; i++) {
					var cp = onEnabledCps[i];
					console.log("Calling " + cp.onEnabled + " cap=" + cp.id + " account=" + args.accountId + " enabled=false");
					PalmCall.call(cp.onEnabled, "", {
						accountId: args.accountId,
						capabilityProviderId: cp.id,
						enabled: false
					}).then(onEnabledCps[i], function(f) {
						var r = f.result;
						if (r.returnValue || r === true) {
							console.log("onEnabled for " + this.onEnabled + " was successful");
							// Only increment the count on success to help determine which transports might be failing
							if (++c == len)
								future.result = {};
						} 
						else 
							console.warn("onEnabled=false for " + this.onEnabled + " failed:" + JSON.stringify(r));
					});
				}
			}
			else {
				console.log("No capabilities were enabled for account " + args.accountId);
				future.result = {};
			}
		});
			
		future.then(function() {
			var r = future.result;

			// Get the template for this account
			var template = _.detect(serviceAssistant.getTemplates(), function(t) {
				return t.templateId === account.templateId;
			});
			
			if (template) {
				// Get the array of onDelete methods
				var onDeletes = _.compact(_.pluck(template.capabilityProviders, "onDelete"));

				if (onDeletes.length) {
					// Call onDelete for each capability
					for (var i=0, c=0, len=onDeletes.length; i < len; i++) {
						console.log("Calling onDelete " + onDeletes[i] + " account=" + args.accountId);
						PalmCall.call(onDeletes[i], "", {
							accountId: args.accountId
						}).then(onDeletes[i], function(f){
							var r = f.result;
							if (r.returnValue || r === true) {
								console.log("onDelete for " + this + " was successful");
								// Only increment the count on success to help determine which transports might be failing
								if (++c == len)
									future.result = {};
							}
							else
								console.log("onDelete for " + this + " failed:" + JSON.stringify(r));
						});
					}
				}
				else {
					console.log("onDelete: no onDelete methods for template " + account.templateId);
					future.result= {};
				}
			}
			else {
				console.log("onDelete: no template for " + account.templateId);
				future.result= {};
			}
		});
			
		future.then(function() {
			var r = future.result;
			// Determine if the account has credentials that need to be deleted
			future.nest(KeyStore.has(account._id));
		});
		
		future.then(function() {
			var r;
			r = future.result;
			
			// Delete the credentials if they exist
			if (r.value) {
				future.nest(KeyStore.del(account._id));
			} else {
				future.result = {};
			}
		});
		
		// Clear any existing status for this account		
		future.then(function() {
			var r = future.result;

			future.nest(PalmCall.call("palm://com.palm.tempdb/del", "", {
				"query":{
					"from": "com.palm.account.syncstate:1",
					"where":[{
						"prop":"accountId",
						"op":"=",
						"val": args.accountId
					}]}
			}));
		});
		
		// Delete the account
		future.then(function() {
			var r = future.result;

			future.nest(DB.del([args.accountId]));
		});
		
		future.then(function() {
			var r = future.result;

			future.result = {};
			console.log("notifyAccountDeletedCommandAssistant: deleted account " + args.accountId);
		});
	};
}
