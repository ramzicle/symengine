/**
 *  \file visitor.h
 *  Class Visitor
 *
 **/
#ifndef SYMENGINE_VISITOR_H
#define SYMENGINE_VISITOR_H

#include <symengine/basic.h>
#include <symengine/add.h>
#include <symengine/mul.h>
#include <symengine/pow.h>
#include <symengine/polynomial.h>
#include <symengine/functions.h>
#include <symengine/symbol.h>
#include <symengine/integer.h>
#include <symengine/rational.h>
#include <symengine/complex.h>
#include <symengine/constants.h>
#include <symengine/real_double.h>
#include <symengine/complex_double.h>
#include <symengine/real_mpfr.h>
#include <symengine/complex_mpc.h>
#include <symengine/series_generic.h>
#include <symengine/series.h>
#include <symengine/series_piranha.h>
#include <symengine/series_flint.h>

namespace SymEngine {

class Visitor {
public:
#   define SYMENGINE_ENUM( TypeID , Class) \
    virtual void visit(const Class &) = 0;
#   include "symengine/type_codes.inc"
#   undef SYMENGINE_ENUM
};

void preorder_traversal(const Basic &b, Visitor &v);
void postorder_traversal(const Basic &b, Visitor &v);

template<class Derived, class Base = Visitor>
class BaseVisitor : public Base {

public:
#   define SYMENGINE_ENUM( TypeID , Class) \
    virtual void visit(const Class &x) { static_cast<Derived*>(this)->bvisit(x); };
#   include "symengine/type_codes.inc"
#   undef SYMENGINE_ENUM
};

class StopVisitor : public Visitor {
public:
    bool stop_;
};

void preorder_traversal_stop(const Basic &b, StopVisitor &v);

class HasSymbolVisitor : public BaseVisitor<HasSymbolVisitor, StopVisitor> {
protected:
    RCP<const Symbol> x_;
    bool has_;
public:

    void bvisit(const Symbol &x) {
        if (x_->__eq__(x)) {
            has_ = true;
            stop_ = true;
        }
    }

    void bvisit(const Basic &x) { };

    bool apply(const Basic &b, const RCP<const Symbol> &x) {
        x_ = x;
        has_ = false;
        stop_ = false;
        preorder_traversal_stop(b, *this);
        return has_;
    }
};

bool has_symbol(const Basic &b, const RCP<const Symbol> &x);

class CoeffVisitor : public BaseVisitor<CoeffVisitor, StopVisitor> {
protected:
    RCP<const Symbol> x_;
    RCP<const Basic> n_;
    RCP<const Basic> coeff_;
public:

    void bvisit(const Add &x) {
        umap_basic_num dict;
        RCP<const Number> coef = zero;
        for (auto &p: x.dict_) {
            p.first->accept(*this);
            if (neq(*coeff_, *zero)) {
                Add::coef_dict_add_term(outArg(coef), dict, p.second, coeff_);
            }
        }
        coeff_ = Add::from_dict(coef, std::move(dict));
    }

    void bvisit(const Mul &x) {
        for (auto &p: x.dict_) {
            if (eq(*p.first, *x_) and eq(*p.second, *n_)) {
                map_basic_basic dict = x.dict_;
                dict.erase(p.first);
                coeff_ = Mul::from_dict(x.coef_, std::move(dict));
                return;
            }
        }
        coeff_ = zero;
    }

    void bvisit(const Pow &x) {
        if (eq(*x.get_base(), *x_) and eq(*x.get_exp(), *n_)) {
            coeff_ = one;
        }
    }

    void bvisit(const Symbol &x) {
        if (eq(x, *x_) and eq(*one, *n_)) {
            coeff_ = one;
        } else {
            coeff_ = zero;
        }
    }

    void bvisit(const Basic &x) {
        coeff_ = zero;
    }

    RCP<const Basic> apply(const Basic &b, const RCP<const Symbol> &x,
            const RCP<const Basic> &n) {
        x_ = x;
        n_ = n;
        coeff_ = zero;
        b.accept(*this);
        return coeff_;
    }
};

RCP<const Basic> coeff(const Basic &b, const RCP<const Basic> &x,
        const RCP<const Basic> &n);

set_basic free_symbols(const Basic &b);

class NeedsSymbolicExpansionVisitor : public BaseVisitor<NeedsSymbolicExpansionVisitor, StopVisitor> {
protected:
    RCP<const Symbol> x_;
    bool needs_;
public:

    void bvisit(const TrigFunction &f) {
        auto arg = f.get_arg();
        map_basic_basic subsx0{{x_, integer(0)}};
        if (arg->subs(subsx0)->__neq__(*integer(0))) {
            needs_ = true;
            stop_ = true;
        }
    }

    void bvisit(const HyperbolicFunction &f) {
        auto arg = f.get_arg();
        map_basic_basic subsx0{{x_, integer(0)}};
        if (arg->subs(subsx0)->__neq__(*integer(0))) {
            needs_ = true;
            stop_ = true;
        }
    }

    void bvisit(const Pow &pow) {
        auto base = pow.get_base();
        auto exp = pow.get_exp();
        map_basic_basic subsx0{{x_, integer(0)}};
        // exp(const) or x^-1
        if ((base->__eq__(*E) and exp->subs(subsx0)->__neq__(*integer(0)))
            or (is_a_Number(*exp) and static_cast<const Number&>(*exp).is_negative()
                and base->subs(subsx0)->__eq__(*integer(0)))) {
            needs_ = true;
            stop_ = true;
        }
    }

    void bvisit(const Log &f) {
        auto arg = f.get_arg();
        map_basic_basic subsx0{{x_, integer(0)}};
        if (arg->subs(subsx0)->__eq__(*integer(0))) {
            needs_ = true;
            stop_ = true;
        }
    }

    void bvisit(const LambertW &x) { needs_ = true; stop_ = true; }

    void bvisit(const Basic &x) { }

    bool apply(const Basic &b, const RCP<const Symbol> &x) {
        x_ = x;
        needs_ = false;
        stop_ = false;
        preorder_traversal_stop(b, *this);
        return needs_;
    }
};

} // SymEngine

#endif
