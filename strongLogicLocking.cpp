#include<iostream>
#include<fstream>
#include<vector>
#include<string>
#include<vector>
#include<sstream>
#include<cstdlib>
#include<ctime>

using namespace std;


struct Gate {
    string type; 		// AND, OR, NAND, NOR, NOT, XOR, NXOR
    vector<string> inputs;
    string output; 		// 0 or 1
    bool isKeyGate;
    bool isLocked;
};

vector<string> inputs; 		// input signal name set
vector<string> outputs; 	// output signal name set
vector<Gate> netlist;		// circuit gate set
string keyString;

void outputLockedCircuit(const string& filename, const string& keyString);

void parseBenchFile(const string& filename) {
	ifstream file(filename);
	string line;
	while (getline(file, line)) {
		
		if (line.find("INPUT")==0 ) {
			inputs.push_back(line.substr(6, line.size() - 7));
		} else if (line.find("OUTPUT")==0) {
			outputs.push_back(line.substr(7, line.size() - 8));
			
		} else {
			
			// Skipped when line is empty..
			if(line=="\0") continue;
			if( line.find('(') < line.size() ) line.replace( line.find('('), 1, " ");
			if( line.find(')') < line.size() ) line.replace( line.find(')'), 1, " ");
			while( line.find(',') < line.size() ) line.replace( line.find(','), 1, " ");
			if( line.find('=') < line.size() ) line.replace( line.find('='), 1, " ");

			stringstream ss(line);
						
			Gate gate;
			ss >> gate.output >> gate.type;
			
			string input;
			
			while (ss >> input) {
				gate.inputs.push_back(input);
			}
			gate.isKeyGate = false;
			gate.isLocked = false;
			
			netlist.push_back(gate);
		}
	}
	
}

void addNotGate(const int loc, vector<Gate>& keyGateLocations) {
	// loc: the location of a gate. The key gate will be insert at its output.
	// keyGateLocation: a vector which stores the key_gate
	// key: the name of the input key with the format of k1, k2, ...;


	Gate NotGate;
	NotGate.type = "NOT"; //when the corresponding key_value is zero;

	// Change the name of the output wire of the gate to be locked;

	string org_out = netlist[loc].output;
	netlist[loc].output = org_out+"_inv";
	netlist[loc].isLocked = true;
	//The output [of the locked gate] will be the input of the keygate
	NotGate.inputs.push_back(netlist[loc].output);


	//The output of the key gate use the original name for simplisity of implementation.
	NotGate.output = org_out;
	NotGate.isKeyGate = true;
	NotGate.isLocked = true;
	vector<Gate>::iterator it;
	it = netlist.begin();
	
	// The location is the place of the locked gate; therefore the key gate must insert at its next position.
	netlist.insert(it+loc+1, NotGate);

	keyGateLocations.push_back(NotGate);
}

void addKeyGate(const int loc, vector<Gate>& keyGateLocations, const int& key_bit) {
	// loc: the location of a gate. The key gate will be insert at its output.
	// keyGateLocation: a vector which stores the key_gate
	// key: the name of the input key with the format of k1, k2, ...;

	string key = "keyinput"+to_string(key_bit); 
	inputs.push_back(key);

	Gate keyGate;
	keyGate.type = "XOR"; //when the corresponding key_value is zero;

	// Change the name of the output wire of the gate to be locked;

	string org_out = netlist[loc].output;
	netlist[loc].output = org_out+"_lock";
	netlist[loc].isLocked = true;

	//The output [of the locked gate] will be the input of the keygate
	keyGate.inputs.push_back(netlist[loc].output);
	keyGate.inputs.push_back(key);

	//The output of the key gate use the original name for simplisity of implementation.
	keyGate.output = org_out;
	keyGate.isKeyGate = true;
	if(keyString[key_bit]=='0') keyGate.isLocked=true;
	
	vector<Gate>::iterator it;
	it = netlist.begin();
	
	// The location is the place of the locked gate; therefore the key gate must insert at its next position.
	netlist.insert(it+loc+1, keyGate);

	keyGateLocations.push_back(keyGate);
	if(keyString[key_bit]=='1') addNotGate(loc+1,keyGateLocations);
	
	string watch;
	int tg;
	if(keyString[key_bit]=='0') tg = loc+2; 
	else tg = loc+3;  
	if(tg<netlist.size()){
		watch = netlist[tg].type;
		if(watch == "NOT"||watch == "not" || watch =="BUF" || watch=="buf") netlist[tg].isLocked=true; 
	}
}
void selectGateLocationRandomly(int& pos){
    do{
        pos = rand()%(netlist.size());

    }while(netlist[pos].isLocked);
//cout<<pos<<endl;	
    return;
}


bool findEdgeType(Gate& gate_j, Gate& key_gate_k){
	return false;
}// return false when mutable 

void applyStrongLogicLocking(int keySize) {

	vector<Gate> keyGateLocations = {};

	//Randomly select the location to insert the first key gate.
	int pos;
	selectGateLocationRandomly(pos);
  
	addKeyGate(pos, keyGateLocations, 0);

	for(int i=1; i < keySize; i++){
		bool foundNonMutable = false;
		for(int j=0;j<netlist.size();j++){
			if(netlist[j].isKeyGate==false && netlist[j].isLocked==false){

				bool edgeTypes = true; // edgeTypes is true when all edges are nonmutable

				for(int k=0; k<keyGateLocations.size(); k++){
					
					// findEdgeType(Gate& gate_j, Gate& key_gate_k) return false when mutable 
					
					edgeTypes = findEdgeType(netlist[j], keyGateLocations[k]);

					if(!edgeTypes) break;
						
				}
				if(edgeTypes){
					addKeyGate(j, keyGateLocations, i);
					foundNonMutable = true;
					break;
				}

			}
		}
		if( foundNonMutable == false ) {
			int rpos;
			selectGateLocationRandomly(rpos);
			addKeyGate(rpos,keyGateLocations,i);
		}
	}
}

void outputLockedCircuit(const string& filename, const string& keyString) {
    ofstream file(filename);
    file << "# key=" << keyString << "\n";
    for (const string& input : inputs) {
        file << "INPUT(" << input << ")\n";
    }
    for (const string& output : outputs) {
        file << "OUTPUT(" << output << ")\n";
    }
    file << "\n";
    for (const Gate& gate : netlist) {
        file << gate.output << " = " << gate.type<<"(";
        int i=0;
	for (const string& input : gate.inputs) {
            if(i==0) {
		file<<input;
		i=1;
	    }else file << ", " << input;
        }
        file << ")\n";
    }
}


int main(int argv, char* argc[]) {

	string filename = argc[1];

	parseBenchFile(filename);
	srand(time(0));	
	// Apply logical lockign on circuit with SLL technique	
	//cout<<" Numbers of Gates:"<< netlist.size() <<endl;	
	int keySize = netlist.size()-outputs.size();
	if(keySize>128) keySize=128;
	if(keySize<4) keySize = 4; 
	string str = "11101010010001001011111010100100010010111110101001000100101111101010010001001011010111101000101010010100010100100101000010010101";
	keyString = str.substr(0,keySize);
	applyStrongLogicLocking(keySize);  

	outputLockedCircuit("locked_"+filename, keyString);

	return 0;
}
