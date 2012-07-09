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
// Notify capability providers that an account has been created
// 1. Notify each capability provider using 'onCreate' callback
// 2. Notify each enabled capability provider using 'onEnabled' callback


function notifyAccountCreatedCommandAssistant() {
	
	this.run = function(future) {
		var args = this.controller.args;
		var serviceAssistant = this.controller.service.assistant;
		var message = this.controller.message;
		var annotated;

		console.log("notifyAccountCreated: letting transports know of new account " + args.accountId);
		
		// Read existing account record
		future.nest(DB.get([args.accountId]));

		// Wait a little bit to allow the UI to respond to the account that has just been created
		future.then(function() {
			var r = future.result;
			setTimeout(future.callback(this, function() {
			  future.result = r;
			}), 1000);
		});

		future.then(function() {
			var result = future.result.results;
						
			Assert.require(result && result.length);
			try {
				// Combine the account and the template
				annotated = Account.annotate.call(result[0], serviceAssistant.getTemplates());
			} catch (e) {
				console.log("ServiceAccounts::CREATE - Annotate failed = " + JSON.stringify(e));
			}

			// Get the array of onCreate methods
			var onCreates = _.compact(_.pluck(annotated.capabilityProviders, "onCreate"));

			if (onCreates.length) {
				// Call onCreate for each capability
				for (var i=0, c=0, len=onCreates.length; i < len; i++) {
					console.log("Calling onCreate " + onCreates[i] + " account=" + args.accountId);
					PalmCall.call(onCreates[i], "", {
						accountId: args.accountId,
						config: args.config
					}).then(onCreates[i], function(f){
						var r = f.result;
						if (r.returnValue || r === true) {
							console.log("onCreate for " + this + " was successful");
							// Only increment the count on success to help determine which transports might be failing
							if (++c == len)
								future.result = {};
						}
						else
							console.log("onCreate for " + this + " failed:" + JSON.stringify(r));
					});
				}
			}
			else {
				console.log("onCreate: no onCreate methods for " + annotated.templateId);
				future.result= {};
			}
		});
			
		future.then(function() {
			var r = future.result;

			// Get the array of enabled capabilities
			var onEnabledCps = annotated.capabilityProviders.filter(function(cp) {
				return (!!cp.onEnabled && cp._id);
			});
			
			// Call onEnabled=true for each enabled capability
			if (onEnabledCps.length) {
				for (var i=0, c=0, len=onEnabledCps.length; i < len; i++) {
					var cp = onEnabledCps[i];
					console.log("Calling " + cp.onEnabled + " cap=" + cp.id + " account=" + args.accountId + " enabled=true");
					PalmCall.call(cp.onEnabled, "", {
						accountId: args.accountId,
						capabilityProviderId: cp.id,
						enabled: true
					}).then(onEnabledCps[i], function(f) {
						var r = f.result;
						if (r.returnValue || r === true) {
							console.log("onEnabled for " + this.onEnabled + " was successful");
							// Only increment the count on success to help determine which transports might be failing
							if (++c == len)
								future.result = {};
						} 
						else 
							console.warn("onEnabled=true for " + this.onEnabled + " failed:" + JSON.stringify(r));
					});
				}
			}
			else {
				//console.log("No onEnabled handlers for template " + annotated.templateId);
				future.result = {};
			}
		});
		
		future.then(function() {
			var r = future.result;
			// Let the main template provide know which capabilities have been enabled
			if (annotated.onCapabilitiesChanged) {
				console.log("Calling onCapabilitiesChanged: " + annotated.onCapabilitiesChanged);
				// Use the template for the full list of supported capabilities
				var template = _.detect(serviceAssistant.getTemplates(), function(t) {
					return (t.templateId === annotated.templateId);
				});
				
				var providerStates = Utils.getProviderStates(template, annotated.capabilityProviders);
				future.nest(PalmCall.call(annotated.onCapabilitiesChanged, "", {accountId: annotated._id, capabilityProviders: providerStates}));
			}
			else {
				//console.log("template doesn't have onCapabilitiesChanged");
				future.result = {}
			}
		});

		future.then(function() {
			var r = future.result;

			future.result = {};
			console.log("notifyAccountCreated: transports have been notified " + args.accountId);
		});
	};
}
