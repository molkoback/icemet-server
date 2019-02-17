#ifndef DATABASE_H
#define DATABASE_H

#include "core/file.hpp"

#include <mysql/mysql.h>
#include <opencv2/core.hpp>

#include <cstdarg>
#include <mutex>
#include <string>
#include <vector>

typedef struct _connection_info {
	std::string host;
	int port;
	std::string user;
	std::string passwd;
} ConnectionInfo;

typedef struct _database_info {
	std::string name;
	std::string particleTable;
	std::string statsTable;
} DatabaseInfo;

typedef struct _particle_row {
	unsigned int id;
	DateTime dt;
	unsigned int sensor;
	unsigned int frame;
	unsigned int particle;
	float x, y, z;
	float diam;
	float circularity;
	unsigned char dnr;
	float effpsz;
	cv::Rect sub;
} ParticleRow;

typedef struct _stats_row {
	unsigned int id;
	DateTime dt;
	float lwc;
	float mvd;
	float conc;
	unsigned int frames;
	unsigned int particles;
} StatsRow;

class Database {
private:
	ConnectionInfo m_connInfo;
	DatabaseInfo m_dbInfo;
	MYSQL* m_mysql;
	std::mutex m_mutex;
	
	void exec(const char *fmt, va_list args);
	void query(const char *fmt, ...);
	MYSQL_RES* queryRes(const char *fmt, ...);
	bool tableExists(const char *table);

public:
	Database();
	Database(const ConnectionInfo& connInfo, const DatabaseInfo& dbInfo);
	~Database();
	
	void connect(const ConnectionInfo& connInfo);
	void open(const DatabaseInfo& dbInfo);
	void close();
	
	void lock() { m_mutex.lock(); }
	void unlock() { m_mutex.unlock(); }
	
	void writeParticle(const ParticleRow& row);
	void writeStats(const StatsRow& row);
	
	void readParticles(std::vector<ParticleRow>& rows, unsigned int minId=0);
	
	static Database* getDefaultPtr();
	static void setDefaultPtr(Database* cfg);
};

#endif
