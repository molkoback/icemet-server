#include "database.hpp"

#include <stdexcept>

#include "util/log.hpp"
#include "util/strfmt.hpp"

#define FLOAT_REPR "%.24f"
#define MAX_ROWS "1000"

static const char* createDBQuery = "CREATE DATABASE %s;";
static const char* createParticleTableQuery = "CREATE TABLE %s ("
"ID INT UNSIGNED NOT NULL AUTO_INCREMENT,"
"DateTime DATETIME NOT NULL,"
"DateTimeMS SMALLINT UNSIGNED NOT NULL,"
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
"PRIMARY KEY (ID)"
");";
static const char* createStatsTableQuery = "CREATE TABLE %s ("
"ID INT UNSIGNED NOT NULL AUTO_INCREMENT,"
"DateTime DATETIME NOT NULL,"
"LWC FLOAT NOT NULL,"
"MVD FLOAT NOT NULL,"
"Conc FLOAT NOT NULL,"
"Frames INT UNSIGNED NOT NULL,"
"Particles INT UNSIGNED NOT NULL,"
"PRIMARY KEY (ID)"
");";
static const char* insertParticleQuery = "INSERT INTO %s ("
"ID, DateTime, DateTimeMS, Sensor, Frame, Particle, X, Y, Z, EquivDiam, EquivDiamCorr, Circularity, DynRange, EffPxSz, SubX, SubY, SubW, SubH"
")VALUES("
"NULL, '%s', %u, %u, %u, %u, " FLOAT_REPR ", " FLOAT_REPR ", " FLOAT_REPR ", " FLOAT_REPR ", " FLOAT_REPR ", " FLOAT_REPR ", %u, " FLOAT_REPR ", %u, %u, %u, %u"
");";
static const char* insertStatsQuery = "INSERT INTO %s ("
"ID, DateTime, LWC, MVD, Conc, Frames, Particles"
")VALUES("
"NULL, '%s', " FLOAT_REPR ", " FLOAT_REPR ", " FLOAT_REPR ", %u, %u"
");";
static const char* selectParticlesQuery = "SELECT "
"ID, DateTime, DateTimeMS, Sensor, Frame, Particle, X, Y, Z, EquivDiam, EquivDiamCorr, Circularity, DynRange, EffPxSz, SubX, SubY, SubW, SubH "
"FROM %s WHERE ID>=%u ORDER BY ID ASC LIMIT " MAX_ROWS ";";

static Database* dbDefault = NULL;

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
		row.dt.str().c_str(), row.dt.MS,
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

/* TODO: mysql_free_result() in case of failure */
void Database::readParticles(std::vector<ParticleRow>& rows, unsigned int minId)
{
	MYSQL_RES* res = queryRes(selectParticlesQuery, m_dbInfo.particleTable.c_str(), minId);
	int nrows = mysql_num_rows(res);
	for (int i = 0; i < nrows; i++) {
		MYSQL_ROW row = mysql_fetch_row(res);
		DateTime dt(row[1]);
		dt.MS = std::stoi(row[2]);
		rows.push_back({
			(unsigned int)std::stoi(row[0]), dt,
			(unsigned int)std::stoi(row[3]), (unsigned int)std::stoi(row[4]), (unsigned int)std::stoi(row[5]),
			std::stof(row[6]), std::stof(row[7]), std::stof(row[8]),
			std::stof(row[9]), std::stof(row[10]), 
			std::stof(row[11]), (unsigned char)std::stoi(row[12]), std::stof(row[13]),
			cv::Rect(std::stoi(row[14]), std::stoi(row[15]), std::stoi(row[16]), std::stoi(row[17]))
		});
	}
	mysql_free_result(res);
}

Database* Database::getDefaultPtr()
{
	return dbDefault;
}

void Database::setDefaultPtr(Database* cfg)
{
	dbDefault = cfg;
}
