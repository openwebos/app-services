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

function stayRunningCommandAssistant() {
	
	this.run = function(future) {
		var args = this.controller.args;
		var timeout = 30; // Default 30 second timeout
		
		if (args && args.seconds)
			timeout = (args.seconds > 3600)? 3600:args.seconds;
		
		console.log("Accounts service running for " + timeout + " seconds ...");
		
		setTimeout(future.callback(this, function() {
		  future.result = "passed";
		}), timeout*1000);
		
		future.then(function(future) {
			// Continue deleting any accounts where deletion may have been interrupted
			resumeAccountDeletes();

			console.log("Accounts service stopping after running for " + timeout + " seconds (this is normal)");
			return {};
		});
	};
	
	
	function resumeAccountDeletes() {
		var accounts;
		
		// find all account records with beingDeleted=true
		// for each record, abort if retries >= 5
		// else increment retries, commit the new record,
		// and call 'removeAccount' through the normal bus approach
		// (this should prolong the timeout to 3600 seconds while deletes are pending)

		console.log("resumeAccountDeletes: Any to delete?");
		
		var future = DB.find({
			from: Account.kind,
			where: [{
				prop: "beingDeleted",
				op: "=",
				val: true
			}]
		});
		
		future.then(function() {
			var r = future.result;
			
			accounts = r.results.filter(function(a) {
				return !a.retries || a.retries < 5;
			});
			
			if (accounts.length === 0) {
				console.log("resumeAccountDeletes: No accounts to delete");
				return;
			}
			
			console.log("resumeAccountDeletes: Num accounts=" + accounts.length);
			future.nest(DB.merge(accounts.map(function(acct) {
				acct.retries = (acct.retries) ? acct.retries + 1 : 1;
				return {
					_id: acct._id,
					retries: acct.retries
				};
			})));
		});
		
		future.then(function() {
			var r;
			r = future.result;
			
			function deleteAccount(acct) {
				console.log("resumeAccountDeletes: resuming delete of account, id=" + acct._id + ", this is retry #" + acct.retries);
				return PalmCall.call("palm://com.palm.service.accounts/", "notifyAccountDeleted", {
					accountId: acct._id
				});
			}
			
			future.nest(Foundations.Control.mapReduce({
				map: deleteAccount
			}, accounts));
		});
		
		future.then(function() {
			var r;
			r = future.result;
			
			console.log("resumeAccountDeletes: done");
		});
	}
}
