/*
auto f =
    (compile_time_pattern<is_integral<_1> >() = [](auto i) { std::cout << i; })
|   (_1 > 2 && _1 < 4 = [](auto x) { std::cout << x; })
;
*/

template <typename Pattern>
struct compile_time_pattern {
    template <typename Expression>
    constexpr bool operator()(Expression const&) const {
        return boost::proto::matches<Pattern, Expression>();
    }
};

struct fusion_or_ {
    template <typename Maybe, typename Otherwise>
    auto operator()(Maybe const& maybe, Otherwise const& otherwise) const {
        typename boost::result_of<Maybe()>::type R;
        R result = maybe();
        if (result)
            return result;
        return otherwise();
    }
};


namespace haskell { namespace pattern_matching_detail {
    namespace proto = boost::proto;

    struct eval_clause {
        template <typename Predicate, typename Function, typename Args>
        maybe<typename boost::result_of<Function(Args)>::type>
        operator()(Predicate const& predicate, Function const& function, Args const& args) const {
            if (predicate(args))
                return function(args);
            return boost::none;
        }
    };

    struct lambda_expr : proto::_ { };

    struct clause
        : proto::when<proto::assign<lambda_expr, lambda_expr>,
            eval_clause(proto::_value(proto::_left),
                        proto::_value(proto::_right),
                        proto::_data)
        >
    { };

    struct guards
        : proto::or_<
            clause
            proto::when<proto::bitwise_or<guards, clause>,
                fusion_or_(guards(proto::_left, proto::_data),
                           guards(proto::_right, proto::_data))
            >
        >
    { };


    template <typename Expr> struct expression;
    struct domain : proto::domain<proto::generator<expression>, guards> { };

    template <typename Expr>
    struct expression : proto::extends<Expr, expression<Expr>, domain> {
        typedef proto::extends<Expr, expression<Expr>, domain> base_type;
        expression(Expr const &expr = Expr()) : base_type(expr) { }
        BOOST_PROTO_EXTENDS_USING_ASSIGN(expression)

        template <typename ...Args>
        auto operator()(Args&& ...args) const ->  {

        }
    };

    template <int i>
    struct placeholder
        : expression<
            typename proto::terminal<placeholder<i> >::type
        >
    { };
}}
