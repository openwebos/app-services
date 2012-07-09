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

/*global _ DB ListAccountsCommandAssistant Utils Foundations KeyStore PalmCall console */

// Called by First Use to finish restoring accounts.  No credentials have been entered
// for any account at this point.  Accounts should not display "bad credentials" (401_UNAUTHORIZED)  
// dashboards; they should display "missing credentials" (CREDENTIALS_NOT_FOUND) instead.
function RestoreCompletedCommandAssistant() {
	
	function generateProvidersArray(capabilityProviders) {
		var cpArray = [];
		
		if (capabilityProviders && _.isArray(capabilityProviders)) {
			cpArray = capabilityProviders.map(function(cap) {
				return {
					id: cap.id,
					capability: cap.capability
				};
			});
		}
		return cpArray;
	}

	this.run = function (future) {
		var serviceAssistant = this.controller.service.assistant,
			templates = serviceAssistant.getTemplates(),
			toCall = [],
			accounts;
		
		console.log("RestoreCompletedCommandAssistant");
		
		// Get an array of all the accounts (even the ones being deleted)
		future.nest(DB.find({from: Account.kind}));

		// call 'onCapabilitiesChanged' and 'onEnabled' on each capabilityProvider
		// for EVERY account that has them.
		// This will kick off sync jobs for each transport, or do nothing for transports that
		// do not sync.  Transports that encounter missing credentials (and they will because
		// no credentials have been entered by the user yet) shouldn't do anything
		future.then(function () {
			var i, j, account, cp, template;
			
			function templateIdMatch(t) {
				return t.templateId === this.templateId;
			}
			
			accounts = future.result.results;
			
			i = accounts.length;
			while (i--) {
				account = _.extend(new Account(), accounts[i]);
				
				// Skip accounts being deleted
				if (account.beingDeleted)
					continue;
				
				template = _.detect(templates, templateIdMatch, account);
				if (!template)
					continue;
				account = account.annotate([template]);
					
				if (account.onCapabilitiesChanged) {
					toCall.push({
						address: account.onCapabilitiesChanged,
						params: {
							accountId: account._id,
							capabilityProviders: Utils.getProviderStates(template, account.capabilityProviders)
						}
					});
				}
				j = account.capabilityProviders.length;
				while (j--) {
					cp = account.capabilityProviders[j];
					if (cp.onEnabled) {
						toCall.push({
							address: cp.onEnabled,
							params: {
								accountId: account._id,
								capabilityProviderId: cp.id,
								enabled: true
							}
						});
					}
				}
				
				// For the profile template, make sure all capabilities are enabled
				if (account.templateId === "com.palm.palmprofile") {
					var cpArray = generateProvidersArray(template.capabilityProviders);
					var toSave = {
						_id: account._id,
						capabilityProviders: cpArray
					};
					DB.merge([toSave]);
				}
			}
			
			// We should wait for the results so the device doesn't reset after
			// OTA until the services had time to setup their sync activities
			future.nest(Foundations.Control.mapReduce({
				map: function (call) {
					return PalmCall.call(call.address, "", call.params);
				},
				reduce: function (results) {
					var f = new Foundations.Control.Future(),
						i = results.length,
						result;
					while (i--) {
						result = results[i];
						if (result.exception) {
							console.warn("WARNING: Transport 'onCapabilitiesChanged/onEnabled' service call failed. call=" + result.item + " error=" + result.exception);
						}
					}					
					return f.immediate({}); // we could return 'results' but the previous implementation didn't and we don't want to break anything
				}
			}, toCall));
			
			future.then(function() {
				var result = future.result;
				
				// Cleanup the multiple Telephony accounts and accounts that are marked as "being deleted"
				var accountsToDelete = accounts.filter(function(a) {
					// Delete telephony accounts where the sync flag is true (sync flag should be false)
					if (a.templateId === "com.palm.telephony" && a._sync) {
						console.log("Deleting unused telephony account " + a._id);
						return true;
					}
					// Delete accounts that are marked as "being deleted"
					if (a.beingDeleted) {
						console.log("Deleting " + a.templateId + " account beingDeleted " + a._id);
						return true;
					}
					return false;
				});
				
				for (var i=0; i < accountsToDelete.length; i++)
					DB.del([accountsToDelete[i]._id]);
				
				// Get the device name
				future.nest(PalmCall.call("palm://com.palm.systemservice/getPreferences", "", {"keys": ["deviceName"]}));
			});
			// Save the device name to the Profile account so that it can be used in account lists
			future.then(function() {
				var deviceName = future.result.deviceName;

				deviceName = deviceName || "";
				PalmCall.call("palm://com.palm.db/merge", "", {
					"props":{"deviceName":deviceName},
					"query":{
						"from":"com.palm.account:1",
						"where":[
							{"prop":"templateId","op":"=","val":"com.palm.palmprofile"},
							{"prop":"beingDeleted","op":"=","val":false}
						]
					}
				});
				
				return {};
			});
		});
	};
}
