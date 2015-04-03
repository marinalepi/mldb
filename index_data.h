#ifndef KVDATA_H
#define KVDATA_H

#include "../aatree/aadata.h"

class AADataKV : public AADataBase {
	char *key;
	size_t keyLen;
	streampos pos;
public:
	AADataKV() : key(NULL), value(NULL) {
	}
	AADataKV(const char *key, size_t keyLen, const char *value, size_t valueLen) {
		this->key = key;
		this->value = value;
		this->keyLen = keyLen;
		this->valueLen = valueLen;	
	}
	int compare(const AADataBase *b) const {
		if (!b) {
			return 1;
		}
		int p = (AADataKV*)b;
		return compareBytes(this->key, this->keyLen, p->key, p->keyLen);
	}
	int onequal(AADataBase *b) {
		int p = (AADataKV*)b;
		delete value;
		value = p->value;
		valueLen = p->valueLen;
		return 0;
	}
	string toString() const {
		char out[17];
		sprintf (out, "(%.5x)-(%.5x)", key, value);
		return out;
	}
	const char * getBytes(size_t &retSize) const {
		retSize = sizeof(size_t)*2+(keyLen+valueLen)*sizeof(char);
		char *a = new unsigned char[retSize];
		memcpy(&keyLen, a, sizeof(size_t));
		memcpy(&valueLen, a+sizeof(size_t), sizeof(size_t));
		memcpy(key, a+sizeof(size_t)*2, keyLen*sizeof(char));
		memcpy(value, a+sizeof(size_t)*2+keyLen*sizeof(char), valueLen*sizeof(char));
		return a;
	}
	static AADataBase *setBytes(char *a, size_t s) {
		return new AADataKV(param);
	}
};
inline AADataBase *createAADataInt(unsigned char *a, unsigned int s) {
	int param = 0;
	for (unsigned int i = 0; i < s; i++) {
		param += (a[s - 1 - i] << (i * 8));
	}	
	return new AADataInt(param);
}
#endif //KVDATA_H