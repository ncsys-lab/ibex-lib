//============================================================================
//                                  I B E X                                   
// File        : HC4Revise Algorithm (The famous forward-backward contraction algorithm).
// Author      : Gilles Chabert
// Copyright   : Ecole des Mines de Nantes (France)
// License     : See the LICENSE file
// Created     : Dec 31, 2011
// Last Update : 
//============================================================================

#ifndef __IBEX_HC4_REVISE_H__
#define __IBEX_HC4_REVISE_H__

#include "ibex_Eval.h"

namespace ibex {

/**
 * \ingroup symbolic
 * \brief The famous forward-backward contraction algorithm.
 *
 */
class HC4Revise : public BwdAlgorithm {
public:

	/**
	 * \brief Build the gradient algorithm.
	 *
	 * For memory saving, the gradient is built from an
	 * already existing evaluator "e".
	 */
	HC4Revise(Eval& e);

	/**
	 * \brief Project f(x)=y onto x (forward/backward algorithm)
	 *
	 * \brief true if f(x) is included in y (inactive constraint)
	 *
	 * \note if x is outside the definition domain of f, then
	 *       x is set to the empty set although f([x])\subseteq [y]
	 *       and the return value is "false".
	 */
	bool proj(const Domain& y, IntervalVector& x, const std::function<void(int index, const Interval &old_value, const Interval &new_value)> &callback);

	/**
	 * \brief Ratio for the contraction of a
	 * matrix-vector / matrix-matrix multiplication.
	 *
	 * Set to 0.1.
	 */
	static constexpr double RATIO = 0.1;

protected:
	/**
	 * Contract x w.r.t. f(x)=y, with forward + backward.
	 *
	 * \return false iff a domain emptied (a contradiction).
	 */
//	bool proj(const Domain& y, const Array<const Domain>& x);

	/**
	 * Contract x w.r.t. f(x)=y, with forward + backward.
	 *
	 * \return false iff a domain emptied (a contradiction).
	 */
	bool proj(const Domain& y, Array<Domain>& x);

	/**
	 * Backward of f(x)=y.
	 *
	 * \return false iff a domain emptied (root intersection or a deep *_bwd
	 *         contradiction); the caller then marks the box empty. No
	 *         exception is thrown.
	 */
	bool backward(const Domain& y);

	Function& f;
	Eval& eval;
	ExprDomain& d;

public: // because called from CompiledFunction
	inline bool idx_bwd    (int, int)          { return true; }
	       bool idx_cp_bwd (int, int);
	       bool vector_bwd (int* x, int y);
	inline bool symbol_bwd (int)                 { return true; }
	inline bool cst_bwd    (int)                 { return true; }
	       bool apply_bwd  (int* x, int y);
	inline bool chi_bwd(int a, int b, int c, int y){ return bwd_chi(d[y].i(),d[a].i(),d[b].i(),d[c].i()); }
	       bool gen2_bwd   (int x1, int x2, int y);
	inline bool add_bwd    (int x1, int x2, int y) { return bwd_add(d[y].i(),d[x1].i(),d[x2].i()); }
	inline bool add_V_bwd  (int x1, int x2, int y) { return bwd_add(d[y].v(),d[x1].v(),d[x2].v()); }
	inline bool add_M_bwd  (int x1, int x2, int y) { return bwd_add(d[y].m(),d[x1].m(),d[x2].m()); }
	inline bool mul_bwd    (int x1, int x2, int y) { return bwd_mul(d[y].i(),d[x1].i(),d[x2].i()); }
	inline bool mul_SV_bwd (int x1, int x2, int y) { return bwd_mul(d[y].v(),d[x1].i(),d[x2].v()); }
	inline bool mul_SM_bwd (int x1, int x2, int y) { return bwd_mul(d[y].m(),d[x1].i(),d[x2].m()); }
	inline bool mul_VV_bwd (int x1, int x2, int y) { return bwd_mul(d[y].i(),d[x1].v(),d[x2].v()); }
	inline bool mul_MV_bwd (int x1, int x2, int y) { return bwd_mul(d[y].v(),d[x1].m(),d[x2].v(), RATIO); }
	inline bool mul_VM_bwd (int x1, int x2, int y) { return bwd_mul(d[y].v(),d[x1].v(),d[x2].m(), RATIO); }
	inline bool mul_MM_bwd (int x1, int x2, int y) { return bwd_mul(d[y].m(),d[x1].m(),d[x2].m(), RATIO); }
	inline bool sub_bwd    (int x1, int x2, int y) { return bwd_sub(d[y].i(),d[x1].i(),d[x2].i()); }
	inline bool sub_V_bwd  (int x1, int x2, int y) { return bwd_sub(d[y].v(),d[x1].v(),d[x2].v()); }
	inline bool sub_M_bwd  (int x1, int x2, int y) { return bwd_sub(d[y].m(),d[x1].m(),d[x2].m()); }
	inline bool div_bwd    (int x1, int x2, int y) { return bwd_div(d[y].i(),d[x1].i(),d[x2].i()); }
	inline bool max_bwd    (int x1, int x2, int y) { return bwd_max(d[y].i(),d[x1].i(),d[x2].i()); }
	inline bool min_bwd    (int x1, int x2, int y) { return bwd_min(d[y].i(),d[x1].i(),d[x2].i()); }
	inline bool atan2_bwd  (int x1, int x2, int y) { return bwd_atan2(d[y].i(),d[x1].i(),d[x2].i()); }
	       bool gen1_bwd   (int x, int y);
	inline bool minus_bwd  (int x, int y)          { return !((d[x].i() &=-d[y].i())).is_empty(); }
	inline bool minus_V_bwd(int x, int y)          { return !((d[x].v() &=-d[y].v())).is_empty(); }
	inline bool minus_M_bwd(int x, int y)          { return !((d[x].m() &=-d[y].m())).is_empty(); }
    inline bool trans_V_bwd(int x, int y)          { return !((d[x].v() &= d[y].v())).is_empty(); }
    inline bool trans_M_bwd(int x, int y)          { return !((d[x].m() &= d[y].m().transpose())).is_empty(); }
	inline bool sign_bwd   (int x, int y)          { return bwd_sign(d[y].i(),d[x].i()); }
	inline bool abs_bwd    (int x, int y)          { return bwd_abs(d[y].i(),d[x].i()); }
	inline bool power_bwd  (int x, int y, int p)   { return bwd_pow(d[y].i(),p, d[x].i()); }
	inline bool sqr_bwd    (int x, int y)          { return bwd_sqr(d[y].i(),d[x].i()); }
	inline bool sqrt_bwd   (int x, int y)          { return bwd_sqrt(d[y].i(),d[x].i()); }
	inline bool exp_bwd    (int x, int y)          { return bwd_exp(d[y].i(),d[x].i()); }
	inline bool log_bwd    (int x, int y)          { return bwd_log(d[y].i(),d[x].i()); }
	inline bool cos_bwd    (int x, int y)          { return bwd_cos(d[y].i(),d[x].i()); }
	inline bool sin_bwd    (int x, int y)          { return bwd_sin(d[y].i(),d[x].i()); }
	inline bool tan_bwd    (int x, int y)          { return bwd_tan(d[y].i(),d[x].i()); }
	inline bool cosh_bwd   (int x, int y)          { return bwd_cosh(d[y].i(),d[x].i()); }
	inline bool sinh_bwd   (int x, int y)          { return bwd_sinh(d[y].i(),d[x].i()); }
	inline bool tanh_bwd   (int x, int y)          { return bwd_tanh(d[y].i(),d[x].i()); }
	inline bool acos_bwd   (int x, int y)          { return bwd_acos(d[y].i(),d[x].i()); }
	inline bool asin_bwd   (int x, int y)          { return bwd_asin(d[y].i(),d[x].i()); }
	inline bool atan_bwd   (int x, int y)          { return bwd_atan(d[y].i(),d[x].i()); }
	inline bool acosh_bwd  (int x, int y)          { return bwd_acosh(d[y].i(),d[x].i()); }
	inline bool asinh_bwd  (int x, int y)          { return bwd_asinh(d[y].i(),d[x].i()); }
	inline bool atanh_bwd  (int x, int y)          { return bwd_atanh(d[y].i(),d[x].i()); }
	inline bool floor_bwd  (int x, int y)          { return bwd_floor(d[y].i(),d[x].i()); }
	inline bool ceil_bwd  (int x, int y)           { return bwd_ceil(d[y].i(),d[x].i()); }
	inline bool saw_bwd  (int x, int y)            { return bwd_saw(d[y].i(),d[x].i()); }
};

} // namespace ibex

#endif // __IBEX_HC4_REVISE_H__
