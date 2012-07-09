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

/*global DB _ Account console PublicWhitelist */
// This service method is intended for transports who need to query for accounts.
// Apps should use the 'accounts.ui' library for both a subscribed utility method
// and an accounts list widget.
function ListAccountsCommandAssistant() {
	
	this.run = function(future) {
		var args = this.controller.args;
		var serviceAssistant = this.controller.service.assistant;
		var senderId = this.controller.message? this.controller.message.applicationID(): "";	
		
		// what params?
		// list by alpha, optionally filter by templateId OR capability
		
		// also fetch template data and weave template data into the account records
		
		var where;
		
		function whereEquals(prop, val) {
			return {
				prop: prop,
				op: "=",
				val: val
			};
		}
		
		if (args.templateId) {
			where = [whereEquals("templateId", args.templateId)];
			console.log("ListAccounts: templateId=" + args.templateId + " by " + senderId);
		} else if (args.capability) {
			where = [whereEquals("capabilityProviders.capability", args.capability)];
			console.log("ListAccounts: capability=" + args.capability + " by " + senderId);
		} else {
			where = [];
			console.log("ListAccounts: no template or capability" + " by " + senderId);
		}
		where.push(whereEquals("beingDeleted", false));
		
		future.nest(DB.find({
			from: Account.kind,
			where: where
		}));
		
		future.then(function() {
			var results = future.result.results;
			var templates = serviceAssistant.getTemplates();
			
			results = results.map(function(o) {
				return _.extend(new Account(), o);
			});
			
			// Handle errors
			var annotatedResults = [];
			for (var i = 0; i < results.length; i++) {
				try {
					annotatedResults.push(results[i].annotate(templates));
				} catch (e) {
					console.log("ListAccounts: Skipping account because template " + results[i].templateId + " is missing");
				}
			}
			
			future.result = {
				results: annotatedResults
			};
		});
		
	};
	
}

function ListAccountsPublicCommandAssistant() {
	
	this.run = function(future) {
		var args = this.controller.args;
		var senderId = this.controller.message? this.controller.message.applicationID(): "";	

		if (PublicWhitelist.hasPermissionsByIdAndCapability(senderId, args.capability)){
			var assistant = new ListAccountsCommandAssistant();
			assistant.run.bind(this)(future);
		} else {
			var e = new Error("Permission denied.");
			e.errorCode = "";
			throw e;
		}	
	};
	
}

