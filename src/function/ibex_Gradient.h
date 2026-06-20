/* ============================================================================
 * I B E X - Gradient of a function
 * ============================================================================
 * Copyright   : Ecole des Mines de Nantes (FRANCE)
 * License     : This program can be distributed under the terms of the GNU LGPL.
 *               See the file COPYING.LESSER.
 *
 * Author(s)   : Gilles Chabert
 * Created     : Jan 27, 2012
 * ---------------------------------------------------------------------------- */

#ifndef __IBEX_GRADIENT_H__
#define __IBEX_GRADIENT_H__

#include "ibex_Eval.h"
#include "ibex_BwdAlgorithm.h"
#include "ibex_Agenda.h"

namespace ibex {

/**
 * \ingroup symbolic
 * \brief Calculates the gradient of a function.
 */
class Gradient : public FwdAlgorithm, public BwdAlgorithm {

public:
	/**
	 * \brief Build the gradient algorithm.
	 *
	 * For memory saving, the gradient is built from an
	 * already existing Eval object (note: calculating
	 * the gradient requires an evaluation). This way,
	 * the data of the evaluator is shared.
	 */
	Gradient(Eval& eval);

	/**
	 * \brief Delete this.
	 */
	~Gradient();

	/**
	 * \brief Calculate the gradient of f on the domains \a d and store the result in \a g.
	 */
	void gradient(const Array<Domain>& d, IntervalVector& g);

	/**
	 * \brief Calculate the gradient of f on the box \a box and store the result in \a g.
	 */
	void gradient(const IntervalVector& box, IntervalVector& g);

	/**
	 * \brief Calculate the Jacobian of f on the box \a box and store the result in \a J.
	 *
	 * \param v - only update the vth column of J (useful for Hansen's scheme). Default value is -1
	 *            (means: update all the columns).
	 */
	void jacobian(const IntervalVector& box, IntervalMatrix& J, int v=-1);

	/**
	 * \brief Calculate some components of the Jacobian of f on the box \a box and store the result in \a J.
	 *
	 * \param v - only update the vth column of J (useful for Hansen's scheme). Default value is -1
	 *            (means: update all the columns).
	 */
	void jacobian(const IntervalVector& box, IntervalMatrix& J, const BitSet& components, int v=-1);

	/**
	 * \brief Calculate the Jacobian on the domains \a d and store the result in \a J.
	 */
	void jacobian(const Array<Domain>& d, IntervalMatrix& J);

	/* ====================================== Forward =================================== */

	inline void idx_fwd(int , int ) { /* nothing to do */ }
	       void idx_cp_fwd(int , int y)       { g[y].clear(); }
	       void vector_fwd(int* x, int y);
	       void cst_fwd(int y)                { g[y].clear(); }
	       void symbol_fwd(int y)             { g[y].clear(); }
	       void apply_fwd(int*, int y)        { g[y].clear(); }
	inline void chi_fwd(int, int, int, int y) { g[y].i()=0; }
	inline void gen2_fwd(int, int, int y)     { g[y].clear(); }
	inline void add_fwd(int, int, int y)      { g[y].i()=0; }
	inline void mul_fwd(int, int, int y)      { g[y].i()=0; }
	inline void sub_fwd(int, int, int y)      { g[y].i()=0; }
	inline void div_fwd(int, int, int y)      { g[y].i()=0; }
	inline void max_fwd(int, int, int y)      { g[y].i()=0; }
	inline void min_fwd(int, int, int y)      { g[y].i()=0; }
	inline void atan2_fwd(int, int, int y)    { g[y].i()=0; }
	inline void gen1_fwd(int, int y)          { g[y].clear(); }
	inline void minus_fwd(int, int y)         { g[y].i()=0; }
	inline void minus_V_fwd(int, int y)       { g[y].v().clear(); }
	inline void minus_M_fwd(int, int y)       { g[y].m().clear(); }
	inline void trans_V_fwd(int, int y)       { g[y].v().clear(); }
	inline void trans_M_fwd(int, int y)       { g[y].m().clear(); }
	inline void sign_fwd(int, int y)          { g[y].i()=0; }
	inline void abs_fwd(int, int y)           { g[y].i()=0; }
	inline void power_fwd(int, int y, int)    { g[y].i()=0; }
	inline void sqr_fwd(int, int y)           { g[y].i()=0; }
	inline void sqrt_fwd(int, int y)          { g[y].i()=0; }
	inline void exp_fwd(int, int y)           { g[y].i()=0; }
	inline void log_fwd(int, int y)           { g[y].i()=0; }
	inline void cos_fwd(int, int y)           { g[y].i()=0; }
	inline void sin_fwd(int, int y)           { g[y].i()=0; }
	inline void tan_fwd(int, int y)           { g[y].i()=0; }
	inline void cosh_fwd(int, int y)          { g[y].i()=0; }
	inline void sinh_fwd(int, int y)          { g[y].i()=0; }
	inline void tanh_fwd(int, int y)          { g[y].i()=0; }
	inline void acos_fwd(int, int y)          { g[y].i()=0; }
	inline void asin_fwd(int, int y)          { g[y].i()=0; }
	inline void atan_fwd(int, int y)          { g[y].i()=0; }
	inline void acosh_fwd(int, int y)         { g[y].i()=0; }
	inline void asinh_fwd(int, int y)         { g[y].i()=0; }
	inline void atanh_fwd(int, int y)         { g[y].i()=0; }
	inline void floor_fwd(int, int y)         { g[y].i()=0; }
	inline void ceil_fwd(int, int y)          { g[y].i()=0; }
	inline void saw_fwd(int, int y)           { g[y].i()=0; }
	inline void add_V_fwd(int, int, int y)    { g[y].v().clear(); }
	inline void add_M_fwd(int, int, int y)    { g[y].m().clear(); }
	inline void mul_SV_fwd(int, int, int y)   { g[y].v().clear(); }
	inline void mul_SM_fwd(int, int, int y)   { g[y].m().clear(); }
	inline void mul_VV_fwd(int, int, int y)   { g[y].i()=0; }
	inline void mul_MV_fwd(int, int, int y)   { g[y].v().clear(); }
	inline void mul_VM_fwd(int, int, int y)   { g[y].v().clear(); }
	inline void mul_MM_fwd(int, int, int y)   { g[y].m().clear(); }
	inline void sub_V_fwd(int, int, int y)    { g[y].v().clear(); }
	inline void sub_M_fwd(int, int, int y)    { g[y].m().clear(); }

	/* ====================================== Backward =================================== */

	inline bool idx_bwd    (int, int) {  return true; }
	       bool idx_cp_bwd (int, int);
	       bool vector_bwd (int* x, int y);
	inline bool symbol_bwd (int) { /*cout << "symbol bwd=" << g[y].v() << endl;*/ /* nothing to do */  return true; }
	inline bool cst_bwd    (int) { /* nothing to do */  return true; }
	       bool apply_bwd  (int* x, int y);
	       bool chi_bwd    (int x1, int x2, int x3, int y);
	       bool gen2_bwd   (int x1, int x2, int y);
	inline bool add_bwd    (int x1, int x2, int y) { g[x1].i() += g[y].i();  g[x2].i() += g[y].i();  return true; }
	inline bool mul_bwd    (int x1, int x2, int y) { g[x1].i() += g[y].i() * d[x2].i(); g[x2].i() += g[y].i() * d[x1].i();  return true; }
	inline bool sub_bwd    (int x1, int x2, int y) { g[x1].i() += g[y].i();  g[x2].i() += -g[y].i();  return true; }
	inline bool div_bwd    (int x1, int x2, int y) { g[x1].i() += g[y].i() / d[x2].i(); g[x2].i() += g[y].i()*(-d[x1].i())/sqr(d[x2].i());  return true; }
	       bool max_bwd    (int x1, int x2, int y);
	       bool min_bwd    (int x1, int x2, int y);
	       bool atan2_bwd  (int x1, int x2, int y);
	       bool gen1_bwd   (int x, int y);
	inline bool minus_bwd  (int x, int y) { g[x].i() += -1.0*g[y].i();  return true; }
	inline bool minus_V_bwd(int x, int y) { g[x].v() += -1.0*g[y].v();  return true; }
	inline bool minus_M_bwd(int x, int y) { g[x].m() += -1.0*g[y].m();  return true; }
        inline bool trans_V_bwd(int, int) { /* nothing to do because g[x].v() is a reference to g[y].v() */  return true; }
        inline bool trans_M_bwd(int x, int y) { g[x].m() += g[y].m().transpose();  return true; }
	       bool sign_bwd   (int x, int y);
	       bool abs_bwd    (int x, int y);
	inline bool power_bwd  (int x, int y, int p) { g[x].i() += g[y].i() * p * pow(d[x].i(), p-1);  return true; }
	inline bool sqr_bwd   (int x, int y) { g[x].i() += g[y].i() * 2.0 * d[x].i();  return true; }
	inline bool sqrt_bwd  (int x, int y) { g[x].i() += g[y].i() * 0.5 / sqrt(d[x].i());   return true; }
	inline bool exp_bwd   (int x, int y) { g[x].i() += g[y].i() * exp(d[x].i());  return true; }
	inline bool log_bwd   (int x, int y) { g[x].i() += g[y].i() / d[x].i();  return true; }
	inline bool cos_bwd   (int x, int y) { g[x].i() += g[y].i() * -sin(d[x].i());  return true; }
	inline bool sin_bwd   (int x, int y) { g[x].i() += g[y].i() * cos(d[x].i());  return true; }
	inline bool tan_bwd   (int x, int y) { g[x].i() += g[y].i() * (1.0 + sqr(tan(d[x].i())));  return true; }
	inline bool cosh_bwd  (int x, int y) { g[x].i() += g[y].i() * sinh(d[x].i());  return true; }
	inline bool sinh_bwd  (int x, int y) { g[x].i() += g[y].i() * cosh(d[x].i());  return true; }
	inline bool tanh_bwd  (int x, int y) { g[x].i() += g[y].i() * (1.0 - sqr(tanh(d[x].i())));  return true; }
	inline bool acos_bwd  (int x, int y) { g[x].i() += g[y].i() * -1.0 / sqrt(1.0-sqr(d[x].i()));  return true; }
	inline bool asin_bwd  (int x, int y) { g[x].i() += g[y].i() * 1.0 / sqrt(1.0-sqr(d[x].i()));  return true; }
	inline bool atan_bwd  (int x, int y) { g[x].i() += g[y].i() * 1.0 / (1.0+sqr(d[x].i()));  return true; }
	inline bool acosh_bwd (int x, int y) { g[x].i() += g[y].i() * 1.0 / sqrt(sqr(d[x].i()) -1.0);  return true; }
	inline bool asinh_bwd (int x, int y) { g[x].i() += g[y].i() * 1.0 / sqrt(1.0+sqr(d[x].i()));  return true; }
	inline bool atanh_bwd (int x, int y) { g[x].i() += g[y].i() * 1.0 / (1.0-sqr(d[x].i()));  return true; }
	inline bool floor_bwd (int x, int y);
	inline bool ceil_bwd  (int x, int y);
	inline bool saw_bwd   (int x, int y);

	inline bool add_V_bwd (int x1, int x2, int y) { g[x1].v() += g[y].v();        g[x2].v() += g[y].v();  return true; }
	inline bool add_M_bwd (int x1, int x2, int y) { g[x1].m() += g[y].m();        g[x2].m() += g[y].m();  return true; }
	inline bool mul_SV_bwd(int x1, int x2, int y) { g[x1].i() += g[y].v()*d[x2].v(); g[x2].v() += d[x1].i()*g[y].v();  return true; }
	inline bool mul_SM_bwd(int x1, int x2, int y) { for (int i=0; i<d[y].m().nb_rows(); i++) g[x1].i()+=g[y].m()[i]*d[x2].m()[i]; g[x2].m() += d[x1].i()*g[y].m();   return true; }
	inline bool mul_VV_bwd(int x1, int x2, int y) { g[x1].v() += g[y].i()*d[x2].v(); g[x2].v() += g[y].i()*d[x1].v();  return true; }
	inline bool mul_MV_bwd(int x1, int x2, int y) { g[x1].m() += outer_product(g[y].v(),d[x2].v()); g[x2].v() += d[x1].m().transpose()*g[y].v();  return true; }
	inline bool mul_MM_bwd(int x1, int x2, int y) { g[x1].m() += g[y].m()*d[x2].m().transpose(); g[x2].m() += d[x1].m().transpose()*g[y].m();  return true; }
	inline bool mul_VM_bwd(int x1, int x2, int y) { g[x1].v() += d[x2].m()*g[y].v(); g[x2].m() += outer_product(d[x1].v(),g[y].v());  return true; }
	inline bool sub_V_bwd (int x1, int x2, int y) { g[x1].v() += g[y].v(); g[x2].v() -= g[y].v();  return true; }
	inline bool sub_M_bwd (int x1, int x2, int y) { g[x1].m() += g[y].m(); g[x2].m() -= g[y].m();  return true; }

	Function& f;
	Eval& _eval;
	ExprDomain& d;
	ExprDomain  g;
	// Store the "linear part" of f so
	// that these coefficients are only calculated once.
	IntervalMatrix coeff_matrix;
	// True if the ith component is linear (wrt all variables)
	bool *is_linear;
};

} // namespace ibex

#endif // __IBEX_GRADIENT_H__
