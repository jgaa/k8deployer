#include "k8deployer/exprtk_fn.h"

#include "exprtk.hpp"

namespace k8deployer {

namespace {
    // http://www.partow.net/programming/exprtk/
    template<typename T>
    T eval(const std::string& arg) {
        using expression_t = exprtk::expression<T>;
        using parser_t = exprtk::parser<T>;

        expression_t expr;
        parser_t parser;
        parser.compile(arg, expr);
        auto result = expr.value();
        return result;
    }
}

double exprtkDouble(const std::string &arg)
{
    return eval<double>(arg);
}

} // ns
