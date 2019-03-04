
#pragma once


namespace test {


// Failures are not fatal by default.
void setFailureIsFatal(bool newIsFatal);


void failure();


int getNumFailures();


}
