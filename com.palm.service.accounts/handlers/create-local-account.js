// @@@LICENSE
//
//      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

function CreateLocalAccountAssistant() {}

CreateLocalAccountAssistant.prototype.run = function(future) {
	this.args = this.controller.args;
	//create a local account if firstuse is skipped.
	future.now(function() {
		return PalmCall.call("palm://com.palm.service.accounts", "listAccounts", {"templateId": "com.palm.palmprofile"});
	});
	
	future.then(this, function() {
		var results = future.result.results;
		if(results.length === 0) {
			this.createLocalAccount("Open webOS");
			return {returnValue:true, accountCreated:true};
		}
		return {returnValue:true, accountCreated:false};
		
	});
	
};

CreateLocalAccountAssistant.prototype.createLocalAccount =  function (username) {
    var account = {
            "templateId": "com.palm.palmprofile",
            "capabilityProviders": [
                    {"id": "com.palm.palmprofile.contacts"},
                    {"id": "com.palm.palmprofile.calendar"},
                    {"id": "com.palm.palmprofile.tasks"},
                    {"id": "com.palm.palmprofile.memos"},
                    {"id": "com.palm.palmprofile.sms"},
                    {"id": "com.palm.palmprofile.voice"},
                    {"id": "com.palm.palmprofile.localfilestore"}
            ],
    "username": username,
    "credentials": {}
    };
    console.log("Creating a palmprofile account with username:" + username);
   
    var acctFuture = PalmCall.call("palm://com.palm.service.accounts/", "createAccount", account);
    acctFuture.then(this, function(){
			    if (acctFuture.exception){
			            console.log("----------- local acct creation failed ------------" + JSON.stringify(acctFuture.exception));
			    }else{
			            if (acctFuture.result) {
			                    console.log("----------- local acct created successfully ------------" + JSON.stringify(acctFuture.result));
			                    this.createAccountCreatedFlag();
			            }
			    }
    });    
};

//Creation flag is used to create a default palmprofile account if firstuse is skipped
CreateLocalAccountAssistant.prototype.createAccountCreatedFlag =  function () {
		var node_fs = require('fs');
		var pathLib = require('path');
        var accountCreationFlagPath = "/var/luna/preferences/first-use-profile-created";
        if (pathLib.existsSync(accountCreationFlagPath)){
                console.log(accountCreationFlagPath + " already exists");
        } else {
                //Touches the file
                node_fs.openSync(accountCreationFlagPath, "w");
        }
};
