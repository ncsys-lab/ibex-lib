/* ============================================================================
 * I B E X - CtcExist Tests
 * ============================================================================
 * Copyright   : Ecole des Mines de Nantes (FRANCE)
 * License     : This program can be distributed under the terms of the GNU LGPL.
 *               See the file COPYING.LESSER.
 *
 * Author(s)   : Gilles Chabert
 * Created     : May 05, 2014
 * ---------------------------------------------------------------------------- */

#include "TestCtcExist.h"
#include "ibex_CtcExist.h"
#include "ibex_Solver.h"
#include "ibex_RoundRobin.h"
#include "ibex_CellStack.h"


namespace ibex {

void TestCtcExist::test01() {

	Variable x,y;
	Function f(x,y,1.5*sqr(x)+1.5*sqr(y)-x*y-0.2);

	double prec=1e-05;

	NumConstraint c(f,LEQ);
	CtcExist exist_y(c,y,IntervalVector(1,Interval(-10,10)),prec);
	CtcExist exist_x(c,x,IntervalVector(1,Interval(-10,10)),prec);

	IntervalVector box(1,Interval(-10,10));

	double right_bound=+0.3872983346072957;

	std::stack<IntervalVector> std::stack;
	std::stack.push(box);
	IntervalVector sol=box;
	while (!std::stack.empty() && sol.max_diam()>1e-03) {
		sol=std::stack.top();
		std::stack.pop();
		exist_y.contract(sol);
		if (!sol.is_empty()) {
			std::pair<IntervalVector,IntervalVector> p=sol.bisect(0);
			std::stack.push(p.first);
			std::stack.push(p.second);
		}
	}

	// note: we use the fact that the solver always explores the right
	// branch first
	CPPUNIT_ASSERT(sol[0].contains(right_bound));

	while (!std::stack.empty()) std::stack.pop();

	std::stack.push(box);

	sol=box;
	while (!std::stack.empty() && sol.max_diam()>1e-03) {
		sol=std::stack.top();
		std::stack.pop();
		exist_x.contract(sol);
		if (!sol.is_empty()) {
			std::pair<IntervalVector,IntervalVector> p=sol.bisect(0);
			std::stack.push(p.first);
			std::stack.push(p.second);
		}
	}
	// note: we use the fact that the constraint is symmetric in x/y
	CPPUNIT_ASSERT(sol[0].contains(right_bound));

}

} // end namespace
