#pragma once

#include <memory>
#include <string_view>

#include "dp_intl/count_t.h"
#include "dp_intl/impl/c_str_ref.h"


namespace dp::intl {


class DataStream;


class MessageCatalog {
public:
    explicit MessageCatalog(DataStream& stream);
    ~MessageCatalog();

    MessageCatalog(const MessageCatalog&) = delete;
    MessageCatalog& operator=(const MessageCatalog&) = delete;

    MessageCatalog(MessageCatalog&&) = delete;
    MessageCatalog& operator=(MessageCatalog&&) = delete;

    CStrRef translate(std::string_view id) const;
    CStrRef translate(
        std::string_view context, std::string_view id) const;
    CStrRef translate(CountT n, std::string_view id) const;
    CStrRef translate(
        std::string_view context,
        CountT n,
        std::string_view id) const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};


}

