#include "plural/eval.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "dp_intl/error.h"
#include "plural/token.h"
#include "plural/token_map.h"
#include "plural/token_stream.h"
#include "plural/tokenizer.h"


// The Gettext documentation provides an extremely vague description
// of plural expressions, so the only authoritative source of
// information on how to parse and evaluate them is the code:
//
// * gettext-runtime/intl/plural.y
// * gettext-runtime/intl/eval-plural.h


// We use a Pratt parser and its standard terminology:
//
// * NUD - "null denotation", an operation applied when there's no
//   expression to the left of the token. This is used by values and
//   prefix operators.
//
// * LED - "left denotation", an operation applied when the token has
//   an expression to the left. This is used by infix and suffix
//   operators.


namespace dp::intl::plural {
namespace {


struct EvalCtx;


struct Node {
    using Idx = std::uint16_t;
    using EvalFn = CountT (&)(EvalCtx ctx);

    EvalFn evalFn;
    CountT value{};
    std::array<Idx, 3> children{};
};


using Nodes = std::vector<Node>;


struct EvalCtx {
    const Node& self;
    const Nodes& nodes;
    CountT n;

    template<std::size_t childIdx>
    CountT evalChild() const
    {
        const auto childNodeIdx = std::get<childIdx>(self.children);

        assert(childNodeIdx < nodes.size());
        const auto& child = nodes[childNodeIdx];
        return child.evalFn({child, nodes, n});
    }
};


namespace evalFn {


CountT num(EvalCtx ctx)
{
    return ctx.self.value;
}


CountT varN(EvalCtx ctx)
{
    return ctx.n;
}


// Adapter for binary functors from <functional>. It's mainly intended
// for comparisons, since in other cases we need to detect arithmetic
// errors (e.g., overflow and division by 0) and perform short-circuit
// evaluation.
template<typename Op>
CountT bin(EvalCtx ctx)
{
    return Op{}(ctx.evalChild<0>(), ctx.evalChild<1>());
}


CountT logicalAnd(EvalCtx ctx)
{
    return ctx.evalChild<0>() && ctx.evalChild<1>();
}


CountT logicalOr(EvalCtx ctx)
{
    return ctx.evalChild<0>() || ctx.evalChild<1>();
}


CountT logicalNot(EvalCtx ctx)
{
    return !ctx.evalChild<0>();
}


CountT plus(EvalCtx ctx)
{
    const auto a = ctx.evalChild<0>();
    const auto b = ctx.evalChild<1>();

    if (std::numeric_limits<CountT>::max() - a < b)
        throw Error{"Integer overflow on +"};

    return a + b;
}


CountT minus(EvalCtx ctx)
{
    const auto a = ctx.evalChild<0>();
    const auto b = ctx.evalChild<1>();

    if (a < b)
        throw Error{"Integer underflow on -"};

    return a - b;
}


CountT mul(EvalCtx ctx)
{
    const auto a = ctx.evalChild<0>();
    const auto b = ctx.evalChild<1>();

    if (b == 0)
        return 0;

    if (a > std::numeric_limits<CountT>::max() / b)
        throw Error{"Integer overflow on *"};

    return a * b;
}


CountT div(EvalCtx ctx)
{
    const auto a = ctx.evalChild<0>();
    const auto b = ctx.evalChild<1>();

    if (b == 0)
        throw Error{"Division by 0 in /"};

    return a / b;
}


CountT mod(EvalCtx ctx)
{
    const auto a = ctx.evalChild<0>();
    const auto b = ctx.evalChild<1>();

    if (b == 0)
        throw Error{"Division by 0 in %"};

    return a % b;
}


CountT conditional(EvalCtx ctx)
{
    return ctx.evalChild<0>()
        ? ctx.evalChild<1>() : ctx.evalChild<2>();
}


}


struct ParsingCtx {
    TokenStream tokens;
    Nodes nodes;

    Node::Idx addNode(const Node& node)
    {
        if (nodes.size() == std::numeric_limits<Node::Idx>::max())
            throw Error{"Out of nodes; expression is too complex"};

        nodes.push_back(node);
        return nodes.size() - 1;
    }
};


Node::Idx parse(ParsingCtx& ctx, int minBindingPower);


Node::Idx nudNum(ParsingCtx& ctx, const Token& token)
{
    return ctx.addNode({evalFn::num, token.val});
}


Node::Idx nudVarN(ParsingCtx& ctx, const Token&)
{
    return ctx.addNode({evalFn::varN});
}


Node::Idx nudLParen(ParsingCtx& ctx, const Token&)
{
    const auto result = parse(ctx, 0);

    if (const auto& closingToken = ctx.tokens.consume();
            closingToken.type != Token::Type::RParen)
        throw Error{
            "Expected closing ), but got "
            "\"" + std::string{closingToken.str} + "\""};

    return result;
}


Node::Idx nudNot(ParsingCtx& ctx, const Token&)
{
    return ctx.addNode({evalFn::logicalNot, 0, parse(ctx, 100)});
}


template<Node::EvalFn evalFn>
Node::Idx ledBinary(
    ParsingCtx& ctx, Node::Idx left, const Token&, int bindingPower)
{
    // + 1 for bindingPower for left associativity.
    return ctx.addNode(
        {evalFn, 0, left, parse(ctx, bindingPower + 1)});
}


Node::Idx ledTernary(
    ParsingCtx& ctx, Node::Idx left, const Token&, int bindingPower)
{
    const auto trueBranch = parse(ctx, 0);

    if (const auto& midToken = ctx.tokens.consume();
            midToken.type != Token::Type::Colon)
        throw Error{
            "Expected : in ternary operator, but got "
            "\"" + std::string{midToken.str} + "\""};

    const auto falseBranch = parse(ctx, bindingPower);

    return ctx.addNode({
        evalFn::conditional, 0, left, trueBranch, falseBranch});
}


struct OpInfo {
    int bindingPower{};

    using Nud = Node::Idx(*)(ParsingCtx&, const Token&);
    Nud nud{};

    using Led = Node::Idx(*)(
        ParsingCtx&, Node::Idx left, const Token&, int bindingPower);
    Led led{};
};


TokenTypeMap<OpInfo> buildOpInfoMap()
{
    TokenTypeMap<OpInfo> result;

    using TT = Token::Type;

    result[TT::Num].nud = nudNum;
    result[TT::VarN].nud = nudVarN;
    result[TT::LParen].nud = nudLParen;

    result[TT::Not] = {100, nudNot};

    const auto set =
        [&result](TT type, int bindingPower, OpInfo::Led led)
        {
            result[type] = {bindingPower, {}, led};
        };

    using namespace evalFn;

    set(TT::Question,     1, ledTernary);
    set(TT::Or,           2, ledBinary<logicalOr>);
    set(TT::And,          3, ledBinary<logicalAnd>);
    set(TT::Equal,        4, ledBinary<bin<std::equal_to<>>>);
    set(TT::NotEqual,     4, ledBinary<bin<std::not_equal_to<>>>);
    set(TT::Less,         5, ledBinary<bin<std::less<>>>);
    set(TT::LessEqual,    5, ledBinary<bin<std::less_equal<>>>);
    set(TT::Greater,      5, ledBinary<bin<std::greater<>>>);
    set(TT::GreaterEqual, 5, ledBinary<bin<std::greater_equal<>>>);
    set(TT::Plus,         6, ledBinary<plus>);
    set(TT::Minus,        6, ledBinary<minus>);
    set(TT::Star,         7, ledBinary<mul>);
    set(TT::Slash,        7, ledBinary<div>);
    set(TT::Percent,      7, ledBinary<mod>);

    return result;
}


const OpInfo& getOpInfo(Token::Type type)
{
    static const auto map = buildOpInfoMap();
    return map[type];
}


Node::Idx parse(ParsingCtx& ctx, int minBindingPower)
{
    const auto& token = ctx.tokens.consume();
    const auto& firstInfo = getOpInfo(token.type);

    if (!firstInfo.nud)
        throw Error{
            "Expected literal or prefix operation, but got "
            "\""+ std::string{token.str} + "\""};

    auto left = firstInfo.nud(ctx, token);

    while (true) {
        const auto& nextToken = ctx.tokens.peek();
        const auto& nextInfo = getOpInfo(nextToken.type);

        if (!nextInfo.led || nextInfo.bindingPower < minBindingPower)
            break;

        ctx.tokens.consume();

        left = nextInfo.led(
            ctx, left, nextToken, nextInfo.bindingPower);
    }

    return left;
}


}


struct Eval::Impl {
    Nodes nodes;
    Node::Idx rootNodeIdx;

    explicit Impl(std::string_view expr)
    {
        ParsingCtx ctx{TokenStream{tokenize(expr)}, {}};

        rootNodeIdx = parse(ctx, 0);

        if (const auto& lastToken = ctx.tokens.peek();
                lastToken.type != Token::Type::End)
            throw Error{
                "Trailing \"" + std::string{lastToken.str} + "\" "
                "token"};

        nodes = std::move(ctx.nodes);
    }

    CountT eval(CountT n) const
    {
        assert(rootNodeIdx < nodes.size());
        const auto& node = nodes[rootNodeIdx];
        return node.evalFn({node, nodes, n});
    }
};


Eval::Eval(std::string_view expr)
    : impl{std::make_unique<Impl>(expr)}
{
}


Eval::~Eval() = default;


CountT Eval::operator()(CountT n) const
{
    return impl->eval(n);
}


}
