//============================================================================
//                                  I B E X
// File        : ExprSplitOcc.h
// Author      : Gilles Chabert
// Copyright   : Ecole des Mines de Nantes (France)
// License     : See the LICENSE file
// Created     : Dec 17, 2013
// Last Update : Jul 22, 2026 (port from the pre-45365e0c kernel to 2.9.1)
//============================================================================

#ifndef __IBEX_EXPR_SPLIT_OCC_H__
#define __IBEX_EXPR_SPLIT_OCC_H__

#include "ibex_Array.h"
#include "ibex_NodeMap.h"
#include "ibex_ExprVisitor.h"

#include <map>
#include <utility>
#include <vector>

namespace ibex {

/**
 * \ingroup symbolic
 *
 * \brief Split occurrences of symbols in an expression
 *
 * This class transforms an expression "y" involving symbols "x" into
 * a new expression where each occurrence of a symbol in x results in
 * a new different symbol.
 *
 * (Ported 2026 from the last mainline version before its removal in
 * 45365e0c "Mohc & ExprSplitOcc removed (temporarily?)". The removed
 * ExprNode::fathers array is reconstructed locally; ExprIndex's plain
 * int index became a DoubleIndex, of which only single-element
 * indexation is supported -- exactly the set of shapes the original
 * code could receive.)
 */
class ExprSplitOcc : public virtual ExprVisitor<void> {
public:
	/**
	 * \param x - the original array of symbols
	 * \param y - the expression to transform
	 */
	ExprSplitOcc(const Array<const ExprSymbol>& x, const ExprNode& y);

	/**
	 * \brief Get the new array of symbols.
	 */
	const Array<const ExprSymbol>& get_x() const;

	/**
	 * \brief Get the new expression.
	 */
	const ExprNode& get_y() const;

	/**
	 * \brief Get the original node.
	 *
	 * Get the subexpression in the original expression "y" that corresponds
	 * to the symbol x.
	 */
	const ExprNode& node(const ExprSymbol& x) const;

	/**
	 * \brief Variable indices map structure
	 *
	 * var is an array (allocated by the function) such
	 * that var[i] gives the index of the variable corresponding
	 * to the ith one in the original expression.
	 *
	 * \return The size of the array
	 */
	int var_map(int*& var) const;

	/**
	 * \brief Delete this.
	 */
	virtual ~ExprSplitOcc();

protected:
	void visit(const ExprNode& e) override;
	void visit(const ExprIndex& i) override;
	void visit(const ExprSymbol& x) override;
	void visit(const ExprConstant& x) override;

	void visit(const ExprVector& e) override;
	void visit(const ExprApply& e) override;
	void visit(const ExprChi& e) override;
	void visit(const ExprGenericBinaryOp& e) override;
	void visit(const ExprAdd& e) override;
	void visit(const ExprMul& e) override;
	void visit(const ExprSub& e) override;
	void visit(const ExprDiv& e) override;
	void visit(const ExprMax& e) override;
	void visit(const ExprMin& e) override;
	void visit(const ExprAtan2& e) override;
	void visit(const ExprGenericUnaryOp& e) override;
	void visit(const ExprMinus& e) override;
	void visit(const ExprTrans& e) override;
	void visit(const ExprSign& e) override;
	void visit(const ExprAbs& e) override;
	void visit(const ExprPower& e) override;
	void visit(const ExprSqr& e) override;
	void visit(const ExprSqrt& e) override;
	void visit(const ExprExp& e) override;
	void visit(const ExprLog& e) override;
	void visit(const ExprCos& e) override;
	void visit(const ExprSin& e) override;
	void visit(const ExprTan& e) override;
	void visit(const ExprCosh& e) override;
	void visit(const ExprSinh& e) override;
	void visit(const ExprTanh& e) override;
	void visit(const ExprAcos& e) override;
	void visit(const ExprAsin& e) override;
	void visit(const ExprAtan& e) override;
	void visit(const ExprAcosh& e) override;
	void visit(const ExprAsinh& e) override;
	void visit(const ExprAtanh& e) override;
	void visit(const ExprFloor& e) override;
	void visit(const ExprCeil& e) override;
	void visit(const ExprSaw& e) override;

	/*
	 * Clone structure associated to indexed symbols (ExprIndex).
	 * The clone for the first occurrence of x[j] is y[j] where y is the
	 * "special clone" of x. Then, other occurrences of x[j] are new
	 * symbols named x[j]_xxx_.
	 */
	struct IndexClone {
		IndexClone() : nb_clones(0), clones(NULL), clone_counter(0) { }

		// count the clones for each occurence of an indexed symbol (like x[i])
		// (the integer is incremented at each occurrence, including the first one).
		int nb_clones;

		// The clones.
		const ExprNode** clones;

		// For the visitor
		int clone_counter;
	};

	/*
	 * Clone structure associated to a symbol. There is one clone for each
	 * occurrence of the symbol except when the occurrence corresponds to
	 * an indexation (like x[0]). All the occurrences that are indexations
	 * of a symbol become indexations of the same "special" clone of the symbol.
	 */
	struct SymbolClone {
		SymbolClone() : nb_clones(0), clones(NULL), clone_counter(0), special_clone(NULL) { }

		// the special clone is not included in the count
		int nb_clones;

		// all the clones except the "special" one.
		const ExprSymbol** clones;

		// For the visitor: count the clones for each occurence of the symbol
		// (the integer is incremeted at each occurrence), except in
		// indexation
		int clone_counter;

		// The special clone of a symbol, the one shared by all indexations
		// of the symbol. E.g., in x[0]+x[1] -> y[0]+y[1] where y is the
		// special clone. This field is NULL if there is no indexation.
		const ExprSymbol* special_clone;

		// (port note: was an unordered_map; std::map makes the iteration
		// order -- hence the order of the index clones in get_x() --
		// deterministic and ascending, which is what the unit tests pin.)
		std::map<int,IndexClone*> indices;
	};

	// Store the peer node of each node in the origin expression
	// (including unused symbols). The peer node of a symbol
	// change dynamically.
	NodeMap<const ExprNode*> clone;

	// Origin variables
	// (port note: was a reference; a temporary Array -- e.g. the implicit
	// single-symbol conversion in "ExprSplitOcc eso(x,y)" -- dies at the end
	// of the constructor call, leaving the reference dangling for the later
	// var_map() call. Array's copy is a shallow, non-owning pointer-table
	// copy, so holding it by value is semantics-preserving and safe.)
	const Array<const ExprSymbol> old_x;

	// Origin expression
	const ExprNode& old_y;

	// Array of new symbols resulting from occurrence splitting
	Array<const ExprSymbol> new_x;

	// Clone structure for each symbol
	NodeMap<SymbolClone*> symbol_clone;

	// Gives the subexpression of the origin expression
	// that corresponds to a symbol in the new expression
	NodeMap<const ExprNode*> maps_to;
};

/*================================== inline implementations ========================================*/

inline const Array<const ExprSymbol>& ExprSplitOcc::get_x() const {
	return new_x;
}

inline const ExprNode& ExprSplitOcc::get_y() const {
	return *(((ExprSplitOcc*) this)->clone[old_y]);
}

inline const ExprNode& ExprSplitOcc::node(const ExprSymbol& x) const {
	return *maps_to[x];
}

} // end namespace ibex
#endif // __IBEX_EXPR_SPLIT_OCC_H__
