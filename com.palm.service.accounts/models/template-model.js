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

/*global _ console Json fs */
function AccountTemplate(copy, prefixPath) {
	
	// TODO: use JSON Schema to validate rather than blindly accepting any template data
	
	var that = this;
	var templateSchema = JSON.parse(fs.readFileSync("schemas/template.json", "utf8"));
	
	var result = Json.Schema.validate(copy, templateSchema);
	if (!result.valid) {
		console.log("validator errors=" + JSON.stringify(result.errors, undefined, 2));
		console.log("len=" + result.errors.length);
		throw new Error("validation errors in template: " + copy.templateId + " errors=" + JSON.stringify(result.errors));
	}
	
	// consider how to best automatically decorate model objects to provide a consistent interface
	
	_.extend(this, copy);
	
	if (prefixPath.substr(-1) !== "/") {
		prefixPath += "/";
	}
	
	this.absolutizePaths(prefixPath, this.icon);
	if (this.capabilityProviders) {
		this.capabilityProviders.forEach(function(cp) {
			that.absolutizePaths(prefixPath, cp.icon);
		});
	}
	
	// TODO: localize 'loc_' fields following localization file structure convention
	
}

AccountTemplate.prototype = {
	
	absolutizePaths: function(prefix, subObject) {
		if (_.isUndefined(subObject)) {
			return;
		}
		Object.keys(subObject).forEach(function(key) {
			subObject[key] = prefix + subObject[key];
		});
	}
	
};
