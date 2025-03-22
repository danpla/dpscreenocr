#pragma once


namespace ui {


// Initialize default data in the user data directory, such as OCR
// engine language files. On failure, sets an error message
// (dpsoGetError()) and returns false.
bool initUserData();


}
