#include "mldb.h"

namespace mldb
{

MLDB::MLDB()
{
}

MLDB::~MLDB()
{
}

int MLDB::Add(const unsigned char *key, unsigned int keyLen, const unsigned char *value, unsigned int valueLen) {
	
}
	int Delete(const unsignedchar *key, unsigned int keyLen);
	int Get(const unsigned char *key, unsigned int keyLen, const unsigned char *&value, unsigned int &valueLen);
	class Iterator {
		unsigned char *key;
		unsigned char *value;
		unsigned int keyLen, valueLen;
	public:
		const unsigned char * getKey(unsigned int &keyLen);
		const unsigned char * getValue(unsigned int &valueLen);
		int next();
		int isValid();
	}
	Iterator *CreateIterator(const unsigned char *key, unsigned int keyLen);
	void DestroyIterator(Iterator *it);

}

