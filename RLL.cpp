#include<iostream>
#include<fstream>
#include<vector>
#include<string>
#include<vector>
#include<sstream>
#include<cstdlib>
#include<ctime>
#include <unordered_map>

using namespace std;


struct Gate {
    string type; 		// AND, OR, NAND, NOR, NOT, XOR, NXOR
    vector<string> inputs;
    string output; 		// 0 or 1
    bool isKeyGate;
    bool isLocked;
};
struct Node{
	string type;
	vector<Node*>out_node;
	vector<Node*>in_node;
};
vector<string> inputs; 		// input signal name set
vector<string> outputs; 	// output signal name set
vector<Gate> netlist;		// circuit gate set
string keyString;

void outputLockedCircuit(const string& filename, const string& keyString);

string * pointto;
Node *pointtonode;
unordered_map<string, int> stringToID;

int max_key_length = 128;

void parseBenchFile(const string& filename) {
	int numberofgate=0;
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
			Node *node;
			while (ss >> input) {
				gate.inputs.push_back(input);
			}
			gate.isKeyGate = false;
			gate.isLocked = false;
			
			netlist.push_back(gate);
			stringToID[gate.output] = numberofgate;
			numberofgate=numberofgate+1;
		}
	}
	pointto = new string[numberofgate];
	pointtonode = new Node[numberofgate];
	for(int i=0;i<numberofgate;i++){
		pointto[i] = netlist[i].output;
		pointtonode[i].type = netlist[i].type;
		for (const auto& str : netlist[i].inputs) {
			pointtonode[i].in_node.push_back(&pointtonode[stringToID[str]]);
			pointtonode[stringToID[str]].out_node.push_back(&pointtonode[i]);
		}
	}
}

int searching_for_unlock_gate(int preasent_pos){
	int srearching_pos = preasent_pos;
	while(true){
		if(!netlist[srearching_pos].isLocked && netlist[srearching_pos].type != "OUTPUT" && netlist[srearching_pos].type !="not" && netlist[srearching_pos].type !="NOT" && 
		netlist[srearching_pos].type != "buf" && netlist[srearching_pos].type != "BUF"){
			return srearching_pos;
		}
		else{
			srearching_pos ++;
		}
	}
	return srearching_pos;
}

void addKeyGate(const int loc, const int& key_bit) {
	// loc: the location of a gate. The key gate will be insert at its output.
	// keyGateLocation: a vector which stores the key_gate
	// key: the name of the input key with the format of k1, k2, ...;

	// srand(time(0));

	string key = "keyinput"+to_string(key_bit); 
	inputs.push_back(key);

	Gate keyGate;
	if(keyString[key_bit]=='0') keyGate.type = "XOR"; //when the corresponding key_value is zero;
	else keyGate.type = "XNOR";

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
	keyGate.isLocked=true;
	
	vector<Gate>::iterator it;
	it = netlist.begin();
	
	// The location is the place of the locked gate; therefore the key gate must insert at its next position.
	netlist.insert(it+loc+1, keyGate);

		
}

void selectGateLocationRandomly(int& pos){

	pos = searching_for_unlock_gate(rand()%(netlist.size()));
    
    return;
}



bool findEdgeType(Gate& gate_j, Gate& key_gate_k){
	return false;
}// return false when mutable 

void applyRandomLogicLocking(int keySize) {

	//Randomly select the location to insert the first key gate.
	for(int i=0; i < keySize; i++){
		int pos;
		selectGateLocationRandomly(pos);
  
		addKeyGate(pos, i);
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


string key_generate(int length) {
    string binary = "";
    for (int i = length-1; i >= 0; --i) {
        binary += (rand()%2)?"1":"0";
    }
    if (binary.empty()) {
        binary = "0";
    }
    return binary;
}

int not_buf_counter(){
	int counter = 0;
	for(int srearching_pos = 0; srearching_pos < netlist.size(); srearching_pos++){

		if( netlist[srearching_pos].type == "not" || netlist[srearching_pos].type =="NOT" || 
			netlist[srearching_pos].type == "buf" || netlist[srearching_pos].type == "BUF"){
				counter ++;
			}
	}
	return counter;
}

int main(int argv, char* argc[]) {


	string filename = argc[1];

	parseBenchFile(filename);	
	// Apply logical lockign on circuit with SLL technique	
	//cout<<" Numbers of Gates:"<< netlist.size() <<endl;	
	int keySize = netlist.size() - outputs.size() - not_buf_counter();
	if(keySize>max_key_length) keySize=128; 

	// cout << keySize<<endl;


	keyString = key_generate(keySize);


	applyRandomLogicLocking(keySize);  

	outputLockedCircuit("rll_"+filename, keyString);

	return 0;
}
