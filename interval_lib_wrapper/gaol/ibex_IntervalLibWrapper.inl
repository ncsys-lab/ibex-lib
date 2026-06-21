#ifndef _IBEX_INTERVALLIBWRAPPER_INL_
#define _IBEX_INTERVALLIBWRAPPER_INL_

#include "ibex_Exception.h"

#include <limits>  // std::numeric_limits (smallest normal, for underflow_saturate)

#ifdef _WIN32
#include <float.h>
#endif

namespace ibex {

inline void fpu_round_down() {
	round_downward();
}

inline void fpu_round_up() {
	round_upward();
}

inline void fpu_round_near() {
	round_nearest();
}

inline double previous_float(double x) {
// ---------------------------------------------
// nextafter crashes on MaCOS with ARM64 architecture
// if rounding mode is not set to nearest.
#if __arm64__ || __aarch64__	
	GAOL_RND_PRESERVE();
	round_nearest();
	double y = gaol::previous_float(x);
	GAOL_RND_RESTORE();
	return y;
// ---------------------------------------------
#else
	return gaol::previous_float(x);
#endif
}

inline double next_float(double x) {
// ---------------------------------------------
// nextafter crashes on MaCOS with ARM64 architecture
// if rounding mode is not set to nearest.
#if __arm64__ || __aarch64__	
	GAOL_RND_PRESERVE();
	round_nearest();
	double y = gaol::next_float(x);
	GAOL_RND_RESTORE();
	return y;
#else
// ---------------------------------------------
	return gaol::next_float(x);
#endif
}

//inline void fpu_round_zero() {
//	round_zero();
//}

inline Interval::Interval(const gaol::interval& x) : itv(x) {

}

inline Interval& Interval::operator=(const gaol::interval& x) {
	this->itv = x;
	return *this;
}

inline Interval& Interval::operator+=(double d) {
	if (d==POS_INFINITY || d==NEG_INFINITY)
		set_empty();
	else
		itv+=d;
	return *this;
}

inline Interval& Interval::operator-=(double d) {
	if (d==POS_INFINITY || d==NEG_INFINITY)
		set_empty();
	else
		itv-=d;
	return *this;

}

inline Interval& Interval::operator*=(double d) {
	if (d==POS_INFINITY || d==NEG_INFINITY)
		set_empty();
	else
		itv*=d;
	return *this;
}

inline Interval& Interval::operator/=(double d) {
	if (d==POS_INFINITY || d==NEG_INFINITY)
		set_empty();
	else
		itv/=d;
	return *this;
}

inline Interval& Interval::operator+=(const Interval& x) {
	itv+=x.itv;
	return *this;
}

inline Interval& Interval::operator-=(const Interval& x) {
	itv-=x.itv;
	return *this;
}

inline Interval& Interval::operator*=(const Interval& x) {
	itv*=x.itv;
	return *this;
}

inline Interval& Interval::operator/=(const Interval& x) {
	//return div2_inter(*this,x);
	itv/=x.itv; // question: does Gaol perform generalized division here?
	return *this;
}

/*inline Interval Interval::operator+() const {
	return *this;
}*/

inline Interval Interval:: operator-() const {
	return -itv;
}

inline Interval& Interval::div2_inter(const Interval& x, const Interval& y) {
	Interval out2;
	div2_inter(x,y,out2);
	return *this |= out2;
}

inline void Interval::set_empty() {
	*this = EMPTY_SET;
}

inline Interval& Interval::operator&=(const Interval& x) {
	itv&=x.itv;
	return *this;
}

inline Interval& Interval::operator|=(const Interval& x) {
	itv|=x.itv;
	return *this;
}

inline double Interval::lb() const {
	return itv.left();
}

inline double Interval::ub() const {
	return itv.right();
}

inline double Interval::mid() const {
	double m=itv.midpoint();
	//fpu_round_up();
	return m;
}

inline bool Interval::is_empty() const {
	return itv.is_empty();
}

inline bool Interval::is_degenerated() const {
	return is_empty() || itv.is_a_double();
}

inline bool Interval::is_unbounded() const {
	return !itv.is_finite();
}


inline double Interval::diam() const {
	double d=itv.width();
	//fpu_round_up();
	return d;
}

inline double Interval::mig() const {
	return itv.mig();
}

inline double Interval::mag() const {
	return itv.mag();
}

inline Interval operator&(const Interval& x1, const Interval& x2) {
	if (x1.is_empty() || x2.is_empty() || x1.ub()<x2.lb())
		return Interval::empty_set();
	else
		return x1.itv & x2.itv;
}

inline Interval operator|(const Interval& x1, const Interval& x2) {
	return x1.itv | x2.itv;
}

inline double hausdorff(const Interval &x1, const Interval &x2) {
//	double h=hausdorff(x1.itv,x2.itv);
//	//fpu_round_up();
//	return h;
	return hausdorff(x1.itv,x2.itv);
}

inline Interval operator+(const Interval& x, double d) {
	if(d==NEG_INFINITY || d==POS_INFINITY)
		return Interval::empty_set();
	else
		return x.itv+d;
}

inline Interval operator-(const Interval& x, double d) {
	if(d==NEG_INFINITY || d==POS_INFINITY)
		return Interval::empty_set();
	else
		return x.itv-d;
}

inline Interval operator*(const Interval& x, double d) {
	if(d==NEG_INFINITY || d==POS_INFINITY)
		return Interval::empty_set();
	else
		return x.itv*d;
}

inline Interval operator/(const Interval& x, double d) {
	if(d==NEG_INFINITY || d==POS_INFINITY)
		return Interval::empty_set();
	else
		return x.itv/d;
}

inline Interval operator+(double d,const Interval& x) {
	if(d==NEG_INFINITY || d==POS_INFINITY)
		return Interval::empty_set();
	else
		return d+x.itv;
}

inline Interval operator-(double d, const Interval& x) {
	if(d==NEG_INFINITY || d==POS_INFINITY)
		return Interval::empty_set();
	else
		return d-x.itv;
}

inline Interval operator*(double d, const Interval& x) {
	if(d==NEG_INFINITY || d==POS_INFINITY)
		return Interval::empty_set();
	else
		return d*x.itv;
}

inline Interval operator/(double d, const Interval& x) {
	if(d==NEG_INFINITY || d==POS_INFINITY)
		return Interval::empty_set();
	else
		return d/x.itv;
}

inline Interval operator+(const Interval& x1, const Interval& x2) {
	return x1.itv+x2.itv;
}

inline Interval operator-(const Interval& x1, const Interval& x2) {
	return x1.itv-x2.itv;
}

inline Interval operator*(const Interval& x1, const Interval& x2) {
	return x1.itv*x2.itv;
}

inline Interval operator/(const Interval& x1, const Interval& x2) {
	return x1.itv/x2.itv;
}

inline Interval sqr(const Interval& x) {
	return gaol::sqr(x.itv);
}

inline Interval sqrt(const Interval& x) {
	Interval res=gaol::sqrt(x.itv);
	//fpu_round_up();
	return res;
}

inline Interval pow(const Interval& x, int n) {
	return gaol::pow(x.itv,n);
}

inline Interval pow(const Interval &x, double d) {
	if(d==NEG_INFINITY || d==POS_INFINITY)
		return Interval::empty_set();
	else {
		// gaol exposes pow(interval, interval) but not pow(interval, double):
		// the scalar overload silently fails on fractional d (e.g.
		// pow([1,4],0.5) returns [1,1] instead of [1,2]). Wrap d to a
		// degenerate interval to take the (interval, interval) overload.
		Interval res=gaol::pow(x.itv, gaol::interval(d, d));
		//fpu_round_up();
		return res;
	}
}

inline Interval pow(const Interval &x, const Interval &y) {
	Interval res=gaol::pow(x.itv, y.itv);
	//fpu_round_up();
	return res;
}

inline Interval root(const Interval& x, int n) {

	// get the root of the positive part (gaol does
	// not consider negative values to be in the definition
	// domain of the root function)
	gaol::interval res = gaol::nth_root(x.itv,n>=0? n : -n);

	if (n%2==1 && x.lb()<0) {
		res |= -gaol::nth_root(-x.itv,n>=0? n : -n);
	}

	if (n<0) res = 1.0/res;

	//fpu_round_up();
	return res;
}

inline Interval exp(const Interval& x) {
	Interval res = gaol::exp(x.itv);
	//fpu_round_up();
	return res;
}

inline Interval log(const Interval& x) {
	if (x.ub()<0) // gaol returns (-oo,-DBL_MAX) if x.ub()==0, instead of EMPTY_SET; use strict < so log([0,0]) yields (-oo, -DBL_MAX], not empty.
		return Interval::empty_set();
	else {
		Interval res=gaol::log(x.itv);
		//fpu_round_up();
		return res;
	}
}

inline Interval cos(const Interval& x) {
	Interval res = gaol::cos(x.itv);
	//fpu_round_up();
	return res;
}

inline Interval sin(const Interval& x) {
	Interval res = gaol::sin(x.itv);
	//fpu_round_up();
	return res;
}

inline Interval tan(const Interval& x) {
	Interval res = gaol::tan(x.itv);
	//fpu_round_up();
	return res;
}

inline Interval acos(const Interval& x) {
	Interval res = gaol::acos(x.itv);
	//fpu_round_up();
	return res;
}

inline Interval asin(const Interval& x) {
	Interval res = gaol::asin(x.itv);
	//fpu_round_up();
	return res;
}

inline Interval atan(const Interval& x) {
	Interval res = gaol::atan(x.itv);
	//fpu_round_up();
	return res;
}

inline Interval cosh(const Interval& x) {
	Interval res;
	if (x.is_unbounded()) 
		res=Interval(gaol::cosh(x.itv).left(),POS_INFINITY);
	else
		res=gaol::cosh(x.itv);
	//fpu_round_up();
	return res;
}

inline Interval sinh(const Interval& x) {
	Interval res = gaol::sinh(x.itv);
	//fpu_round_up();
	return res;
}

inline Interval tanh(const Interval& x) {
	Interval res = gaol::tanh(x.itv);
	//fpu_round_up();
	return res;
}

inline Interval acosh(const Interval& x) {
	Interval res = gaol::acosh(x.itv);
	//fpu_round_up();
	return res;
}

inline Interval asinh(const Interval& x) {
	Interval res;
	if (x.is_empty()) res=Interval::empty_set();
	else if (x.lb()>=0) res=gaol::asinh(x.itv);
	else if (x.ub()<=0) res=-gaol::asinh(-x.itv);
	else {
		gaol::interval y1=gaol::asinh(gaol::interval(0,x.ub()));
		gaol::interval y2=gaol::asinh(gaol::interval(0,-x.lb()));
		res=Interval(-y2.right(),y1.right());
	}
	//fpu_round_up();
	return res;
}

inline Interval atanh(const Interval& x) {
	Interval res = gaol::atanh(x.itv);
	//fpu_round_up();
	return res;
}

inline Interval abs(const Interval &x) {
	return gaol::abs(x.itv);
}

inline Interval max(const Interval& x, const Interval& y) {
	return gaol::max(x.itv,y.itv);
}

inline Interval min(const Interval& x, const Interval& y) {
	return gaol::min(x.itv,y.itv);
}

inline Interval floor(const Interval& x) {
	return gaol::floor(x.itv);
}

inline Interval integer(const Interval& x) {
	return gaol::integer(x.itv);
}

inline Interval ceil(const Interval& x) {
	return gaol::ceil(x.itv);
}

// dreal/dreal4#321 underflow-consistency helper. A forward op (pow/exp/mul/...)
// soundly over-approximates an underflowed result up to the subnormal ceiling
// [0, DBL_TRUE_MIN] (resp. down to [-DBL_TRUE_MIN, 0]). A backward op that then
// inverts a target y lying entirely in that subnormal band via a *tight* gaol
// primitive (nth_root / sqrt_rel / div_rel / log) is tighter than the forward
// and can wrongly empty a feasible operand domain -> false unsat. Saturating y
// to include 0 before the tight inverse restores forward/backward consistency;
// it only ever ENLARGES y, so it can never prune a feasible point (sound). It
// fires only when the WHOLE interval lies in the subnormal band (every |value|
// <= the smallest normal), i.e. y could only have come from an underflowed
// forward result; a y that merely reaches down into the subnormal range but
// extends into the normal range (e.g. [DBL_TRUE_MIN, +inf]) has normal-magnitude
// preimages and must NOT be widened. So the test is on the endpoint *farthest*
// from 0 (ub for a positive interval, lb for a negative one).
inline Interval underflow_saturate(const Interval& y) {
	const double tiny = std::numeric_limits<double>::min();  // smallest normal
	if (0.0 < y.lb() && y.ub() <= tiny)  return Interval(0.0, y.ub());
	if (y.ub() < 0.0 && y.lb() >= -tiny) return Interval(y.lb(), 0.0);
	return y;
}

inline bool bwd_mul(const Interval& y, Interval& x1, Interval& x2) {
	const Interval ys = underflow_saturate(y);
	x1 = gaol::div_rel(ys.itv, x2.itv, x1.itv);
	x2 = gaol::div_rel(ys.itv, x1.itv, x2.itv);
	return (!x1.is_empty()) && (!x2.is_empty());
}

inline bool bwd_sqr(const Interval& y, Interval& x) {
	x = gaol::sqrt_rel(underflow_saturate(y).itv, x.itv);
	return !x.is_empty();
}

inline bool bwd_pow(const Interval& y, int expon, Interval& x) {

	// there is a bug in nth_root_rel
	//x = gaol::nth_root_rel(y.itv, expon, x.itv);

	// ---> follows a temporary copy of the code
	//      in ibex_bias_interval

	// Note: this implem assumes that we can compute root
	// with negative exponents (that is why the root function
	// has also been wrapped)

	// dreal/dreal4#321: keep the backward root consistent with the forward pow's
	// underflow over-approximation (see underflow_saturate above).
	const Interval ys = underflow_saturate(y);
	if (expon % 2 ==0) {
		Interval proj=root(ys,expon);
		Interval pos_proj= proj & x;
		Interval neg_proj = (-proj) & x;
		//std::cout << "expon=" << expon << " proj=" << proj << " x=" << x << std::endl;
		x = pos_proj | neg_proj;
	} else {
		x &= root(ys, expon);
	}

	return !x.is_empty();
}

inline bool bwd_pow(const Interval& , Interval& , Interval& ) {
	ibex_error("bwd_power(y,x1,x2) (with x1 and x2 intervals) not implemented yet with Gaol");
	return false;
}

inline bool bwd_cos(const Interval& y,  Interval& x) {
	x &= gaol::acos_rel(y.itv,x.itv);
	//fpu_round_up();
	return !x.is_empty();
}

inline bool bwd_sin(const Interval& y,  Interval& x) {
	/*//fpu_round_up();
	interval tmp=gaol::asin_rel(y.itv,x.itv);
	if (!x.itv.set_contains(tmp)) {
		std::cout << "bug x=" << x.itv << " y=" << y.itv << " tmp=" << tmp << std::endl;
	}*/
	x &= gaol::asin_rel(y.itv,x.itv);
	//fpu_round_up();
	return !x.is_empty();
}

inline bool bwd_tan(const Interval& y,  Interval& x) {
	x = gaol::atan_rel(y.itv,x.itv);
	//fpu_round_up();
	return !x.is_empty();
}

inline bool bwd_cosh(const Interval& y,  Interval& x) {
	//x = gaol::acosh_rel(y.itv,x.itv);
	Interval proj=acosh(y);
	Interval pos_proj= proj & x;
	Interval neg_proj = (-proj) & x;

	x = pos_proj | neg_proj;

	//fpu_round_up();
	return !x.is_empty();
}

inline bool bwd_sinh(const Interval& y,  Interval& x) {
	//x = gaol::asinh_rel(y.itv,x.itv);
	x &= asinh(y);
	//fpu_round_up();
	return !x.is_empty();
}

inline bool bwd_tanh(const Interval& y,  Interval& x) {
	//x = gaol::atanh_rel(y.itv,x.itv);
	x &= atanh(y);
	//fpu_round_up();
	return !x.is_empty();
}

inline bool bwd_abs(const Interval& y,  Interval& x) {
	x = gaol::invabs_rel(y.itv,x.itv);
	return !x.is_empty();
}

} // end namespace ibex

#endif /* _IBEX_INTERVALLIBWRAPPER_INL_ */
