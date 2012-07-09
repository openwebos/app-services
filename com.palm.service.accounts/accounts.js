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

/*global KeyStore console quit Foundations DB Json _ AccountTemplate PalmCall Globalization fs process Account */

// -----------------------------

function AccountsServiceAssistant() {
	var templates;
	
	var TEMPLATE_ROOTS = [
		"/usr/palm/public/accounts",
		"/media/cryptofs/apps/usr/palm/accounts"
	];
	
	function getTemplatePaths() {
		// list all top-level subdirectories of each member of TEMPLATE_ROOTS
		// for each subdirectory, get all "*.json" files
		// return all "*.json" files.
		
		var dotJsonRegex = /\.json$/i;
		var jsonFiles = [];
		var future = new Foundations.Control.Future();
		
		function addJSONFileToResults(relativePath) {
			var fullPath = [this.parent, relativePath].join("/");
			if (dotJsonRegex.test(fullPath) && fs.statSync(fullPath).isFile()) {
				jsonFiles.push({
					parent: this.parent,
					file: relativePath
				});
			}
		}
		
		TEMPLATE_ROOTS.forEach(function(root) {
			var dirs;
			try {
				dirs = fs.readdirSync(root);
				dirs.forEach(function(dir) {
					var files;
					var dirPath = [root, dir].join("/");
					if (fs.statSync(dirPath).isDirectory()) {
						files = fs.readdirSync(dirPath);
						files.forEach(addJSONFileToResults, { parent: dirPath });
					}
				});
			} catch (e) {
				// console.warn("Did not find account templates here (no public synergy apps?): " + root);
			}
		});
		future.result = jsonFiles;
		
		return future;
	}
	
	function handleTemplatePaths(future) {
		var paths = future.result;
		
		var hash = {};
		var fileSchema = JSON.parse(fs.readFileSync("schemas/template-file.json", "utf8"));

		console.log("Found " + paths.length + " account templates");
		
		function addTemplate(path, locale, obj) {
			// Should this template be excluded?
			if (obj.allowed_locales && !obj.allowed_locales.some(function(l){return l === locale})) {
				console.log("Ignoring template " + obj.templateId + " because " + locale + " is not in allowed_locales");
				return;
			}
			if (obj.disallowed_locales && obj.disallowed_locales.some(function(l){return l === locale})) {
				console.log("Ignoring template " + obj.templateId + " because " + locale + " is in disallowed_locales");
				return;
			}
			
			var newTemplate = new AccountTemplate(obj, path);

			if (typeof hash[newTemplate.templateId] !== "undefined") {
				console.log("*** WARNING: ignoring duplicate account template: " + newTemplate.templateId);
			} else {
				hash[newTemplate.templateId] = newTemplate;
			}
		}

		function addTemplates(path) {
			var toAdd;
			var RB;
			var str;

			try {
				RB = new Globalization.ResourceBundleFactory(path.parent).getResourceBundle();
				var locale = (RB && RB.locale && RB.locale.locale)? RB.locale.locale : ""; 
				str = RB.getLocalizedResource(path.file);
				toAdd = JSON.parse(str);
				
				// some backwards compat here.  accept both arrays of templates and a bare template object
				// the bare template is the way to go moving forward.
				if (!_.isArray(toAdd)) {
					toAdd = [toAdd];
				}
				
				var result = Json.Schema.validate(toAdd, fileSchema);
				if (!result.valid) {
					throw new Error(path + " failed validation: " + JSON.stringify(result.errors));
				}
				
				toAdd.forEach(_.bind(addTemplate, undefined, path.parent, locale));
			} catch (e) {
				console.error("Parse error for file: '" + (path.parent+"/"+path.file) + "', stack=\n" + e.stack);
			}
		}

		paths.forEach(addTemplates);

		templates = _.values(hash);

		// FIXME: this doesn't seem to respect typical en_US collation rules so e.g. (e = é < f < z).
		// Instead, you get this: (e < f < z < é)
		// In Firefox 3, by contrast, localeCompare alone is case-insensitive and does ignore diacritics.
		templates.sort(function(a, b) {
			var nameA = (a.loc_name || "").toLocaleUpperCase();
			var nameB = (b.loc_name || "").toLocaleUpperCase();
			return nameA.localeCompare(nameB);
		});
		
		future.result = undefined;
	}
	
	// Apps watch tempdb for changes to the templates so they know when to reload them
	// Reloading is necessary if the list has changed because a template has been added
	// or removed as part of app installation so accounts may disappear (template deleted)
	// or a new account type can be added (template added)
	function updateAppTemplateList(future) {
		PalmCall.call("palm://com.palm.tempdb/find", "", {
			"query":{
				"from": "com.palm.signaling:1",
				"where":[{
					"prop":"appId",
					"op":"=",
					"val": "com.palm.accounts.templates"
				}]}
		}).then(this, function(f) {
			var r = f.result;

			// Create a string containing a list of all templates in alphabetical order
			var newTemplatesStr, newTemplates = [];
			for (var i=0, l = templates.length; i < l; i++)
				newTemplates.push(templates[i].templateId);
			newTemplatesStr = newTemplates.sort().toString();
			
			// Is the list of templates the same?
			if (!r.results || !r.results[0] || newTemplatesStr !== r.results[0].templates) {
				console.log("There are " + templates.length + " templates. A template was added or deleted");
				// Templates have changed so update tempdb so that apps watching for changes will be notified
				PalmCall.call("palm://com.palm.tempdb/del", "", {
					"query":{
						"from": "com.palm.signaling:1",
						"where":[{
							"prop":"appId",
							"op":"=",
							"val": "com.palm.accounts.templates"
						}]}
				}).then(this, function(f) {
					PalmCall.call("palm://com.palm.tempdb/put", "", {
						"objects":[{
							"_kind": "com.palm.signaling:1",
							"appId": "com.palm.accounts.templates",
							"templates": newTemplatesStr
						}]
					})
				});
			}
			else {
				// The templates didn't change
				console.log("There are " + templates.length + " templates, same as before.");
			}
		});
		future.result = {};
	}
	
	function parseTemplates() {
		var future = getTemplatePaths();
		
		future.then(handleTemplatePaths);
		
		future.then(updateAppTemplateList);
		
		return future;
	}
	
	this.getTemplates = function() {
		return templates;
	}
	
	this.reloadTemplates = function() {
		console.log("Reloading templates ...");
		return parseTemplates();
	}
	
	function watchForLocaleChanges() {
		var locale;
		
		console.log("Subscribing to locale changes ...");
		var localeWatcherFuture = PalmCall.call("luna://com.palm.systemservice", "getPreferences", {'keys':['locale'], 'subscribe': true});
		localeWatcherFuture.then(function localeChanged() {
			// The service responds immediately with the current locale, then again whenever the locale changes
			var r = localeWatcherFuture.result.locale;
			var newLocale = r.languageCode + "_" + r.countryCode;
			if (!locale)
				locale = newLocale;
			else {
				// Has the locale changed?
				if (newLocale !== locale) {
					// Quit the Accounts service now, so that locale-specific templates are loaded
					// Unfortunately there is no way to force the globalization library to reload the locale
					console.log("Restarting service because of locale change from " + locale + " to " + newLocale);
					localeWatcherFuture.cancel();
					PalmCall.call("palm://com.palm.service.accounts/__quit", "", {});
				}
	 		}
			 localeWatcherFuture.then(localeChanged);
		 });
	}

	
	this.setup = function() {
		var setupFuture = parseTemplates();
		
		setupFuture.onError(function(future) {
			console.error("accounts-service error: " + JSON.stringify(future.exception));
			process.exit();
		});

		setupFuture.then(function() {
			watchForLocaleChanges();
			setupFuture.result = {};
		});
		
		// Since we're in "setup", the accounts service wasn't running.  Having it run
		// most of the time will shave around 1.5 seconds off every service call
		// Keep it running for an hour ...
		PalmCall.call("palm://com.palm.service.accounts/stayRunning", "", {"seconds":"3500"});
		
		return setupFuture;
	}
	
}
