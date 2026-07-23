/* ============================================================================
 * I B E X - Test CtcMohc (smoke tests for the 2026 port)
 * ============================================================================
 * License     : This program can be distributed under the terms of the GNU LGPL.
 *               See the file COPYING.LESSER.
 *
 * Created     : Jul 22, 2026
 * ---------------------------------------------------------------------------- */

#include "TestCtcMohc.h"
#include "ibex_CtcMohc.h"
#include "ibex_ContractContext.h"

using namespace std;

namespace ibex {

void TestCtcMohc::contract_eq_multi_occ() {
	const ExprSymbol& x=ExprSymbol::new_("x");
	const ExprSymbol& y=ExprSymbol::new_("y");
	NumConstraint c(x,y,x*y-x=0);
	Array<NumConstraint> csp(c);
	CtcMohc mohc(csp);

	IntervalVector box(2,Interval(0.5,2));
	IntervalVector init(box);
	mohc.contract(box);

	CPPUNIT_ASSERT(!box.is_empty());
	CPPUNIT_ASSERT(box.is_subset(init));
	// (x,y)=(1,1) solves x*y-x=0 and must never be lost
	IntervalVector sol(2,Interval(1,1));
	CPPUNIT_ASSERT(sol.is_subset(box));
}

void TestCtcMohc::contract_single_var_multi_occ() {
	const ExprSymbol& x=ExprSymbol::new_("x");
	NumConstraint c(x,x+x-1=0);
	Array<NumConstraint> csp(c);
	CtcMohc mohc(csp);

	IntervalVector box(1,Interval(-10,10));
	IntervalVector init(box);
	mohc.contract(box);

	CPPUNIT_ASSERT(!box.is_empty());
	CPPUNIT_ASSERT(box.is_subset(init));
	// x=0.5 solves x+x-1=0 and must never be lost
	CPPUNIT_ASSERT(box[0].contains(0.5));
}

void TestCtcMohc::inactive_flag() {
	const ExprSymbol& x=ExprSymbol::new_("x");
	NumConstraint c(x,sqr(x)-10<=0);
	CtcMohcRevise revise(c, CtcMohc::default_epsilon, CtcMohc::default_univ_newton_min_width,
			CtcMohc::default_tau_mohc, false);

	IntervalVector box(1,Interval(-1,1));
	IntervalVector init(box);
	ContractContext context(box);
	revise.contract(box,context);

	CPPUNIT_ASSERT(box==init);
	CPPUNIT_ASSERT(context.output_flags[Ctc::INACTIVE]);
	CPPUNIT_ASSERT(context.output_flags[Ctc::FIXPOINT]);
}

void TestCtcMohc::infeasible_empties() {
	const ExprSymbol& x=ExprSymbol::new_("x");
	NumConstraint c(x,sqr(x)+1<=0);
	Array<NumConstraint> csp(c);
	CtcMohc mohc(csp);

	IntervalVector box(1,Interval(-1,1));
	mohc.contract(box);

	CPPUNIT_ASSERT(box.is_empty());
}

void TestCtcMohc::empty_input() {
	const ExprSymbol& x=ExprSymbol::new_("x");
	NumConstraint c(x,sqr(x)-1=0);
	CtcMohcRevise revise(c, CtcMohc::default_epsilon, CtcMohc::default_univ_newton_min_width,
			CtcMohc::default_tau_mohc, false);

	IntervalVector box(1);
	box.set_empty();
	ContractContext context(box);
	revise.contract(box,context);

	CPPUNIT_ASSERT(box.is_empty());
	CPPUNIT_ASSERT(context.output_flags[Ctc::INACTIVE]);
	CPPUNIT_ASSERT(context.output_flags[Ctc::FIXPOINT]);
}

} // end namespace
