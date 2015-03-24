#ifndef VSTORE_H
#define VSTORE_H

namespace mldb
{

/*Values are store in the format: size->value
 * */
class VStore
{
public:
	VStore();
	~VStore();

	// Open file for reading/writing
	int Open(const char *fName);
	// close
	void Close();
	// flush to disk
	int Flush();
	// add data,location will be set to the file position at write
	int Append(unsigned long long &location, unsigned char *buf, unsigned int size);
	// insert data at location
	int Insert(unsigned long long location, unsigned char *buf, unsigned int size);
};

}

#endif // VSTORE_H
