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

/*global include MojoLoader _ console Test */
include("mojoloader.js");

MojoLoader.override(
	{ name: "foundations", trunk: true },
	{ name: "foundations.json", trunk: true },
	{ name: "mojotest", trunk: true }
);

try {
	var libraries = MojoLoader.require(
		{ name: "foundations", trunk: true }
	);
	var Future = libraries.foundations.Control.Future;
	var PalmCall = libraries.foundations.Comms.PalmCall;
	var DB = libraries.foundations.Data.DB;
} catch (Error) {
	console.error(Error);
}

function Generator() {
}

Generator.prototype.createAccount = function(extraInfo, templateIndex, done) {
	
	// create an account using the first template found
	var template;
	var future = PalmCall.call("palm://com.palm.service.accounts/", "listAccountTemplates", {});
	future.then(function() {
		var result = future.result;
		var params = {};
		
		template = result.results[templateIndex];
		
		params = {
			templateId: template.templateId,
			capabilities: [ { id: template.capabilities[0].id } ]  // create an account with just the first capability enabled
		};
		
		_.extend(params, extraInfo);
		
		future.nest(PalmCall.call("palm://com.palm.service.accounts/", "createAccount", params));
		
	});
	
	future.then(function() {
		var result;
		try {
			result = future.result;
			console.log("createAccount returned: " + JSON.stringify(result));
			Test.require(result.returnValue);
			Test.require(result.templateId === template.templateId);
			Test.require(result.capabilities.length === 1);
			Test.require(result.capabilities[0].id === template.capabilities[0].id);
			Test.require(typeof result._id !== "undefined");
			
			done(Test.passed);
		} catch (e) {
			done(e.message);
		}
	});

};

var g = new Generator();
g.createAccount({
	username: "testuser@domain.com",
	credentials: {
		common: {
			password: "secret",
			authToken: "321891387984670935"
		},
		imap: {
			authToken: "FBAEDBECDA9A98D675DC7E90936F310"
		}
	}
}, 0, function(status) {
	console.log("done. " + status);
	
	g.createAccount({
		username: "test2@gmail.com"
	}, 1, function() {
		console.log("done. " + status);
	});
});
