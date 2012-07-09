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

/*global _ PalmCall Assert Foundations DB */
// KeyStore abstraction layer for encrypted storage of passwords, auth tokens, etc.

// To run in the simulator, comment out everything from 'putCredentials' through
// the 'return'. Uncomment the rest of the code to the end of the file, which
// depends on MojoDB instead of the native keymanager service.

var KeyStore = function() {

	function Credentials() {
		this._kind = Credentials.kind;
	}
	
	Credentials.kind = "com.palm.account.credentials:1";
	Credentials.COMMON = "common";

	function makeKey(accountId, key) {
		return accountId + ':' + key;
	}
	
	function mojodbGetRaw(accountId, key) {
		return DB.find({
			from: Credentials.kind,
			where: [{
				prop: "key",
				op: "=",
				val: makeKey(accountId, key)
			}]
		});
	}
	
	function mojodbPut(accountId, key, val) {
		var future = mojodbGetRaw(accountId, key);
		
		future.then(function() {
			var results = future.result.results;
			var c = new Credentials();
			
			if (results && results.length > 0) {
				c._id = results[0]._id;
				c._rev = results[0]._rev;
			}
			
			c.key = makeKey(accountId, key);
			c.val = val;
			
			future.nest(DB.put([c]));
		});
		
		return future;
	}
	
	function mojodbGet(accountId, key) {
		var future = mojodbGetRaw(accountId, key);
		
		// unwrap array of results into a single expected result (or throw an error)
		future.then(function() {
			var arr = future.result.results;
			Assert.require(arr && arr.length > 0, "Credentials not found: " + key);
			future.result = {
				credentials: arr[0].val
			};
		});
		
		return future;
	}
	
	function mojodbDel(accountId) {
		return DB.del({
			from: Credentials.kind,
			where: [{
				prop: "key",
				op: "%",
				val: makeKey(accountId, "")
			}]
		});
	}
	
	function mojodbHas(accountId) {
		var future = DB.find({
			from: Credentials.kind,
			where: [{
				prop: "key",
				op: "%",
				val: accountId
			}]
		});
		future.then(function() {
			var arr = future.result.results;
			future.result = {
				value: (arr.length > 0)
			};
		});
		return future;
	}
	
	return {
		put: mojodbPut,
		get: mojodbGet,
		del: mojodbDel,
		has: mojodbHas
	};
	
}();
