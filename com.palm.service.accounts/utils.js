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

/*global console Foundations _ KeyStore PublicWhitelist */
var node_fs = require('fs');

var Utils = {
	getProviderStates: function(template, enabledCapabilities) {
		function idMatch(s) {
			return s.id === this.id;
		}

		return template.capabilityProviders.map(function(cp) {
			return {
				id: cp.id,
				enabled: enabledCapabilities.some(idMatch, cp)
			};
		});
	},

	saveCredentials: function(accountId, credentials) {
		var future;
		var keys;
		if (credentials) {
			// Use MapReduce to save multiple passed-in credentials (concurrently, since they're independent)
			// and wait for all of the saves to finish

			keys = _.keys(credentials);
			future = Foundations.Control.mapReduce({
				map: function(v) {
					return KeyStore.put(accountId, v, credentials[v]);
				}
			}, keys);

		} else {
			future = new Foundations.Control.Future().immediate(undefined);
		}
		return future;
	},
	
	hasPermission: function(permissions, account, message) {
		var callingAppId = message? message.applicationID() || message.senderServiceName() : "";
		callingAppId = PublicWhitelist._stripProcessNumFromId(callingAppId);

		// The accounts app/service always has permission
		if (callingAppId === "com.palm.app.accounts" || callingAppId === "com.palm.service.accounts")
			return true;
			
		// Check against the permissions specified in the templates
		var p = account[permissions];
		if (p && p.some(function (permit) {return new RegExp(Utils.createRegexFromGlob(permit)).test(callingAppId);})) 
			return true;
		
		// Permission denied!
		var errMsg = "Permission denied! " + callingAppId + " is not specified in template " + account.templateId + " " + permissions;
		console.log(errMsg);

		// Throw an exception
		var e = new Error(errMsg);
		e.errorCode = "";
		throw e;
		return false;
	},
	
	createRegexFromGlob: function(glob) {
	    var out = "^";
	    for (var i = 0; i < glob.length; ++i)
	    {
	        var c = glob.charAt(i);
	        switch (c) {
		        case '*': out += ".*"; break;
		        case '?': out += '.'; break;
		        case '.': out += "\\."; break;
		        case '\\': out += "\\\\"; break;
		        default: out += c;
	        }
	    }
	    out += '$';
	    return out;
	}
	
};
