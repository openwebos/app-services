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

var PublicWhitelist = {
	whitelistedApps: [
						{id: "com.quickoffice.webos", capability: "documents"},
						{id: "com.quickoffice.ar", capability: "documents"}
					],
	
	_stripProcessNumFromId: function(id){
		// because of a bug, the id will come in as [appid processid], see DFISH-5527
		var strippedId = id;
		var spacePos = id.indexOf(' ');
		if (spacePos != -1){
			strippedId = id.substr(0, spacePos);
		}
		return strippedId;
	},
	
	_findPermissionById: function(id){
		var strippedId = this._stripProcessNumFromId(id);
		var permission;
		for (var i = 0; i < this.whitelistedApps.length; i++){
			if (this.whitelistedApps[i].id === strippedId){
				permission = this.whitelistedApps[i];
				break;
			}
		}
		return permission;
	},
	
	hasPermissionsById: function(id){
		return this._findPermissionById(id) !== undefined;
	},
	
	hasPermissionsByIdAndCapability: function(id, capability){
		var permission = this._findPermissionById(id);
		
		if (permission && capability && permission.capability === capability.toLowerCase()){
			return true;
		} else {
			return false;
		}
	}
};