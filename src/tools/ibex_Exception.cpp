/* ============================================================================
 * I B E X - Root class of all exceptions raised by IBEX
 * ============================================================================
 * Copyright   : Ecole des Mines de Nantes (FRANCE)
 * License     : This program can be distributed under the terms of the GNU LGPL.
 *               See the file COPYING.LESSER.
 *
 * Author(s)   : Gilles Chabert
 * Created     : Dec 13, 2011
 * ---------------------------------------------------------------------------- */

#include "ibex_Exception.h"
#include <stdlib.h>
#include <cassert>


namespace ibex {

void ibex_error(const std::string& message) {
	std::cerr << "error: " << message << std::endl;
	//throw std::runtime_error(message); //
	assert(false); // allow tracing with gdb
	exit(-1);
}

void ibex_error(const char* message) {
	std::cerr << "error: " << message << std::endl;
	//throw std::runtime_error(message); //
	assert(false); // allow tracing with gdb
	exit(-1);
}

void ibex_warning(const char* message) {
	std::cerr << "\033[33mwarning: " << message << "\033[0m" << std::endl;
}

void ibex_warning(const std::string& message) {
	std::cerr << "warning: " << message << std::endl;
}

void not_implemented(const char* feature) {
	std::cerr << "***********************************************************************" << std::endl;
	std::cerr << "IBEX has crashed because the following feature is not implemented yet:" << std::endl;
	std::cerr << feature << std::endl;
	std::cerr << "Please, submit a new feature request." << std::endl;
	std::cerr << "***********************************************************************" << std::endl;
	exit(-1);
}
} /* namespace ibex */
