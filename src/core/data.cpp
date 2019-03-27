#include "data.hpp"

static Data* ctxDefault = NULL;

Data::Data() :
	original(4),
	preproc(2),
	recon(2),
	analysisSaver(2),
	analysisStats(2) {}

Data* Data::getDefaultPtr()
{
	return ctxDefault;
}

void Data::setDefaultPtr(Data* ctx)
{
	ctxDefault = ctx;
}
