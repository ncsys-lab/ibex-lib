//============================================================================
//                                  I B E X
// File        : ibex_ExprSplitOcc.cpp
// Author      : Gilles Chabert
// Copyright   : Ecole des Mines de Nantes (France)
// License     : See the LICENSE file
// Created     : Dec 17, 2013
// Last Update : Jul 22, 2026 (port from the pre-45365e0c kernel to 2.9.1)
//============================================================================

#include "ibex_ExprSplitOcc.h"
#include "ibex_Expr.h"
#include "ibex_ExprSubNodes.h"
#include "ibex_Exception.h"
#include "ibex_String.h"
#include <stdlib.h>

using namespace std;

namespace ibex {

namespace {

// ---------------------------------------------------------------------------
// The 2.9.1 kernel removed ExprNode::fathers (the field is commented out in
// ibex_Expr.h). The original ExprSplitOcc counted symbol occurrences by
// iterating that array, with one entry per DAG *edge* (e.g., in x+x where x
// is a single node, the ExprAdd node appeared twice among x's fathers).
// Reconstruct exactly that: enumerate each distinct DAG node once
// (ExprSubNodes) and record one father entry per argument slot.
// ---------------------------------------------------------------------------
typedef NodeMap<vector<const ExprNode*> > FatherMap;

void add_edge(FatherMap& fathers, const ExprNode& child, const ExprNode& father) {
	if (!fathers.found(child)) fathers.insert(child, vector<const ExprNode*>());
	fathers[child].push_back(&father);
}

void build_fathers(const ExprNode& y, FatherMap& fathers) {
	ExprSubNodes nodes(y); // each DAG node appears exactly once
	for (int i=0; i<nodes.size(); i++) {
		const ExprNode& n=nodes[i];
		if (const ExprIndex* e=dynamic_cast<const ExprIndex*>(&n))
			add_edge(fathers, e->expr, n);
		else if (const ExprUnaryOp* e=dynamic_cast<const ExprUnaryOp*>(&n))
			add_edge(fathers, e->expr, n);
		else if (const ExprBinaryOp* e=dynamic_cast<const ExprBinaryOp*>(&n)) {
			add_edge(fathers, e->left, n);
			add_edge(fathers, e->right, n);
		}
		else if (const ExprNAryOp* e=dynamic_cast<const ExprNAryOp*>(&n))
			for (int k=0; k<e->nb_args; k++)
				add_edge(fathers, e->arg(k), n);
	}
}

const vector<const ExprNode*> no_fathers;

// The 2.9.1 kernel replaced the plain int of ExprIndex::index by a
// DoubleIndex. Occurrence splitting only handles scalar/vector symbols
// (matrix symbols are rejected, as in the original), so an admissible
// indexation selects exactly one element; its flat index is
// first_row()+first_col() (the other one is 0).
int elt_index(const DoubleIndex& idx) {
	if (!idx.one_elt())
		not_implemented("occurrence splitting with non-scalar indexation");
	return idx.first_row()+idx.first_col();
}

} // end anonymous namespace

ExprSplitOcc::ExprSplitOcc(const Array<const ExprSymbol>& x, const ExprNode& y) : old_x(x), old_y(y) {

	FatherMap fathers;
	build_fathers(y, fathers);

	// count the number of variables

	int nb_new_var=0;

	// This loops initializes nb_clones for
	// all symbols & indices and special_node
	// for symbols.
	for (int i=0; i<x.size(); i++) {

		SymbolClone* s=new SymbolClone();
		symbol_clone.insert(x[i],s);

		s->nb_clones=0; // by default

		if (x[i].dim.type()==Dim::MATRIX)
			not_implemented("occurrence splitting with matrix symbols");

		const vector<const ExprNode*>& fa = fathers.found(x[i]) ? fathers[x[i]] : no_fathers;

		if (fa.empty()) {
			// a variable must be considered
			// even if not involved in the expression
			s->nb_clones=1;
			nb_new_var++;
		}
		else {
			for (size_t k=0; k<fa.size(); k++) {
				// check if the kth father of x is an expression x[j]
				const ExprIndex* idx=dynamic_cast<const ExprIndex*>(fa[k]);
				if (idx!=NULL) {
					// the special clone is only created once.
					if (s->special_clone==NULL) {
						s->special_clone=&ExprSymbol::new_(x[i].name, x[i].dim);
						nb_new_var++;
					}

					int iv=elt_index(idx->index);
					int idx_nb_fathers=(int) (fathers.found(*idx) ? fathers[*idx].size() : 0);

					// then count the number of fathers of x[j]: the first
					// occurrence corresponds to an indexation of the special
					// symbol. Additional occurrences will be new symbols.
					if (s->indices.find(iv)!=s->indices.end()) {
						IndexClone* ic=s->indices[iv];
						ic->nb_clones += idx_nb_fathers;
						nb_new_var    += idx_nb_fathers;
					} else {
						IndexClone* ic=new IndexClone();
						s->indices.insert(std::pair<int,IndexClone*>(iv,ic));
						if (idx_nb_fathers>1) { // rem fathers.size()==0 possible if x[j] is the entire DAG
							// "-1"++ because 1st occurrence uses the special clone of the symbol.
							ic->nb_clones  = idx_nb_fathers;
							nb_new_var    += idx_nb_fathers-1;
						} else { // 0 or 1
							ic->nb_clones  = 1;
						}
					}
				} else {
					s->nb_clones++;
					nb_new_var++;
				}
			}
		}
	}

	new_x.resize(nb_new_var);

	//fill the array of new variables
	int j=0;   // index for the new symbol
	int j2=0;  // clone counter for each symbol
	for (int i=0; i<x.size(); i++) {
		SymbolClone* s=symbol_clone[x[i]];

		const vector<const ExprNode*>& fa = fathers.found(x[i]) ? fathers[x[i]] : no_fathers;

		if (fa.empty()) {
			s->clones=new const ExprSymbol*[1];
			const ExprSymbol* sbl=&ExprSymbol::new_(x[i].name, x[i].dim);
			s->clones[0]=sbl;
			new_x.set_ref(j, *sbl);
			maps_to.insert(*sbl,&x[i]);
			j++;
		} else {
			if (s->special_clone!=NULL) {
				new_x.set_ref(j,*s->special_clone);
				maps_to.insert(*s->special_clone,&x[i]); // map is done here (not in visit)
				j++;
			}

			s->clones=new const ExprSymbol*[s->nb_clones];
			j2=0;
			for (size_t k=0; k<fa.size(); k++) {
				// check if the kth father of x is an expression x[j]
				const ExprIndex* idx=dynamic_cast<const ExprIndex*>(fa[k]);
				if (idx==NULL) { // indices are handled after
					char* name=append_index(x[i].name, '_','_',j2);
					const ExprSymbol* sbl=&ExprSymbol::new_(name, x[i].dim);
					s->clones[j2++]=sbl;
					free(name);
					new_x.set_ref(j,*sbl);
					j++;
				}
			}
			assert(j2==s->nb_clones);

			// indices
			for (std::map<int,IndexClone*>::iterator it=s->indices.begin(); it!=s->indices.end(); it++) {
				IndexClone* ic=it->second;
				ic->clones = new const ExprNode*[ic->nb_clones];
				ic->clones[0] = &((*s->special_clone)[it->first]); // create new subexpression

				for (int k=1; k<ic->nb_clones; k++) { // additional occurrence --> new symbol
					char* name=append_index(s->special_clone->name, '[',']',it->first);
					char* name2=append_index(name,'_','_',k);
					// (port note: was x[i].dim.index_dim(); only one-element
					// indexation of scalar/vector symbols is admissible here,
					// so the element is a scalar.)
					const ExprSymbol* sbl=&ExprSymbol::new_(name2,Dim::scalar());
					ic->clones[k]=sbl;
					free(name2);
					free(name);
					new_x.set_ref(j,*sbl);
					j++;
				}
			}
		}
	}
	assert(j==new_x.size());

	visit(y);
}

int ExprSplitOcc::var_map(int*& var) const {

	// get the first index of each original symbol
	// in the "flat" array of original variables
	int k=0;
	NodeMap<int> origin_index;
	for (int i=0; i<old_x.size(); i++) {
		origin_index.insert(old_x[i],k);
		k+=old_x[i].dim.size();
	}

	// calculate total size of the "flat" array
	// of new variables
	int n=0;
	for (int i=0; i<new_x.size(); i++) {
		n+=new_x[i].dim.size();
	}

	// build the map array
	var=new int[n];

	// fill it
	k=0;
	for (int i=0; i<new_x.size(); i++) {
		const ExprNode& _node=node(new_x[i]);
		const ExprIndex* idx=dynamic_cast<const ExprIndex*>(&_node);
		if (idx==NULL) {
			const ExprSymbol* sbl=dynamic_cast<const ExprSymbol*>(&_node);
			assert(sbl);
			int start_index=origin_index[*sbl];
			for (int j=0; j<new_x[i].dim.size(); j++) {
				var[k++]=start_index+j;
			}
		} else {
			assert(new_x[i].dim==Dim::scalar());
			var[k++]=origin_index[idx->expr]+elt_index(idx->index);
		}
	}
	return n;
}

ExprSplitOcc::~ExprSplitOcc() {
	for (IBEX_NODE_MAP(SymbolClone*)::iterator it=symbol_clone.begin(); it!=symbol_clone.end(); it++) {
		for (std::map<int,IndexClone*>::iterator it2=it->second->indices.begin(); it2!=it->second->indices.end(); it2++) {
			delete[] it2->second->clones;
			delete it2->second;
		}
		delete[] it->second->clones;
		delete it->second;
	}
}

void ExprSplitOcc::visit(const ExprNode& e) {
	// if height=0, it might be symbol: we
	// need to visit it even if it has already been visited
	// (custom cache policy: the base ExprVisitor cache is bypassed entirely,
	// the "clone" map plays its role)
	if (dynamic_cast<const ExprIndex*>(&e)!=NULL ||
		dynamic_cast<const ExprSymbol*>(&e)!=NULL ||
		!clone.found(e)) {
		e.accept_visitor(*this);
	}
}

void ExprSplitOcc::visit(const ExprIndex& i) {
	if (i.indexed_symbol()) {
		const ExprSymbol& s=(const ExprSymbol&) i.expr;
		IndexClone* ic=symbol_clone[s]->indices[elt_index(i.index)];
		const ExprNode* occurrence=ic->clones[ic->clone_counter];

		if (ic->clone_counter>0) {
			assert(dynamic_cast<const ExprSymbol*>(occurrence));
			maps_to.insert(*occurrence,&i);
		} else {
			// visit(i.expr); no-> "visit" is not called for the special clone
		}

		ic->clone_counter++;

		if (!clone.found(i)) {
			clone.insert(i,occurrence); // insert the first occurrence
		} else {
			clone[i] = occurrence;      // or replace the old one
		}

	} else {
		visit(i.expr);
		clone.insert(i, &(*clone[i.expr])[i.index]);
	}
}

void ExprSplitOcc::visit(const ExprSymbol& e) {
	// each call to this function correspond
	// to a new occurrence of the symbol
	// that is placed in the the "clone" structure
	SymbolClone* sc = symbol_clone[e];
	const ExprNode* occurrence=sc->clones[sc->clone_counter++];
	maps_to.insert(*occurrence,&e); // note: "visit" is not called for the special clone

	if (!clone.found(e)) {
		clone.insert(e,occurrence); // insert the first occurrence
	} else {
		clone[e] = occurrence;      // or replace the old one
	}
}

void ExprSplitOcc::visit(const ExprConstant& c) {
	if (!clone.found(c)) {
		clone.insert(c,&c.copy());
	}
}

void ExprSplitOcc::visit(const ExprVector& e) {
	Array<const ExprNode> new_args(e.nb_args);
	for (int i=0; i<e.nb_args; i++) {
		visit(e.arg(i));
		// warning: if the same symbol appears several times in e.args,
		// the node "clone[e.arg(i)]" change each time.
		new_args.set_ref(i,*clone[e.arg(i)]);
	}
	clone.insert(e, &ExprVector::new_(new_args,e.orient));
}

void ExprSplitOcc::visit(const ExprApply& e) {
	Array<const ExprNode> new_args(e.nb_args);
	for (int i=0; i<e.nb_args; i++) {
		visit(e.arg(i));
		// warning: if the same symbol appears several times in e.args,
		// the node "clone[e.arg(i)]" change each time.
		new_args.set_ref(i,*clone[e.arg(i)]);
	}
	clone.insert(e, &ExprApply::new_(e.func,new_args));
}

void ExprSplitOcc::visit(const ExprChi& e) {
	Array<const ExprNode> new_args(e.nb_args);
	for (int i=0; i<e.nb_args; i++) {
		visit(e.arg(i));
		// warning: if the same symbol appears several times in e.args,
		// the node "clone[e.arg(i)]" change each time.
		new_args.set_ref(i,*clone[e.arg(i)]);
	}
	clone.insert(e, &ExprChi::new_(new_args));
}

void ExprSplitOcc::visit(const ExprPower& e) {
	visit(e.expr);
	clone.insert(e, &pow(*clone[e.expr],e.expon));
}

// warning: if e.left is the same symbol as e.right,
// now the node "clone" points to has changed.
// (the two sub-visits are deliberately sequenced statements: the
// left-to-right occurrence order determines which clone each occurrence
// receives)
#define BINARY_COPY(f) visit(e.left); \
		               const ExprNode& l=*clone[e.left]; \
		               visit(e.right); \
		               const ExprNode& r=*clone[e.right]; \
		               clone.insert(e, &f(l,r));

#define UNARY_COPY(f) visit(e.expr); \
					  clone.insert(e,&f(*clone[e.expr]));

void ExprSplitOcc::visit(const ExprGenericBinaryOp& e) {
	visit(e.left);
	const ExprNode& l=*clone[e.left];
	visit(e.right);
	const ExprNode& r=*clone[e.right];
	clone.insert(e, &ExprGenericBinaryOp::new_(e.name,l,r));
}

void ExprSplitOcc::visit(const ExprGenericUnaryOp& e) {
	visit(e.expr);
	clone.insert(e, &ExprGenericUnaryOp::new_(e.name,*clone[e.expr]));
}

void ExprSplitOcc::visit(const ExprAdd& e)   { BINARY_COPY(operator+); }
void ExprSplitOcc::visit(const ExprMul& e)   { BINARY_COPY(operator*); }
void ExprSplitOcc::visit(const ExprSub& e)   { BINARY_COPY(operator-); }
void ExprSplitOcc::visit(const ExprDiv& e)   { BINARY_COPY(operator/); }
void ExprSplitOcc::visit(const ExprMax& e)   { BINARY_COPY(max); }
void ExprSplitOcc::visit(const ExprMin& e)   { BINARY_COPY(min); }
void ExprSplitOcc::visit(const ExprAtan2& e) { BINARY_COPY(atan2); }
void ExprSplitOcc::visit(const ExprMinus& e) { UNARY_COPY(operator-); }
void ExprSplitOcc::visit(const ExprTrans& e) { UNARY_COPY(transpose); }
void ExprSplitOcc::visit(const ExprSign& e)  { UNARY_COPY(sign); }
void ExprSplitOcc::visit(const ExprAbs& e)   { UNARY_COPY(abs); }
void ExprSplitOcc::visit(const ExprSqr& e)   { UNARY_COPY(sqr); }
void ExprSplitOcc::visit(const ExprSqrt& e)  { UNARY_COPY(sqrt); }
void ExprSplitOcc::visit(const ExprExp& e)   { UNARY_COPY(exp); }
void ExprSplitOcc::visit(const ExprLog& e)   { UNARY_COPY(log); }
void ExprSplitOcc::visit(const ExprCos& e)   { UNARY_COPY(cos); }
void ExprSplitOcc::visit(const ExprSin& e)   { UNARY_COPY(sin); }
void ExprSplitOcc::visit(const ExprTan& e)   { UNARY_COPY(tan); }
void ExprSplitOcc::visit(const ExprCosh& e)  { UNARY_COPY(cosh); }
void ExprSplitOcc::visit(const ExprSinh& e)  { UNARY_COPY(sinh); }
void ExprSplitOcc::visit(const ExprTanh& e)  { UNARY_COPY(tanh); }
void ExprSplitOcc::visit(const ExprAcos& e)  { UNARY_COPY(acos); }
void ExprSplitOcc::visit(const ExprAsin& e)  { UNARY_COPY(asin); }
void ExprSplitOcc::visit(const ExprAtan& e)  { UNARY_COPY(atan); }
void ExprSplitOcc::visit(const ExprAcosh& e) { UNARY_COPY(acosh); }
void ExprSplitOcc::visit(const ExprAsinh& e) { UNARY_COPY(asinh); }
void ExprSplitOcc::visit(const ExprAtanh& e) { UNARY_COPY(atanh); }
void ExprSplitOcc::visit(const ExprFloor& e) { UNARY_COPY(floor); }
void ExprSplitOcc::visit(const ExprCeil& e)  { UNARY_COPY(ceil); }
void ExprSplitOcc::visit(const ExprSaw& e)   { UNARY_COPY(saw); }

} // end namespace ibex
