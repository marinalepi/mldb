#ifndef INDEX_H
#define INDEX_H

using namespace std;

namespace mldb
{

class Index
{
	string path;
	string prefix;

	typedef struct TreeData {
		AATree *tree;
		BloomFilter *filter;
		unsigned int nodeCount;
		TreeData() : tree(NULL), filter(NULL), nodeCount(0) {};
		~TreeData() {
			if (tree) {
				delete tree;
			}
			if (filter) {
				delete filter;
			}
		}
		static TreeData* create ();
	} TreeData;
	// Cache
	string filePath;	
	TreeData *cache;

	map<string, TreeData*> index;
	createDataElement foo;
	
	vector<string> scanPath();
	int readFilterData(const char *fileName, TreeData* &d);
	int fetchTree();
	int saveTree();
	void updateFilter(AADataBase* data);
	int add(AADataBase* data);
	int treeToCache(string name, TreeData *d);
	string nextFilename();
	
public:
	Index(createDataElement =  createAADataBase);
	~Index();

	// scan the path, finds the prefix files, initializes the index
	int Init(const char *path, const char *prefix);
	// remove index, delete files if needed
	int Destroy(bool removeFiles);
	// add index data
	int Add(AADataBase* data);
	// get index data
	int Get(AADataBase* &data);
	int Delete(AADataBase* data);
};

}

#endif // INDEX_H
