//============================================================================
//                                  I B E X                                   
// File        : ibex_InHC4Revise.h
// Author      : Gilles Chabert
// Copyright   : Ecole des Mines de Nantes (France)
// License     : See the LICENSE file
// Created     : Jul 12, 2012
// Last Update : Jul 12, 2012
//============================================================================

#ifndef __IBEX_IN_HC4_REVISE_H__
#define __IBEX_IN_HC4_REVISE_H__

#include "ibex_Eval.h"
#include "ibex_Exception.h"
#include "ibex_InnerArith.h"

namespace ibex {

class InHC4Revise : public BwdAlgorithm {
public:
	InHC4Revise(Eval& e);

	void iproj(const Domain& y, IntervalVector& x);

	void iproj(const Domain& y, IntervalVector& x, const IntervalVector& xin);

	/*
	 * Return true if inHC4 can be run without making the
	 * program crash with a "not implemented" message.
	 *
	 * This hack is necessary as many operators are sill not implemented.
	 */
	bool implemented() const;

	Function& f;

	Eval& eval;
	ExprDomain& d;

	Eval p_eval;
	ExprDomain& p;

protected:
	// \return false iff a domain emptied (a contradiction).
	bool iproj(const Domain& y, Array<Domain>& x, const Array<Domain>& xin);

public: // because called from CompiledFunction

	inline bool symbol_bwd (int)                    { return true; }
	inline bool cst_bwd    (int y)                  { /* TODO: improve this. */ return !(d[y]!=((const ExprConstant&) f.nodes[y]).get()); }
	inline bool idx_bwd    (int , int)              { return true; }
	       bool idx_cp_bwd (int , int);
	       bool vector_bwd (int* , int)             { not_implemented("Inner projection of \"vector\""); }
	inline bool apply_bwd  (int* x, int y);
	inline bool chi_bwd    (int, int, int, int)     { not_implemented("Inner projection of \"chi\""); }
	inline bool add_bwd    (int x1, int x2, int y)  { return ibwd_add(d[y].i(),d[x1].i(),d[x2].i(),p[x1].i(),p[x2].i()); }
	inline bool gen2_bwd   (int , int , int)        { not_implemented("Inner projection of binary generic operator"); }
	inline bool add_V_bwd  (int , int , int)        { not_implemented("Inner projection of \"add_V\""); }
	inline bool add_M_bwd  (int , int , int)        { not_implemented("Inner projection of \"add_M\""); }
	inline bool mul_bwd    (int x1, int x2, int y)  { return ibwd_mul(d[y].i(),d[x1].i(),d[x2].i(),p[x1].i(),p[x2].i()); }
	inline bool mul_SV_bwd (int , int , int)        { not_implemented("Inner projection of \"mul_SV\""); }
	inline bool mul_SM_bwd (int , int , int)        { not_implemented("Inner projection of \"mul_SM\""); }
	inline bool mul_VV_bwd (int , int , int)        { not_implemented("Inner projection of \"mul_VV\""); }
	inline bool mul_MV_bwd (int , int , int)        { not_implemented("Inner projection of \"mul_MV\""); }
	inline bool mul_VM_bwd (int , int , int)        { not_implemented("Inner projection of \"mul_VM\""); }
	inline bool mul_MM_bwd (int , int , int)        { not_implemented("Inner projection of \"mul_MM\""); }
	inline bool sub_bwd    (int x1, int x2, int y)  { return ibwd_sub(d[y].i(),d[x1].i(),d[x2].i(),p[x1].i(),p[x2].i()); }
	inline bool sub_V_bwd  (int , int, int)         { not_implemented("Inner projection of \"sub_V\""); }
	inline bool sub_M_bwd  (int , int, int)         { not_implemented("Inner projection of \"sub_M\""); }
	inline bool div_bwd    (int x1, int x2, int y)  { return ibwd_div(d[y].i(),d[x1].i(),d[x2].i(),p[x1].i(),p[x2].i()); }
	inline bool max_bwd    (int x1, int x2, int y)  { return ibwd_max(d[y].i(),d[x1].i(),d[x2].i(),p[x1].i(),p[x2].i()); }
	inline bool min_bwd    (int x1, int x2, int y)  { return ibwd_min(d[y].i(),d[x1].i(),d[x2].i(),p[x1].i(),p[x2].i()); }
	inline bool atan2_bwd  (int , int , int)        { not_implemented("Inner projection of \"atan2\""); }
	inline bool gen1_bwd   (int , int)              { not_implemented("Inner projection of generic unary operator"); }
	inline bool minus_bwd  (int x, int y)           { return ibwd_minus(d[y].i(),d[x].i()); }
	inline bool minus_V_bwd(int x, int y)           { not_implemented("Inner projection of \"minus_V\""); }
	inline bool minus_M_bwd(int x, int y)           { not_implemented("Inner projection of \"minus_M\""); }
    inline bool trans_V_bwd(int , int)              { not_implemented("Inner projection of \"transpose\""); }
    inline bool trans_M_bwd(int , int)              { not_implemented("Inner projection of \"transpose\""); }
	inline bool sign_bwd   (int , int)              { not_implemented("Inner projection of \"sign\""); }
	inline bool abs_bwd    (int x, int y)           { return ibwd_abs(d[y].i(),d[x].i()); }
	inline bool power_bwd  (int x, int y, int expo) { return ibwd_pow(d[y].i(),d[x].i(),expo,p[x].i()); }
	inline bool sqr_bwd    (int x, int y)           { return ibwd_sqr(d[y].i(),d[x].i(),p[x].i()); }
	inline bool sqrt_bwd   (int x, int y)           { return ibwd_sqrt(d[y].i(),d[x].i()); }
	inline bool exp_bwd    (int x, int y)           { return ibwd_exp(d[y].i(),d[x].i()); }
	inline bool log_bwd    (int x, int y)           { return ibwd_log(d[y].i(),d[x].i()); }
	inline bool cos_bwd    (int x, int y)           { return ibwd_cos(d[y].i(),d[x].i(),p[x].i()); }
	inline bool sin_bwd    (int x, int y)           { return ibwd_sin(d[y].i(),d[x].i(),p[x].i()); }
	inline bool tan_bwd    (int x, int y)           { return ibwd_tan(d[y].i(),d[x].i(),p[x].i()); }
	inline bool cosh_bwd   (int , int)              { not_implemented("Inner projection of \"cosh\""); }
	inline bool sinh_bwd   (int , int)              { not_implemented("Inner projection of \"sinh\""); }
	inline bool tanh_bwd   (int , int)              { not_implemented("Inner projection of \"tanh\""); }
	inline bool acos_bwd   (int , int)              { not_implemented("Inner projection of \"acos\""); }
	inline bool asin_bwd   (int , int)              { not_implemented("Inner projection of \"asin\""); }
	inline bool atan_bwd   (int , int)              { not_implemented("Inner projection of \"atan\""); }
	inline bool acosh_bwd  (int , int)              { not_implemented("Inner projection of \"acosh\""); }
	inline bool asinh_bwd  (int , int)              { not_implemented("Inner projection of \"asinh\""); }
	inline bool atanh_bwd  (int , int)              { not_implemented("Inner projection of \"atanh\""); }
	inline bool floor_bwd  (int , int)              { not_implemented("Inner projection of \"floor\""); }
	inline bool ceil_bwd   (int , int)              { not_implemented("Inner projection of \"ceil\""); }
	inline bool saw_bwd   (int , int)               { not_implemented("Inner projection of \"saw\""); }
};

} // end namespace ibex

#endif // __IBEX_IN_HC4_REVISE_H__
