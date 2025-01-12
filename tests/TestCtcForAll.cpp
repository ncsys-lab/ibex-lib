/* ============================================================================
 * I B E X - CtcForAll Tests
 * ============================================================================
 * Copyright   : Ecole des Mines de Nantes (FRANCE)
 * License     : This program can be distributed under the terms of the GNU LGPL.
 *               See the file COPYING.LESSER.
 *
 * Author(s)   : Gilles Chabert
 * Created     : Aug 21, 2014
 * ---------------------------------------------------------------------------- */

#include "TestCtcForAll.h"
#include "ibex_CtcForAll.h"
#include "ibex_Solver.h"
#include "ibex_RoundRobin.h"
#include "ibex_CellStack.h"


namespace ibex {

void TestCtcForAll::test01() {

	Variable x,y;
	Function f(x,y,1.5*sqr(x)+1.5*sqr(y)-x*y-0.2);

	double prec=1e-05;

	NumConstraint c(f,LEQ);

	IntervalVector parambox(1,Interval(-0.01,0.01));
	//IntervalVector y_init(1,Interval(-0.1,0.1));

	CtcForAll forall_y(c, y, parambox, prec);
	CtcForAll forall_x(c, x, parambox, prec);

	IntervalVector box(1,Interval(-10,10));

	double right_bound=+0.3616933019201018; // the positive one such that f(x,-0.01)=0
	//double right_bound=+0.3194335081419453; // the positive one such that f(x,-0.1)=0

	std::stack<IntervalVector> std::stack;
	std::stack.push(box);
	IntervalVector sol=box;
	while (!std::stack.empty() && sol.max_diam()>1e-03) {
		sol=std::stack.top();
		std::stack.pop();
		forall_y.contract(sol);
		if (!sol.is_empty()) {
			std::pair<IntervalVector,IntervalVector> p=sol.bisect(0);
			std::stack.push(p.first);
			std::stack.push(p.second);
		}
	}

	// note: we use the fact that the solver always explores the right
	// branch first
	CPPUNIT_ASSERT(!std::stack.empty());
	CPPUNIT_ASSERT(sol[0].contains(right_bound));

	while (!std::stack.empty()) std::stack.pop();

	std::stack.push(box);
	sol=box;
	while (!std::stack.empty() && sol.max_diam()>1e-03) {
		sol=std::stack.top();
		std::stack.pop();
		forall_x.contract(sol);
		if (!sol.is_empty()) {
			std::pair<IntervalVector,IntervalVector> p=sol.bisect(0);
			std::stack.push(p.first);
			std::stack.push(p.second);
		}
	}

	// note: we use the fact that the constraint is symmetric in x/y
	CPPUNIT_ASSERT(!std::stack.empty());
	CPPUNIT_ASSERT(sol[0].contains(right_bound));
}

} // end namespace
