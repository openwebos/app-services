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

/*global MojoLoader IMPORTS require global */
var Foundations = IMPORTS.foundations;
var Json = IMPORTS["foundations.json"];
var Assert = Foundations.Assert;
var _ = IMPORTS.underscore._;
var DB = Foundations.Data.DB;
var Err = Foundations.Err;
var PalmCall = Foundations.Comms.PalmCall;

var Globalization = MojoLoader.require({name: "globalization", version: "1.0"}).globalization.Globalization;

if (typeof require === "undefined") {
	global.require = IMPORTS.require;
}

var fs = require("fs");
