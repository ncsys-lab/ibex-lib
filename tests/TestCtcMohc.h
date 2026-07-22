/* ============================================================================
 * I B E X - Test CtcMohc (smoke tests for the 2026 port)
 * ============================================================================
 * License     : This program can be distributed under the terms of the GNU LGPL.
 *               See the file COPYING.LESSER.
 *
 * Created     : Jul 22, 2026
 * ---------------------------------------------------------------------------- */

#ifndef __TEST_CTC_MOHC_H__
#define __TEST_CTC_MOHC_H__

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "utils.h"

namespace ibex {

/*
 * Sound-property smoke tests: solutions are never lost, the result is a
 * sub-box of the input, inactive constraints are flagged INACTIVE, and a
 * constraint whose forward image excludes the right-hand side empties the
 * box. No tightness (completeness-strength) assertions.
 */
class TestCtcMohc : public CppUnit::TestFixture {

public:

	CPPUNIT_TEST_SUITE(TestCtcMohc);

		CPPUNIT_TEST(contract_eq_multi_occ);
		CPPUNIT_TEST(contract_single_var_multi_occ);
		CPPUNIT_TEST(inactive_flag);
		CPPUNIT_TEST(infeasible_empties);
		CPPUNIT_TEST(empty_input);
	CPPUNIT_TEST_SUITE_END();

	// x*y-x=0 on [0.5,2]^2: (1,1) kept, result is a sub-box
	void contract_eq_multi_occ();
	// x+x-1=0 on [-10,10]: 0.5 kept, result is a sub-box
	void contract_single_var_multi_occ();
	// sqr(x)-10<=0 on [-1,1]: feasible everywhere -> INACTIVE, box unchanged
	void inactive_flag();
	// sqr(x)+1<=0 on [-1,1]: forward image excludes rhs -> empty
	void infeasible_empties();
	// empty input box -> INACTIVE+FIXPOINT, still empty
	void empty_input();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestCtcMohc);


} // namespace ibex
#endif // __TEST_CTC_MOHC_H__
