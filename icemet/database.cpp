#include "database.hpp"

#include <stdexcept>

#include "icemet/util/log.hpp"

#define FLOAT_REPR "{:.24f}"

static const char* createDBQuery = "CREATE DATABASE IF NOT EXISTS `{}`;";
static const char* createParticlesTableQuery = "CREATE TABLE IF NOT EXISTS `{}` ("
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
static const char* createStatsTableQuery = "CREATE TABLE IF NOT EXISTS `{}` ("
"ID INT UNSIGNED NOT NULL AUTO_INCREMENT,"
"DateTime DATETIME NOT NULL,"
"LWC FLOAT NOT NULL,"
"MVD FLOAT NOT NULL,"
"Conc FLOAT NOT NULL,"
"Frames INT UNSIGNED NOT NULL,"
"Particles INT UNSIGNED NOT NULL,"
"Temp FLOAT,"
"Wind FLOAT,"
"PRIMARY KEY (ID),"
"INDEX (DateTime)"
");";
static const char* createMetaTableQuery = "CREATE TABLE IF NOT EXISTS `{}` ("
"ID INT UNSIGNED NOT NULL AUTO_INCREMENT,"
"DateTime DATETIME NOT NULL,"
"ParticlesTable TEXT NOT NULL,"
"StatsTable TEXT NOT NULL,"
"Version TEXT NOT NULL,"
"Config TEXT NOT NULL,"
"PRIMARY KEY (ID),"
"INDEX (DateTime)"
");";
static const char* insertParticleQuery = "INSERT INTO `{}` ("
"DateTime, Sensor, Frame, Particle, X, Y, Z, EquivDiam, EquivDiamCorr, Circularity, DynRange, EffPxSz, SubX, SubY, SubW, SubH"
") VALUES ("
"'{}', {}, {}, {}, " FLOAT_REPR ", " FLOAT_REPR ", " FLOAT_REPR ", " FLOAT_REPR ", " FLOAT_REPR ", " FLOAT_REPR ", {}, " FLOAT_REPR ", {}, {}, {}, {}"
");";
static const char* insertStatsQuery = "INSERT INTO `{}` ("
"DateTime, LWC, MVD, Conc, Frames, Particles, Temp, Wind"
") VALUES ("
"'{}', " FLOAT_REPR ", " FLOAT_REPR ", " FLOAT_REPR ", {}, {}, {}, {}"
");";
static const char* insertMetaQuery = "INSERT INTO `{}` ("
"ID, DateTime, ParticlesTable, StatsTable, Version, Config"
") VALUES ("
"NULL, '{}', '{}', '{}', '{}', '{}'"
");";
static const char* selectParticlesQuery = "SELECT "
"ID, DateTime, Sensor, Frame, Particle, X, Y, Z, EquivDiam, EquivDiamCorr, Circularity, DynRange, EffPxSz, SubX, SubY, SubW, SubH "
"FROM `{}` ORDER BY DateTime ASC;";

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

MYSQL_RES* Database::exec(const std::string& sql, bool storeRes)
{
	lock();
	
	// Make sure we're still connected
	if (mysql_ping(m_mysql)) {
		close();
		connect(m_connInfo);
		open(m_dbInfo);
	}
	
	// Query
	mysql_query(m_mysql, sql.c_str());
	MYSQL_RES* res = storeRes ? mysql_store_result(m_mysql) : NULL;
	
	// Check for errors
	std::string err = mysql_error(m_mysql);
	if (!err.empty())
		throw std::runtime_error(strfmt("SQL error: {}", err));
	
	unlock();
	return res;
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
	query("SET sql_notes = 0;");
	query(createDBQuery, dbInfo.name);
	mysql_select_db(m_mysql, dbInfo.name.c_str());
	if (!dbInfo.particlesTable.empty())
		query(createParticlesTableQuery, dbInfo.particlesTable);
	if (!dbInfo.statsTable.empty())
		query(createStatsTableQuery, dbInfo.statsTable);
	if (!dbInfo.metaTable.empty())
		query(createMetaTableQuery, dbInfo.metaTable);
	query("SET sql_notes = 1;");
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
		insertParticleQuery, m_dbInfo.particlesTable,
		row.dt.str(),
		row.sensor, row.frame, row.particle,
		row.x, row.y, row.z,
		row.diam, row.diamCorr,
		row.circularity, row.dynRange, row.effPxSz,
		row.sub.x, row.sub.y, row.sub.width, row.sub.height
	);
}

void Database::writeStats(const StatsRow& row)
{
	std::string temp = IS_NAN(row.temp) ? "NULL" : strfmt(FLOAT_REPR, row.temp);
	std::string wind = IS_NAN(row.wind) ? "NULL" : strfmt(FLOAT_REPR, row.wind);
	query(
		insertStatsQuery, m_dbInfo.statsTable,
		row.dt.str(),
		row.lwc, row.mvd, row.conc,
		row.frames, row.particles,
		temp, wind
	);
}

void Database::writeMeta(const MetaRow& row)
{
	query(
		insertMetaQuery, m_dbInfo.metaTable,
		row.dt.str(),
		row.particlesTable,
		row.statsTable,
		row.version.str(),
		row.config
	);
}

bool Database::readParticles(DatabaseIterator& iter, ParticleRow& par)
{
	if (!iter.m_res)
		iter.m_res = queryRes(selectParticlesQuery, m_dbInfo.particlesTable);
	
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
