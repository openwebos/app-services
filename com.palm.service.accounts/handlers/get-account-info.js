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

/*global _ DB Assert Account console Utils */
function InfoCommandAssistant() {
	
	this.run = function(future) {
		var args = this.controller.args;
		var serviceAssistant = this.controller.service.assistant;
		var message = this.controller.message;

		//console.log("InfoCommandAssistant: id=" + args.accountId);
		future.nest(DB.get([args.accountId]));
		future.then(function() {
			var result = future.result.results;
			var account, annotated;
			Assert.require(result && result.length);
			
			account = _.extend(new Account(), result[0]);
			annotated = account.annotate(serviceAssistant.getTemplates());
			
			// Test for read permissions.  An exception will be thrown if it doesn't have permission
			Utils.hasPermission("readPermissions", annotated, message);
			
			future.result = {
				result: annotated
			};
		});
	};
	
}
