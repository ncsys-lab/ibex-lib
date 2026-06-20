//============================================================================
//                                  I B E X                                   
// File        : HC4Revise Algorithm
// Author      : Gilles Chabert
// Copyright   : Ecole des Mines de Nantes (France)
// License     : See the LICENSE file
// Created     : Dec 31, 2011
// Last update : Jan 28, 2020
//============================================================================

#include "ibex_Function.h"
#include "ibex_HC4Revise.h"

namespace ibex {

constexpr double HC4Revise::RATIO;

HC4Revise::HC4Revise(Eval& e) : f(e.f), eval(e), d(e.d) {

}

bool HC4Revise::proj(const Domain& y, Array<Domain>& x) {
	eval.eval(x);

	// backward() returns false iff a domain emptied (root intersection or a
	// deep *_bwd contradiction). Propagate that to apply_bwd as the return
	// value — no exception. On emptiness we skip the read-back, exactly as the
	// old throw path did (it never reached read_arg_domains).
	if (!backward(y))
		return false;

	d.read_arg_domains(x);

	return true;
//	return proj(y,(const Array<const Domain>&) x);
}

//bool HC4Revise::proj(const Domain& y, const Array<const Domain>& x) {
//}

bool HC4Revise::proj(const Domain& y, IntervalVector& x, const std::function<void(int index, const Interval &old_value, const Interval &new_value)> &callback) {
	eval.eval(x);
	//std::cout << "forward:" << std::endl; f.cf.print(d);

	// backward() returns false iff a domain emptied (root intersection or a
	// deep *_bwd contradiction) — signalled by return value, no exception.
	const bool non_empty = backward(y);

	// read_arg_domains is called in BOTH cases so callers using the callback to
	// track narrowed variables still see the partial narrowings that completed
	// before a contradiction (the patch-d2b978b9 contract); we then mark the
	// box empty. is_inner is always false in this implementation (backward()
	// runs the full sweep and never reports a constraint inactive), so the
	// return value is unconditionally false — matching the pre-conversion code.
	d.read_arg_domains(x, callback);

	if (!non_empty)
		x.set_empty();

	return false;
}

bool HC4Revise::backward(const Domain& y) {

	Domain& root=*d.top;

	// note: we can't just return early if the domain of the root is a subset
	// of y, because the domains of variables may also be outside of the
	// definition domain of the function. See issue #431. The full backward
	// sweep always runs unless the root intersection is already empty.

	root &= y;

	// Return value is "non-empty": false iff a domain emptied. The root
	// intersection empty is the highest-frequency case (replaces the former
	// root throw); deep *_bwd contradictions now propagate via the
	// short-circuiting CompiledFunction::backward<HC4Revise> return value
	// rather than a thrown EmptyBoxException.
	if (root.is_empty())
		return false;

	return eval.f.backward<HC4Revise>(*this);
}

bool HC4Revise::idx_cp_bwd(int x, int y) {
	assert(dynamic_cast<const ExprIndex*> (&f.node(y)));

	const ExprIndex& e = (const ExprIndex&) f.node(y);

	d[x].put(e.index.first_row(), e.index.first_col(), d[y]);
	return true;
}

bool HC4Revise::apply_bwd(int* x, int y) {
	assert(dynamic_cast<const ExprApply*> (&f.node(y)));

	const ExprApply& a = (const ExprApply&) f.node(y);

	assert(&a.func!=&f); // recursive calls not allowed

	Array<Domain> d2(a.func.nb_arg());

	for (int i=0; i<a.func.nb_arg(); i++) {
		d2.set_ref(i,d[x[i]]);
	}

	// proj() returns false iff the nested function emptied a domain; propagate
	// that up as this node's return value so the enclosing backward sweep
	// short-circuits (no exception).
	return a.func.hc4revise().proj(d[y],d2);
}

bool HC4Revise::vector_bwd(int* x, int y) {
	assert(dynamic_cast<const ExprVector*>(&(f.node(y))));

	const ExprVector& v = (const ExprVector&) f.node(y);

	assert(v.type()!=Dim::SCALAR);

	int j=0;

	if (v.dim.is_vector()) {
		for (int i=0; i<v.length(); i++) {
			if (v.arg(i).dim.is_vector()) {
				if ((d[x[i]].v() &= d[y].v().subvector(j,j+v.arg(i).dim.vec_size()-1)).is_empty())
						return false;
				j+=v.arg(i).dim.vec_size();
			} else {
				if ((d[x[i]].i() &= d[y].v()[j]).is_empty())
					return false;
				j++;
			}
		}

		assert(j==v.dim.vec_size());
	}
	else {
		if (v.row_vector()) {
			for (int i=0; i<v.length(); i++) {
				if (v.arg(i).dim.is_matrix()) {
					if ((d[x[i]].m()&=d[y].m().submatrix(0,v.dim.nb_rows()-1,j,j+v.arg(i).dim.nb_cols()-1)).is_empty())
						return false;
					j+=v.arg(i).dim.nb_cols();
				} else if (v.arg(i).dim.is_vector()) {
					if ((d[x[i]].v()&=d[y].m().col(j)).is_empty())
						return false;
					j++;
				}
			}
		} else {
			for (int i=0; i<v.length(); i++) {
				if (v.arg(i).dim.is_matrix()) {
					if ((d[x[i]].m()&=d[y].m().submatrix(j,j+v.arg(i).dim.nb_rows()-1,0,v.dim.nb_cols()-1)).is_empty())
						return false;
					j+=v.arg(i).dim.nb_rows();
				} else if (v.arg(i).dim.is_vector()) {
					if ((d[x[i]].v()&=d[y].m().row(j)).is_empty())
						return false;
					j++;
				}
			}
		}
	}
	return true;
}

bool HC4Revise::gen2_bwd(int x1, int x2, int y) {
	assert(dynamic_cast<const ExprGenericBinaryOp*>(&(f.node(y))));

	const ExprGenericBinaryOp& e = (const ExprGenericBinaryOp&) f.node(y);
	e.bwd(d[y], d[x1], d[x2]);
	if (d[x1].is_empty() || d[x2].is_empty()) return false;
	return true;
}

bool HC4Revise::gen1_bwd(int x, int y) {
	assert(dynamic_cast<const ExprGenericUnaryOp*>(&(f.node(y))));

	const ExprGenericUnaryOp& e = (const ExprGenericUnaryOp&) f.node(y);
	e.bwd(d[y], d[x]);
	if (d[x].is_empty()) return false;
	return true;
}


} /* namespace ibex */
