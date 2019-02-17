#include "datetime.hpp"

#include "util/strfmt.hpp"

#include <stdexcept>

DateTime::DateTime() :
	d(0), m(0), y(0), H(0), M(0), S(0), MS(0) {}

DateTime::DateTime(const DateTime& dt) :
	d(dt.d), m(dt.m), y(dt.y), H(dt.H), M(dt.M), S(dt.S), MS(dt.MS) {}

DateTime::DateTime(int d, int m, int y, int H, int M, int S, int MS) :
	d(d), m(m), y(y), H(H), M(M), S(S), MS(MS) {}

DateTime::DateTime(const std::string& str)
{
	try {
		y = std::stoi(str.substr(0, 4));
		m = std::stoi(str.substr(5, 2));
		d = std::stoi(str.substr(8, 2));
		H = std::stoi(str.substr(11, 2));
		M = std::stoi(str.substr(14, 2));
		S = std::stoi(str.substr(17, 2));
		MS = 0;
	}
	catch (std::exception& e) {
		throw std::invalid_argument("Invalid datetime");
	}
}

std::string DateTime::str() const
{
	return strfmt("%04d-%02d-%02d %02d:%02d:%02d", y, m, d, H, M, S);
}

bool operator==(const DateTime& dt1, const DateTime& dt2)
{
	return dt1.y==dt2.y && dt1.m==dt2.m && dt1.d==dt2.d && dt1.H==dt2.H && dt1.M==dt2.M && dt1.S==dt2.S && dt1.MS==dt2.MS;
}
bool operator!=(const DateTime& dt1, const DateTime& dt2) { return !(dt1 == dt2); }
bool operator<(const DateTime& dt1, const DateTime& dt2)
{
	return (
		(dt1.y<dt2.y) ||
		(dt1.y==dt2.y && dt1.m<dt2.m) ||
		(dt1.y==dt2.y && dt1.m==dt2.m && dt1.d<dt2.d) ||
		(dt1.y==dt2.y && dt1.m==dt2.m && dt1.d==dt2.d && dt1.H<dt2.H) ||
		(dt1.y==dt2.y && dt1.m==dt2.m && dt1.d==dt2.d && dt1.H==dt2.H && dt1.M<dt2.M) ||
		(dt1.y==dt2.y && dt1.m==dt2.m && dt1.d==dt2.d && dt1.H==dt2.H && dt1.M==dt2.M && dt1.S<dt2.S) ||
		(dt1.y==dt2.y && dt1.m==dt2.m && dt1.d==dt2.d && dt1.H==dt2.H && dt1.M==dt2.M && dt1.S==dt2.S && dt1.MS<dt2.MS)
	);
}
bool operator<=(const DateTime& dt1, const DateTime& dt2) { return dt1==dt2 || dt1<dt2; }
bool operator>(const DateTime& dt1, const DateTime& dt2) { return !(dt1==dt2 || dt1<dt2); }
bool operator>=(const DateTime& dt1, const DateTime& dt2) { return !(dt1<dt2); }
