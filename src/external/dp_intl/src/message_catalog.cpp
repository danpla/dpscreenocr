#include "dp_intl/impl/message_catalog.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <limits>
#include <numeric>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "byte_order.h"
#include "data_stream_utils.h"
#include "dp_intl/data_stream.h"
#include "dp_intl/error.h"
#include "gnu_header.h"
#include "plural/eval.h"
#include "str_utils.h"


using namespace std::literals::string_literals;


namespace dp::intl {
namespace {


using U32 = std::uint32_t;
const auto U32Max = std::numeric_limits<U32>::max();


struct StrIRef {
    U32 pos;
    U32 size;

    CStrRef getCStrRef(const std::string& data) const
    {
        assert(pos < data.size());
        assert(size < data.size() - pos);
        assert(data[pos + size] == 0);

        return {data.data() + pos, size};
    }

    std::string_view getView(const std::string& data) const
    {
        assert(pos <= data.size());
        assert(size <= data.size() - pos);

        return {data.data() + pos, size};
    }
};


template<typename T>
struct Item {
    StrIRef id;
    T val;

    using Val = T;
    using LookupKey = std::string_view;

    struct Cmp {
        const std::string& strData;

        template<typename T1, typename T2>
        bool operator()(const T1& a, const T2& b) const
        {
            return getId(a) < getId(b);
        }
    private:
        auto getId(const Item& item) const
        {
            return item.id.getView(strData);
        }

        auto getId(const LookupKey& lk) const
        {
            return lk;
        }
    };
};


template<typename T>
struct ContextItem {
    StrIRef context;
    StrIRef id;
    T val;

    using Val = T;

    struct LookupKey {
        std::string_view context;
        std::string_view id;
    };

    struct Cmp {
        const std::string& strData;

        template<typename T1, typename T2>
        bool operator()(const T1& a, const T2& b) const
        {
            if (const auto cmp = getContext(a).compare(getContext(b));
                    cmp != 0)
                return cmp < 0;

            return getId(a) < getId(b);
        }
    private:
        auto getContext(const ContextItem& item) const
        {
            return item.context.getView(strData);
        }

        auto getContext(const LookupKey& lk) const
        {
            return lk.context;
        }

        auto getId(const ContextItem& item) const
        {
            return item.id.getView(strData);
        }

        auto getId(const LookupKey& lk) const
        {
            return lk.id;
        }
    };
};


struct PluralForms {
    CountT numPlurals;
    plural::Eval eval;

    PluralForms(CountT numPlurals, std::string_view expr)
        : numPlurals{numPlurals}
        , eval{expr}
    {
    }
};


}


struct MessageCatalog::Impl {
    std::string strData;
    std::vector<StrIRef> pluralStrIRefs;

    std::vector<Item<StrIRef>> items;
    std::vector<ContextItem<StrIRef>> contextItems;
    std::vector<Item<U32>> pluralItems;
    std::vector<ContextItem<U32>> pluralContextItems;

    std::optional<PluralForms> pluralForms;

    template<typename ItemT>
    void sort(std::vector<ItemT>& items) const
    {
        const typename ItemT::Cmp cmp{strData};
        std::sort(items.begin(), items.end(), cmp);
    }

    template<typename ItemT>
    const typename ItemT::Val* findVal(
        const std::vector<ItemT>& items,
        const typename ItemT::LookupKey& lk) const
    {
        const typename ItemT::Cmp cmp{strData};

        const auto iter = std::lower_bound(
            items.begin(), items.end(), lk, cmp);

        if (iter != items.end() && !cmp(lk, *iter))
            return &iter->val;

        return {};
    }

    CStrRef resolvePlural(CountT n, U32 valIdx) const
    {
        assert(pluralForms);
        assert(valIdx < pluralStrIRefs.size());
        assert(
            pluralStrIRefs.size() - valIdx
            >= pluralForms->numPlurals);

        CountT idx;
        try {
            idx = pluralForms->eval(n);
            if (idx >= pluralForms->numPlurals)
                idx = 0;
        } catch (const Error&) {
            idx = 0;
        }

        return pluralStrIRefs[valIdx + idx].getCStrRef(strData);
    }

    template<typename ItemT>
    CStrRef translate(
        const std::vector<ItemT>& items,
        const typename ItemT::LookupKey& lk) const
    {
        if (const auto* val = findVal(items, lk))
            return val->getCStrRef(strData);

        return {};
    }

    template<typename ItemT>
    CStrRef translate(
        const std::vector<ItemT>& items,
        CountT n,
        const typename ItemT::LookupKey& lk) const
    {
        if (const auto* val = findVal(items, lk))
            return resolvePlural(n, *val);

        return {};
    }
};


MessageCatalog::MessageCatalog(DataStream& stream)
    : impl{std::make_unique<Impl>()}
{
    // The MO file format specification:
    // https://www.gnu.org/software/gettext/manual/html_node/MO-Files.html

    const auto checkSign = [&stream]
    {
        using Sign = std::array<std::uint8_t, 4>;

        Sign sign;
        try {
            read(stream, sign.data(), sign.size());
        } catch (const Error& e) {
            throw Error{"Can't read MO signature: "s + e.what()};
        }

        if (sign == Sign{0x95, 0x04, 0x12, 0xde})
            return ByteOrder::Big;

        if (sign == Sign{0xde, 0x12, 0x04, 0x95})
            return ByteOrder::Little;

        throw Error{"Signature mismatch; not an MO file"};
    };

    const auto byteOrder = checkSign();

    const auto readU32 =
    [
        &stream,
        loadU32 = byteOrder == ByteOrder::Little
            ? load<ByteOrder::Little, U32>
            : load<ByteOrder::Big, U32>]
    {
        std::uint8_t data[sizeof(U32)];
        read(stream, data, sizeof(U32));
        return loadU32(data);
    };

    const auto processRevision = [&readU32]
    {
        U32 rev;
        try {
            rev = readU32();
        } catch (const Error& e) {
            throw Error{"Can't read revision: "s + e.what()};
        }

        // We only care about the major revision.
        if (const auto revMajor = rev >> 16; revMajor > 1)
            throw Error{
                "Unsupported major revision " + toStr(revMajor)};
    };

    processRevision();

    static const U32 strCountLimit{500'000};

    const auto readStrCount = [&readU32]
    {
        U32 strCount;
        try {
            strCount = readU32();
        } catch (const Error& e) {
            throw Error{"Can't read string count: "s + e.what()};
        }

        if (strCount > strCountLimit)
            throw Error{
                "Suspiciously large string count " + toStr(strCount)
                + "; limit is " + toStr(strCountLimit)};

        return strCount;
    };

    const auto strCount = readStrCount();

    static const U32 fileSizeLimit{64'000'000};

    const auto readOffset = [&readU32]
    {
        const auto offset = readU32();
        if (offset > fileSizeLimit)
            throw Error{
                "Offset " + toStr(offset) + " exceeds file size "
                "limit " + toStr(fileSizeLimit)};

        return offset;
    };

    struct StrTables {
        std::vector<StrIRef> source;
        std::vector<StrIRef> translated;
        // This includes null terminators for each string.
        U32 strDataSize;
    };

    const auto loadStrTables =
    [&stream, &readU32, &readOffset, strCount]() -> StrTables
    {
        struct TableInfo {
            const char* name;
            U32 offset{};
            std::vector<StrIRef> items{};
        };

        TableInfo tableInfos[]{
            {"source strings"}, {"translated strings"}};

        for (auto& tableInfo : tableInfos)
            try {
                tableInfo.offset = readOffset();
            } catch (const Error& e) {
                throw Error{
                    "Can't read "s + tableInfo.name + " table "
                    "offset: " + e.what()};
            }

        U32 strDataSize{};

        const auto readTable = [&](U32 tableOffset)
        {
            try {
                stream.setPosition(tableOffset);
            } catch (const Error& e) {
                throw Error{
                    "Can't set stream position to "
                    + toStr(tableOffset) + ": " + e.what()};
            }

            const auto readStrDescriptior = [&]
            {
                U32 size;
                try {
                    size = readU32();
                } catch (const Error& e) {
                    throw Error{
                        "Can't read string size: "s + e.what()};
                }

                U32 offset;
                try {
                    offset = readOffset();
                } catch (const Error& e) {
                    throw Error{
                        "Can't read string offset: "s + e.what()};
                }

                static const U32 strSizeLimit{1'000'000};
                if (size > strSizeLimit)
                    throw Error{
                        "Suspiciously long "
                        "(" + toStr(size) + " bytes) string; limit "
                        "is " + toStr(strSizeLimit)};

                const auto sizeWith0 = size + 1;
                if (sizeWith0 > fileSizeLimit - offset)
                    throw Error{
                        "String offset (" + toStr(offset) + ") + "
                        "size (" + toStr(sizeWith0) + ", including "
                        "null terminator) is beyond file size limit "
                        + toStr(fileSizeLimit)};

                return StrIRef{offset, size};
            };

            std::vector<StrIRef> result(strCount);

            auto descriptorOffset = tableOffset;

            for (U32 i{}; i < strCount; ++i) {
                StrIRef fileStrIRef;
                try {
                    fileStrIRef = readStrDescriptior();
                } catch (const Error& e) {
                    throw Error{
                        "Can't read string descriptor at offset "
                        + toStr(descriptorOffset) + ": " + e.what()};
                }

                descriptorOffset += sizeof(U32) * 2;

                result[i] = fileStrIRef;

                static const auto strDataSizeLimit = fileSizeLimit;

                const auto sizeWith0 = fileStrIRef.size + 1;
                if (sizeWith0 > strDataSizeLimit - strDataSize)
                    throw Error{
                        "Total string data size exceeds limit "
                        + toStr(strDataSizeLimit)};

                strDataSize += sizeWith0;
            }

            return result;
        };

        for (auto& tableInfo : tableInfos)
            try {
                tableInfo.items = readTable(tableInfo.offset);
            } catch (const Error& e) {
                throw Error{
                    "Can't read "s + tableInfo.name +  " table at "
                    "offset " + toStr(tableInfo.offset) + ": "
                    + e.what()};
            }

        return {
            std::move(tableInfos[0].items),
            std::move(tableInfos[1].items),
            strDataSize};
    };

    const auto strTables = loadStrTables();

    impl->strData.resize(strTables.strDataSize);

    auto readStr =
    [
        &stream,
        &strData = impl->strData,
        strDataPos = U32{}]
    (StrIRef fileStrIRef) mutable
    {
        const auto sizeWith0 = fileStrIRef.size + 1;

        assert(strDataPos < strData.size());
        assert(sizeWith0 <= strData.size() - strDataPos);

        try {
            stream.setPosition(fileStrIRef.pos);
        } catch (const Error& e) {
            throw Error{
                "Can't set stream position to "
                + toStr(fileStrIRef.pos) + ": " + e.what()};
        }

        try {
            read(stream, strData.data() + strDataPos, sizeWith0);
        } catch (const Error& e) {
            throw Error{
                "Can't read " + toStr(sizeWith0) + " bytes of string "
                "data from position " + toStr(fileStrIRef.pos) + ": "
                + e.what()};
        }

        if (strData[strDataPos + fileStrIRef.size] != 0)
            throw Error{
                "No null terminator at offset "
                + toStr(fileStrIRef.pos + fileStrIRef.size)};

        const auto pos = strDataPos;
        strDataPos += sizeWith0;

        return StrIRef{pos, fileStrIRef.size};
    };

    enum ItemFlag : U32 {
        ItemFlagNone = 0,
        ItemFlagContext = 1 << 0,
        ItemFlagPlural = 1 << 1,
        ItemFlagAll = ItemFlagContext | ItemFlagPlural,
    };

    using ItemFlags = U32;
    static const auto numItemFlagCombos = ItemFlagAll + 1;

    struct ItemInfo {
        // In case of a plural item, the second from (including its
        // leading null separator) is already removed.
        StrIRef strIRef;
        U32 contextSepPos;
        ItemFlags flags;

        StrIRef getContext() const
        {
            assert(contextSepPos <= strIRef.size);
            return {
                strIRef.pos,
                contextSepPos == strIRef.size ? 0 : contextSepPos};
        }

        StrIRef getId() const
        {
            assert(contextSepPos <= strIRef.size);
            const auto idPos =
                contextSepPos == strIRef.size ? 0 : contextSepPos + 1;

            return {strIRef.pos + idPos, strIRef.size - idPos};
        }
    };

    static const char pluralSep{0};

    const auto createItemInfo =
    [&strData = impl->strData](StrIRef strIRef) -> ItemInfo
    {
        static const char contextSep{4};
        static const char allSeps[]{contextSep, pluralSep};

        const auto s = strIRef.getView(strData);

        const auto sepPos = s.find_first_of(
            allSeps, 0, sizeof(allSeps));
        if (sepPos == s.npos)
            return {strIRef, strIRef.size, ItemFlagNone};

        const auto sep = s[sepPos];
        if (sep == contextSep) {
            ItemFlags flags{ItemFlagContext};

            const auto pluralSepPos = s.find(pluralSep, sepPos + 1);
            if (pluralSepPos != s.npos) {
                flags |= ItemFlagPlural;
                strIRef.size = static_cast<U32>(pluralSepPos);
            }

            return {strIRef, static_cast<U32>(sepPos), flags};
        }

        assert(sep == pluralSep);
        strIRef.size = static_cast<U32>(sepPos);
        return {strIRef, strIRef.size, ItemFlagPlural};
    };

    std::vector<ItemInfo> itemInfos(strCount);
    std::array<U32, numItemFlagCombos> itemTypeCount{};

    std::optional<U32> headerEntryIdx;

    for (U32 i{}; i < strCount; ++i) {
        StrIRef sourceStrIRef;
        try {
            sourceStrIRef = readStr(strTables.source[i]);
        } catch (const Error& e) {
            throw Error{"Can't read source string: "s + e.what()};
        }

        itemInfos[i] = createItemInfo(sourceStrIRef);
        const auto& itemInfo = itemInfos[i];

        // Although we don't return the context string from the API,
        // make it null-terminated like the other strings for
        // consistency.
        if (itemInfo.contextSepPos < itemInfo.strIRef.size)
            impl->strData[
                itemInfo.strIRef.pos + itemInfo.contextSepPos] = 0;

        ++itemTypeCount[itemInfo.flags];

        if (sourceStrIRef.size == 0) {
            if (headerEntryIdx)
                throw Error{
                    "Duplicate header entry (empty source string) at "
                    "offset " + toStr(strTables.source[i].pos)
                    + ". The first one was at "
                    + toStr(strTables.source[*headerEntryIdx].pos)};

            headerEntryIdx = i;
        }
    }

    assert(
        std::accumulate(
            itemTypeCount.begin(), itemTypeCount.end(), U32{})
        == strCount);

    const auto resizeItems =
    [&itemTypeCount](auto& items, ItemFlags itemFlags)
    {
        items.resize(itemTypeCount[itemFlags]);
    };

    resizeItems(impl->items, ItemFlagNone);
    resizeItems(impl->contextItems, ItemFlagContext);
    resizeItems(impl->pluralItems, ItemFlagPlural);
    resizeItems(
        impl->pluralContextItems, ItemFlagPlural | ItemFlagContext);

    // Returns the index of the first plural form in
    // Impl::pluralStrIRefs.
    auto pushPluralForms =
    [
        &strData = impl->strData,
        &pluralStrIRefs = impl->pluralStrIRefs,
        &pluralForms = impl->pluralForms,
        pluralStrIRefsPos = U32{}]
    (StrIRef strIRef) mutable
    {
        assert(pluralForms);
        const auto numPlurals = pluralForms->numPlurals;

        assert(numPlurals > 0);
        assert(pluralStrIRefsPos <= pluralStrIRefs.size());
        assert(
            numPlurals <= pluralStrIRefs.size() - pluralStrIRefsPos);

        const auto result = pluralStrIRefsPos;

        auto s = strIRef.getView(strData);
        for (U32 i{}; i < numPlurals - 1; ++i) {
            const auto sepPos = s.find(pluralSep);
            if (sepPos == s.npos)
                throw Error{
                    "Less than " + toStr(numPlurals) + " plural "
                    "forms in translated string (missing null "
                    "separators)"};

            pluralStrIRefs[pluralStrIRefsPos++] = {
                strIRef.pos, static_cast<U32>(sepPos)};

            const auto numRemove = sepPos + 1;

            strIRef.pos += numRemove;
            strIRef.size -= numRemove;
            s.remove_prefix(numRemove);
        }

        pluralStrIRefs[pluralStrIRefsPos++] = strIRef;
        return result;
    };

    std::array<U32, numItemFlagCombos> itemArrayPos{};

    const auto assignTranslatedStr = [&](U32 idx, StrIRef strIRef)
    {
        assert(idx < itemInfos.size());
        const auto& itemInfo = itemInfos[idx];
        const auto itemFlags = itemInfo.flags;

        const auto pos = itemArrayPos[itemFlags]++;
        assert(pos < itemTypeCount[itemFlags]);

        switch (itemFlags) {
        case ItemFlagNone:
            impl->items[pos] = {itemInfo.getId(), strIRef};
            break;
        case ItemFlagPlural:
            impl->pluralItems[pos] = {
                itemInfo.getId(), pushPluralForms(strIRef)};
            break;
        case ItemFlagContext:
            impl->contextItems[pos] = {
                itemInfo.getContext(), itemInfo.getId(), strIRef};
            break;
        case ItemFlagPlural | ItemFlagContext:
            impl->pluralContextItems[pos] = {
                itemInfo.getContext(),
                itemInfo.getId(),
                pushPluralForms(strIRef)};
            break;
        };
    };

    auto processHeader =
    [&pluralForms = impl->pluralForms](std::string_view str) mutable
    {
        GnuHeader header;
        try {
            header = GnuHeader::parse(str);
        } catch (const Error& e) {
            throw Error{"Can't parse header entry: "s + e.what()};
        }

        if (!header.charset.empty()
                && !equalIgnoreCase(header.charset, "UTF-8"))
            throw Error{
                "Unsupported charset "
                "\"" + std::string{header.charset} + "\"; "
                "use \"UTF-8\" instead"};

        if (!header.pluralForms)
            return;

        if (header.pluralForms->nplurals == 0)
            throw Error{"Number of plural forms must be > 0"};

        // The maximum number of plural forms for a real-world
        // language is 6, which is used by Arabic and a few other
        // languages.
        //
        // https://www.gnu.org/software/gettext/manual/html_node/Plural-forms.html
        // https://www.unicode.org/cldr/charts/48/supplemental/language_plural_rules.html
        // https://docs.translatehouse.org/projects/localization-guide/en/latest/l10n/pluralforms.html
        //
        // We use a slightly larger limit, but not too much since
        // this number is used to calculate the size of an array.
        static const CountT maxPluralForms{15};
        // Ensure that our limits are sane for the edge case when all
        // strings have plural forms.
        static_assert(U32Max / maxPluralForms >= strCountLimit);

        if (header.pluralForms->nplurals > maxPluralForms)
            throw Error{
                "Suspiciously large number of plural forms "
                + toStr(header.pluralForms->nplurals) + "; limit "
                "is " + toStr(maxPluralForms)};

        try {
            pluralForms.emplace(
                header.pluralForms->nplurals,
                header.pluralForms->plural);
        } catch (const Error& e) {
            throw Error{
                "Can't compile plural form expression \""
                + std::string{header.pluralForms->plural}
                + "\": " + e.what()};
        }
    };

    if (headerEntryIdx) {
        StrIRef headerStrIRef;
        try {
            headerStrIRef = readStr(
                strTables.translated[*headerEntryIdx]);
        } catch (const Error& e) {
            throw Error{"Can't read header entry: "s + e.what()};
        }

        try {
            processHeader(headerStrIRef.getView(impl->strData));
        } catch (const Error& e) {
            throw Error{"Can't process header entry: "s + e.what()};
        }

        assignTranslatedStr(*headerEntryIdx, headerStrIRef);
    }

    if (!impl->pluralForms)
        // If there is no header or if the header doesn't have a
        // "Plural-Forms" entry, follow the gettext behavior and use
        // the standard Germanic plural rule.
        impl->pluralForms.emplace(2, "n == 1 ? 0 : 1");

    const auto numPluralItems =
        itemTypeCount[ItemFlagPlural]
        + itemTypeCount[ItemFlagContext | ItemFlagPlural];

    // The numbers are already checked against sane limits. Double
    // check anyway.
    if (U32Max / impl->pluralForms->numPlurals < numPluralItems)
        throw Error{
            "Too large number of plural strings ("
            + toStr(numPluralItems) + ") and forms ("
            + toStr(impl->pluralForms->numPlurals) + "): "
            "multiplication will overflow U32 ("
            + toStr(U32Max) + ")"};

    impl->pluralStrIRefs.resize(
        numPluralItems * impl->pluralForms->numPlurals);

    for (U32 i{}; i < strCount; ++i) {
        if (headerEntryIdx && i == *headerEntryIdx)
            continue;

        StrIRef translatedStrIRef;
        try {
            translatedStrIRef = readStr(strTables.translated[i]);
        } catch (const Error& e) {
            throw Error{"Can't read translated string: "s + e.what()};
        }

        assignTranslatedStr(i, translatedStrIRef);
    }

    assert(itemArrayPos == itemTypeCount);

    impl->sort(impl->items);
    impl->sort(impl->pluralItems);
    impl->sort(impl->contextItems);
    impl->sort(impl->pluralContextItems);
}


MessageCatalog::~MessageCatalog() = default;


CStrRef MessageCatalog::translate(std::string_view id) const
{
    return impl->translate(impl->items, id);
}


CStrRef MessageCatalog::translate(
    std::string_view context, std::string_view id) const
{
    return impl->translate(impl->contextItems, {context, id});
}


CStrRef MessageCatalog::translate(
    CountT n, std::string_view id) const
{
    return impl->translate(impl->pluralItems, n, id);
}


CStrRef MessageCatalog::translate(
    std::string_view context, CountT n, std::string_view id) const
{
    return impl->translate(
        impl->pluralContextItems, n, {context, id});
}


}
