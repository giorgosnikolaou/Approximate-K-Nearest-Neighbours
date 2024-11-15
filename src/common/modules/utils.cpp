#include "utils.hpp"
#include <endian.h>

using namespace std;


///////////////
// Stopwatch //
///////////////


Stopwatch::Stopwatch() : time(chrono::high_resolution_clock::now()) { }
void Stopwatch::start() { time = chrono::high_resolution_clock::now(); }
double Stopwatch::stop() { 
    auto end_time = chrono::high_resolution_clock::now(); 
    return chrono::duration_cast<chrono::duration<float>>(end_time - time).count();
}

////////////////
// Data Point //
////////////////

DataPoint::DataPoint(ifstream& input_, uint32_t size, uint32_t id_) : id(id_), vector(new Vector<uint8_t>(input_, size)) { }
DataPoint::~DataPoint() { delete vector; }

uint32_t DataPoint::label() const { return id; }
Vector<uint8_t>& DataPoint::data() const { return *vector; }



//////////////
// Data Set //
//////////////

DataSet::DataSet(string path, uint32_t files) {
    
    ifstream input(path.data(), ios::binary);

    if (input.fail())
        throw runtime_error("Exception during DataSet creation: " + path + " could not be opened!\n");

    uint32_t count, h, w;
    input.read((char*)&count, 4); // Discard magic number
    input.read((char*)&count, 4);
    input.read((char*)&h, 4);
    input.read((char*)&w, 4);
    
    count = files == 0 ? be32toh(count) : files;
    h     = be32toh(h);
    w     = be32toh(w);
    vector_size = h * w;


    for (uint32_t i = 0; i < count; i++) 
        points.push_back(new DataPoint(input, vector_size, i + 1));
    
    input.close();
}

DataSet::~DataSet() {
    for (auto point : points)
        delete point;
}

uint32_t DataSet::dim() const{ return vector_size; }
uint32_t DataSet::size() const{ return points.size(); }

DataPoint* DataSet::operator[](uint32_t i) const { return points[i]; }

vector<DataPoint*>::iterator DataSet::begin() { return points.begin(); }
vector<DataPoint*>::iterator DataSet::end() { return points.end(); }