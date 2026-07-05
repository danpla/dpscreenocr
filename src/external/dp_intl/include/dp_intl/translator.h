#pragma once

#include <memory>
#include <string_view>

#include "dp_intl/count_t.h"
#include "dp_intl/impl/message_catalog.h"


namespace dp::intl {


class DataStream;


/**
 * Translator
 *
 *
 * PluralSpec
 * ==========
 *
 * The PluralSpec template argument defines a plural form
 * specification used by methods such as translateN() and
 * translateContextN(). Think of it as the "Plural-Forms" entry from
 * Gettext PO files, expressed in C++ code. This type should have the
 * following:
 *
 * * A static constant number `numPlurals` of type CountT, equal to
 *   the total number of plural forms (must be > 0). This value
 *   defines the number of arguments taken by the plural-form
 *   methods such as translateN().
 *
 * * A call operator with a signature:
 *
 *       CountT operator()(CountT n) const
 *
 *   The operator takes the `n` argument from a plural-form method
 *   such as translateN(), and returns an index of the plural form
 *   within the range [0, numPlurals).
 *
 * The `translator_plural_spec.h` header provides GermanicPluralSpec
 * (for English and other Germanic languages) and SingleFormPluralSpec
 * (for languages that use a single form, or when using ID-based
 * translations). Plural rules for other languages are available at
 * the following resources; use the "nplurals=" number for
 * `numPlurals` and paste the "plural=" formula directly into the call
 * operator:
 *
 * * https://php-gettext.github.io/Languages
 * * https://docs.translatehouse.org/projects/localization-guide/en/latest/l10n/pluralforms.html
 * * https://www.gnu.org/software/gettext/manual/html_node/Plural-forms.html
 *
 *
 * API overview
 * ============
 *
 * The translation methods of the Translator class are modeled after
 * the functions of the Gettext runtime library:
 *
 * Gettext   | %dp::intl::Translator
 * --------- | ---------------------
 * gettext   | translate(), translateCStr()
 * ngettext  | translateN(), translateNCStr()
 * pgettext  | translateContext(), translateContextCStr()
 * npgettext | translateContextN(), translateContextNCStr()
 *
 * Each method has two variants: one for `std::string_view` and the
 * other (ending with "CStr") for `const char*`. The latter is
 * intended for APIs that only accept C strings, since in general
 * `std::string_view` is not guaranteed to be null-terminated.
 *
 * Keep in mind that all translation methods return the original
 * strings if the text has not been translated. For example,
 * translate() can return the `id` argument, and translateN() can
 * return one of the `plurals` strings. Since both `std::string_view`
 * and `const char*` are non-owning types, it's your responsibility to
 * ensure that a string returned by a translation method does not
 * outlive the referenced data. For example:
 *
 * ```{.cpp}
 * std::string foo();
 * void bar(std::string_view str);
 *
 * // WRONG: Dangling std::string_view to a temporary std::string.
 * auto str1 = translator.translate(foo());
 * bar(str1);
 *
 * // OK: The result is explicitly copied to a std::string before the
 * // temporary std::string is destroyed at the end of the expression.
 * std::string str2{translator.translate(foo())};
 * bar(str2);
 *
 * // OK: The result is used before the temporary std::string is
 * // destroyed at the end of the expression.
 * bar(translator.translate(foo()));
 * ```
 *
 *
 * Setting up xgettext
 * ===================
 *
 * For the `xgettext` tool to extract your translatable strings, you
 * must provide keywords with the `-k/--keyword` option. Here is the
 * full list, together with the leading `-k`:
 *
 *     -ktranslate
 *     -ktranslateCStr
 *     -ktranslateContext:1c,2
 *     -ktranslateContextCStr:1c,2
 *     -ktranslateN:2,3
 *     -ktranslateNCStr:2,3
 *     -ktranslateContextN:1c,3,4
 *     -ktranslateContextNCStr:1c,3,4
 *
 * Note that for the plural-form "N" methods, the above keywords
 * assume that the source language uses at least two plural forms.
 * This, however, is not a requirement, and you are free to tell
 * `xgettext` to use the same argument for both cases, like
 * `-ktranslateN:2,2` instead of `-ktranslateN:2,3`. In fact, you will
 * have to do this if your source strings use a language with only one
 * plural form, such as Japanese or Korean.
 *
 * You may also want to add an empty `-k` at the beginning of the list
 * to disable the default `xgettext` keywords, since they only apply
 * when using the functions from the Gettext library
 * (`gettext.h/libintl.h`).
 *
 * For the details, see
 * [Invoking the xgettext Program](https://www.gnu.org/software/gettext/manual/html_node/xgettext-Invocation.html).
 */
template<typename PluralSpec>
class Translator {
    static_assert(
        PluralSpec::numPlurals > 0,
        "Number of plural forms must be > 0");

    template<typename StrT>
    struct PluralsArray {
        template<typename... Plurals>
        static PluralsArray create(const Plurals&... plurals)
        {
            static_assert(
                sizeof...(plurals) == PluralSpec::numPlurals,
                "Wrong number of plural forms");

            return {plurals...};
        }

        operator std::string_view() const
        {
            return items[0];
        }

        StrT items[PluralSpec::numPlurals];
    };
public:
    /**
     * Create a no-op translator.
     *
     * The default-constructed Translator does not have a message
     * catalog file loaded, so its translation methods always return
     * original, untranslated message IDs.
     */
    Translator() = default;

    /**
     * Create a translator with a message catalog (MO file) loaded
     * from a data stream.
     *
     * \throws DataStream::Error Also propagates any other
     *     exception thrown by DataStream.
     */
    explicit Translator(DataStream& stream)
        : messageCatalog{std::make_unique<MessageCatalog>(stream)}
    {
    }

    ///@{
    /**
     * Translate a message.
     *
     * The method searches the message catalog (MO file) for a message
     * with the given ID. If the message is not present in the catalog
     * (i.e., the message has not been translated), the ID itself is
     * returned.
     */
    std::string_view translate(std::string_view id) const
    {
        return translateImpl<std::string_view>(id);
    }

    const char* translateCStr(const char* id) const
    {
        return translateImpl<const char*>(id);
    }
    ///@}

    ///@{
    /**
     * Translate a message using a context to resolve ambiguities.
     *
     * The method is similar to translate(), but takes an additional
     * `context` string to resolve ambiguities in cases when the same
     * message ID can have different meanings depending on the
     * context. For example, the word "Filter" can be both a noun
     * (e.g., when used as a label for a text input field) and a verb
     * (when used a button text).
     *
     * See the Gettext manual for details:
     *
     * * [Using contexts for solving ambiguities](https://www.gnu.org/software/gettext/manual/html_node/Contexts.html)
     * * [Entries with Context](https://www.gnu.org/software/gettext/manual/html_node/Entries-with-Context.html)
     */
    std::string_view translateContext(
        std::string_view context, std::string_view id) const
    {
        return translateImpl<std::string_view>(context, id);
    }

    const char* translateContextCStr(
        std::string_view context, const char* id) const
    {
        return translateImpl<const char*>(context, id);
    }
    ///@}

    ///@{
    /**
     * Translate a plural form message.
     *
     * The `n` parameter is an arbitrary number used to select the
     * plural form based on the rules of the language. Keep in mind
     * that `n` is an unsigned integer: negative and floating-point
     * numbers generally do not represent physical entities and,
     * therefore, should not be translated as plurals.
     *
     * The `plurals` parameter represents the plural forms of the
     * source language. Despite being a parameter pack, it requires
     * exactly `PluralSpec::numPlurals` arguments of the same string
     * type as returned by the method (that is, `std::string_view` or
     * `const char*`). A different number of arguments will result in
     * a compile-time error.
     *
     * The method searches the message catalog (MO file) for a message
     * with the ID equal to the first string in the `plurals`
     * argument. If the message is found, the plural rule from the
     * catalog is used to select the translated plural form. In this
     * case, `PluralSpec` and the remaining `plurals` argument strings
     * are not used. If the message is not present in the catalog
     * (i.e., the message has not been translated), the `PluralSpec`
     * is used to select a plural form from the `plurals` argument.
     *
     * See the Gettext manual for details:
     *
     * * [Additional functions for plural forms](https://www.gnu.org/software/gettext/manual/html_node/Plural-forms.html)
     * * [Entries with Plural Forms](https://www.gnu.org/software/gettext/manual/html_node/Entries-with-Plural-Forms.html)
     * * [Translating plural forms](https://www.gnu.org/software/gettext/manual/html_node/Translating-plural-forms.html)
     */
    template<typename... Plurals>
    std::string_view translateN(
        CountT n, const Plurals&... plurals) const
    {
        return translateImpl<std::string_view>(
            n, PluralsArray<std::string_view>::create(plurals...));
    }

    template<typename... Plurals>
    const char* translateNCStr(
        CountT n, const Plurals&... plurals) const
    {
        return translateImpl<const char*>(
            n, PluralsArray<const char*>::create(plurals...));
    }
    ///@}

    ///@{
    /**
     * Translate a plural form message using a context to resolve
     * ambiguities.
     *
     * This method is similar to translateN(), but also takes a
     * `context` string for the same purpose as translateContext().
     */
    template<typename... Plurals>
    std::string_view translateContextN(
        std::string_view context,
        CountT n,
        const Plurals&... plurals) const
    {
        return translateImpl<std::string_view>(
            context,
            n,
            PluralsArray<std::string_view>::create(plurals...));
    }

    template<typename... Plurals>
    const char* translateContextNCStr(
        std::string_view context,
        CountT n,
        const Plurals&... plurals) const
    {
        return translateImpl<const char*>(
            context,
            n,
            PluralsArray<const char*>::create(plurals...));
    }
    ///@}
private:
    PluralSpec pluralSpec{};
    std::unique_ptr<MessageCatalog> messageCatalog;

    template<typename StrT, typename... Args>
    StrT translateImpl(const Args&... args) const
    {
        if (messageCatalog) {
            const auto s = messageCatalog->translate(args...);
            if (s.size > 0)
                return s;
        }

        return fallback(args...);
    }

    template<typename StrT>
    StrT fallback(StrT id) const
    {
        return id;
    }

    template<typename StrT>
    StrT fallback(std::string_view /*context*/, StrT id) const
    {
        return id;
    }

    template<typename StrT>
    StrT fallback(CountT n, const PluralsArray<StrT>& plurals) const
    {
        const auto idx = pluralSpec(n);
        return plurals.items[idx < pluralSpec.numPlurals ? idx : 0];
    }

    template<typename StrT>
    StrT fallback(
        std::string_view /*context*/,
        CountT n,
        const PluralsArray<StrT>& plurals) const
    {
        return fallback(n, plurals);
    }
};


}
