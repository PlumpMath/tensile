/*
 * Copyright (c) 2015  Artem V. Andreev
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */
/**
 * @brief expression operator declarations
 *
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 */
#ifndef OPS_H
#define OPS_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include "engine.h"

#define DECLARE_EXPR_OP(_name)                  \
    extern const operator_info expr_op_##_name;

DECLARE_EXPR_OP(assignment);
DECLARE_EXPR_OP(link);
DECLARE_EXPR_OP(const);
DECLARE_EXPR_OP(plus_assign);
DECLARE_EXPR_OP(join_assign);
DECLARE_EXPR_OP(default_assign);
DECLARE_EXPR_OP(local_assign);
DECLARE_EXPR_OP(make_iter);

DECLARE_EXPR_OP(uplus);
DECLARE_EXPR_OP(uminus);
DECLARE_EXPR_OP(noop);
DECLARE_EXPR_OP(floor);
DECLARE_EXPR_OP(frac);
DECLARE_EXPR_OP(typeof);
DECLARE_EXPR_OP(loosen);
DECLARE_EXPR_OP(length);
DECLARE_EXPR_OP(sort);
DECLARE_EXPR_OP(plus);
DECLARE_EXPR_OP(minus);
DECLARE_EXPR_OP(mul);
DECLARE_EXPR_OP(div);
DECLARE_EXPR_OP(rem);
DECLARE_EXPR_OP(join);
DECLARE_EXPR_OP(power);
DECLARE_EXPR_OP(defaulted);
DECLARE_EXPR_OP(merge);
DECLARE_EXPR_OP(max);
DECLARE_EXPR_OP(min);
DECLARE_EXPR_OP(max_ci);
DECLARE_EXPR_OP(min_ci);

DECLARE_EXPR_OP(aggregate_sum);
DECLARE_EXPR_OP(aggregate_prod);
DECLARE_EXPR_OP(aggregate_join);
DECLARE_EXPR_OP(aggregate_avg);
DECLARE_EXPR_OP(aggregate_min);
DECLARE_EXPR_OP(aggregate_max);
DECLARE_EXPR_OP(aggregate_min_ci);
DECLARE_EXPR_OP(aggregate_max_ci);

DECLARE_EXPR_OP(getfield);
DECLARE_EXPR_OP(children);
DECLARE_EXPR_OP(count);
DECLARE_EXPR_OP(parent);
DECLARE_EXPR_OP(item);
DECLARE_EXPR_OP(range);
DECLARE_EXPR_OP(makenode);
DECLARE_EXPR_OP(bind);

#define DECLARE_PREDICATE(_name) \
    extern const action_def predicate_##_name

DECLARE_PREDICATE(eq);
DECLARE_PREDICATE(ne);
DECLARE_PREDICATE(less);
DECLARE_PREDICATE(greater);
DECLARE_PREDICATE(le);
DECLARE_PREDICATE(ge);
DECLARE_PREDICATE(match);
DECLARE_PREDICATE(not_match);

DECLARE_PREDICATE(eq_ci);
DECLARE_PREDICATE(ne_ci);
DECLARE_PREDICATE(less_ci);
DECLARE_PREDICATE(greater_ci);
DECLARE_PREDICATE(le_ci);
DECLARE_PREDICATE(ge_ci);
DECLARE_PREDICATE(match_ci);
DECLARE_PREDICATE(not_match_ci);

DECLARE_PREDICATE(identity);
DECLARE_PREDICATE(not_identity);

DECLARE_PREDICATE(has_var);
DECLARE_PREDICATE(has_field);
DECLARE_PREDICATE(has_children);
DECLARE_PREDICATE(has_parent);

DECLARE_PREDICATE(trf_negate);
DECLARE_PREDICATE(trf_alt);
DECLARE_PREDICATE(trf_and_not);
DECLARE_PREDICATE(trf_and);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* OPS_H */
