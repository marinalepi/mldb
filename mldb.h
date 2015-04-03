#ifndef MLDB_H
#define MLDB_H

namespace mldb
{

class MLDB
{
public:
	MLDB();
	~MLDB();

	int Add(const char *key, size_t keyLen, const unsigned char *value, size_t valueLen);
	int Delete(const char *key, size_t keyLen);
	int Get(const char *key, size_t keyLen, const char *&value, size_t &valueLen);
	class Iterator {
		char *key;
		char *value;
		size_t keyLen, valueLen;
	public:
		const char * getKey(size_t &keyLen);
		const char * getValue(size_t &valueLen);
		int next();
		int isValid();
	}
	Iterator *CreateIterator(const char *key, size_t keyLen);
	void DestroyIterator(Iterator *it);
};

}

#endif // MLDB_H
