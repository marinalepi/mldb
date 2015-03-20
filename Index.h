#ifndef INDEX_H
#define INDEX_H

#include "../aatree.h"

namespace mldb
{

/*This class stores key->location pair in the AATree
 * 
 * */
class Index
{
	AATree *tree;
public:
	Index();
	~Index();

	// read tree from disk
	int Load();
	// save tree to disk
	int Save();
	// flush tree to disk
	int Flush();
	// insert a new key->location pair
	int Insert(unsigned char key, unsigned long long location);
	// get location for the key
	unsigned long long Get(unsigned char key);
	int Remove(unsigned char key);
};

}

#endif // INDEX_H
