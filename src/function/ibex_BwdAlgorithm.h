/* ============================================================================
 * I B E X - Interface for backward algorithms
 * ============================================================================
 * Copyright   : Ecole des Mines de Nantes (FRANCE)
 * License     : This program can be distributed under the terms of the GNU LGPL.
 *               See the file COPYING.LESSER.
 *
 * Author(s)   : Gilles Chabert
 * Created     : Jan 22, 2012
 * ---------------------------------------------------------------------------- */

#ifndef __IBEX_BWD_ALGORITHM_H__
#define __IBEX_BWD_ALGORITHM_H__

#include "ibex_Expr.h"

namespace ibex {

/**
 * \ingroup symbolic
 * \brief Interface for backward Algorithms.
 */
class BwdAlgorithm {

protected:
	/** TO BE DEFINED (by the subclass) */
	bool idx_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool idx_cp_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool vector_bwd(int* x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool symbol_bwd(int y);

	/** TO BE DEFINED (by the subclass) */
	bool cst_bwd(int y);

	/** TO BE DEFINED (by the subclass) */
	bool apply_bwd(int* x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool chi_bwd(  int a, int b, int c, int y);

	/*==================== binary operators =========================*/
	/** TO BE DEFINED (by the subclass) */
	bool gen2_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool add_bwd(int x1, int x2, int y);

	/** TO BE DEFINED (by the subclass) */
	bool add_V_bwd(int x1, int x2, int y);

	/** TO BE DEFINED (by the subclass) */
	bool add_M_bwd(int x1, int x2, int y);

	/** TO BE DEFINED (by the subclass) */
	bool mul_bwd(int x1, int x2, int y);

	/** TO BE DEFINED (by the subclass) */
	bool mul_SV_bwd(int x1, int x2, int y);

	/** TO BE DEFINED (by the subclass) */
	bool mul_SM_bwd(int x1, int x2, int y);

	/** TO BE DEFINED (by the subclass) */
	bool mul_VV_bwd(int x1, int x2, int y);

	/** TO BE DEFINED (by the subclass) */
	bool mul_MV_bwd(int x1, int x2, int y);

	/** TO BE DEFINED (by the subclass) */
	bool mul_VM_bwd(int x1, int x2, int y);

	/** TO BE DEFINED (by the subclass) */
	bool mul_MM_bwd(int x1, int x2, int y);

	/** TO BE DEFINED (by the subclass) */
	bool sub_bwd(int x1, int x2, int y);

	/** TO BE DEFINED (by the subclass) */
	bool sub_V_bwd(int x1, int x2, int y);

	/** TO BE DEFINED (by the subclass) */
	bool sub_M_bwd(int x1, int x2, int y);

	/** TO BE DEFINED (by the subclass) */
	bool div_bwd(int x1, int x2, int y);

	/** TO BE DEFINED (by the subclass) */
	bool max_bwd(int x1, int x2, int y);

	/** TO BE DEFINED (by the subclass) */
	bool min_bwd(int x1, int x2, int y);

	/** TO BE DEFINED (by the subclass) */
	bool atan2_bwd(int x1, int x2, int y);

	/*==================== unary operators =========================*/

	/** TO BE DEFINED (by the subclass) */
	bool gen1_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool minus_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool minus_V_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool minus_M_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool trans_V_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool trans_M_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool sign_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool abs_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool power_bwd(int x, int y, int p);

	/** TO BE DEFINED (by the subclass) */
	bool sqr_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool sqrt_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool exp_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool log_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool cos_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool sin_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool tan_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool cosh_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool sinh_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool tanh_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool acos_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool asin_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool atan_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool acosh_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool asinh_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool atanh_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool floor_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool ceil_bwd(int x, int y);

	/** TO BE DEFINED (by the subclass) */
	bool saw_bwd(int x, int y);
};

} // namespace ibex

#endif // __IBEX_BWD_ALGORITHM_H__
