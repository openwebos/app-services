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

/*global _ DB Account Assert PalmCall KeyStore Foundations Utils console */
var ModifyCommandAssistant = (function() {
	
	function TransportError(failed) {
		this.failed = failed;
	}
	
	function idMatch(other) {
		return other.id === this.id;
	}
	
	function getDiffs(memo, item) {
		if (memo.other.indexOf(item) === -1) {
			memo.results.push(item);
		}
		return memo;
	}
	
	function diffLists(oldList, newList) {
		return {
			add: newList.reduce(getDiffs, { other: oldList, results: []}).results,
			remove: oldList.reduce(getDiffs, { other: newList, results: []}).results
		};
	}
	
	function call(transport) {
		return PalmCall.call(transport.address, "", transport.params);
	}
	
	function handleResults(results) {
		var f = new Foundations.Control.Future();
		var failed = results.filter(function(result) {
			return result.exception;
		});
		
		if (failed.length > 0) {
			f.exception = new TransportError(failed);
		} else {
			f.result = {};
		}
		
		return f;
	}
	
	function notifyTransports(transports) {
		return Foundations.Control.mapReduce({
			map: call,
			reduce: handleResults
		}, transports);
	}
	
	function generateProvidersArray(template, providerIds) {
		var cpArray;
		var newCPs = providerIds;
		
		if (template && newCPs && _.isArray(newCPs)) {
			cpArray = newCPs.map(function(newCp) {
				var cp = _.detect(template.capabilityProviders, idMatch, newCp);
				return {
					id: cp.id,
					capability: cp.capability
				};
			});
		}
		
		return cpArray;
	}
	
	function run(future) {
		var args = this.controller.args;
		var serviceAssistant = this.controller.service.assistant;
		var account;
		var newAccount = args.object;
		var templates = serviceAssistant.getTemplates();
		var template;
		var diff;
		var savedException;
		var hadCredentials;
		var message = this.controller.message;

		console.log("ModifyCommandAssistant: id=" + args.accountId);

		// update the status in tempdb
		if (newAccount.credentials) {
			// Remove all status for the account
			// This should make credential error dashboards disappear
			future.nest(PalmCall.call("palm://com.palm.tempdb/del", "", {
				"query":{
					"from": "com.palm.account.syncstate:1",
					"where":[{
						"prop":"accountId",
						"op":"=",
						"val": args.accountId
					}]}
			}));
		} else {
			future.result = {};
		}
		
		// read existing record
		future.then(function() {
			future.nest(DB.get([args.accountId]));
		});
		
		// determine if credentials already exist
		// only when restoring an account will credentials be given and not already exist
		// need to call 'onCredentialsChanged' methods if changing,
		// else call 'onCapabilitiesChanged'/'onEnabled' for each capability restored
		future.then(function() {
			var result = future.result.results;
			
			Assert.require(result && result.length);
			account = result[0];
			
			template = _.detect(templates, function(t) {
				return t.templateId === account.templateId;
			});
			
			// Test for write permissions.  An exception will be thrown if it doesn't have permission
			Utils.hasPermission("writePermissions", template, message);
			
			future.nest(KeyStore.has(account._id));
		});
		
		// delete credentials (if given and previously existed)
		future.then(function() {
			var r;
			r = future.result;
			
			hadCredentials = r.value;
			
			if (hadCredentials && newAccount.credentials) {
				future.nest(KeyStore.del(account._id));
			} else {
				future.result = undefined;
			}
		});
		
		// write new credentials (if given)
		future.then(function() {
			var r;
			r = future.result;
			future.nest(Utils.saveCredentials(account._id, newAccount.credentials));
		});
		
		// save the new record before notifying transports of the change
		future.then(function() {
			var r;
			r = future.result;
			var toSave;
			var cpArray;
			var reject;
			
			if (newAccount.capabilityProviders) {
				diff = diffLists(
					_.pluck(account.capabilityProviders, "id"),
					_.pluck(newAccount.capabilityProviders, "id")
				);
			} else {
				diff = {
					add: [],
					remove: []
				};
			}
			
			// reject any request to disable "alwaysOn" capabilties
			if (template && diff.remove.length > 0) {
				reject = diff.remove.some(function(id) {
					var match = _.detect(template.capabilityProviders, idMatch, { id: id });
					return match && match.alwaysOn;
				});
				if (reject) {
					future.setException(Foundations.Err.create("400_BAD_REQUEST", "can't disable 'alwaysOn' capabilities"));
					return;
				}
			}
			
			cpArray = generateProvidersArray(template, newAccount.capabilityProviders);
			
			toSave = {
				_id: args.accountId,
				username: newAccount.username,
				alias: newAccount.alias,
				capabilityProviders: cpArray
			};
			
			future.nest(DB.merge([toSave]));
		});
		
		// determine which capabilities were enabled or disabled
		// call 'onCapabilitiesChanged' if present
		future.then(function() {
			var providerStates;
			var r;
			r = future.result;
			
			if (diff.add.length + diff.remove.length === 0) {
				future.result = {};
				return;
			}
			
			if (template && template.onCapabilitiesChanged) {
				providerStates = Utils.getProviderStates(template, newAccount.capabilityProviders);
				future.nest(PalmCall.call(template.onCapabilitiesChanged, "", {
					accountId: args.accountId,
					capabilityProviders: providerStates
				}));

			} else {
				future.result = {};
			}
		});
		
		// call 'onCredentialsChanged' for each affected and interested transport
		// (and the top-level callback if present)
		future.then(function() {
			var r;
			var notifying;
			var addedIds;
			var providers;
			var transports;
			var params;
			var method = "onCredentialsChanged";
			r = future.result;
			
			if (newAccount.credentials) {
				if (!args.suppressNotifications) {
					// modifying existing credentials; send 'onCredentialsChanged' to
					// enabled transports that are also NOT being disabled or enabled.
					// also send the message to the top level handler, if present
					
					params = {
						accountId: args.accountId
					};
					
					addedIds = _.pluck(diff.add, "id");
					
					transports = [];
					if (template && template[method]) {
						transports.push({
							address: template[method],
							params: params
						});
					}
					
					providers = newAccount.capabilityProviders || account.capabilityProviders;
					providers.forEach(function(cp) {
						if (template && addedIds.indexOf(cp.id) === -1) {
							var provider = _.detect(template.capabilityProviders, idMatch, cp);
							if (provider && provider[method]) {
								transports.push({
									address: provider[method],
									params: params,
									capabilityProviderId: provider.id
								});
							}
						}
					});
					
					future.nest(notifyTransports(transports));
					notifying = true;
				} //else {
					// restore case; First Use should call 'restoreCompleted' later.
					// accounts.ui will handle that by calling 'onEnabled' on each
					// enabled capability
				//}
			}
			if (!notifying) {
				future.result = undefined;
			}
		});
		
		// call 'onEnabled'=true/false for each affected and interested transport
		future.then(function() {
			var transports = [];
			
			var r;
			r = future.result;
			
			if (diff.add.length + diff.remove.length === 0) {
				future.result = {};
				return;
			}
			
			// notify transports of newly created and deleted capabilities
			
			// turn the add/remove lists of ids into a single list of
			// bus address/method/params tuples for MapReduce to eat.
			
			function mapAddRemoveToBusCalls(prop, params) {
				return function(id) {
					if(template) {
						var provider = _.detect(template.capabilityProviders, idMatch, { id: id });
						if (provider && provider[prop]) {
							params.capabilityProviderId = id;
							transports.push({
								address: provider[prop],
								params: params,
								capabilityProviderId: provider.id
							});
						}
					}
				};
			}
			
			diff.add.forEach(mapAddRemoveToBusCalls("onEnabled", {
				accountId: args.accountId,
				enabled: true
			}));
			diff.remove.forEach(mapAddRemoveToBusCalls("onEnabled", {
				accountId: args.accountId,
				enabled: false
			}));
			
			future.nest(notifyTransports(transports));
		});
		
		// roll back upon encountering any 'onEnabled' error
		// TODO: consider making this the error handler for the whole sequence,
		// or at least for the transport callback portions
		future.then(function() {
			var r;
			var failed;
			var failedAddrs;
			var msg;
			var newCps;
			var cpIds;
			var toSave;
			var err;
			
			try {
				r = future.result;
			} catch (e) {
				if (e instanceof TransportError) {
					// console.log("got a transport error!  need to revert modify changes to affected capabilities");

					failed = e.failed;
					
					err = failed[0].exception;

					// revert failed ids by inverting their presence in the provider array.
					// regenerate the corresponding capabilities and merge the new array.
					cpIds = _.pluck(newAccount.capabilityProviders, "id");
					failed.forEach(function(bad) {
						var idx = cpIds.indexOf(bad.item.capabilityProviderId);
						if (idx !== -1) {
							cpIds.splice(idx, 1);
						} else {
							cpIds.push(bad.item.capabilityProviderId);
						}
					});
					
					newCps = cpIds.map(function(id) {
						return {
							id: id
						};
					});

					newCps = generateProvidersArray(template, newCps);
					
					// console.log("reverted array =\n" + JSON.stringify(newCps, undefined, 2));
					toSave = {
						_id: args.accountId,
						capabilityProviders: newCps
					};

					future.nest(DB.merge([toSave]));
					
					failedAddrs = failed.map(function(result) {
						return result.item.address;
					});

					msg = "Failed to notify transports: " + failedAddrs.join(",");
					savedException = Foundations.Err.create("TRANSPORT_FAILURE", msg, err);
					
					return;
				} else {
					throw e;
				}
			}
			
			future.result = {};
		});
		
		future.then(function() {
			var r;
			r = future.result;
			
			if (savedException) {
				future.exception = savedException;
			} else {
				future.result = {};
			}
		});
	}
	
	return function() {
		this.run = run;
	};
	
})();
