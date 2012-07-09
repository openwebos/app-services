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

/*global _ console */
function Account() {
	this._kind = Account.kind;
}

Account.kind = "com.palm.account:1";

Account.prototype.annotate = Account.annotate = function(templates) {
	// annotate results
	
	// net effect is totally stitched together acct+template obj,
	// where template props override acct props at both the top level
	// and per-capability
	// (but extra acct props are still present in case accounts need to store supplemental data)
	
	var result;
	var subset;
	var templateId = this.templateId;
	var template = _.detect(templates, function(t) {
		return (t.templateId === templateId);
	});
	
	if (!template) {
		throw new Error("template not found: " + templateId);
	}
	
	subset = this.capabilityProviders;
	result = _.extend(this, template);
	result.capabilityProviders = subset;
	
	// A previous implementation that allowed acct props to override template props
	// result = Object.clone(template);
	// delete result.capabilities;
	// Object.extend(result, this);
	
	result.capabilityProviders = result.capabilityProviders.map(function(c) {
		var tmplCap = _.detect(template.capabilityProviders, function(tc) {
			return (tc.id === c.id);
		});
		
		if (!tmplCap) {
			console.warn("capability provider not found: " + c.id + " for template: " + template.templateId);
			return c;
		}
		
		return _.extend(c, tmplCap);
		
		// A previous implementation that allowed acct props to override template props
		// return Object.extend(Object.clone(tmplCap), c);
	});
	
	return result;
};
