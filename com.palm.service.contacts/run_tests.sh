#! /bin/sh
FRAMEWORK=/usr/palm/frameworks
TESTRUNNER=$FRAMEWORK/unittest/version/1.0/tools/test_runner.js

/usr/bin/triton -I $FRAMEWORK $TESTRUNNER -- test/all_tests.json
