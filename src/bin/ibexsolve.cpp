//============================================================================
//                                  I B E X
//
//                               ************
//                                 IbexSolve
//                               ************
//
// Author      : Gilles Chabert
// Copyright   : IMT Atlantique (France)
// License     : See the LICENSE file
// Last Update : Oct 01, 2017
//============================================================================

#include "ibex.h"
#include "parse_args.h"

#include <sstream>

using namespace ibex;

int main(int argc, char** argv) {

	std::stringstream _random_seed, _eps_x_min, _eps_x_max;
	_random_seed << "Random seed (useful for reproducibility). Default value is " << DefaultSolver::default_random_seed << ".";
	_eps_x_min << "Minimal width of output boxes. This is a criterion to _stop_ bisection: a "
			"non-validated box will not be larger than 'eps-min'. Default value is 1e" << round(::log10(DefaultSolver::default_eps_x_min)) << ".";
	_eps_x_max << "Maximal width of output boxes. This is a criterion to _force_ bisection: a "
			"validated box will not be larger than 'eps-max' (unless there is no equality and it is fully inside inequalities)."
			" Default value is +oo (none)";

	args::ArgumentParser parser("********* IbexSolve (defaultsolver) *********.", "Solve a Minibex file.");
	args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
	args::Flag version(parser, "version", "Display the version of this plugin (same as the version of Ibex).", {'v',"version"});
	args::ValueFlag<double> eps_x_min(parser, "float", _eps_x_min.str(), {'e', "eps-min"});
	args::ValueFlag<double> eps_x_max(parser, "float", _eps_x_max.str(), {'E', "eps-max"});
	args::ValueFlag<double> timeout(parser, "float", "Timeout (time in seconds). Default value is +oo (none).", {'t', "timeout"});
	args::ValueFlag<int>    simpl_level(parser, "int", "Expression simplification level. Possible values are:\n"
			"\t\t* 0:\tno simplification at all (fast).\n"
			"\t\t* 1:\tbasic simplifications (fairly fast). E.g. x+1+1 --> x+2\n"
			"\t\t* 2:\tmore advanced simplifications without developing (can be slow). E.g. x*x + x^2 --> 2x^2\n"
			"\t\t* 3:\tsimplifications with full polynomial developing (can blow up!). E.g. x*(x-1) + x --> x^2\n"
			"Default value is : 1.", {"simpl"});
	args::ValueFlag<std::string> input_file(parser, "filename", "COV input file. The file contains a "
			"(intermediate) description of the manifold with boxes in the COV (binary) format.", {'i',"input"});
	args::ValueFlag<std::string> output_file(parser, "filename", "COV output file. The file will contain the "
			"description of the manifold with boxes in the COV (binary) format. See --format", {'o',"output"});
	args::Flag format(parser, "format", "Give a description of the COV format used by IbexSolve", {"format"});
	args::Flag bfs(parser, "bfs", "Perform breadth-first search (instead of depth-first search, by default)", {"bfs"});
	args::Flag trace(parser, "trace", "Activate trace. \"Solutions\" (output boxes) are displayed as and when they are found.", {"trace"});
	args::Flag stop_at_first(parser, "stop-a-first", "Stop at first solution/boundary/unknown box found.", {"stop-at-first"});
	args::ValueFlag<std::string> boundary_test_arg(parser, "true|full-rank|half-ball|false", "Boundary test strength. Possible values are:\n"
			"\t\t* true:\talways satisfied. Set by default for under constrained problems (0<m<n).\n"
			"\t\t* full-rank:\tthe gradients of all constraints (equalities and potentially activated inequalities) must be linearly independent.\n"
			"\t\t* half-ball:\t(**not implemented yet**) the intersection of the box and the solution set is homeomorphic to a half-ball of R^n\n"
	        "\t\t* false: never satisfied. Set by default if m=0 or m=n (inequalities only/square systems)",
			{"boundary"});
	args::Flag sols(parser, "sols", "Display the \"solutions\" (output boxes) on the standard output.", {'s',"sols"});
	args::ValueFlag<double> random_seed(parser, "float", _random_seed.str(), {"random-seed"});
	args::Flag quiet(parser, "quiet", "Print no report on the standard output.",{'q',"quiet"});
	args::ValueFlag<std::string> forced_params(parser, "vars","Force some variables to be parameters in the parametric proofs, separated by '+'. Example: --forced-params=x+y",{"forced-params"});
	args::Positional<std::string> filename(parser, "filename", "The name of the MINIBEX file.");

	try
	{
		parser.ParseCLI(argc, argv);
	}
	catch (args::Help&)
	{
		std::cout << parser;
		return 0;
	}
	catch (args::ParseError& e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return 1;
	}
	catch (args::ValidationError& e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return 1;
	}

	if (version) {
		std::cout << "IbexSolve Release " << _IBEX_RELEASE_ << std::endl;
		exit(0);
	}

	if (format) {
		std::cout << CovSolverData::format() << std::endl;
		exit(0);
	}

	if (filename.Get()=="") {
		ibex_error("no input file (try ibexsolve --help)");
		exit(1);
	}

	try {

		if (!quiet) {
			std::cout << std::endl << "***************************** setup *****************************" << std::endl;
			std::cout << "  file loaded:\t\t" << filename.Get() << std::endl;

			if (eps_x_min)
				std::cout << "  eps-x:\t\t" << eps_x_min.Get() << "\t(precision on variables domain)" << std::endl;

			if (simpl_level)
				std::cout << "  symbolic simpl level:\t" << simpl_level.Get() << "\t" << std::endl;

			// Fix the random seed for reproducibility.
			if (random_seed)
				std::cout << "  random-seed:\t\t" << random_seed.Get() << std::endl;

			if (bfs)
				std::cout << "  bfs:\t\t\tON" << std::endl;

			if (stop_at_first)
				std::cout << "  stop at first box found" << std::endl;
		}

		// Load a system of equations
		System sys(filename.Get().c_str(), simpl_level? simpl_level.Get() : ExprNode::default_simpl_level);

		std::string output_manifold_file; // manifold output file
		bool overwitten=false;       // is it overwritten?
		std::string manifold_copy;

		if (output_file) {
			output_manifold_file = output_file.Get();
		} else {
			// got from stackoverflow.com:
			std::string::size_type const p(filename.Get().find_last_of('.'));
			// filename without extension
			std::string filename_no_ext=filename.Get().substr(0, p);
			std::stringstream ss;
			ss << filename_no_ext << ".cov";
			output_manifold_file=ss.str();

			std::ifstream file;
			file.open(output_manifold_file.c_str(), std::ios::in); // to check if it exists

			if (file.is_open()) {
				overwitten = true;
				std::stringstream ss;
				ss << output_manifold_file << "~";
				manifold_copy=ss.str();
				// got from stackoverflow.com:
				std::ofstream dest(manifold_copy, std::ios::binary);

			    std::istreambuf_iterator<char> begin_source(file);
			    std::istreambuf_iterator<char> end_source;
			    std::ostreambuf_iterator<char> begin_dest(dest);
			    copy(begin_source, end_source, begin_dest);
			}
			file.close();
		}

		if (!quiet) {
			std::cout << "  output file:\t\t" << output_manifold_file << "\n";
		}

		// Build the default solver
		DefaultSolver s(sys,
				eps_x_min ? eps_x_min.Get() : DefaultSolver::default_eps_x_min,
				eps_x_max ? eps_x_max.Get() : DefaultSolver::default_eps_x_max,
				!bfs,
				random_seed? random_seed.Get() : DefaultSolver::default_random_seed);

		if (boundary_test_arg) {

			if (boundary_test_arg.Get()=="true")
				s.boundary_test = Solver::ALL_TRUE;
			else if (boundary_test_arg.Get()=="full-rank")
				s.boundary_test = Solver::FULL_RANK;
			else if (boundary_test_arg.Get()=="half-ball")
				s.boundary_test = Solver::HALF_BALL;
			else if (boundary_test_arg.Get()=="false")
				s.boundary_test = Solver::ALL_FALSE;
			else {
				std::cerr << "\nError: \"" << boundary_test_arg.Get() << "\" is not a valid option (try --help)\n";
				exit(0);
			}

			if (!quiet)
				std::cout << "  boundary test:\t" << boundary_test_arg.Get() << std::endl;
		}

		if (forced_params) {
			if (!quiet)
				std::cout << "  forced params:\t";
			SymbolMap<const ExprSymbol*> symbols;
			for (int i=0; i<sys.args.size(); i++)
				symbols.insert_new(sys.args[i].name, &sys.args[i]);

			std::string vars=args::get(forced_params);

			std::vector<const ExprNode*> params;
			int j;
			do {
				j=vars.find("+");
				if (j!=-1) {
					params.push_back(&parse_indexed_symbol(symbols,vars.substr(0,j)));
					vars=vars.substr(j+1,vars.size()-j-1);
 				} else {
 					params.push_back(&parse_indexed_symbol(symbols,vars));
 				}
				if (!quiet) std::cout << *params.back() << ' ';
			} while (j!=-1);

			if (!quiet) std::cout << std::endl;

			if (!params.empty()) {
				s.set_params(VarSet(sys.f_ctrs,params,false)); //Array<const ExprNode>(params)));
				for (std::vector<const ExprNode*>::iterator it=params.begin(); it!=params.end(); it++) {
					cleanup(**it,false);
				}
			}
		}

		// This option limits the search time
		if (timeout) {
			if (!quiet)
				std::cout << "  timeout:\t\t" << timeout.Get() << "s" << std::endl;
			s.time_limit=timeout.Get();
		}

		// This option prints each better feasible point when it is found
		if (trace) {
			if (!quiet)
				std::cout << "  trace:\t\tON" << std::endl;
			s.trace=trace.Get();
		}

		if (!quiet) {
			std::cout << "*****************************************************************" << std::endl << std::endl;
		}

		if (strcmp(_IBEX_LP_LIB_,"NONE")==0) {
			ibex_warning("No LP solver available, which may impact performances \n\t\t(try cmake with -DLP_LIB=...)");
			std::cout << std::endl;
		}

		if (!quiet)
			std::cout << "running............" << std::endl << std::endl;

		// Get the solutions
		if (input_file)
			s.solve(input_file.Get().c_str(), stop_at_first.Get());
		else
			s.solve(sys.box, stop_at_first.Get());

		if (trace) std::cout << std::endl;

		if (!quiet) s.report();

		if (sols) std::cout << s.get_data() << std::endl;

		s.get_data().save(output_manifold_file.c_str());

		if (!quiet) {
			std::cout << " results written in " << output_manifold_file << "\n";
			if (overwitten)
				std::cout << " (old file saved in " << manifold_copy << ")\n";
		}
		//		if (!quiet && !sols) {
//			cout << " (note: use --sols to display solutions)" << endl;
//		}

	}
	catch(ibex::UnknownFileException& e) {
		std::cerr << "Error: cannot read file '" << filename.Get() << "'" << std::endl;
	}
	catch(ibex::SyntaxError& e) {
		std::cout << e << std::endl;
	}
	catch(ibex::DimException& e) {
		std::cout << e << std::endl;
	}
}
