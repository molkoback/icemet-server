#include "database.hpp"

#include <stdexcept>

#include "icemet/util/log.hpp"
#include "icemet/util/strfmt.hpp"

#define FLOAT_REPR "%.24f"

static const char* createDBQuery = "CREATE DATABASE `%s`;";
static const char* createParticleTableQuery = "CREATE TABLE `%s` ("
"ID INT UNSIGNED NOT NULL AUTO_INCREMENT,"
"DateTime DATETIME(3) NOT NULL,"
"Sensor TINYINT UNSIGNED NOT NULL,"
"Frame INT UNSIGNED NOT NULL,"
"Particle INT UNSIGNED NOT NULL,"
"X FLOAT NOT NULL,"
"Y FLOAT NOT NULL,"
"Z FLOAT NOT NULL,"
"EquivDiam FLOAT NOT NULL,"
"EquivDiamCorr FLOAT NOT NULL,"
"Circularity FLOAT NOT NULL,"
"DynRange TINYINT UNSIGNED NOT NULL,"
"EffPxSz FLOAT NOT NULL,"
"SubX INT UNSIGNED NOT NULL,"
"SubY INT UNSIGNED NOT NULL,"
"SubW INT UNSIGNED NOT NULL,"
"SubH INT UNSIGNED NOT NULL,"
"PRIMARY KEY (ID),"
"INDEX (DateTime)"
");";
static const char* createStatsTableQuery = "CREATE TABLE `%s` ("
"ID INT UNSIGNED NOT NULL AUTO_INCREMENT,"
"DateTime DATETIME NOT NULL,"
"LWC FLOAT NOT NULL,"
"MVD FLOAT NOT NULL,"
"Conc FLOAT NOT NULL,"
"Frames INT UNSIGNED NOT NULL,"
"Particles INT UNSIGNED NOT NULL,"
"PRIMARY KEY (ID),"
"INDEX (DateTime)"
");";
static const char* insertParticleQuery = "INSERT INTO `%s` ("
"ID, DateTime, Sensor, Frame, Particle, X, Y, Z, EquivDiam, EquivDiamCorr, Circularity, DynRange, EffPxSz, SubX, SubY, SubW, SubH"
")VALUES("
"NULL, '%s', %u, %u, %u, " FLOAT_REPR ", " FLOAT_REPR ", " FLOAT_REPR ", " FLOAT_REPR ", " FLOAT_REPR ", " FLOAT_REPR ", %u, " FLOAT_REPR ", %u, %u, %u, %u"
");";
static const char* insertStatsQuery = "INSERT INTO `%s` ("
"ID, DateTime, LWC, MVD, Conc, Frames, Particles"
")VALUES("
"NULL, '%s', " FLOAT_REPR ", " FLOAT_REPR ", " FLOAT_REPR ", %u, %u"
");";
static const char* selectParticlesQuery = "SELECT "
"ID, DateTime, Sensor, Frame, Particle, X, Y, Z, EquivDiam, EquivDiamCorr, Circularity, DynRange, EffPxSz, SubX, SubY, SubW, SubH "
"FROM `%s` ORDER BY DateTime ASC;";

Database::Database() :
	m_mysql(NULL) {}

Database::Database(const ConnectionInfo& connInfo, const DatabaseInfo& dbInfo)
{
	connect(connInfo);
	open(dbInfo);
}

Database::~Database()
{
	close();
}

void Database::exec(const char* fmt, va_list args)
{
	// Make sure we're still connected
	if (mysql_ping(m_mysql)) {
		close();
		connect(m_connInfo);
		open(m_dbInfo);
	}
	
	// Query
	mysql_query(m_mysql, vstrfmt(fmt, args).c_str());
	
	// Check for errors
	std::string err = mysql_error(m_mysql);
	if (!err.empty())
		throw std::runtime_error(strfmt("SQL error: %s", err.c_str()));
}

void Database::query(const char* fmt, ...)
{
	lock();
	
	va_list args;
	va_start(args, fmt);
	exec(fmt, args);
	va_end(args);
	
	unlock();
}

MYSQL_RES* Database::queryRes(const char* fmt, ...)
{
	lock();
	
	va_list args;
	va_start(args, fmt);
	exec(fmt, args);
	va_end(args);
	
	MYSQL_RES* res = mysql_store_result(m_mysql);
	
	unlock();
	return res;
}

bool Database::tableExists(const char* table)
{
	MYSQL_RES* res = NULL;
	if (!(res = queryRes("SHOW TABLES LIKE \"%s\"", table)))
		return false;
	bool ret = mysql_num_rows(res);
	mysql_free_result(res);
	return ret;
}

void Database::connect(const ConnectionInfo& connInfo)
{
	if (!(m_mysql = mysql_init(NULL)))
		throw std::runtime_error("Couldnt initialize SQL handle");
	m_mysql = mysql_real_connect(
		m_mysql,
		connInfo.host.c_str(),
		connInfo.user.c_str(), connInfo.passwd.c_str(),
		NULL, 
		connInfo.port,
		NULL, 0
	);
	if (!m_mysql)
		throw std::runtime_error("Couldnt connect to SQL server");
	
	m_connInfo = connInfo;
}

void Database::open(const DatabaseInfo& dbInfo)
{
	// Make sure that database exists
	if (mysql_select_db(m_mysql, dbInfo.name.c_str())) {
		query(createDBQuery, dbInfo.name.c_str());
		mysql_select_db(m_mysql, dbInfo.name.c_str());
	}
	
	// Make sure that tables exist
	if (!tableExists(dbInfo.particleTable.c_str()))
		query(createParticleTableQuery, dbInfo.particleTable.c_str());
	if (!tableExists(dbInfo.statsTable.c_str()))
		query(createStatsTableQuery, dbInfo.statsTable.c_str());
	
	m_dbInfo = dbInfo;
}

void Database::close()
{
	mysql_close(m_mysql);
	m_mysql = NULL;
}

void Database::writeParticle(const ParticleRow& row)
{
	query(
		insertParticleQuery, m_dbInfo.particleTable.c_str(),
		row.dt.str().c_str(),
		row.sensor, row.frame, row.particle,
		row.x, row.y, row.z,
		row.diam, row.diamCorr,
		row.circularity, row.dynRange, row.effPxSz,
		row.sub.x, row.sub.y, row.sub.width, row.sub.height
	);
}

void Database::writeStats(const StatsRow& row)
{
	query(
		insertStatsQuery, m_dbInfo.statsTable.c_str(),
		row.dt.str().c_str(),
		row.lwc, row.mvd, row.conc,
		row.frames, row.particles
	);
}

bool Database::readParticles(DatabaseIterator& iter, ParticleRow& par)
{
	if (!iter.m_res)
		iter.m_res = queryRes(selectParticlesQuery, m_dbInfo.particleTable.c_str());
	
	MYSQL_ROW row = mysql_fetch_row(iter.m_res);
	if (row == NULL)
		return false;
	
	par.id = (unsigned int)std::stoi(row[0]);
	par.dt = DateTime(row[1]);
	par.sensor = (unsigned int)std::stoi(row[2]);
	par.frame = (unsigned int)std::stoi(row[3]);
	par.particle = (unsigned int)std::stoi(row[4]);
	par.x = std::stof(row[5]);
	par.y = std::stof(row[6]);
	par.z = std::stof(row[7]);
	par.diam = std::stof(row[8]);
	par.diamCorr = std::stof(row[9]);
	par.circularity = std::stof(row[10]);
	par.dynRange = (unsigned char)std::stoi(row[11]);
	par.effPxSz = std::stof(row[12]);
	par.sub = cv::Rect(std::stoi(row[13]), std::stoi(row[14]), std::stoi(row[15]), std::stoi(row[16]));
	return true;
}

DatabaseIterator::~DatabaseIterator()
{
	if (m_res)
		mysql_free_result(m_res);
}
