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

/*jslint white: true, onevar: true, undef: true, eqeqeq: true, plusplus: true, bitwise: true, 
 regexp: true, newcap: true, immed: true, nomen: false, maxerr: 500 */
/*global */

// Object that goes along with the people that met some rule criteria.



///////// Weighting Utilities /////////
function AutoLinkUnit() {
	var contact,
		person,
		weight,
		similarOn;
}

////////////////// Linking similarity values /////////////////////
// Modifying the values below will greatly affect how the linking
// rules determine if a contact is similar.

// Threshold for saying there is enough criteria to link -- Private
AutoLinkUnit.WEIGHT_MATCH_THRESHOLD = 100;

// Max and min weights -- Private
AutoLinkUnit.WEIGHT_MAX = 99999;
AutoLinkUnit.WEIGHT_MIN = -99999;

// The specific weights for each rule -- Private
AutoLinkUnit.WEIGHT_SIMILAR_NAME = 100;
AutoLinkUnit.WEIGHT_SIMILAR_MOBILE_PHONE_NUMBER = 100;
AutoLinkUnit.WEIGHT_SIMILAR_PHONE_NUMBER = 55;
AutoLinkUnit.WEIGHT_SIMILAR_EMAIL = 100;
AutoLinkUnit.WEIGHT_SIMILAR_IM = 100;
AutoLinkUnit.WEIGHT_CLB_MANUAL_LINK = AutoLinkUnit.WEIGHT_MAX;
AutoLinkUnit.WEIGHT_CLB_MANUAL_UNLINK = AutoLinkUnit.WEIGHT_MIN;

///////////////////////////////////////////////////////////////////
// Please be sure to add a label for each rule you add -- Private

AutoLinkUnit.SIMILAR_NAME_LABEL = "Similar Name";
AutoLinkUnit.SIMILAR_MOBILE_PHONE_NUMBER = "Similar Mobile Phone Number";
AutoLinkUnit.SIMILAR_PHONE_NUMBER_LABEL = "Similar Phone Number";
AutoLinkUnit.SIMILAR_EMAIL_LABEL = "Similar Email";
AutoLinkUnit.SIMILAR_IM_LABEL = "Similar IM";
AutoLinkUnit.SIMILAR_CLB_MANUAL_LINK_LABEL = "CLB Manual Link";
AutoLinkUnit.SIMILAR_CLB_MANUAL_UNLINK_LABEL = "CLB Manual Unlink";

///////////////////////////////////////////////////////////////////
// Holder for the label and the weight -- Public
AutoLinkUnit.SIMILAR_NAME = { weight: AutoLinkUnit.WEIGHT_SIMILAR_NAME, label: AutoLinkUnit.SIMILAR_NAME_LABEL };
AutoLinkUnit.SIMILAR_MOBILE_PHONE_NUMBER = { weight: AutoLinkUnit.WEIGHT_SIMILAR_MOBILE_PHONE_NUMBER, label: AutoLinkUnit.SIMILAR_MOBILE_PHONE_NUMBER };
AutoLinkUnit.SIMILAR_PHONE_NUMBER = { weight: AutoLinkUnit.WEIGHT_SIMILAR_PHONE_NUMBER, label: AutoLinkUnit.SIMILAR_PHONE_NUMBER_LABEL };
AutoLinkUnit.SIMILAR_EMAIL = { weight: AutoLinkUnit.WEIGHT_SIMILAR_EMAIL, label: AutoLinkUnit.SIMILAR_EMAIL_LABEL };
AutoLinkUnit.SIMILAR_IM = { weight: AutoLinkUnit.WEIGHT_SIMILAR_IM, label: AutoLinkUnit.SIMILAR_IM_LABEL };
AutoLinkUnit.SIMILAR_CLB_MANUAL_LINK = { weight: AutoLinkUnit.WEIGHT_CLB_MANUAL_LINK, label: AutoLinkUnit.SIMILAR_CLB_MANUAL_LINK_LABEL };
AutoLinkUnit.SIMILAR_CLB_MANUAL_UNLINK = { weight: AutoLinkUnit.WEIGHT_CLB_MANUAL_UNLINK, label: AutoLinkUnit.SIMILAR_CLB_MANUAL_UNLINK_LABEL };

//////////////////////////////////////////////////////////////////

AutoLinkUnit.prototype.addSimilarity = function (similarity, additive) {
	this.addWeight(similarity.weight);
	this.addSimilarOn(similarity.label, additive);
};

AutoLinkUnit.prototype.meetsWeightRequirement = function () {
	return this.weight >= AutoLinkUnit.WEIGHT_MATCH_THRESHOLD;
};


// Don't call these methods in code, instead use addSimilarity
AutoLinkUnit.prototype.addWeight = function (weightToAdd) {
	if (this.weight) {	
		this.weight += weightToAdd;
	} else {
		this.weight = weightToAdd;
	}
};

AutoLinkUnit.prototype.addSimilarOn = function (labelToAdd, additive) {
	var toAdd = {
		label: labelToAdd, 
		additive: additive 
	};
	
	if (this.similarOn) {
		this.similarOn[this.similarOn.length] = toAdd;
	} else {
		this.similarOn = [];
		this.similarOn[0] = toAdd;
	}
};

