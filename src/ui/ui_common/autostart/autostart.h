#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string_view>


namespace ui {


class Autostart {
public:
    class Error : public std::runtime_error {
        using runtime_error::runtime_error;
    };

    // See UiAutostartArgs
    struct Args {
        std::string_view appName;
        std::string_view appFileName;
        const std::string_view* args;
        std::size_t numArgs;
    };

    static std::unique_ptr<Autostart> create(const Args& args);

    virtual ~Autostart() = default;

    virtual bool getIsEnabled() const = 0;
    virtual void setIsEnabled(bool newIsEnabled) = 0;
};


}
