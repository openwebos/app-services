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
// Delete an account.
// 1. Set the "beingDeleted' flag on the account so that it becomes invisible
// 2. Kick-off a background deletion of the account

// Deleting an account is NOT the same as modifying it to remove
// all capability providers. Modifying it leaves the account around
// so that credentials can still be used.

function DeleteCommandAssistant() {
	
	this.run = function(future) {
		var args = this.controller.args;
		var serviceAssistant = this.controller.service.assistant;
		var message = this.controller.message;
		var account;
		var annotated;
		
		console.log("DeleteCommandAssistant: deleting account " + args.accountId);
		// read existing record
		future.nest(DB.get([args.accountId]));

		future.then(function() {
			var result = future.result.results;
			
			if (!result || !result.length) {
				console.log("Unable to find account " + args.accountId);
				throw new Error ("Unable to find account");
			}
			account = _.extend(new Account(), result[0]);
			if (account._del) {
				console.log("Account has been deleted " + args.accountId);
				throw new Error ("Account has been deleted");
			}
			if (account.beingDeleted) {
				console.log("Account is being deleted "  + args.accountId);
				throw new Error ("Account is being deleted");
			}
			annotated = account.annotate(serviceAssistant.getTemplates());

			// Test for write permissions.  An exception will be thrown if it doesn't have permission
			Utils.hasPermission("writePermissions", annotated, message);

			// Set the "beingDeleted" flag on the account
			future.nest(DB.merge([{_id: args.accountId, beingDeleted: true}]));
		});
		
		// Wait a little bit to allow account lists to be updated in all the apps
		future.then(function() {
			var r = future.result;
			setTimeout(future.callback(this, function() {
			  future.result = r;
			}), 1500);
		});

		future.then(function() {
			var r;
			r = future.result;

			// Set the sync state for the account to "DELETE" so that a "Deleting Account" dashboard appears.
			// Ideally the individual capabilities will do this (and they can) but so many of them don't that
			// doing this here will at least show "Removing Account" for all accounts 		
			future.nest(PalmCall.call("palm://com.palm.tempdb/put", "", {
				"objects":[{
					_kind:"com.palm.account.syncstate:1",
					accountId: args.accountId,
					syncState: "DELETE",
					capabilityProvider:"com.palm.service.accounts"
				}]
			}));
		});
		
		future.then(function() {
			var r;
			r = future.result;
			
			// Kickoff account deletion in the background (don't wait for a response)
			PalmCall.call("palm://com.palm.service.accounts/notifyAccountDeleted", "", {"accountId":args.accountId});

			console.log("DeleteCommandAssistant: deleted account " + args.accountId);
			return {};
		});
	};
}
