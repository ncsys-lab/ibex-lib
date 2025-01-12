//============================================================================
//                                  I B E X                                   
// File        : swim01.cpp
// Author      : Gilles Chabert
// Copyright   : Ecole des Mines de Nantes (France)
// License     : See the LICENSE file
// Created     : Jun 4, 2013
// Last Update : Jun 4, 2013
//============================================================================


#include "ibex.h"
#include "data.h"

using namespace ibex;

int main() {

  init_data();

  // ![dist-decl]
  // create the distance function beforehand
  Variable a(2);       // "local" variable
  Variable b(2);
  Function dist(a,b,sqrt(sqr(a[0]-b[0])+sqr(a[1]-b[1])));
  // ![dist-decl]
  // the initial box [0,L]x[0,L]x[0,L]x[0,L]
  IntervalVector box(T*2,Interval(0,L));
  Variable x(T,2);

  std::vector<Ctc*> ctc;
  for (int t=0; t<T; t++) {

	// ![inv]
    std::vector<Ctc*> cdist;
    for (int b=0; b<N; b++) {
      // Create the distance constraint with 2
      // (instead of 2*T) variables
      Variable xt(2);
      NumConstraint* c=new NumConstraint(
               xt,dist(xt,beacons[b])=d[t][b]);
      cdist.push_back(new CtcFwdBwd(*c));
    }

    // q-intersection with 2 variables only
    CtcQInter* q=new CtcQInter(cdist,N-NB_OUTLIERS);
    // Push in the main vector "ctc" the application
    // of the latter contractor to x[t]
    ctc.push_back(new CtcInverse(*q,*new Function(x,x[t])));
	// ![inv]

    if (t<T-1) {
      NumConstraint* c=new NumConstraint(x,x[t+1]-x[t]=transpose(v[t]));
      ctc.push_back(new CtcFwdBwd(*c));
    }
  }
  // Fixpoint
  CtcCompo compo(ctc);

  // FixPoint
  CtcFixPoint fix(compo);

  std::cout << std::endl << "initial box =" << box << std::endl;
  fix.contract(box);
  std::cout << std::endl << "final box =" << box << std::endl << std::endl;

  return 0;
}
