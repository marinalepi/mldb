#include <stdio.h>
#include <fstream>
#include <dirent.h>
#include <vector>
#include <map>
#include <queue>
#include <sys/stat.h>

#include "../aatree/aatree.h"
#include "../bloomfilter/bloomfilter.h"
#include "index.h"

namespace mldb
{

#define MAXNUM_TREENODES 1000000

Index::Index(createDataElement f) : cache(NULL) {
	foo = f;
}

Index::~Index() {
	Destroy(false);
}

// scan the path, finds the prefix files, initializes the index
int Index::Init(const char *path, const char *prefix) {
	// save path and prefix
	this->path = path;
	this->prefix = prefix;
	// scan the path for prefixTADA files
	vector<string> files = scanPath();
	// for every found file:
	for (size_t i = 0; i<files.size(); i++) {
		// read <size><num_nodes><bloomfilter>
		TreeData *d;
		int result = readFilterData(files[i].c_str(), d);
		if (result != 0) {
			return result;
		}
		// save that index map: filename -> file data(tree & filter)
		index[files[i]] = d;
	}
	if (index.size() == 0) { // empty path
		// create new data
		TreeData *d = TreeData::create();
		if (!d) {
			return ENOMEM;
		}
		int result;
		if ((result = treeToCache(nextFilename(), d)) != 0) {
			delete d;
			return result;
		}
	}
	return 0;
}

// remove index, delete files
int Index::Destroy(bool removeFiles) {
	// for all the pairs from filter map:
	for (std::map<string, TreeData*>::iterator it = this->index.begin(); it != this->index.end(); ++it) {
		// remove file
		if (removeFiles) {
			remove (((string)it->first).c_str());
		}
		// delete index data, tree+
		delete it->second;
	}
	// clear map;
	this->index.clear();
	this->cache = NULL;
	this->filePath.clear();
	return 0;
}

// add index data
int Index::Add(AADataBase* data) {
	int result;
	
	// add data to the current tree
	if ((result = add(data)) != 0) {
		return result;
	}
	if (this->cache->nodeCount > MAXNUM_TREENODES) {
		return fetchTree();
	}
	return 0;
}

// get index data
int Index::Get(AADataBase* &data) {
	unsigned int size;
	const unsigned char * bytes = data->getBytes(size);
	
	const AATreeNode *foundNode = NULL;
	// if cache -> first check if data can be in the cache
	if (this->cache->filter->Check(bytes, size)) {
		foundNode = this->cache->tree->get(data);
	}
	for (std::map<string, TreeData*>::iterator it = this->index.begin(); !foundNode && it != this->index.end(); ++it) {
		// check on what tree(file) the data can be found, skip the cached tree
		TreeData* d = (TreeData*)it->second;
		if (d != this->cache && 
		    d->filter->Check(bytes, size)) {
			// for every positive - load file, do find data
			int result;
			if ((result = treeToCache((string)it->first, d)) == 0) {
				foundNode = this->cache->tree->get(data);
			} else {
				return result;
			}
		}
	}
	// got the node with data
	if (foundNode) {
		delete data;
		data = foundNode->data;
		return 0;
	}
	return -1;
}

int Index::Delete(AADataBase* data) {
	// do get
	int result;
	if ((result = Get(data)) != 0) {
		return result == -1? 0: result;
	}
	// found the data, delete it from the tree
	this->cache->tree->remove(data);
	this->cache->nodeCount--;		
	// update tree filter
	updateFilter(NULL);
	// saveTree
	return saveTree();
}

vector<string> Index::scanPath() {
	// scan the path, look for files with prefix name
	vector<string> flist;
	
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (this->path.c_str())) != NULL) {
		while ((ent = readdir (dir)) != NULL) {
			flist.push_back(ent->d_name);
		}
	}
	closedir (dir);
	return flist;
}

Index::TreeData* Index::TreeData::create() {
	TreeData *d = new TreeData();
	d->tree = new AATree();
	d->filter = new BloomFilter();
	if (!d->tree || !d->filter) {
		delete d;
		return NULL;
	}
	if (d->filter->Create(512, 2, murmur3, fnv) != 0) {
		delete d;
		return NULL;
	}
	return d;
}

int Index::readFilterData(const char *fileName, TreeData* &d) {
	int err = 0;
	// open file
	ifstream f;
	f.open(fileName, ios_base::in | ios_base::binary);
	if (!f.is_open()) {
		return errno;
	}
	// read size <unsigned int>
	unsigned int size;
	f.read(reinterpret_cast< char *>(&size), sizeof(size));
	if (f.fail()) {
		return EIO;
	}
	
	unsigned char *filterData = NULL;
	d = NULL;
		
	// read data - size*sizeof(unsigned char)
	filterData = new unsigned char[size];
	if (!filterData)  {
		err = ENOMEM;
		goto clean;
	}
	f.read((char*)filterData, size);
	if (f.fail()) {
		err = EIO;
		goto clean;
	}
	// read num nodes
	unsigned int nodeCount;
	f.read(reinterpret_cast< char *>(&nodeCount), sizeof(nodeCount));
	if (f.fail()) {
		err = EIO;
		goto clean;
	}
	
	d = TreeData::create();
	if (!d) {
		err = ENOMEM;
		goto clean;
	}
	d->nodeCount = nodeCount;
	err = d->filter->Set(filterData, size);
	
clean:
	f.close();
	if (err != 0) {
		if (filterData) {
			delete filterData;
		}
		if (d) {
			delete d;
		}
	}
	return err;
}

int Index::fetchTree() {
	// iterate and find the available tree 
	for (std::map<string,TreeData*>::iterator it = this->index.begin(); it != this->index.end(); ++it) {
		if (it->second->nodeCount < MAXNUM_TREENODES) {
			// load tree
			return treeToCache(it->first, it->second);
		}
	}
	// if tree is not found
	// create new data
	TreeData *d = TreeData::create();
	if (!d) {
		return ENOMEM;
	}
	int result = treeToCache(nextFilename(), d);	
	if (result != 0) {
		delete d;
	}
	return result;
}

string Index::nextFilename() {
	char fileName[512];
	sprintf(fileName, "%s/%s%lu", this->path.c_str(), this->prefix.c_str(), this->index.size()+1);
	return fileName;
}

int Index::saveTree() {
	// open file
	ofstream f;
	f.open(this->filePath, ios_base::out | ios_base::binary);
	if (!f.is_open()) {
		return errno;
	}
	int result;
	
	// save filter
	size_t size;
	const unsigned char *filterData = this->cache->filter->Get(size);
	f.write(reinterpret_cast< char *>(&size), sizeof(size));
	if (f.fail()) {
		f.close();
		return EIO;
	}
	f.write((char*)filterData, size);
	if (f.fail()) {
		f.close();
		return EIO;
	}
	f.write(reinterpret_cast< char *>(&this->cache->nodeCount), sizeof(this->cache->nodeCount));
	if (f.fail()) {
		f.close();
		return EIO;
	}
	
	// save tree
	result = this->cache->tree->save(f);
	f.close();	
	return result;
}

void Index::updateFilter(AADataBase* data) {
	unsigned int size;
	const unsigned char * bytes;
	
	// if data
	if (data) {
		// add it to filter
		bytes = data->getBytes(size);
		this->cache->filter->Add(bytes, size);
	} else {
		// if !data
		// reset filter
		this->cache->filter->Clear();
		// do tree scan and set the filter
		queue<const AATreeNode*> q;
		q.push(this->cache->tree->get());
		while (!q.empty()) {
			const AATreeNode *n = q.front();
			q.pop();
			if (!n) {
				continue;
			}
			bytes = q.front()->data->getBytes(size);
			this->cache->filter->Add(bytes, size);
			q.push(n->left);
			q.push(n->right);
		}
	}
}

int Index::add(AADataBase* data) {
	int result;
	//   add element to a tree
	if ((result = this->cache->tree->insert(data)) != 0) {
		return result;
	}
	this->cache->nodeCount++;
	//   add element to a filter	
	updateFilter(data);
	//   save file
	return saveTree();
}

int exists(string name) {
	struct stat buffer;   
	return stat (name.c_str(), &buffer);
} 

int Index::treeToCache(string name, TreeData *d) {
	// unload tree
	this->cache->tree->erase();

	// check if file exists
	if (exists(name)) {
		ifstream f;
		// open file, skip first N bytes(filter data)
		f.open(name, ios_base::in | ios_base::binary);
		if (!f.is_open()) {
			return errno;
		}
		// read size <unsigned int>
		unsigned int size;
		f.read(reinterpret_cast< char *>(&size), sizeof(size));
		if (f.fail()) {
			f.close();
			return EIO;
		}
		f.seekg(sizeof(unsigned int)*2 + sizeof(unsigned char)*size);
		// load tree
		int result = d->tree->load(f, this->foo);
		if (result != 0) {
			f.close();
			return result;
		}
		f.close();
	}
	
	// set map and cache
	this->filePath = filePath;
	this->cache = d;
	this->index[filePath] = d;
	return 0;
}

}
