#include "gnu_header.h"

#include <algorithm>
#include <charconv>
#include <string>

#include "dp_intl/error.h"
#include "str_utils.h"


namespace dp::intl {
namespace {


[[noreturn]]
void throwUnknownFieldPropertyError(std::string_view prop)
{
    throw Error{"Unknown property \"" + std::string{prop} + "\""};
}


[[noreturn]]
void throwMissingFieldPropertyError(std::string_view prop)
{
    throw Error{"Missing \"" + std::string{prop} + "\" property"};
}


[[noreturn]]
void throwInvalidFieldPropertyError(
    std::string_view prop, std::string_view description)
{
    throw Error{
        "Invalid \"" + std::string{prop} + "\" property: "
        + std::string{description}};
}


struct PropKV {
    std::string_view key;
    std::string_view val;
};


PropKV splitProp(std::string_view prop)
{
    const auto p = prop.find('=');
    if (p == prop.npos)
        throw Error{"No \"=\" separator"};

    const auto key = trimBlanks(prop.substr(0, p));
    if (key.empty())
        throw Error{"Key is empty (nothing before \"=\")"};

    const auto val = trimBlanks(prop.substr(p + 1));
    if (val.empty())
        throw Error{"Value is empty (nothing after \"=\")"};

    return {key, val};
}


template<typename Fn>
void forEachProp(std::string_view fieldVal, Fn fn)
{
    while (!fieldVal.empty()) {
        // ";" after the last property is optional.
        const auto p = std::min(fieldVal.find(';'), fieldVal.size());

        const auto prop = trimBlanks(fieldVal.substr(0, p));
        if (!prop.empty())
            fn(prop);

        if (p == fieldVal.size())
            break;

        fieldVal.remove_prefix(p + 1);
    }
}


void processContentTypeField(GnuHeader& header, std::string_view val)
{
    static const std::string_view charsetProp{"charset"};

    forEachProp(
        val,
        [&](std::string_view prop)
        {
            if (prop == "text/plain")
                return;

            PropKV propKv;
            try {
                propKv = splitProp(prop);
            } catch (const Error& e) {
                throwInvalidFieldPropertyError(prop, e.what());
            }

            if (propKv.key == charsetProp)
                header.charset = propKv.val;
            else
                throwUnknownFieldPropertyError(propKv.key);
        });

    if (header.charset.empty())
        throwMissingFieldPropertyError(charsetProp);
}


void processPluralFormsField(GnuHeader& header, std::string_view val)
{
    static const std::string_view npluralsProp{"nplurals"};
    static const std::string_view pluralProp{"plural"};

    std::string_view nplurals;
    std::string_view plural;

    forEachProp(
        val,
        [&](std::string_view prop)
        {
            PropKV propKv;
            try {
                propKv = splitProp(prop);
            } catch (const Error& e) {
                throwInvalidFieldPropertyError(prop, e.what());
            }

            if (propKv.key == npluralsProp)
                nplurals = propKv.val;
            else if (propKv.key == pluralProp)
                plural = propKv.val;
            else
                throwUnknownFieldPropertyError(propKv.key);
        });

    if (nplurals.empty())
        throwMissingFieldPropertyError(npluralsProp);
    if (plural.empty())
        throwMissingFieldPropertyError(pluralProp);

    const auto* nBegin = nplurals.data();
    const auto* nEnd = nBegin + nplurals.size();
    CountT n;
    const auto [ptr, ec] = std::from_chars(nBegin, nEnd, n);
    if (ec != std::errc{} || ptr != nEnd) {
        if (ec == std::errc::result_out_of_range)
            throwInvalidFieldPropertyError(
                npluralsProp,
                "Number "
                    + std::string{nBegin, ptr}
                    + "[...] is out of range");

        throwInvalidFieldPropertyError(
            npluralsProp,
            "Invalid number format \""
                + std::string{nplurals} + "\"");
    }

    header.pluralForms = {n, plural};
}


void processField(
    GnuHeader& header, std::string_view name, std::string_view val)
{
    if (name == "Content-Type")
        processContentTypeField(header, val);
    else if (name == "Plural-Forms")
        processPluralFormsField(header, val);
}


}


GnuHeader GnuHeader::parse(std::string_view s)
{
    GnuHeader result{};

    while (true) {
        const auto p = s.find_first_of(":\n");
        if (p == s.npos)
            break;

        if (s[p] == '\n') {
            // A valid header entry in PO/MO file always consists of
            // "key: value" pairs. However, when using the msgcat
            // utility, the resulting PO can have conflicts that will
            // be marked by "#-#-#-#-#" comments, like:
            //
            // "#-#-#-#-# a.po #-#-#-#-#"
            // "Text A"
            // "#-#-#-#-# b.po #-#-#-#-#"
            // "Text B"
            //
            // See:
            // https://www.gnu.org/software/gettext/manual/html_node/Creating-Compendia.html
            //
            // This type of comment can appear in any "msgstr" entry
            // of a PO file, including the header. There are projects
            // that actually have such headers in their MO files:
            // https://github.com/python/cpython/issues/80420
            //
            // Although such a header is clearly incorrect (as it can
            // have conflicting definitions of character encodings,
            // plural rules, etc.), we skip such entries since we are
            // not doing the full header validation anyway, and other
            // software (including the Gettext runtime) accepts them.
            s.remove_prefix(p + 1);
            continue;
        }

        const auto valP = p + 1;
        const auto valEndP = std::min(s.find('\n', valP), s.size());

        const auto name = trimBlanks(s.substr(0, p));
        const auto val = trimBlanks(s.substr(valP, valEndP - valP));

        try {
            processField(result, name, val);
        } catch (const Error& e) {
            throw Error{
                "\"" + std::string{name} + "\" field: " + e.what()};
        }

        if (valEndP == s.size())
            break;

        s.remove_prefix(valEndP + 1);
    }

    return result;
}


}
