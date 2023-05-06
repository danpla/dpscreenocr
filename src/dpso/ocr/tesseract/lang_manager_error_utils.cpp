
#include "ocr/tesseract/lang_manager_error_utils.h"

#include <cassert>
#include <string>

#include "dpso_net/error.h"

#include "ocr/lang_manager_error.h"


namespace dpso::ocr::tesseract {


void rethrowNetErrorAsLangManagerError(const char* message)
{
    try {
        throw;
    } catch (net::ConnectionError& e) {
        throw LangManagerNetworkConnectionError{message};
    } catch (net::Error& e) {
        throw LangManagerError{message};
    } catch (...) {
        assert(false);
        throw LangManagerError{
            std::string{message} + " (net::Error was not caught)"};
    }
}


}
