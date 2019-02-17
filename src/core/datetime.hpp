#ifndef DATETIME_H
#define DATETIME_H

#include <string>

class DateTime {
public:
	int d, m, y;
	int H, M, S, MS;
	
	DateTime();
	DateTime(const DateTime& dt);
	DateTime(int d, int m, int y, int H, int M, int S, int MS);
	DateTime(const std::string& str);
	
	std::string str() const;
	
	friend bool operator==(const DateTime& dt1, const DateTime& dt2);
	friend bool operator!=(const DateTime& dt1, const DateTime& dt2);
	friend bool operator<(const DateTime& dt1, const DateTime& dt2);
	friend bool operator<=(const DateTime& dt1, const DateTime& dt2);
	friend bool operator>(const DateTime& dt1, const DateTime& dt2);
	friend bool operator>=(const DateTime& dt1, const DateTime& dt2);
};

#endif
