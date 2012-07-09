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

/*global KeyStore DB Assert _ PalmCall Foundations console PublicWhitelist Utils */
function ReadCredentialsCommandAssistant() {
	this.run = function(future) {
		var args = this.controller.args;
		var templates = this.controller.service.assistant.getTemplates();
		var message = this.controller.message;

		console.log("ReadCredentialsCommandAssistant: id=" + args.accountId);
		future.nest(DB.get([args.accountId]));

		future.then(this, function () {
			var result = future.result.results;
			
			Assert.require(result && result.length);
			var account = result[0];
			var template = _.detect(templates, function(t) {
				return t.templateId === account.templateId;
			});

			// Test for read permissions.  An exception will be thrown if it doesn't have permission
			Utils.hasPermission("readPermissions", template, message);

			future.nest(KeyStore.get(args.accountId, args.name));
		});
	};
}

function ReadCredentialsPublicCommandAssistant() {
	this.run = function(future) {
		var senderId = this.controller.message? this.controller.message.applicationID(): "";	

		if (PublicWhitelist.hasPermissionsById(senderId)){
			var assistant = new ReadCredentialsCommandAssistant();
			assistant.run.bind(this)(future);
		} else {
			var e = new Error("Permission denied.");
			e.errorCode = "";
			throw e;
		}
	};
}

function WriteCredentialsCommandAssistant() {
	this.run = function(future) {
		var args = this.controller.args;
		var account, annotated;
		var templates = this.controller.service.assistant.getTemplates();
		var message = this.controller.message;
		
		console.log("WriteCredentialsCommandAssistant: id=" + args.accountId);
		future.nest(DB.get([args.accountId]));
		
		future.then(function() {
			var result = future.result.results;
			
			Assert.require(result && result.length);
			account = _.extend(new Account(), result[0]);
			annotated = account.annotate(templates);

			// Test for write permissions.  An exception will be thrown if it doesn't have permission
			Utils.hasPermission("writePermissions", annotated, message);

			future.nest(KeyStore.put(args.accountId, args.name, args.credentials));
		});
		
		future.then(function() {
			var r;
			r = future.result;
			
			var params = {accountId: args.accountId};
			
			// Call onCredentialsChanged for the template (don't wait for a response)
			if (annotated.onCredentialsChanged)
				PalmCall.call(annotated.onCredentialsChanged, "", params);

			// Call onCredentialsChanged for enabled capabilities (don't wait for a response)
			annotated.capabilityProviders.forEach(function(cp) {
				if (cp.onCredentialsChanged && cp._id)
					PalmCall.call(cp.onCredentialsChanged, "", params);
			});
			
			future.result = {};
		});
	};
}

function HasCredentialsCommandAssistant() {
	this.run = function(future) {
		var args = this.controller.args;
		console.log("HasCredentialsCommandAssistant: id=" + args.accountId);
		future.nest(KeyStore.has(args.accountId));
	};
}
