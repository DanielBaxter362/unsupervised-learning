#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <random>
#include <algorithm>
#include <iomanip>

struct CSVData {
    std::vector<std::string> headers;
    std::vector<std::vector<int>> rows;
    std::size_t maxPrintEntryLen = 15;
};

std::vector<int> cluster(const std::vector<std::vector<int>>& data, int k) {
	size_t entrySize = data[0].size();
	size_t numEntries = data.size();
	std::vector<int> clusters(data.size());

	// Bounds for random centroids
	std::vector<int> centroidLowerBounds;
	std::vector<int> centroidUpperBounds;

	for (size_t i = 0; i < entrySize; i++) {
		int lowest = data[0][i];
		int highest = data[0][i];

		for (const std::vector<int>& row : data) {
			int val = row[i];

			if (val < lowest) { lowest = val; };
			if (val > highest) { highest = val; };
		}

		centroidLowerBounds.push_back(lowest);
		centroidUpperBounds.push_back(highest);
	}

	// Create k random centroids
	std::mt19937 rng(std::random_device{}());
	std::vector<std::vector<int>> centroids(k);

	for (size_t i = 0; i < entrySize; i++) {
		std::uniform_int_distribution<int> dist(centroidLowerBounds[i], centroidUpperBounds[i]);
		
		for (size_t j = 0; j < k; j++) {
			centroids[j].push_back(dist(rng));
		}
	}

	for (size_t i = 0; i < entrySize; i++) {
		std::cout << "Column: " << i + 1 << std::endl;
		std::cout << "Largest: " << centroidUpperBounds[i] << std::endl;
		std::cout << "Smallest: " << centroidLowerBounds[i] << std::endl;

		for (size_t j = 0; j < centroids.size(); j++) {
			std::cout << "Centroid " << j + 1 << ": " << centroids[j][i] << std::endl;
		}
		std::cout << std::endl;
	}

	// Allocate and update clusters until no points are changed
	bool changed = true;
	while (changed) {
		changed = false;

		for (size_t i = 0; i < numEntries; i++) {
			const std::vector<int>& entry = data[i];
			std::vector<float> dist;
			
			for (size_t j = 0; j < k; j++) {
				const std::vector<int>& centroid = centroids[j];

				float currentDist = 0;
				for (size_t l = 0; l < entrySize; l++) {
					float diff = entry[l] - centroid[l];
					currentDist += diff * diff;
				}

				dist.push_back(currentDist);
			}

			int newCluster = std::distance(dist.begin(), std::min_element(dist.begin(), dist.end()));
			if (newCluster != clusters[i]) { changed = true; };
			clusters[i] = newCluster;
		}

		// Update centroids to the mean of all their data points
		for (size_t i = 0; i < k; i++) {
			std::vector<int> positions(entrySize);
			int entryCount = 0;

			for (size_t j = 0; j < numEntries; j++) {
				if (clusters[j] != i) { continue; };

				entryCount++;
				for (size_t l = 0; l < entrySize; l++) {
					positions[l] += data[j][l];
				}
			}
			if (entryCount == 0) { continue; };

			for (int& num : positions) {
				num = num / entryCount;
			}

			centroids[i] = positions;
		}
	}

	return clusters;
}

std::string trimWhitespace(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    size_t end = str.find_last_not_of(" \t\r\n");
            
    if (start == std::string::npos) {
        return ""; 
    };  
        
    return str.substr(start, end - start + 1);
}   

std::vector<std::string> parseCSVLine(const std::string& line) {
    std::vector<std::string> entry;
    std::string item;
    bool inQuotes = false;
    
    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        
        if (c == ',' && !inQuotes) {
            entry.push_back(item);
            item.clear();
        }
        else if (c == '"') {
            inQuotes = !inQuotes;
        }
        else {
            item += c;
        }
        
        if (entry.size() > 100) { throw std::invalid_argument("Too many fields"); };
    }
    
    entry.push_back(item);
    std::transform(entry.begin(), entry.end(), entry.begin(), trimWhitespace);
    return entry;
}

CSVData readCSV(std::ifstream& stream) {
    CSVData data;
    std::string line;
    
    // Still a bug where header length is not checked and cannot set maxLen

    // Get headers
    std::getline(stream, line);
    
    if (line.empty()) { throw std::invalid_argument("Empty CSV provided"); };
    std::vector<std::string> headers = parseCSVLine(line);

    // Get data rows
    std::size_t maxLen = 0;
    std::vector<std::vector<int>> rows;
    std::vector<int> row;

    while (std::getline(stream, line)) {
        row.clear();
           
        for (std::string& entry : parseCSVLine(line)) {
            std::size_t size = entry.size();

            if (size == 0) {
                throw std::invalid_argument("Empty value in input CSV");
            }
            else if (size > 15) {
                throw std::invalid_argument("Values must be 15 digits or less");
            }
            else if (size > maxLen) {
                maxLen = size;
            }
                
            // Allow only numbers
            for (char c : entry) {
                if (!isdigit(c)) {
                    throw std::invalid_argument("Invalid character in input data item");
                }   
            }   
            row.push_back(std::stoi(entry));
        }   
        rows.push_back(row);   
    }

    // Check shape of rows
    if (rows.empty()) {
        throw std::invalid_argument("CSV has no input data rows"); 
    }   
    else {
        int rowSize = headers.size();

        if (rowSize < 2) { throw std::invalid_argument("CSV must have two or more columns of data"); };
        
        for (std::vector<int>& row : rows) {
            if (row.size() != rowSize) {
                throw std::invalid_argument("CSV rows must have consistent length");
            }   
        }   
    } 
    
    data.maxPrintEntryLen = maxLen;
    data.headers = headers;
    data.rows = rows;
    return data;
}

void printStrRow(const std::vector<std::string>& row, const std::size_t& length) {
    for (size_t i = 0; i < row.size(); i++) {
        if (i != 0) { std::cout << ", "; };

        if (row[i].size() <= length) {
            std::cout << std::left << std::setw(length) << row[i];
            continue;
        };
     
        std::cout << row[i];
    }
}

void printCSV(const CSVData& data) {
    std::vector<std::string> row = data.headers;
    printStrRow(row, data.maxPrintEntryLen);
    std::cout << std::endl;

    for (size_t i = 0; i < data.rows.size(); i++) {
        std::transform(data.rows[i].begin(), data.rows[i].end(), row.begin(),
            [](int x) { return std::to_string(x); });

        printStrRow(row, data.maxPrintEntryLen);        
        std::cout << std::endl;
    }
}

int main(int argc, char* argv[]) {
	// Check arguments are correct
	if (argc != 2) {
		std::string name = std::filesystem::path(argv[0]).filename().string();
		std::cerr << "Usage: " << name << " <filename>\n";
		return 1;
	}

	std::string filename = argv[1];
	int nameLen = filename.size();
	if (nameLen <= 4 || filename.substr(nameLen - 4) != ".csv") {
		std::cerr << "Invalid file type (must be csv)\n";
		return 1;
	}

	std::ifstream csvin(filename);
	if (!csvin.is_open()) {
		std::cerr << "Could not open file: " << filename << "\n";
		return 1;
	}
    
    CSVData data;
	
    try {
	    data = readCSV(csvin);
    } 
    catch (const std::invalid_argument& e) {
        std::cerr << "Error parsing CSV: " << e.what() << std::endl;
        return 1;
    }
    
    printCSV(data);

    //std::vector<int> out = cluster(entries, 2);

	//todo: create new csv containing the input data and the cluster each row belongs to
}
