#ifndef ICEMET_DATABASE_H
#define ICEMET_DATABASE_H

#include "icemet/file.hpp"
#include "icemet/util/time.hpp"
#include "icemet/util/version.hpp"

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
	std::string particlesTable;
	std::string statsTable;
	std::string metaTable;
} DatabaseInfo;

typedef struct _particle_row {
	unsigned int id;
	DateTime dt;
	unsigned int sensor;
	unsigned int frame;
	unsigned int particle;
	float x, y, z;
	float diam;
	float diamCorr;
	float circularity;
	unsigned char dynRange;
	float effPxSz;
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

typedef struct _meta_row {
	unsigned int id;
	DateTime dt;
	std::string particlesTable;
	std::string statsTable;
	VersionInfo version;
	std::string config;
} MetaRow;

class DatabaseIterator;

class Database {
private:
	ConnectionInfo m_connInfo;
	DatabaseInfo m_dbInfo;
	MYSQL* m_mysql;
	std::mutex m_mutex;
	
	void exec(const char *fmt, va_list args);
	void query(const char *fmt, ...);
	MYSQL_RES* queryRes(const char *fmt, ...);

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
	void writeMeta(const MetaRow& row);
	
	bool readParticles(DatabaseIterator& iter, ParticleRow& par);
};

class DatabaseIterator {
private:
	MYSQL_RES* m_res;

public:
	DatabaseIterator() : m_res(NULL) {}
	DatabaseIterator(const DatabaseIterator& iter) : m_res(iter.m_res) {}
	~DatabaseIterator();
	friend Database;
};

#endif
