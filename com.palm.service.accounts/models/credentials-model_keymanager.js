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

/*global _ PalmCall Assert Foundations */
// KeyStore abstraction layer for encrypted storage of passwords, auth tokens, etc.

var KeyStore = function() {
	
	var keyStoreFuture = new Foundations.Control.Future(null);

	var KEYMGR_URI = "palm://com.palm.keymanager/";
	
	function putCredentials(accountId, key, value) {
		var keydata;
		var future = PalmCall.call(KEYMGR_URI, "fetchKey", {
			keyname: accountId
		});
		future.then(function() {
			try {
				// Fire the exception if there is one
				future.getResult();
			} catch (e) {
				keydata = {};
				future.result = {};
				return;
			}

			keydata = JSON.parse(future.result.keydata);

			// Remove the key with this accountId, so it can be replaced
			future.nest(PalmCall.call(KEYMGR_URI, "remove", {
				"keyname": accountId
			}));
		});
		future.then(function() {
			future.getResult();
			keydata[key] = value;

			future.nest(PalmCall.call(KEYMGR_URI, "store", {
				keyname: accountId,
				keydata: JSON.stringify(keydata),
				type:    "ASCIIBLOB",
				nohide:  true
			}));
		});
		return future;
	}

	function getCredentials(accountId, key) {
		var future = PalmCall.call(KEYMGR_URI, "fetchKey", {
			keyname: accountId
		});
		future.then(function() {
			var success;
			
			try {
				var credentials = JSON.parse(future.result.keydata)[key];

				if (credentials) {
					future.result = {
						credentials: credentials
					};
					success = true;
				}
			} catch (e) {
			}
			
			if (!success) {
				future.setException({
					message: "Credentials for key '" + key + "' not found " +
						"in account '" + accountId + "'",
					errorCode: "CREDENTIALS_NOT_FOUND"
				});
			}
			
		});
		return future;
	}

	function deleteCredentials(accountId) {
		return PalmCall.call(KEYMGR_URI, "remove", {
			keyname: accountId
		});
	}

	function hasCredentials(accountId) {
		var future = PalmCall.call(KEYMGR_URI, "keyInfo", {
			keyname: accountId
		});
		future.then(function() {
			var r;
			var success;
			
			try {
				r = future.result;
				success = true;
			} catch (e) {
				success = false;
			}
			future.result = {
				value: success
			};
		});
		return future;
	}

	function enqueueCall(f) {
		return function() {
			var args = Array.prototype.slice.call(arguments);

			var external = new Foundations.Control.Future(null);
			external.then(function() {
				keyStoreFuture.then(function() {
					keyStoreFuture.nest(f.apply(null, args));
				});
				keyStoreFuture.then(function() {
					try {
						external.result = keyStoreFuture.result;
					} catch (e) {
						external.exception = keyStoreFuture.exception;
					}
					keyStoreFuture.result = undefined;  // re-prime the future for the next command
				});
			});
			return external;
		};
	}

	return {
		put: enqueueCall(putCredentials),
		get: enqueueCall(getCredentials),
		del: enqueueCall(deleteCredentials),
		has: enqueueCall(hasCredentials)
	};
}();
